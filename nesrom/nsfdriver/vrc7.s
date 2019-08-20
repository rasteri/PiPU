; VRC7 commands
;  0 = halt
;  1 = trigger
;  80 = update

VRC7_HALT       = $00
VRC7_TRIGGER    = $01
VRC7_HOLD_NOTE  = $80

ft_init_vrc7:
    lda #$00
    tax
:   stx $9010
    jsr ft_vrc7_delay
    sta $9030
    jsr ft_vrc7_delay
    inx
    cpx #$3F
    bne :-
    rts

ft_translate_note_vrc7:
	; Calculate Fnum & Bnum
	; Input: A = note + 1
	; Result: EXT = Fnum index, ACC = Bnum
	sta ACC
	lda #12
	sta AUX
	lda #$00
	sta ACC + 1
	sta AUX + 1
	jsr DIV
	rts

ft_clear_vrc7:
	clc
	txa
	adc #$20	; $20: Clear channel
	sta $9010
	jsr ft_vrc7_delay
 	lda var_ch_vrc7_FnumHi, x
	ora var_ch_vrc7_Bnum, x
	sta $9030
	jsr ft_vrc7_delay
	rts

; Update all VRC7 channel registers
;
ft_update_vrc7:

	lda var_PlayerFlags
	bne @Play
	; Close all channels
	ldx #$06
:	txa
	clc
	adc #$1F
	sta $9010
	jsr ft_vrc7_delay
	lda #$00
	sta $9030
	jsr ft_vrc7_delay
	dex
	bne :-
	rts
@Play:

	ldx #$00					; x = channel
@LoopChannels:

    ; Check note off
	lda var_ch_Note + VRC7_CHANNEL, x
	bne :+
	lda #VRC7_HALT
	sta var_ch_vrc7_Command, x

    ; See if retrigger is needed
:	lda var_ch_vrc7_Command, x
	cmp #VRC7_TRIGGER
	bne @UpdateChannel

	; Clear channel, this also serves as a retrigger
	jsr ft_clear_vrc7

	; Remove trigger command
 	lda #VRC7_HOLD_NOTE
	sta var_ch_vrc7_Command, x

@UpdateChannel:
    ; Load and cache period if there is an active note
    lda var_ch_Note + VRC7_CHANNEL, x
    beq @SkipPeriod
	lda var_ch_PeriodCalcLo + VRC7_CHANNEL, x
	sta var_ch_vrc7_FnumLo, x
	lda var_ch_PeriodCalcHi + VRC7_CHANNEL, x
	and #$07
	sta var_ch_vrc7_FnumHi, x
@SkipPeriod:

	clc
	txa
	adc #$10	; $10: Low part of Fnum
	sta $9010
	jsr ft_vrc7_delay
    lda var_ch_vrc7_FnumLo, x
	sta $9030
	jsr ft_vrc7_delay

	; Note on or off
	lda #$00
	sta var_Temp2
	; Skip if halt
	lda var_ch_vrc7_Command, x
	beq :+

	; Check release
	lda var_ch_State + VRC7_CHANNEL, x
	and #$01
	tay
	lda ft_vrc7_cmd, y
	sta var_Temp2

:	clc
	txa
	adc #$30	; $30: Patch & Volume
	sta $9010
	jsr ft_vrc7_delay
	lda var_ch_VolColumn + VRC7_CHANNEL, x
	lsr a
	lsr a
	lsr a
    sec
    sbc var_ch_TremoloResult + VRC7_CHANNEL, x
    bpl :+
    lda #$00
:	eor #$0F
	ora var_ch_vrc7_Patch, x
	sta $9030
	jsr ft_vrc7_delay

	clc
	txa
	adc #$20	; $20: High part of Fnum, Bnum, Note on & sustain on
	sta $9010
	jsr ft_vrc7_delay
	lda var_ch_vrc7_Bnum, x
	asl a
	ora var_ch_vrc7_FnumHi, x
	ora var_Temp2
	sta $9030
	jsr ft_vrc7_delay


@NextChan:
	inx
	cpx #$06
	beq :+
	jmp @LoopChannels
:	rts

; Used to adjust Bnum when portamento is used
;
ft_vrc7_adjust_octave:

	; Get octave
	lda var_ch_vrc7_ActiveNote - VRC7_CHANNEL, x
	sta ACC
	lda #12
	sta AUX
	lda #$00
	sta ACC + 1
	sta AUX + 1
	tya
	pha
	jsr DIV
	pla
	tay

	lda	ACC					; if new octave > old octave
	cmp var_ch_vrc7_OldOctave
	bcs :+
	; Old octave > new octave, shift down portamento frequency
	lda var_ch_vrc7_OldOctave
	sta var_ch_vrc7_Bnum - VRC7_CHANNEL, x
	sec
	sbc ACC
	jsr @ShiftFreq2
	rts
:	lda	var_ch_vrc7_OldOctave	; if old octave > new octave
	cmp ACC
	bcs @Return
	; New octave > old octave, shift down old frequency
	lda ACC
	sta var_ch_vrc7_Bnum - VRC7_CHANNEL, x
	sec
	sbc var_ch_vrc7_OldOctave
	jsr @ShiftFreq
@Return:
	rts

@ShiftFreq:
	sty var_Temp
	tay
:	lsr var_ch_TimerPeriodHi, x
	ror var_ch_TimerPeriodLo, x
	dey
	bne :-
	ldy var_Temp
	rts

@ShiftFreq2:
	sty var_Temp
	tay
:	lsr var_ch_PortaToHi, x
	ror var_ch_PortaToLo, x
	dey
	bne :-
	ldy var_Temp
	rts

; Called when a new note is found from pattern reader
ft_vrc7_trigger:

    lda var_ch_vrc7_Patch - VRC7_CHANNEL, x
    bne @SkipCustomPatch

    lda var_ch_vrc7_CustomLo - VRC7_CHANNEL, x
    sta var_CustomPatchPtr
    lda var_ch_vrc7_CustomHi - VRC7_CHANNEL, x
    sta var_CustomPatchPtr + 1
    jsr ft_load_vrc7_custom_patch
@SkipCustomPatch:
 	cpx #$06
 	bne :+
:   lda var_ch_Effect, x
    cmp #EFF_PORTAMENTO
   	bne :+
    lda var_ch_vrc7_Command - VRC7_CHANNEL, x
    bne :++
:	lda #VRC7_TRIGGER							; Trigger VRC7 channel
	sta var_ch_vrc7_Command - VRC7_CHANNEL, x
	; Adjust Fnum if portamento is enabled
:	lda var_ch_Effect, x
	cmp #EFF_PORTAMENTO
	bne @Return
	; Load portamento
	lda var_ch_Note, x
	beq @Return
	lda var_ch_vrc7_OldOctave
	bmi @Return
	jsr ft_vrc7_adjust_octave
@Return:
	rts

ft_vrc7_get_freq:

	tya
	pha

	lda var_ch_vrc7_Command - VRC7_CHANNEL, x
	cmp #VRC7_HALT
	bne :+
	; Clear old frequency if channel was halted
	lda #$00
	sta var_ch_TimerPeriodLo, x
	sta var_ch_TimerPeriodHi, x

:	lda var_ch_vrc7_Bnum - VRC7_CHANNEL, x
	sta var_ch_vrc7_OldOctave

	; Retrigger channel
	lda var_ch_vrc7_ActiveNote - VRC7_CHANNEL, x
	jsr ft_translate_note_vrc7
	ldy EXT	; note index -> y

	lda var_ch_Effect, x
	cmp #EFF_PORTAMENTO
	bne @NoPorta
	lda ft_note_table_vrc7_l, y
	sta var_ch_PortaToLo, x
	lda ft_note_table_vrc7_h, y
	sta var_ch_PortaToHi, x

	; Check if previous note was silent, move this frequency directly to it
	lda var_ch_TimerPeriodLo, x
	ora var_ch_TimerPeriodHi, x
	bne :+

	lda var_ch_PortaToLo, x
	sta var_ch_TimerPeriodLo, x
	lda var_ch_PortaToHi, x
	sta var_ch_TimerPeriodHi, x

	lda #$80				; Indicate new note (no previous)
	sta var_ch_vrc7_OldOctave

	jmp :+

@NoPorta:
	lda ft_note_table_vrc7_l, y
	sta var_ch_TimerPeriodLo, x
	lda ft_note_table_vrc7_h, y
	sta var_ch_TimerPeriodHi, x

:	lda ACC
	sta var_ch_vrc7_Bnum - VRC7_CHANNEL, x

	pla
	tay

	lda #$00
	sta var_ch_State, x

	; VRC7 patch

	lda var_ch_vrc7_EffPatch
	cmp #$FF
	beq :+
	and #$F0
	sta var_ch_vrc7_Patch - VRC7_CHANNEL, x
	rts
:;	lda var_ch_vrc7_DefPatch - VRC7_CHANNEL, x
	;sta var_ch_vrc7_Patch - VRC7_CHANNEL, x
	rts

ft_vrc7_get_freq_only:
	tya
	pha

	; Retrigger channel
	lda var_ch_vrc7_ActiveNote - VRC7_CHANNEL, x
	jsr ft_translate_note_vrc7
	ldy EXT	; note index -> y

	lda ft_note_table_vrc7_l, y
	sta var_ch_TimerPeriodLo, x
	lda ft_note_table_vrc7_h, y
	sta var_ch_TimerPeriodHi, x

	lda var_ch_vrc7_Bnum - VRC7_CHANNEL, x
	sta var_ch_vrc7_OldOctave

	lda ACC
	sta var_ch_vrc7_Bnum - VRC7_CHANNEL, x

	lda #$00
	sta var_ch_State, x

	pla
	tay

	rts

; Setup note slides
;
ft_vrc7_load_slide:

	lda var_ch_TimerPeriodLo, x
	pha
	lda var_ch_TimerPeriodHi, x
	pha

	; Load note
	lda var_ch_EffParam, x			; Store speed
	and #$0F						; Get note
	sta var_Temp					; Store note in temp

	lda var_ch_Effect, x
	cmp #EFF_SLIDE_UP_LOAD
	beq :+
	lda var_ch_Note, x
	sec
	sbc var_Temp
	jmp :++
:	lda var_ch_Note, x
	clc
	adc var_Temp
:	sta var_ch_Note, x

	jsr ft_translate_freq_only

	lda var_ch_TimerPeriodLo, x
	sta var_ch_PortaToLo, x
	lda var_ch_TimerPeriodHi, x
	sta var_ch_PortaToHi, x

    ; Store speed
	lda var_ch_EffParam, x
	lsr a
	lsr a
	lsr a
	ora #$01
	sta var_ch_EffParam, x

    ; Load old period
	pla
	sta var_ch_TimerPeriodHi, x
	pla
	sta var_ch_TimerPeriodLo, x

    ; change mode to sliding
	clc
	lda var_ch_Effect, x
	cmp #EFF_SLIDE_UP_LOAD
	bne :+
	lda #EFF_SLIDE_DOWN
	sta var_ch_Effect, x
	jsr ft_vrc7_adjust_octave
	rts
:	lda #EFF_SLIDE_UP
	sta var_ch_Effect, x
	jsr ft_vrc7_adjust_octave
    rts

; Load VRC7 custom patch registers
ft_load_vrc7_custom_patch:
    tya
    pha
    ldy #$00
:	lda (var_CustomPatchPtr), y		            ; Load register
	sty $9010						            ; Register index
	jsr ft_vrc7_delay
	sta $9030						            ; Store the setting
	jsr ft_vrc7_delay
	iny
	cpy #$08
	bne :-
	pla
	tay
    rts

ft_vrc7_delay:
	pha
	lda	#$01
:	asl	a
	bcc	:-
	pla
	rts

; Fnum table, multiplied by 4 for higher resolution
.define ft_vrc7_table 688, 732, 776, 820, 868, 920, 976, 1032, 1096, 1160, 1228, 1304

ft_note_table_vrc7_l:
    .lobytes ft_vrc7_table
ft_note_table_vrc7_h:
    .hibytes ft_vrc7_table
ft_vrc7_cmd:
	.byte $30, $20
