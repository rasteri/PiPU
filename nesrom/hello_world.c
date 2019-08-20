#include <stddef.h>
#include <stdint.h>
#include "neslib.h"

// define PPU register aliases
#define PPU_CTRL *((uint8_t *)0x2000)
#define PPU_MASK *((uint8_t *)0x2001)
#define PPU_STATUS *((uint8_t *)0x2002)
#define PPU_SCROLL *((uint8_t *)0x2005)
#define PPU_ADDRESS *((uint8_t *)0x2006)
#define PPU_DATA *((uint8_t *)0x2007)

// define palette color aliases
#define COLOR_BLACK 0x0f
#define COLOR_WHITE 0x20

size_t i;


uint8_t READBUF[32];
uint8_t dumdum;

void __fastcall__ music_init(unsigned char song);

uint8_t controlInput = 0;

uint8_t lastmus = 2;

void funkytown()
{
    READBUF[0] = PPU_DATA; // dummy read
    READBUF[0] = PPU_DATA;
    READBUF[1] = PPU_DATA;
    READBUF[2] = PPU_DATA;
    READBUF[3] = PPU_DATA;
    READBUF[4] = PPU_DATA;
    READBUF[5] = PPU_DATA;
    READBUF[6] = PPU_DATA;
    READBUF[7] = PPU_DATA;
    READBUF[8] = PPU_DATA;
    READBUF[9] = PPU_DATA;
    READBUF[10] = PPU_DATA;
    READBUF[11] = PPU_DATA;
    READBUF[12] = PPU_DATA;
    READBUF[13] = PPU_DATA;
    READBUF[14] = PPU_DATA;
    READBUF[15] = PPU_DATA;
    READBUF[16] = PPU_DATA;
    READBUF[17] = PPU_DATA;
    READBUF[18] = PPU_DATA;
    READBUF[19] = PPU_DATA;
    READBUF[20] = PPU_DATA;
    READBUF[21] = PPU_DATA;
    READBUF[22] = PPU_DATA;
    READBUF[23] = PPU_DATA;
    READBUF[24] = PPU_DATA;
    READBUF[25] = PPU_DATA;
    READBUF[26] = PPU_DATA;
    READBUF[27] = PPU_DATA;
    READBUF[28] = PPU_DATA;
    READBUF[29] = PPU_DATA;
    READBUF[30] = PPU_DATA;
    READBUF[31] = PPU_DATA;

    // Few more dummy reads to flush out the fifo buffer
    dumdum = PPU_DATA;
    dumdum = PPU_DATA;
    dumdum = PPU_DATA;
    dumdum = PPU_DATA;
    dumdum = PPU_DATA;
    dumdum = PPU_DATA;
    dumdum = PPU_DATA;
    dumdum = PPU_DATA;
    dumdum = PPU_DATA;

    controlInput = pad_poll(0);

    PPU_ADDRESS = 0x21;
    PPU_ADDRESS = 0xc0;
    PPU_DATA = controlInput;
    PPU_DATA = controlInput;
    PPU_DATA = controlInput;

    // If magic values found
    if (READBUF[0] == 0x54 && READBUF[1] == 0x17 && READBUF[30] == 0xBE && READBUF[31] == 0xEF)
    {

        // load the palette data into PPU memory $3f00-$3f1f
        PPU_ADDRESS = 0x3f;
        PPU_ADDRESS = 0x00;
        for (i = 2; i < 18; ++i)
        {
            PPU_DATA = READBUF[i];
        }

        if (lastmus != READBUF[20])
        {
            lastmus = READBUF[20];
            music_init(lastmus);
        }
    }
}

void main(void)
{

    // enable NMI and rendering
    PPU_CTRL = 0x80;
    PPU_MASK = 0x1e;

    music_init(2);

    // infinite loop
    while (1)
    {
        ppu_waitnmi();

        // reset scroll location to top-left of screen
        PPU_SCROLL = 0x00;
        PPU_SCROLL = 0x00;
    };
};
