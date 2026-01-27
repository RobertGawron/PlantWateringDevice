; Test setup:
; 1/ Instead of connecting a pump, connect an LED with 470R resistor to GP0 or GP2.
;
; Test expectation:
; 1/ LED connected instead of a pump should blink 
; 2/ Bargraph connected to GP2 should circularly increase shown value. 
;    When the value goes to max, it will go back to minimum.

        PROCESSOR   10F202
        #include    <xc.inc>

        ; CONFIG: Watchdog Off, Code Protect Off, MCLR pin enabled (needed for ICSP reliability)
        CONFIG      WDTE=OFF, CP=OFF, MCLRE=ON

HW_GPIO_REG     EQU     0x06

        PSECT udata, class=RAM, space=1
ut_delay_inner_cnt:      DS  1
ut_delay_outer_cnt:      DS  1

        PSECT MyCode, class=CODE, delta=2
        GLOBAL start
        ORG     0x000

start:
        ; Configure OPTION register.
        ; 0xDF = 1101 1111b sets:
        ;   - T0CS=1  : TMR0 clock source = external pin (not used here)
        ;   - PSA=1   : prescaler assigned to WDT (irrelevant because WDTE=OFF)
        ; Other bits left at '1' defaults (weak pull-ups disabled, etc. depending on device).
        ; This line is not required for GPIO blinking; it just forces a known OPTION value.
        movlw   0xDF
        option

        ; --- Configure GPIO direction (TRIS: 1=input, 0=output) ---
        movlw   0x0A            ; GP0=Out, GP1=In (ICSPCLK), GP2=Out, GP3=In (MCLR/VPP)
        tris    HW_GPIO_REG     ; Write W into TRIS for GPIO

        clrf    HW_GPIO_REG     ; Initialize outputs low

L_MAIN_LOOP:
        call    Delay_200ms

        ; Toggle GP0 and GP2 (bits 0 and 2) without affecting other pins.
        movlw   0x05            ; 0000 0101b -> toggle GP2 and GP0
        xorwf   HW_GPIO_REG, f

        goto    L_MAIN_LOOP

Delay_200ms:
        movlw   70
        movwf   ut_delay_outer_cnt
L_D200_OUTER:
        movlw   250
        movwf   ut_delay_inner_cnt
L_D200_INNER:
        decfsz  ut_delay_inner_cnt, f
        goto    L_D200_INNER
        decfsz  ut_delay_outer_cnt, f
        goto    L_D200_OUTER
        retlw   0