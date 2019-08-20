;
; Nintendo MMC5 expansion sound
;

MMC5_CH1 = 4
MMC5_CH2 = 5

ft_update_mmc5:
	lda var_PlayerFlags
	bne @Play
	lda #$00
	sta $5015
	rts
@Play:
	; MMC5 Square 1
	lda var_ch_Note + MMC5_CH1		; Kill channel if note = off
	beq @KillSquare1
	; Calculate volume
	lda var_ch_VolColumn + MMC5_CH1		; Kill channel if volume column = 0
	asl a
	and #$F0
	beq @KillSquare1
	sta var_Temp
	lda var_ch_Volume + MMC5_CH1		; Kill channel if volume = 0
	beq @KillSquare1
	ora var_Temp
	tay
	; Write to registers
	lda var_ch_DutyCycle + MMC5_CH1
	and #$03
	tax
	lda ft_volume_table, y
    sec
    sbc var_ch_TremoloResult + MMC5_CH1
    bpl :+
    lda #$00
:
	ora ft_duty_table, x		; Add volume
	ora #$30					; and disable length counter and envelope
	sta $5000
	; Period table isn't limited to $7FF anymore
	lda var_ch_PeriodCalcHi + MMC5_CH1
	and #$F8
	beq :+
	lda #$03
	sta var_ch_PeriodCalcHi + MMC5_CH1
	lda #$FF
	sta var_ch_PeriodCalcLo + MMC5_CH1
:	lda var_ch_PeriodCalcLo + MMC5_CH1
	sta $5002
	lda var_ch_PeriodCalcHi + MMC5_CH1
	cmp var_ch_PrevFreqHighMMC5
	beq :+
	sta $5003
	sta var_ch_PrevFreqHighMMC5
:	jmp @Square2
@KillSquare1:
	lda #$30
	sta $5000
@Square2: 	; MMC5 Square 2
	lda var_ch_Note + MMC5_CH2		; Kill channel if note = off
	beq @KillSquare2
	; Calculate volume	
	lda var_ch_VolColumn + MMC5_CH2		; Kill channel if volume column = 0
	asl a
	and #$F0
	beq @KillSquare2
	sta var_Temp
	lda var_ch_Volume + MMC5_CH2		; Kill channel if volume = 0
	beq @KillSquare2
	ora var_Temp
	tay
	; Write to registers
	lda var_ch_DutyCycle + MMC5_CH2
	and #$03
	tax
	lda ft_volume_table, y
    sec
    sbc var_ch_TremoloResult + MMC5_CH2
    bpl :+
    lda #$00
:
	ora ft_duty_table, x		; Add volume
	ora #$30					; and disable length counter and envelope
	sta $5004
	; Period table isn't limited to $7FF anymore
	lda var_ch_PeriodCalcHi + MMC5_CH2
	and #$F8
	beq :+
	lda #$03
	sta var_ch_PeriodCalcHi + MMC5_CH2
	lda #$FF
	sta var_ch_PeriodCalcLo + MMC5_CH2
:	lda var_ch_PeriodCalcLo + MMC5_CH2
	sta $5006
	lda var_ch_PeriodCalcHi + MMC5_CH2
	cmp var_ch_PrevFreqHighMMC5 + 1
	beq @Return
	sta $5007
	sta var_ch_PrevFreqHighMMC5 + 1
@Return:
	rts
@KillSquare2:
	lda #$30
	sta $5004
	rts
	