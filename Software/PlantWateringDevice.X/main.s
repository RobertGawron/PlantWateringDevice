PROCESSOR   10F202
        #include    <xc.inc>

; =============================================================================
; TEST 6 - Complete system with corrected pulse timing:
; - Button advances step counter 1-10 (wraps), pulses bargraph
; - GP0 pulse length = step_count * 5 seconds (shows CURRENT before advance)
; - Every 60 seconds: if GP1=1 (dry), pump runs for step_count * 5 seconds
; - GP2 = pump output
; =============================================================================

        CONFIG      WDTE=OFF, CP=OFF, MCLRE=OFF

        PSECT udata, class=RAM, space=1
prev_pressed:        DS  1
step_count:          DS  1
pulse_seconds_rem:   DS  1
pump_seconds_rem:    DS  1
tick_count:          DS  1
check_seconds_cnt:   DS  1
delay1:              DS  1
delay2:              DS  1

        PSECT MyCode, class=CODE, delta=2
        GLOBAL start
        ORG     0x000

start:
        movlw   0xCF
        option
        
        movlw   0x0A
        tris    GPIO
        clrf    GPIO
        clrf    prev_pressed
        clrf    pulse_seconds_rem
        clrf    pump_seconds_rem
        clrf    tick_count
        clrf    check_seconds_cnt
        
        movlw   1
        movwf   step_count

loop:
        call    Delay_20ms
        
        call    HandleButton
        
        incf    tick_count, f
        movlw   50
        xorwf   tick_count, w
        btfss   STATUS, 2
        goto    loop
        
        clrf    tick_count
        
        call    HandlePulseSecond
        call    HandlePumpSecond
        call    HandleSensorCheck
        
        goto    loop

; -----------------------------------------------------------------------------
; HandleButton - pulse shows CURRENT value, then advances bargraph
; -----------------------------------------------------------------------------
HandleButton:
        btfss   GPIO, 3
        goto    hb_pressed_now
        
        clrf    prev_pressed
        retlw   0

hb_pressed_now:
        movf    prev_pressed, f
        btfss   STATUS, 2
        retlw   0
        
        movlw   1
        movwf   prev_pressed
        
        ; Calculate pulse duration with CURRENT step_count
        clrf    pulse_seconds_rem
        movf    step_count, w
        addwf   pulse_seconds_rem, f
        addwf   pulse_seconds_rem, f
        addwf   pulse_seconds_rem, f
        addwf   pulse_seconds_rem, f
        addwf   pulse_seconds_rem, f
        
        ; Start long pulse
        bsf     GPIO, 0
        clrf    tick_count
        
        ; Now advance step counter 1-10
        incf    step_count, f
        movlw   11
        xorwf   step_count, w
        btfss   STATUS, 2
        goto    hb_do_bargraph_pulse
        
        movlw   1
        movwf   step_count

hb_do_bargraph_pulse:
        ; Short off-pulse to clock the 4017
        bcf     GPIO, 0
        call    Delay_20ms
        bsf     GPIO, 0
        
        retlw   0

; -----------------------------------------------------------------------------
; HandlePulseSecond
; -----------------------------------------------------------------------------
HandlePulseSecond:
        movf    pulse_seconds_rem, f
        btfsc   STATUS, 2
        retlw   0
        
        decfsz  pulse_seconds_rem, f
        retlw   0
        
        bcf     GPIO, 0
        retlw   0

; -----------------------------------------------------------------------------
; HandlePumpSecond
; -----------------------------------------------------------------------------
HandlePumpSecond:
        movf    pump_seconds_rem, f
        btfsc   STATUS, 2
        goto    hps_pump_off
        
        bsf     GPIO, 2
        decfsz  pump_seconds_rem, f
        retlw   0
        
hps_pump_off:
        bcf     GPIO, 2
        retlw   0

; -----------------------------------------------------------------------------
; HandleSensorCheck - every 60 seconds
; -----------------------------------------------------------------------------
HandleSensorCheck:
        incf    check_seconds_cnt, f
        movlw   60
        xorwf   check_seconds_cnt, w
        btfss   STATUS, 2
        retlw   0
        
        clrf    check_seconds_cnt
        
        ; Don't start if pump already running
        movf    pump_seconds_rem, f
        btfss   STATUS, 2
        retlw   0
        
        ; Check sensor: GP1=1 means dry
        btfss   GPIO, 1
        retlw   0
        
        ; Start pump: step_count * 5 seconds
        clrf    pump_seconds_rem
        movf    step_count, w
        addwf   pump_seconds_rem, f
        addwf   pump_seconds_rem, f
        addwf   pump_seconds_rem, f
        addwf   pump_seconds_rem, f
        addwf   pump_seconds_rem, f
        
        bsf     GPIO, 2
        retlw   0

; -----------------------------------------------------------------------------
; Delay_20ms
; -----------------------------------------------------------------------------
Delay_20ms:
        movlw   80
        movwf   delay2
d_outer:
        movlw   83
        movwf   delay1
d_inner:
        decfsz  delay1, f
        goto    d_inner
        decfsz  delay2, f
        goto    d_outer
        retlw   0

        END