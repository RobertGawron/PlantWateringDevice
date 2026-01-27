        PROCESSOR   10F202
        #include    <xc.inc>

; =============================================================================
; PIC10F202 - Button edge detect updates pump time and generates GP0 pulse
; Toolchain : PIC-AS (VS Code)
; Clock     : Internal oscillator nominal 4 MHz (factory OSCCAL preserved)
;
; Button:
;   - Connected to GP3
;   - Active low: GP3=0 when pressed, GP3=1 when released (requires pull-up)
;
; Sensor:
;   - Connected to GP1 (input)
;   - Active high: GP1=1 means "dry" => watering needed
;
; Pulse output:
;   - Connected to GP0 (output)
;   - Active high pulse generated on each new button press (for UI feedback/test)
;
; Pump output:
;   - Connected to GP2 (output)
;   - Active high: GP2=1 turns pump ON
;
; Behaviour:
;   - Sample every ~200 ms
;   - If button is pressed AND it wasn't pressed previously (new press event):
;       1) ui_pump_steps increments by 1 (each step = 5 seconds)
;          range 1..10 => 5..50 s, wraps 10->1
;       2) GP0 is driven high for UI_PULSE_TICKS ticks (non-blocking)
;   - Every 1 minute (test version):
;       If sensor indicates dry (GP1=1) AND pump is currently OFF:
;          Turn pump ON (GP2=1) for ui_pump_steps * 5 seconds (non-blocking).
;
; Implementation note:
;   Pump runtime is counted in 5-second steps (no multiplication needed).
;
; Notes:
;   - No code/data is emitted at 0x1FF to avoid overwriting OSCCAL calibration.
;   - Timing is approximate due to internal oscillator tolerance.
; =============================================================================

        CONFIG      WDTE=OFF, CP=OFF, MCLRE=OFF   ; GP3 used as GPIO input

; -----------------------------------------------------------------------------
; Constants
; -----------------------------------------------------------------------------
UI_PUMP_STEP_MIN             EQU     1
UI_PUMP_STEP_MAX             EQU     10

; GPIO bit assignments
HW_PIN_PULSE_BIT             EQU     0       ; GP0 output pulse
HW_PIN_SENSOR_BIT            EQU     1       ; GP1 input (dry sensor, active high)
HW_PIN_PUMP_OUT_BIT          EQU     2       ; GP2 output (pump enable, active high)
HW_PIN_BUTTON_BIT            EQU     3       ; GP3 input (button)

; STATUS register flag bit positions (baseline core)
HW_STATUS_Z_BIT              EQU     2       ; STATUS.Z

; Button state flag bits
UI_FLAG_PREV_PRESSED_BIT     EQU     0       ; ui_flags.bit0: 1=was pressed previously

; Tick / timekeeping derived from Delay_200ms tick
UI_TICKS_PER_5_SECONDS       EQU     25      ; 25 * 200 ms = 5 seconds
UI_CHECK_PERIOD_5S_BLOCKS    EQU     12      ; TEST: 12 * 5 s = 60 seconds

; Pulse length in ticks of the sampling period (~200 ms each)
UI_PULSE_TICKS               EQU     1       ; 1 => ~200 ms, 2 => ~400 ms, ...

; -----------------------------------------------------------------------------
; RAM
; -----------------------------------------------------------------------------
        PSECT udata, class=RAM, space=1
ui_pump_steps:           DS  1               ; 1..10 => 5..50 seconds (setting)
ui_flags:                DS  1               ; UI flags

ui_pump_steps_rem:       DS  1               ; remaining pump time in 5-second steps (0 => idle)

ui_tick_to_5s_cnt:       DS  1               ; counts 0..(UI_TICKS_PER_5_SECONDS-1)
ui_check_5s_blocks_cnt:  DS  1               ; counts 5-second blocks up to UI_CHECK_PERIOD_5S_BLOCKS

ui_pulse_ticks:          DS  1               ; remaining pulse ticks (0 => inactive)

ui_delay_inner_cnt:      DS  1
ui_delay_outer_cnt:      DS  1

; -----------------------------------------------------------------------------
; Code
; -----------------------------------------------------------------------------
        PSECT MyCode, class=CODE, delta=2
        GLOBAL start
        ORG     0x000

start:
        ; Preserve OSCCAL word by not programming address 0x1FF in this source.

        ; GPIO directions:
        ;   GP3 input  (button)
        ;   GP1 input  (sensor)
        ;   GP2 output (pump)
        ;   GP0 output (pulse)
        movlw   0x0A                ; 0000 1010b => GP3,GP1 inputs; GP2,GP0 outputs
        tris    GPIO

        ; Safe initial states
        clrf    GPIO                ; outputs low (pump off, pulse low)

        clrf    ui_pump_steps_rem   ; pump idle
        clrf    ui_tick_to_5s_cnt
        clrf    ui_check_5s_blocks_cnt
        clrf    ui_pulse_ticks      ; pulse inactive

        movlw   UI_PUMP_STEP_MIN
        movwf   ui_pump_steps

        ; Initialize "previous pressed" = false.
        ; Assumption: at power-up the user is not pressing the button.
        bcf     ui_flags, UI_FLAG_PREV_PRESSED_BIT

; =============================================================================
; Main loop: ~200 ms tick
; =============================================================================
L_MAIN_LOOP:
        call    Delay_200ms

        ; Maintain pulse timing (turn GP0 off when time expires)
        call    ServicePulse

        ; Maintain timebase, periodic dry check, and pump runtime (non-blocking)
        call    ServiceTimebaseAndPump

        ; Sample button and handle new press event
        call    HandleButton

        goto    L_MAIN_LOOP

; -----------------------------------------------------------------------------
; ServicePulse
; Non-blocking pulse generator:
;   If ui_pulse_ticks > 0, decrement each tick; when it reaches 0, clear GP0.
; -----------------------------------------------------------------------------
ServicePulse:
        movf    ui_pulse_ticks, f
        btfsc   STATUS, HW_STATUS_Z_BIT
        retlw   0                   ; no active pulse

        decfsz  ui_pulse_ticks, f
        retlw   0                   ; pulse still active

        ; ui_pulse_ticks just reached 0 => end of pulse
        bcf     GPIO, HW_PIN_PULSE_BIT
        retlw   0

; -----------------------------------------------------------------------------
; ServiceTimebaseAndPump
; Derives 5-second events from the 200 ms tick and uses them to:
;   1) Decrement pump runtime (ui_pump_steps_rem) every 5 seconds
;   2) Trigger periodic sensor check every UI_CHECK_PERIOD_5S_BLOCKS blocks
;
; Pump rules:
;   - GP2 is ON whenever ui_pump_steps_rem > 0
;   - New pump cycle must NOT start if ui_pump_steps_rem > 0
; -----------------------------------------------------------------------------
ServiceTimebaseAndPump:
        ; Keep GP2 consistent with current pump state on every tick
        movf    ui_pump_steps_rem, f
        btfsc   STATUS, HW_STATUS_Z_BIT
        goto    L_PUMP_FORCE_OFF
        bsf     GPIO, HW_PIN_PUMP_OUT_BIT
        goto    L_UPDATE_5S_TICK
L_PUMP_FORCE_OFF:
        bcf     GPIO, HW_PIN_PUMP_OUT_BIT

L_UPDATE_5S_TICK:
        ; Accumulate 200 ms ticks into one 5-second block
        incf    ui_tick_to_5s_cnt, f
        movlw   UI_TICKS_PER_5_SECONDS
        subwf   ui_tick_to_5s_cnt, w        ; Z=1 iff ui_tick_to_5s_cnt == UI_TICKS_PER_5_SECONDS
        btfss   STATUS, HW_STATUS_Z_BIT
        retlw   0                           ; not yet 5 seconds

        ; ---- 5 seconds elapsed ------------------------------------------------
        clrf    ui_tick_to_5s_cnt

        ; 1) If pump active, decrement remaining 5-second steps
        movf    ui_pump_steps_rem, f
        btfsc   STATUS, HW_STATUS_Z_BIT
        goto    L_UPDATE_CHECK_COUNTER

        decfsz  ui_pump_steps_rem, f
        goto    L_UPDATE_CHECK_COUNTER      ; still running

        ; Just reached 0 => stop pump output immediately
        bcf     GPIO, HW_PIN_PUMP_OUT_BIT

L_UPDATE_CHECK_COUNTER:
        ; 2) Update periodic check counter (5-second blocks)
        incf    ui_check_5s_blocks_cnt, f
        movlw   UI_CHECK_PERIOD_5S_BLOCKS
        subwf   ui_check_5s_blocks_cnt, w   ; Z=1 iff block counter == period
        btfss   STATUS, HW_STATUS_Z_BIT
        retlw   0

        ; Period elapsed => reset counter and check sensor
        clrf    ui_check_5s_blocks_cnt

        ; Do not start if pump currently running
        movf    ui_pump_steps_rem, f
        btfss   STATUS, HW_STATUS_Z_BIT
        retlw   0

        ; Sensor check: GP1=1 means dry => start watering
        btfss   GPIO, HW_PIN_SENSOR_BIT
        retlw   0                           ; not dry

        ; Start pump for ui_pump_steps * 5 seconds (in 5-second steps)
        movf    ui_pump_steps, w
        movwf   ui_pump_steps_rem
        bsf     GPIO, HW_PIN_PUMP_OUT_BIT
        retlw   0

; -----------------------------------------------------------------------------
; HandleButton
; Detects a new press (pressed now, not pressed before).
; On new press:
;   - Update ui_pump_steps (wrap after 10)
;   - Start GP0 pulse for UI_PULSE_TICKS ticks
; -----------------------------------------------------------------------------
HandleButton:
        ; Determine current button state.
        ; GP3=0 => pressed, GP3=1 => released
        btfsc   GPIO, HW_PIN_BUTTON_BIT
        goto    L_BTN_RELEASED

L_BTN_PRESSED:
        ; If it was already pressed last sample => not a new event
        btfsc   ui_flags, UI_FLAG_PREV_PRESSED_BIT
        retlw   0

        ; New press event (pressed now, not pressed before)
        bsf     ui_flags, UI_FLAG_PREV_PRESSED_BIT

        ; ---- Update ui_pump_steps (5-second quanta) --------------------------
        incf    ui_pump_steps, f
        movlw   (UI_PUMP_STEP_MAX + 1)  ; 11
        subwf   ui_pump_steps, w        ; Z=1 iff ui_pump_steps == 11
        btfss   STATUS, HW_STATUS_Z_BIT
        goto    L_START_PULSE

        movlw   UI_PUMP_STEP_MIN
        movwf   ui_pump_steps

L_START_PULSE:
        ; ---- Generate GP0 pulse for a defined period ------------------------
        ; Restart behaviour: each new press restarts the pulse deterministically.
        movlw   UI_PULSE_TICKS
        movwf   ui_pulse_ticks
        bsf     GPIO, HW_PIN_PULSE_BIT
        retlw   0

L_BTN_RELEASED:
        ; Record "not pressed" so next press can be detected as a new event
        bcf     ui_flags, UI_FLAG_PREV_PRESSED_BIT
        retlw   0

; -----------------------------------------------------------------------------
; Delay_200ms (approximate at nominal 4 MHz)
; -----------------------------------------------------------------------------
Delay_200ms:
        movlw   70
        movwf   ui_delay_outer_cnt
L_D200_OUTER:
        movlw   250
        movwf   ui_delay_inner_cnt
L_D200_INNER:
        decfsz  ui_delay_inner_cnt, f
        goto    L_D200_INNER
        decfsz  ui_delay_outer_cnt, f
        goto    L_D200_OUTER
        retlw   0

        END