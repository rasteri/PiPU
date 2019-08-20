;
; NSF Header
;
.segment "HEADER"
;
.byte 'N', 'E', 'S', 'M', $1A				; ID
.byte $01									; Version
.byte 16  									; Number of songs
.byte 1										; Start song
.word LOAD
.word INIT
.word PLAY
.byte "ft driver                      ", 0	; Name, 32 bytes
.byte "                               ", 0	; Artist, 32 bytes
.byte "                               ", 0	; Copyright, 32 bytes
.word $411A									; NTSC speed
.byte 0, 0, 0, 0, 0, 0, 0, 0				; Bank values
.word $4E20									; PAL speed
.byte 2   									; Flags, dual PAL/NTSC
.if .defined(USE_VRC6)
.byte 1
.elseif .defined(USE_VRC7)
.byte 2
.elseif .defined(USE_FDS)
.byte 4
.elseif .defined(USE_MMC5)
.byte 8
.elseif .defined(USE_N163)
.byte 16
.elseif .defined(USE_5B)
.byte 32
.else
.byte 0
.endif
;.byte 0										; Sound chip, 2 = VRC7, 4 = FDS
.byte 0,0,0,0								; Reserved
;	
.include "driver.s"
