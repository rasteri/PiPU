;
; Namco 163 expansion sound
;

N163_CH1 = 4
N163_CH2 = 5
N163_CH3 = 6
N163_CH4 = 7
N163_CH5 = 8
N163_CH6 = 9
N163_CH7 = 10
N163_CH8 = 11

ft_init_n163:
    ; Enable sound, copied from PPMCK and verified on a real Namco cart
    ; no sound without this!
    lda #$20
    sta $E000
    ; Enable all channels
    lda #$7F
    sta $F800
    lda #$70
    sta $4800
    ; Clear wave ram
    lda #$80
    sta $F800
    ldx #$40
    lda #$00
:   sta $4800
    dex
    bne :-
    ldx #$07
    lda #$00
:   sta var_ch_N163_LastHiFreq, x
    dex
    bpl :-

    rts

; Update N163 channels
ft_update_n163:
    ; Check player flag
	lda var_PlayerFlags
	bne @Play
	; Kill all channels
	lda #$C0
	sta $F800
	tax
	lda #$00
:   sta $4800
    dex
    bne :-
	rts
@Play:

    ; x = channel
    ldx #$00
@ChannelLoop:
    ; Begin
    ; Set address
    lda #$00
    jsr @LoadAddr

	lda var_ch_Note + N163_OFFSET, x
	beq @KillChannel  ; out of range

	; Get volume
	lda var_ch_VolColumn + N163_OFFSET, x		; Kill channel if volume column = 0
	asl a
	beq @KillChannel
	and #$F0
	sta var_Temp
	lda var_ch_Volume + N163_OFFSET, x
	beq @KillChannel
	ora var_Temp
	tay
	lda ft_volume_table, y
    sec
    sbc var_ch_TremoloResult + N163_OFFSET, x
    bpl :+
    lda #$00
:   bne :+
    lda var_ch_VolColumn + N163_OFFSET, x
    beq :+
    lda #$01
:

	sta var_Temp

	; Load frequency regs
    jsr @LoadPeriod

	; Write regs
	lda var_Temp2
	sta $4800 	; Low part of freq

    lda #$02
    jsr @LoadAddr

	lda var_Temp3
	sta $4800 	; Middle part of freq

    lda #$04
    jsr @LoadAddr

	lda var_Temp4
	and #$03
	sta var_Temp4

    lda var_ch_WaveLen, x
    sec
    sbc #$01
    lsr a
    eor #$FF
    asl a
    asl a
    ora var_Temp4
	sta $4800 	; Wave size, High part of frequency

    lda #$06
    jsr @LoadAddr

    lda var_ch_WavePos, x
	sta $4800 	; Wave position

    lda var_NamcoChannelsReg
    ora var_Temp
	sta $4800 	; Volume

	jmp @SkipChannel

@KillChannel:
    ldy #$07
:   sta $4800
    dey
    bne :-
    lda var_NamcoChannelsReg
    sta $4800

@SkipChannel:
    ; End
    inx
    cpx var_NamcoChannels
    beq :+
    jmp @ChannelLoop
:	rts

@LoadAddr:                    ; Load N163 RAM address
    clc
    adc ft_n163_chan_addr, x
    ora #$80	              ; Auto increment
    sta $F800
    rts

@LoadPeriod:
	lda var_ch_PeriodCalcLo + N163_OFFSET, x
	sta var_Temp2
	lda var_ch_PeriodCalcHi + N163_OFFSET, x
	sta var_Temp3
	lda #$00
	sta var_Temp4

	clc
	rol var_Temp2
	rol var_Temp3
	rol var_Temp4
	rol var_Temp2
	rol var_Temp3
	rol var_Temp4

.if 0
    ; Compensate for shorter wave lengths
    lda var_ch_WaveLen, x
    cmp #$10
    beq :+
    lsr var_Temp4
    ror var_Temp3
    ror var_Temp2
    cmp #$08
    beq :+
    lsr var_Temp4
    ror var_Temp3
    ror var_Temp2
    cmp #$04
    beq :+
    lsr var_Temp4
    ror var_Temp3
    ror var_Temp2
    cmp #$02
    beq :+
    lsr var_Temp4
    ror var_Temp3
    ror var_Temp2
:
.endif
   rts

ft_n163_load_wave2:

    tya
    pha

    ; Get wave pack pointer
    lda var_ch_WavePtrLo - N163_OFFSET, x
    sta var_Temp_Pointer2
    lda var_ch_WavePtrHi - N163_OFFSET, x
    sta var_Temp_Pointer2 + 1

    ; Get number of waves
    ldy #$00
    lda (var_Temp_Pointer2), y
    sta var_Temp3

    ; Setup wave RAM
    lda var_ch_WavePos - N163_OFFSET, x
    lsr a
    ora #$80
    sta $F800

    ; Get wave index
    lda var_ch_DutyCycle, x
    and #$0F
    sta var_Temp3
    beq :++
    clc          ; Multiply wave index with wave len
    lda #$00
:   adc var_ch_WaveLen - N163_OFFSET, x
    dec var_Temp3
    bne :-
:   tay          ; Wave table postition -> y

    txa          ; Save X
    pha
    lda var_ch_WaveLen - N163_OFFSET, x
    tax

    ; Load wave
:	lda (var_Temp_Pointer2), y
	sta $4800
    iny
    dex
    bne :-

    pla     ; Restore x & y
    tax
    pla
    tay

    rts

ft_n163_chan_addr:
;    .byte $40, $48, $50, $58, $60, $68, $70, $78
    .byte $78, $70, $68, $60, $58, $50, $48, $40

