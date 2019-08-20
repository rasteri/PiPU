#define NESCOLORCOUNT 64

// One 8x1 pixel tile of the PPU frame
typedef struct PPUTile {
	unsigned char NT;
	unsigned char AT;
	unsigned char LowBG;
	unsigned char HighBG;
} PPUTile;

// 32 Tiles make up an entire scanline
typedef struct PPUScanline {
	PPUTile tiles[32];
	PPUTile sprites[8];
	PPUTile nexttiles[2];
	unsigned char unusedNTFetches[2];
} PPUScanline;

// 241 scanlines and some extra data make up the entire frame as read by the PPU
typedef struct PPUFrame {
	PPUScanline ScanLines[241];
	unsigned char OtherData[32]; // read manually by the CPU during VBLANK
} PPUFrame;

// Used to sort the color frequency table
typedef struct Colmatch
{
	unsigned char colNo;
	unsigned long frequency;
} Colmatch;


typedef struct Color {
	unsigned char r, g, b;
} Color;

// Memory region to exchange data between SDL thread and this program
struct region { 
    int full;
    pthread_mutex_t fulllock;
    pthread_cond_t fullsignal;
    char buf[307200];
};

// Memory region to set music track and palette
struct palmusdata { 
    unsigned char music;
	unsigned char Palettes[4][3];
};

// GFX thread state, one for each
typedef struct threaddata{
	int threadno;
	int outbuffull;
	pthread_mutex_t donelock;
    pthread_cond_t donesignal;
	int done;

	pthread_t handle;
} threaddata;

// Gamepad scancode
#define PAD_A			0x01
#define PAD_B			0x02
#define PAD_SELECT		0x04
#define PAD_START		0x08
#define PAD_UP			0x10
#define PAD_DOWN		0x20
#define PAD_LEFT		0x40
#define PAD_RIGHT		0x80