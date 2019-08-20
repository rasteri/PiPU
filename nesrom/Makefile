.PHONY: clean all

.PRECIOUS: *.o

all: hello_world.nes

crt0.o: crt0.s
	ca65 crt0.s -D INC_MUSIC

%.o: %.c
	cc65 -Oi $< --add-source
	ca65 $*.s

%.nes: %.o crt0.o
	ld65 -m hello.map -vm -C nes.cfg -o $@ crt0.o $< nes.lib

clean:
	rm *.o *.nes hello_world.s