; Takes care of the FDS registers

ft_fds_volume:
    lda var_Temp				; 5x4 multiplication
	lsr var_Temp2
    bcs :+
    lsr a
:   lsr var_Temp2
    bcc :+
    adc var_Temp
:   lsr a
    lsr var_Temp2
    bcc :+
    adc var_Temp
:   lsr a
    lsr var_Temp2
    bcc :+
    adc var_Temp
:   lsr a
	beq :+
	rts
:	lda var_Temp
	ora var_ch_Volume + FDS_CHANNEL
	beq :+
	lda #$01					; Round up to 1
:	rts

ft_init_fds:
    lda #$00
    sta $4023
    lda #$83
    sta $4023
    rts

; Update FDS
ft_update_fds:
	lda var_PlayerFlags
	bne @Play
	lda #$80
	sta $4080
	rts
@Play:

	lda var_ch_Note + FDS_CHANNEL
	beq @KillFDS

	; Calculate volume
	lda var_ch_VolColumn + FDS_CHANNEL		; Kill channel if volume column = 0
	lsr a
	lsr a
	lsr a
	beq @KillFDS
	sta var_Temp2							; 4 bit vol
	lda var_ch_Volume + FDS_CHANNEL		; Kill channel if volume = 0
	beq @KillFDS
	sta var_Temp							; 5 bit vol
	jsr ft_fds_volume
    sec
    sbc var_ch_TremoloResult + FDS_CHANNEL
    bpl :+
    lda #$00
:
	; Load volume
	ora #$80								; Disable the volume envelope
	sta $4080								; Volume

	; Load frequency
	lda var_ch_PeriodCalcHi + FDS_CHANNEL
	and #$F0
	beq :+
	lda #$FF
	sta var_ch_PeriodCalcLo + FDS_CHANNEL
	lda #$0F
	sta var_ch_PeriodCalcHi + FDS_CHANNEL
:	lda var_ch_PeriodCalcHi + FDS_CHANNEL
	sta $4083	; High
	lda var_ch_PeriodCalcLo + FDS_CHANNEL
	sta $4082	; Low

    lda var_ch_ResetMod
    beq :+
    lda #$00
    sta $4085
    sta var_ch_ResetMod
:

	lda var_ch_ModDelayTick					; Modulation delay
	bne @TickDownDelay
;	lda var_ch_ModDepth						; Skip if modulation is disabled
;	beq @DisableMod

	lda var_ch_ModDepth						; Skip if modulation is disabled
	ora #$80
	sta $4084								; Store modulation depth

	lda var_ch_ModRate						; Modulation freq
	sta $4086
	lda var_ch_ModRate + 1
	sta $4087

@Return:
	rts
@TickDownDelay:
	dec var_ch_ModDelayTick
@DisableMod:
	; Disable modulation
	lda #$80
	sta $4087
	rts
@KillFDS:
	lda #$80
	sta $4080	; Make channel silent
	lda #$80
	sta $4084
	sta $4087
	rts

; Load the waveform, index in A
ft_load_fds_wave:
	sta var_Temp16
	lda #$00
	sta var_Temp16 + 1
	; Multiply by 64
	clc
	ldy #$06
:	rol var_Temp16
	rol var_Temp16 + 1
	dey
	bne :-
	; Setup a pointer to the specified wave
	clc
	lda var_Wavetables
	adc var_Temp16
	sta var_Temp16
	lda var_Wavetables + 1
	adc var_Temp16 + 1
	sta var_Temp16 + 1
	; Write wave
	lda #$80
	sta $4089		; Enable wave RAM
	ldy #$00
:	lda (var_Temp16), y	          	; 5
	sta $4040, y					; 5
	iny								; 2
	cpy #$40						; 2
	bne :-							; 3 = 17 cycles and 64 iterations = 1088 cycles
	lda #$00
	sta $4089		; Disable wave RAM
	rts

ft_reset_modtable:
	lda #$80
	sta $4087
	lda #$00
	sta $4085
	rts

ft_check_fds_effects:
    lda var_ch_ModEffWritten
	and #$01
    beq :+
    ; FDS modulation depth
    lda var_ch_ModEffDepth
    sta var_ch_ModDepth
:   lda var_ch_ModEffWritten
	and #$02
    beq :+
    ; FDS modulation rate high
    lda var_ch_ModEffRateHi
    sta var_ch_ModRate + 1
:   lda var_ch_ModEffWritten
	and #$04
    beq :+
    ; FDS modulation rate low
    lda var_ch_ModEffRateLo
    sta var_ch_ModRate + 0
:
 	lda #$00
 	sta var_ch_ModEffWritten

    rts
