;
; Konami VRC6 expansion sound
;

VRC6_CH1 = 4
VRC6_CH2 = 5
VRC6_CH3 = 6

ft_update_vrc6:
	lda var_PlayerFlags
	bne @Play
	lda #$00
	sta $9002
	sta $A002
	sta $B002
	rts
@Play:

; ==============================================================================
; Pulse 1
; ==============================================================================

	lda var_ch_Note + VRC6_CH1				; Kill channel if note = off
	beq @KillChan1
	; Load volume
	lda var_ch_VolColumn + VRC6_CH1			; Kill channel if volume column = 0
	asl a
	beq @KillChan1
	and #$F0
	sta var_Temp
	lda var_ch_Volume + VRC6_CH1			; Kill channel if volume = 0
	beq @KillChan1
	ora var_Temp
	tax
	lda ft_volume_table, x					; Load from the 16*16 volume table
    sec
    sbc var_ch_TremoloResult + VRC6_CH1
    bpl :+
    lda #$00
:   bne :+
    lda var_ch_VolColumn + VRC6_CH1
    beq :+
    lda #$01
:

	; Pulse width
	pha
	lda var_ch_DutyCycle + VRC6_CH1
	and #$0F
	tax
	pla
	ora ft_duty_table_vrc6, x
	bne :+
	brk
:
	; Write to registers
	sta $9000
	lda	var_ch_PeriodCalcLo + VRC6_CH1
	sta $9001
	lda	var_ch_PeriodCalcHi + VRC6_CH1
	ora #$80
	sta $9002
	bmi @VRC6_Chan2         ; Branch always
@KillChan1:
	lda #$00
	sta $9002

; ==============================================================================
;  Pulse 2
; ==============================================================================

@VRC6_Chan2:
	lda var_ch_Note + VRC6_CH2				; Kill channel if note = off
	beq @KillChan2
	; Load volume
	lda var_ch_VolColumn + VRC6_CH2			; Kill channel if volume column = 0
	asl a
	beq @KillChan2
	and #$F0
	sta var_Temp
	lda var_ch_Volume + VRC6_CH2			; Kill channel if volume = 0
	beq @KillChan2
	ora var_Temp
	tax
	lda ft_volume_table, x
    sec
    sbc var_ch_TremoloResult + VRC6_CH2
    bpl :+
    lda #$00
:   bne :+
    lda var_ch_VolColumn + VRC6_CH2
    beq :+
    lda #$01
:

	; Pulse width
	pha
	lda var_ch_DutyCycle + VRC6_CH2
	and #$0F
	tax
	pla

	ora ft_duty_table_vrc6, x
	; Write to registers
	sta $A000
	lda	var_ch_PeriodCalcLo + VRC6_CH2
	sta $A001
	lda	var_ch_PeriodCalcHi + VRC6_CH2
	ora #$80
	sta $A002
	bmi @VRC6_Chan3           ; Branch always
@KillChan2:
	lda #$00
	sta $A002

; ==============================================================================
;  Update Sawtooth
; ==============================================================================

@VRC6_Chan3:
	lda var_ch_Note + VRC6_CH3				; Kill channel if note = off
	beq @KillChan3
	; Load volume
	lda var_ch_VolColumn + VRC6_CH3			; Kill channel if volume column = 0
	asl a
 	beq @KillChan3
	and #$F0
	sta var_Temp
	lda var_ch_Volume + VRC6_CH3			; Kill channel if volume = 0
	ora var_ch_DutyCycle + VRC6_CH3
	beq @KillChan3
	lda var_ch_Volume + VRC6_CH3
	ora var_Temp
	tax
	lda ft_volume_table, x
    sec
    sbc var_ch_TremoloResult + VRC6_CH3
    bpl :+
    lda #$00
:   bne :+
    lda var_ch_VolColumn + VRC6_CH3
    beq :+
    lda #$01
:
	; Use pulse width table t4o get the high part of volume
    pha
	lda var_ch_DutyCycle + VRC6_CH3
	and #$0F
	tax
	pla

	ora ft_duty_table_vrc6, x
	asl a
	and #$3F
	sta $B000
	lda	var_ch_PeriodCalcLo + VRC6_CH3
	sta $B001
	lda	var_ch_PeriodCalcHi + VRC6_CH3
	ora #$80
	sta $B002
	rts
@KillChan3:
	lda #$00
	sta $B002
	rts

ft_duty_table_vrc6:
	.byte $00, $10, $20, $30, $40, $50, $60, $70
