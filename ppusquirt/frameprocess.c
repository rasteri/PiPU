#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <libusb-1.0/libusb.h>
#include "nesstuff.h"

// Bgcolor is always black
int BgColor = 0x0f;

// This is generated in GFXSetup
Color NesPalette[NESCOLORCOUNT];

struct palmusdata *pmdata;
double contrastFactor, contrast = 50;
#define BRIGHTNESS 30
Colmatch MostCommonColorInFrame[NESCOLORCOUNT];

// Massive pre-calculated color lookup tables - Baseline FPS 82
unsigned char PaletteLookup[256][256][256]; // for matching a color to a palette
signed char ColorLookup[256][256][256][4];  // For matching to a color within the palette

// Ordered dither map
double map[8][8] = {
	{0, 48, 12, 60, 3, 51, 15, 63},
	{32, 16, 44, 28, 35, 19, 47, 31},
	{8, 56, 4, 52, 11, 59, 7, 55},
	{40, 24, 36, 20, 43, 27, 39, 23},
	{2, 50, 14, 62, 1, 49, 13, 61},
	{34, 18, 46, 30, 33, 17, 45, 29},
	{10, 58, 6, 54, 9, 57, 5, 53},
	{42, 26, 38, 22, 41, 25, 37, 21}};

// Nes Palette in string format, will be converted later on
char NesPaletteStrings[NESCOLORCOUNT][6] =
	{
		"7C7C7C",
		"0000FC",
		"0000BC",
		"4428BC",
		"940084",
		"A80020",
		"A81000",
		"881400",
		"503000",
		"007800",
		"006800",
		"005800",
		"004058",
		"000000",
		"000000",
		"000000",
		"BCBCBC",
		"0078F8",
		"0058F8",
		"6844FC",
		"D800CC",
		"E40058",
		"F83800",
		"E45C10",
		"AC7C00",
		"00B800",
		"00A800",
		"00A844",
		"008888",
		"000000",
		"000000",
		"000000",
		"F8F8F8",
		"3CBCFC",
		"6888FC",
		"9878F8",
		"F878F8",
		"F85898",
		"F87858",
		"FCA044",
		"F8B800",
		"B8F818",
		"58D854",
		"58F898",
		"00E8D8",
		"787878",
		"000000",
		"000000",
		"FCFCFC",
		"A4E4FC",
		"B8B8F8",
		"D8B8F8",
		"F8B8F8",
		"F8A4C0",
		"F0D0B0",
		"FCE0A8",
		"F8D878",
		"D8F878",
		"B8F8B8",
		"B8F8D8",
		"00FCFC",
		"F8D8F8",
		"000000",
		"000000"};

// Square a number
float S(float i)
{
	return i * i;
}

void getpixel(char *frameBuf, unsigned int x, unsigned int y, unsigned char *r, unsigned char *g, unsigned char *b)
{
	unsigned int flx = 0;
	flx = (unsigned int)((float)1.25 * (float)x);

	unsigned int off = ((320 * y) + flx) * 4;
	*b = frameBuf[off];
	*g = frameBuf[off + 1];
	*r = frameBuf[off + 2];
}

void setpixel(char *frameBuf, unsigned int x, unsigned int y, unsigned char r, unsigned char g, unsigned char b)
{
	unsigned int flx = 0;
	flx = (unsigned int)((float)1.25 * (float)x);
	unsigned int off = ((320 * y) + flx) * 4;
	*(frameBuf + off) = b;
	*(frameBuf + off + 1) = g;
	*(frameBuf + off + 2) = r;
}

long ColorDistance(Color color1, Color color2)
{
	return (
		S((signed int)color1.r - (signed int)color2.r) +
		S((signed int)color1.g - (signed int)color2.g) +
		S((signed int)color1.b - (signed int)color2.b));
}

// Compare the difference of two RGB values, weigh by CCIR 601 luminosity
float WeightedColorDistance(Color color1, Color color2)
{
	float luma1 = (color1.r * 299 + color1.g * 587 + color1.b * 114) / (255.0 * 1000);
	float luma2 = (color2.r * 299 + color2.g * 587 + color2.b * 114) / (255.0 * 1000);
	float lumadiff = luma1 - luma2;
	float diffR = (color1.r - color2.r) / 255.0, diffG = (color1.g - color2.g) / 255.0, diffB = (color1.b - color2.b) / 255.0;

	return (diffR * diffR * 0.299 + diffG * diffG * 0.587 + diffB * diffB * 0.114) * 0.75 + lumadiff * lumadiff;
}

// Find the best colour match from the entire NES palette, not really used
unsigned char FindBestColorMatch(Color theColor)
{
	double distance;
	double bestScoreSoFar = 99999;
	unsigned char bestColorSoFar = 0;
	for (int cnt = 0; cnt < NESCOLORCOUNT; cnt++)
	{
		distance = WeightedColorDistance(NesPalette[cnt], theColor);
		if (distance < bestScoreSoFar)
		{
			bestColorSoFar = cnt;
			bestScoreSoFar = distance;
		}
	}
	return bestColorSoFar;
}

// Find the best colour match from a specified palette
// Will return -1 if bgcolor is best match
int FindBestColorMatchFromPalette(Color theColor, unsigned char *Palette, int bgColor)
{
	float distance;
	float bestScoreSoFar = 999999999;
	int bestColorSoFar = 9999;
	int cnt = 0;

	bestColorSoFar = -1;
	bestScoreSoFar = WeightedColorDistance(NesPalette[bgColor], theColor);

	if (bestScoreSoFar != 0)
	{
		for (cnt = 0; cnt < 3; cnt++)
		{
			distance = WeightedColorDistance(NesPalette[Palette[cnt]], theColor);
			if (distance < bestScoreSoFar)
			{
				bestColorSoFar = cnt;
				bestScoreSoFar = distance;
			}
		}
	}
	return bestColorSoFar;
}

typedef struct Palmatch
{

	unsigned char palNo;
	int frequency;

} Palmatch;

int ComparePalMatch(const void *s1, const void *s2)
{
	Palmatch *e1 = (Palmatch *)s1;
	Palmatch *e2 = (Palmatch *)s2;
	return e1->frequency - e2->frequency;
}

signed char FindBestPalForPixel(Color currPix)
{
	int p;
	float bestDist;
	signed char bestPal;
	int col;

	bestDist = 999999999;
	bestPal = -1;

	// check the bgcolor first
	bestDist = WeightedColorDistance(NesPalette[BgColor], currPix);

	double dist;

	// if bgcolor wasn't an exact match, match other cols
	if (bestDist != 0)
	{
		for (p = 0; p < 4; p++)
		{
			for (col = 0; col < 3; col++)
			{
				dist = WeightedColorDistance(NesPalette[pmdata->Palettes[p][col]], currPix);
				if (dist < bestDist)
				{
					bestDist = dist;
					bestPal = p;
				}
			}
		}
	}
	return bestPal;
}

int FindBestPalForSlice(char *bmp, unsigned int xoff, unsigned int yoff)
{
	signed char bestPal;

	int j;
	unsigned int x, y;
	Color currPix;
	Palmatch PalMatches[4];

	y = yoff;

	// init matches table
	for (j = 0; j < 4; j++)
	{
		PalMatches[j].palNo = j;
		PalMatches[j].frequency = 0;
	}

	for (x = xoff; x < xoff + 8; x += 1)
	{
		getpixel(bmp, x, y, &currPix.r, &currPix.g, &currPix.b);
		//bestPal = FindBestPalForPixel(currPix);
		bestPal = PaletteLookup[currPix.r][currPix.g][currPix.b];

		if (bestPal != -1)
		{
			PalMatches[bestPal].frequency++;
		}
	}

	qsort(PalMatches, 4, sizeof(Palmatch), ComparePalMatch);

	return PalMatches[3].palNo;
}

long CompareColMatch(const void *s1, const void *s2)
{
	Colmatch *e1 = (Colmatch *)s1;
	Colmatch *e2 = (Colmatch *)s2;
	return e1->frequency - e2->frequency;
}

unsigned char SatAdd8(signed short n1, signed short n2)
{
	signed int result = n1 + n2;
	if (result < 0x00)
		return 0;
	if (result > 0xFF)
		return 0xff;
	else
		return (unsigned char)result;
}

// Initialise palettes, set up memory regions, etc
void GFXSetup()
{

	int i, j;

	char curHex[3] = {0x00, 0x00, 0x00};
	char *curCol;
	char *dummy = 0;


	// Generate NES palette from the string info
	for (i = 0; i < 64; i++)
	{
		curCol = NesPaletteStrings[i];

		curHex[0] = curCol[0];
		curHex[1] = curCol[1];
		NesPalette[i].r = (unsigned char)strtoul(curHex, &dummy, 16);

		curHex[0] = curCol[2];
		curHex[1] = curCol[3];
		NesPalette[i].g = (unsigned char)strtoul(curHex, &dummy, 16);

		curHex[0] = curCol[4];
		curHex[1] = curCol[5];
		NesPalette[i].b = (unsigned char)strtoul(curHex, &dummy, 16);
	}

	// Generate ordered dither map
	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			map[i][j] = (double)80 * (((double)map[i][j] / (double)64) - 0.5);
		}
	}

	// Open palette/music shared memory area
	int fd;
	fd = shm_open("/palmusdata", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd == -1)
		exit(1);

	if (ftruncate(fd, sizeof(struct palmusdata)) == -1)
		exit(1);

	pmdata = mmap(NULL, sizeof(struct palmusdata), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (pmdata == MAP_FAILED)
		exit(1);


	// Default DOOM Palette

	//black for bg
	BgColor = 0x0f;

	//dark greys + green
	pmdata->Palettes[0][0] = 0x10;
	pmdata->Palettes[0][1] = 0x09;
	pmdata->Palettes[0][2] = 0x2d;

	//oranges
	pmdata->Palettes[1][0] = 0x07;
	pmdata->Palettes[1][1] = 0x28;
	pmdata->Palettes[1][2] = 0x18;

	//blues
	pmdata->Palettes[2][0] = 0x02;
	pmdata->Palettes[2][1] = 0x01;
	pmdata->Palettes[2][2] = 0x11;

	//reds and light grey
	pmdata->Palettes[3][0] = 0x06;
	pmdata->Palettes[3][1] = 0x16;
	pmdata->Palettes[3][2] = 0x3d;

	// Pre-gen contrast factor
	contrastFactor = (259 * (contrast + 255)) / (255 * (259 - contrast));

	Color col;

	FILE *fp;
	size_t n;

	// build palette lookup tables to trade memory for speed
	// Try to open them from last time if possible
	if (fp = fopen("lookup.bin", "r"))
	{
		if ((n = fread(&PaletteLookup, 1, 256 * 256 * 256, fp)) != (256 * 256 * 256)){ printf("exit : %lu\n", n); exit(1); }
		if ((n = fread(&ColorLookup, 1, 256 * 256 * 256 * 4, fp)) != (256 * 256 * 256 * 4)){ printf("exit2 : %lu\n", n); exit(1); }
		fclose(fp);
	}
	// If not, generate from scratch (takes 30sec or so)
	else
	{
		for (int r = 0; r < 256; r++)
		{
			for (int g = 0; g < 256; g++)
			{
				for (int b = 0; b < 256; b++)
				{
					col.r = r;
					col.g = g;
					col.b = b;
					PaletteLookup[r][g][b] = FindBestPalForPixel(col);

					for (int p = 0; p < 4; p++)
					{
						ColorLookup[r][g][b][p] = FindBestColorMatchFromPalette(col, pmdata->Palettes[p], BgColor);
					}
				}
			}
			printf("%d / 256 - %d\n", r, PaletteLookup[r][255][255]);
		}

		//Save the lookup tables for next time
		fp = fopen("lookup.bin", "w+");

		fwrite(&PaletteLookup, 256 * 256 * 256, 1, fp);
		fwrite(&ColorLookup, 256 * 256 * 256 * 4, 1, fp);
		fclose(fp);
	}
}

// Convert a single frame segment to NES format
void FitFrame(char *bmp, PPUFrame *theFrame, int startline, int endline)
{
	int x, y, i, offset;
	Color currPix;
	int xoff;
	int palToUse;
	short bestcol;


	if (startline == 0)
	{
		theFrame->OtherData[0] = 0x54;
		theFrame->OtherData[1] = 0x17; // Magic value
		theFrame->OtherData[2] = BgColor;
		theFrame->OtherData[3] = pmdata->Palettes[0][0];
		theFrame->OtherData[4] = pmdata->Palettes[0][1];
		theFrame->OtherData[5] = pmdata->Palettes[0][2];
		theFrame->OtherData[6] = 0;
		theFrame->OtherData[7] = pmdata->Palettes[1][0];
		theFrame->OtherData[8] = pmdata->Palettes[1][1];
		theFrame->OtherData[9] = pmdata->Palettes[1][2];
		theFrame->OtherData[10] = 0;
		theFrame->OtherData[11] = pmdata->Palettes[2][0];
		theFrame->OtherData[12] = pmdata->Palettes[2][1];
		theFrame->OtherData[13] = pmdata->Palettes[2][2];
		theFrame->OtherData[14] = 0;
		theFrame->OtherData[15] = pmdata->Palettes[3][0];
		theFrame->OtherData[16] = pmdata->Palettes[3][1];
		theFrame->OtherData[17] = pmdata->Palettes[3][2];
		theFrame->OtherData[20] = pmdata->music; // Play E1M1 music
		theFrame->OtherData[30] = 0xBE;
		theFrame->OtherData[31] = 0xEF; // Magic Value
	}

	// First apply dithering and brightness/contrast adjustment
	for (y = startline; y < endline; y++)
	{
		for (x = 0; x < 256; x++)
		{
			getpixel(bmp, x, y, &currPix.r, &currPix.g, &currPix.b);

/*			//bump up brightness
			currPix.r = SatAdd8(currPix.r, BRIGHTNESS);
			currPix.g = SatAdd8(currPix.g, BRIGHTNESS);
			currPix.b = SatAdd8(currPix.b, BRIGHTNESS);

			//bump up contrast
			currPix.r = SatAdd8(contrastFactor * ((double)currPix.r - 128), 128);
			currPix.g = SatAdd8(contrastFactor * ((double)currPix.g - 128), 128);
			currPix.b = SatAdd8(contrastFactor * ((double)currPix.b - 128), 128);
*/
			// Ordered Dither
			currPix.r = SatAdd8(currPix.r, map[x % 8][y % 8]);
			currPix.g = SatAdd8(currPix.g, map[x % 8][y % 8]);
			currPix.b = SatAdd8(currPix.b, map[x % 8][y % 8]);
			setpixel(bmp, x, y, currPix.r, currPix.g, currPix.b);
		}
	}

	// Iterate through all the image's pixels 
	// for each scanline
	for (y = startline; y < endline; y++)
	{ 
		int currScanLine = y + 1;

		// For each 8x1 attribute slice
		for (i = 0; i < 32; i++)
		{ 

			//xoff is actual x coord, i is slice no in scanline
			xoff = i * 8;

			// Find the most suitable palette for this slice
			palToUse = FindBestPalForSlice(bmp, xoff, y);

			// Start building the 8x1 tile
			PPUTile currTile;
			currTile.NT = 0x00;																// Nametable is irrelevant
			currTile.AT = (palToUse) | (palToUse << 2) | (palToUse << 4) | (palToUse << 6); // Attribute table can just have 4 copies of the palette number to save calculating which quadrant we're in
			currTile.LowBG = 0;
			currTile.HighBG = 0;

			offset = 7;

			// So now we fit the pixels to the palette we chose
			// For each pixel in the slice
			for (x = xoff; x < xoff + 8; x++)
			{ 
				getpixel(bmp, x, y, &currPix.r, &currPix.g, &currPix.b);

				// find closest color match from palette we chose
				bestcol = FindBestColorMatchFromPalette(currPix, pmdata->Palettes[palToUse], BgColor); // Slow but dynamic palette
				//bestcol = ColorLookup[currPix.r][currPix.g][currPix.b][palToUse]; // Quick but locked to one palette

				// Shift the index up one, so -1 becomes zero, which is how it will appear in the nes palette
				bestcol++;

				// Bit 0 of bestcol goes to the low BG tile byte
				currTile.LowBG = currTile.LowBG | ((bestcol & 0x01) << offset);

				// Bit 1 of bestcol goes to the high BG tile byte
				currTile.HighBG = currTile.HighBG | (((bestcol >> 1) & 0x01) << offset);

				offset--;
			}

			// Store the first two tiles in the previous scanline
			if (i < 2)
			{
				if (currScanLine > 0)
					theFrame->ScanLines[currScanLine - 1].nexttiles[i] = currTile;
			}
			else
			{
				theFrame->ScanLines[currScanLine].tiles[i - 2] = currTile;
			}

		}
	}
}
