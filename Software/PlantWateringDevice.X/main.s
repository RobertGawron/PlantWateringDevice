        PROCESSOR   10F202
        #include    <xc.inc>

; =============================================================================
; PIC10F202 - Button edge detect updates pump time and generates GP0 pulse
; Toolchain : PIC-AS (XC8 / MPLAB X)
; Clock     : Internal oscillator nominal 4 MHz (factory OSCCAL preserved)
;
; Button:
;   - Connected to GP3
;   - Active low: GP3=0 when pressed, GP3=1 when released (requires pull-up)
;
; Behaviour:
;   - Sample every ~200 ms
;   - If button is pressed AND it wasn't pressed previously (new press event):
;       1) pumpSteps increments by 1 (each step = 5 seconds)
;          range 1..10 => 5..50 s, wraps 10->1
;       2) GP0 is driven high for PULSE_TICKS ticks (non-blocking)
;
; Notes for audit:
;   - No code/data is emitted at 0x1FF to avoid overwriting OSCCAL calibration.
;   - Timing is approximate due to internal oscillator tolerance.
; =============================================================================

        CONFIG      WDTE=OFF, CP=OFF, MCLRE=OFF   ; GP3 used as GPIO input

; -----------------------------------------------------------------------------
; Constants
; -----------------------------------------------------------------------------
PUMP_STEP_MIN       EQU     1
PUMP_STEP_MAX       EQU     10

BUTTON_BIT          EQU     3       ; GP3
PULSE_BIT           EQU     0       ; GP0 output pulse

PREV_PRESSED_BIT    EQU     0       ; flags.bit0: 1=was pressed, 0=was not pressed
Z_BIT               EQU     2       ; STATUS.Z bit position (baseline core)

; Pulse length in ticks of the sampling period (~200 ms each)
PULSE_TICKS         EQU     1       ; 1 => ~200 ms, 2 => ~400 ms, ...

; -----------------------------------------------------------------------------
; RAM
; -----------------------------------------------------------------------------
        PSECT udata, class=RAM, space=1
pumpSteps:      DS  1               ; 1..10 => 5..50 seconds
flags:          DS  1               ; prev button state flag(s)
pulseTicks:     DS  1               ; remaining pulse ticks (0 => inactive)
dly1:           DS  1
dly2:           DS  1

; -----------------------------------------------------------------------------
; Code
; -----------------------------------------------------------------------------
        PSECT MyCode, class=CODE, delta=2
        GLOBAL start
        ORG     0x000

start:
        ; Preserve OSCCAL word by not programming address 0x1FF in this source.

        ; GPIO directions: GP3 input, GP0/GP1/GP2 outputs
        movlw   0x08                ; 0000 1000b
        tris    GPIO

        ; Safe initial states
        clrf    GPIO                ; outputs low
        clrf    pulseTicks

        movlw   PUMP_STEP_MIN
        movwf   pumpSteps

        ; Initialize "previous pressed" = false.
        ; Assumption: at power-up the user is not pressing the button.
        bcf     flags, PREV_PRESSED_BIT

; =============================================================================
; Main loop: ~200 ms tick
; =============================================================================
main_loop:
        call    Delay_200ms

        ; Maintain pulse timing (turn GP0 off when time expires)
        call    ServicePulse

        ; Sample button and handle new press event
        call    HandleButton

        goto    main_loop

; -----------------------------------------------------------------------------
; ServicePulse
; Non-blocking pulse generator:
;   If pulseTicks > 0, decrement each tick; when it reaches 0, clear GP0.
; -----------------------------------------------------------------------------
ServicePulse:
        movf    pulseTicks, f
        btfsc   STATUS, Z_BIT
        retlw   0                   ; no active pulse

        decfsz  pulseTicks, f
        retlw   0                   ; pulse still active

        ; pulseTicks just reached 0 => end of pulse
        bcf     GPIO, PULSE_BIT
        retlw   0

; -----------------------------------------------------------------------------
; HandleButton
; Detects a new press (pressed now, not pressed before).
; On new press:
;   - Update pumpSteps (wrap after 10)
;   - Start GP0 pulse for PULSE_TICKS ticks
; -----------------------------------------------------------------------------
HandleButton:
        ; Determine current button state.
        ; GP3=0 => pressed, GP3=1 => released
        btfsc   GPIO, BUTTON_BIT
        goto    BtnReleased

BtnPressed:
        ; If it was already pressed last sample => not a new event
        btfsc   flags, PREV_PRESSED_BIT
        retlw   0

        ; New press event (pressed now, not pressed before)
        bsf     flags, PREV_PRESSED_BIT

        ; ---- Update pumpSteps (5-second quanta) ------------------------------
        incf    pumpSteps, f
        movlw   (PUMP_STEP_MAX + 1) ; 11
        subwf   pumpSteps, w        ; Z=1 iff pumpSteps == 11
        btfss   STATUS, Z_BIT
        goto    StartPulse

        movlw   PUMP_STEP_MIN
        movwf   pumpSteps

StartPulse:
        ; ---- Generate GP0 pulse for a defined period ------------------------
        ; Restart behaviour: each new press restarts the pulse deterministically.
        movlw   PULSE_TICKS
        movwf   pulseTicks
        bsf     GPIO, PULSE_BIT
        retlw   0

BtnReleased:
        ; Record "not pressed" so next press can be detected as a new event
        bcf     flags, PREV_PRESSED_BIT
        retlw   0

; -----------------------------------------------------------------------------
; Delay_200ms (approximate at nominal 4 MHz)
; Adjust loop constants if you need a tighter sampling period.
; -----------------------------------------------------------------------------
Delay_200ms:
        movlw   70
        movwf   dly2
D200_outer:
        movlw   250
        movwf   dly1
D200_inner:
        decfsz  dly1, f
        goto    D200_inner
        decfsz  dly2, f
        goto    D200_outer
        retlw   0

        END