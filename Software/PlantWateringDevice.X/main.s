;##############################################################################
;
; FILE:         main.s
; PROCESSOR:    PIC10F202
; TOOLCHAIN:    PIC-AS (MPLAB X / VS Code)
; DESCRIPTION:  Plant watering controller with user-adjustable pump duration
;
;##############################################################################
;
; HARDWARE CONFIGURATION:
;
;   GP0 (pin 1) - Output: Bargraph clock pulse / duration indicator
;                 Connected to CD4017 clock input for 10-step LED bargraph
;
;   GP1 (pin 3) - Input:  Soil moisture sensor
;                 HIGH = soil is dry (watering needed)
;                 LOW  = soil is wet (no action)
;
;   GP2 (pin 4) - Output: Pump relay control
;                 HIGH = pump running
;                 LOW  = pump stopped
;
;   GP3 (pin 2) - Input:  User pushbutton (active LOW, external pull-up required)
;                 Directly controls pump duration setting via bargraph
;
; FUNCTIONAL DESCRIPTION:
;
;   - Button press advances the pump duration setting (1-10 steps, wrapping)
;   - Each step represents 5 seconds of pump run time (5s to 50s range)
;   - GP0 outputs a pulse equal to current setting * 5 seconds as feedback
;   - Every 60 seconds, sensor is checked; if dry, pump runs for set duration
;   - Pump will not restart if already running
;
; TIMING ASSUMPTIONS:
;
;   - Internal oscillator runs at nominal 4 MHz
;   - Instruction cycle = 1 us (Fosc/4)
;   - Base tick period = 20 ms (50 ticks = 1 second)
;   - Timing accuracy depends on oscillator tolerance (+/- 1%)
;
; MEMORY USAGE:
;
;   - Program memory: < 256 words (device has 512 words)
;   - RAM: 8 bytes (device has 24 bytes available: 0x08-0x1F)
;
;##############################################################################

        PROCESSOR   10F202
        #include    <xc.inc>

;##############################################################################
; CONFIGURATION BITS
;##############################################################################

; WDTE=OFF  : Watchdog timer disabled (software timing used instead)
; CP=OFF    : Code protection disabled
; MCLRE=OFF : GP3 functions as digital input (not master clear)

        CONFIG      WDTE=OFF, CP=OFF, MCLRE=OFF

;##############################################################################
; CONSTANTS
;##############################################################################

; Timing constants (base tick = 20 ms)
TICKS_PER_SECOND        EQU     50          ; 50 * 20 ms = 1000 ms
SENSOR_CHECK_INTERVAL   EQU     60          ; Check sensor every 60 seconds
SECONDS_PER_STEP        EQU     5           ; Each step = 5 seconds pump time

; Step counter limits
STEP_COUNT_MIN          EQU     1           ; Minimum pump duration (5 seconds)
STEP_COUNT_MAX          EQU     10          ; Maximum pump duration (50 seconds)

; GPIO pin assignments
PIN_BARGRAPH            EQU     0           ; GP0: Bargraph clock output
PIN_SENSOR              EQU     1           ; GP1: Moisture sensor input
PIN_PUMP                EQU     2           ; GP2: Pump relay output
PIN_BUTTON              EQU     3           ; GP3: User button input

; GPIO direction mask (1=input, 0=output)
; GP3=input, GP2=output, GP1=input, GP0=output -> 0b00001010 = 0x0A
TRIS_MASK               EQU     0x0A

; OPTION register value
; Bit 7: GPWU = 1 (wake-up on pin change disabled)
; Bit 6: GPPU = 1 (weak pull-ups disabled)
; Bit 5: T0CS = 0 (Timer0 clock = internal Fosc/4, frees GP2)
; Bit 4: T0SE = 0 (Timer0 edge select, not used)
; Bit 3: PSA  = 1 (prescaler assigned to WDT, not Timer0)
; Bit 2-0: PS = 111 (prescaler rate, not used)
; Value: 0b11001111 = 0xCF
OPTION_INIT             EQU     0xCF

;##############################################################################
; RAM VARIABLES
;##############################################################################

        PSECT       udata, class=RAM, space=1

PREV_PRESSED:           DS  1               ; Button state: 0=released, 1=pressed
STEP_COUNT:             DS  1               ; Pump duration setting (1-10)
PULSE_SECONDS_REM:      DS  1               ; Remaining seconds for GP0 pulse
PUMP_SECONDS_REM:       DS  1               ; Remaining seconds for pump
TICK_COUNT:             DS  1               ; 20 ms tick counter (0-49)
CHECK_SECONDS_CNT:      DS  1               ; Seconds counter for sensor check
DELAY_OUTER:            DS  1               ; Delay loop outer counter
DELAY_INNER:            DS  1               ; Delay loop inner counter

;##############################################################################
; CODE SECTION
;##############################################################################

        PSECT       MyCode, class=CODE, delta=2
        GLOBAL      START
        ORG         0x000

;==============================================================================
; START - System initialization
;==============================================================================

START:
        ; Configure OPTION register
        ; T0CS=0 assigns Timer0 to internal clock, releasing GP2 for GPIO use
        movlw       OPTION_INIT
        option

        ; Configure GPIO directions
        movlw       TRIS_MASK
        tris        GPIO

        ; Initialize all outputs LOW (pump off, bargraph idle)
        clrf        GPIO

        ; Initialize RAM variables to known state
        clrf        PREV_PRESSED
        clrf        PULSE_SECONDS_REM
        clrf        PUMP_SECONDS_REM
        clrf        TICK_COUNT
        clrf        CHECK_SECONDS_CNT

        ; Set initial pump duration to minimum (5 seconds)
        movlw       STEP_COUNT_MIN
        movwf       STEP_COUNT

;==============================================================================
; MAIN_LOOP - Core timing loop (20 ms tick period)
;==============================================================================

MAIN_LOOP:
        call        DELAY_20MS

        ; Process button input on every tick for responsive UI
        call        HANDLE_BUTTON

        ; Accumulate ticks until 1 second elapsed
        incf        TICK_COUNT, f
        movlw       TICKS_PER_SECOND
        xorwf       TICK_COUNT, w
        btfss       STATUS, 2               ; STATUS.Z set if TICK_COUNT == 50
        goto        MAIN_LOOP

        ; One second has elapsed - reset tick counter
        clrf        TICK_COUNT

        ; Execute once-per-second tasks
        call        HANDLE_PULSE_SECOND
        call        HANDLE_PUMP_SECOND
        call        HANDLE_SENSOR_CHECK

        goto        MAIN_LOOP

;==============================================================================
; HANDLE_BUTTON - Process button input with edge detection
;
; Detects falling edge (new press) on GP3.
; On new press:
;   1. Starts GP0 indicator pulse (duration = current STEP_COUNT * 5 seconds)
;   2. Advances STEP_COUNT (1 -> 2 -> ... -> 10 -> 1)
;   3. Pulses bargraph clock to advance display
;
; Assumptions:
;   - Button is active LOW with external pull-up resistor
;   - Debouncing not required (20 ms sample rate provides adequate filtering)
;==============================================================================

HANDLE_BUTTON:
        ; Test current button state
        btfss       GPIO, PIN_BUTTON        ; Skip if GP3=1 (released)
        goto        BUTTON_PRESSED

        ; Button is released - clear history flag
        clrf        PREV_PRESSED
        retlw       0

BUTTON_PRESSED:
        ; Button is pressed - check if this is a new press
        movf        PREV_PRESSED, f
        btfss       STATUS, 2               ; Z=1 if PREV_PRESSED==0
        retlw       0                       ; Already pressed, ignore

        ; Record that button is now pressed
        movlw       1
        movwf       PREV_PRESSED

        ; Calculate pulse duration using CURRENT step count
        ; Duration = STEP_COUNT * 5 seconds (multiply by repeated addition)
        clrf        PULSE_SECONDS_REM
        movf        STEP_COUNT, w
        addwf       PULSE_SECONDS_REM, f    ; 1x
        addwf       PULSE_SECONDS_REM, f    ; 2x
        addwf       PULSE_SECONDS_REM, f    ; 3x
        addwf       PULSE_SECONDS_REM, f    ; 4x
        addwf       PULSE_SECONDS_REM, f    ; 5x

        ; Start GP0 indicator pulse
        bsf         GPIO, PIN_BARGRAPH

        ; Reset tick counter for accurate pulse timing
        clrf        TICK_COUNT

        ; Advance step counter with wraparound (1-10)
        incf        STEP_COUNT, f
        movlw       STEP_COUNT_MAX + 1      ; Value 11 triggers wrap
        xorwf       STEP_COUNT, w
        btfss       STATUS, 2
        goto        CLOCK_BARGRAPH

        ; Wrap to minimum
        movlw       STEP_COUNT_MIN
        movwf       STEP_COUNT

CLOCK_BARGRAPH:
        ; Generate short LOW pulse to clock CD4017 bargraph counter
        ; CD4017 advances on rising edge, so we pulse LOW then HIGH
        bcf         GPIO, PIN_BARGRAPH
        call        DELAY_20MS
        bsf         GPIO, PIN_BARGRAPH

        retlw       0

;==============================================================================
; HANDLE_PULSE_SECOND - Manage GP0 indicator pulse countdown
;
; Called once per second. Decrements pulse timer and turns off GP0 when
; timer reaches zero.
;==============================================================================

HANDLE_PULSE_SECOND:
        ; Check if pulse is active
        movf        PULSE_SECONDS_REM, f
        btfsc       STATUS, 2               ; Z=1 if timer is zero
        retlw       0                       ; No active pulse

        ; Decrement timer
        decfsz      PULSE_SECONDS_REM, f
        retlw       0                       ; Timer still running

        ; Timer expired - turn off GP0
        bcf         GPIO, PIN_BARGRAPH
        retlw       0

;==============================================================================
; HANDLE_PUMP_SECOND - Manage pump operation countdown
;
; Called once per second. Maintains pump output state and decrements timer.
; Turns off pump when timer reaches zero.
;==============================================================================

HANDLE_PUMP_SECOND:
        ; Check if pump is active
        movf        PUMP_SECONDS_REM, f
        btfsc       STATUS, 2               ; Z=1 if timer is zero
        goto        PUMP_OFF

        ; Pump is running - ensure output is HIGH
        bsf         GPIO, PIN_PUMP

        ; Decrement timer
        decfsz      PUMP_SECONDS_REM, f
        retlw       0                       ; Timer still running

        ; Timer expired - fall through to turn off pump

PUMP_OFF:
        bcf         GPIO, PIN_PUMP
        retlw       0

;==============================================================================
; HANDLE_SENSOR_CHECK - Periodic soil moisture monitoring
;
; Called once per second. Counts seconds until check interval elapsed,
; then reads sensor and starts pump if soil is dry.
;
; Conditions for pump start:
;   - Check interval has elapsed (60 seconds)
;   - Pump is not already running
;   - Sensor indicates dry soil (GP1 = HIGH)
;==============================================================================

HANDLE_SENSOR_CHECK:
        ; Accumulate seconds until check interval
        incf        CHECK_SECONDS_CNT, f
        movlw       SENSOR_CHECK_INTERVAL
        xorwf       CHECK_SECONDS_CNT, w
        btfss       STATUS, 2               ; Z=1 if interval elapsed
        retlw       0

        ; Interval elapsed - reset counter
        clrf        CHECK_SECONDS_CNT

        ; Do not start pump if already running
        movf        PUMP_SECONDS_REM, f
        btfss       STATUS, 2               ; Z=1 if pump idle
        retlw       0

        ; Read moisture sensor (GP1: HIGH=dry, LOW=wet)
        btfss       GPIO, PIN_SENSOR
        retlw       0                       ; Soil is wet, no action

        ; Soil is dry - start pump for STEP_COUNT * 5 seconds
        clrf        PUMP_SECONDS_REM
        movf        STEP_COUNT, w
        addwf       PUMP_SECONDS_REM, f     ; 1x
        addwf       PUMP_SECONDS_REM, f     ; 2x
        addwf       PUMP_SECONDS_REM, f     ; 3x
        addwf       PUMP_SECONDS_REM, f     ; 4x
        addwf       PUMP_SECONDS_REM, f     ; 5x

        ; Activate pump output
        bsf         GPIO, PIN_PUMP

        retlw       0

;==============================================================================
; DELAY_20MS - Software delay loop
;
; Generates approximately 20 ms delay at 4 MHz oscillator.
;
; Calculation:
;   Inner loop: 83 iterations * 3 cycles = 249 cycles
;   Outer loop: 80 iterations * (249 + 3) = 80 * 252 = 20,160 cycles
;   At 1 us/cycle: 20,160 us = 20.16 ms
;
; Note: Actual timing varies with oscillator tolerance.
;==============================================================================

DELAY_20MS:
        movlw       80
        movwf       DELAY_OUTER

DELAY_20MS_OUTER:
        movlw       83
        movwf       DELAY_INNER

DELAY_20MS_INNER:
        decfsz      DELAY_INNER, f
        goto        DELAY_20MS_INNER
        decfsz      DELAY_OUTER, f
        goto        DELAY_20MS_OUTER

        retlw       0

;##############################################################################
; END OF PROGRAM
;##############################################################################

        END