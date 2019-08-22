#pragma NOIV                    // Do not generate interrupt vectors
#include "fx2.h"
#include "fx2regs.h"
#include "fx2sdly.h"            // SYNCDELAY macro

extern BOOL GotSUD;             // Received setup data flag
extern BOOL Sleep;
extern BOOL Rwuen;
extern BOOL Selfpwr;

BYTE Configuration;
BYTE AlternateSetting;

void TD_Init(void)
{
	CPUCS = 0x12; // CLKSPD[1:0]=10, for 48MHz operation, output CLKOUT

	// Flag output setup
	PINFLAGSAB = 0x00; SYNCDELAY;
	PINFLAGSCD = 0x00; SYNCDELAY;

	PORTACFG |= 0x80; SYNCDELAY; // enable flagD and INT0
	IFCONFIG = 0xCB; SYNCDELAY; //Internal clock, 48 MHz, Slave FIFO interface
	REVCTL = 0x03; SYNCDELAY; // Enhanced features 

	EP2CFG = 0xA0; SYNCDELAY; //out 512 bytes, 4x, bulk
	EP6CFG = 0xE0; SYNCDELAY; // Valid, BULK-IN, 512 byte buffer, double-buffered
	EP4CFG = 0x02; SYNCDELAY; //clear valid bit
	EP8CFG = 0x02; SYNCDELAY; //clear valid bit

	FIFOPINPOLAR = 0x03; SYNCDELAY;

	FIFORESET = 0x80; SYNCDELAY; // activate NAK-ALL to avoid race conditions, see TRM section 15.14
	FIFORESET = 0x02; SYNCDELAY; // reset, FIFO 2
	FIFORESET = 0x04; SYNCDELAY; // reset, FIFO 4
	FIFORESET = 0x06; SYNCDELAY; // reset, FIFO 6
	FIFORESET = 0x08; SYNCDELAY; // reset, FIFO 8
	FIFORESET = 0x00; SYNCDELAY; // deactivate NAK-ALL

	// Fifo 2 is the graphics output
	EP2FIFOCFG = 0x00; SYNCDELAY; // AUTOOUT=0, WORDWIDE=0 (for some reason this needs to be done twice?)
	EP2FIFOCFG = 0x00; SYNCDELAY;

	OUTPKTEND = 0x82; SYNCDELAY;  //arming the EP2 OUT quadruple times, as it's quad buffered.
	OUTPKTEND = 0x82; SYNCDELAY;
	OUTPKTEND = 0x82; SYNCDELAY;
	OUTPKTEND = 0x82; SYNCDELAY;

	// Fifo 6 is the controller input
	EP6FIFOCFG = 0x0C; SYNCDELAY; // AUTOIN=1, ZEROLENIN=1, WORDWIDE=0
	EP6AUTOINLENL = 0x03; SYNCDELAY;
	EP6AUTOINLENH = 0x00; SYNCDELAY;

	OEA = 0x08; // PORTA.3  is an output, all others in

	TCON = 0x00; // INT0 configured as Edge triggered interrupts.

	IE |= 0x01;	// Enable External Interrupts 0
	EA = 1;	// Enable Global Interrupt

}

unsigned char HaltProcessing = 1;
unsigned char RecievedFirstPacket = 0;
unsigned char countUp = 0;

// Called repeatedly while the device is idle
void TD_Poll(void)
{
	//if EP2 is not empty, modify the packet and commit it to the peripheral domain
	if (!(EP2468STAT & 0x01))
	{
		// Delay committing any packets until we're in vblank
		if (!HaltProcessing) {

			// Last packet will be 42 bytes long, halt processing if we detect that
			if (EP2BCH == 0x00 && EP2BCL == 42) {
				HaltProcessing = 1;
				IOA ^= 0x08; SYNCDELAY;
			}

			OUTPKTEND = 0x02; SYNCDELAY; // commit packet
		}
	}

	if (HaltProcessing) {

		// Count the amount of time since RD last went low
		if (IOA & 0x02) {
			countUp++;

			// More than 50 cycles, that means we're in VBLANK, so we can restart processing
			if (countUp > 50) {
				IOA ^= 0x08;
				HaltProcessing = 0;
				countUp = 0;
			}
		}
		else
			countUp = 0;
	}

	SYNCDELAY;
	EP6AUTOINLENL = 0x03;
	SYNCDELAY;
	EP6AUTOINLENH = 0x00;
	SYNCDELAY;

}

BOOL TD_Suspend(void)
{ // Called before the device goes into suspend mode
	return(TRUE);
}

BOOL TD_Resume(void)
{ // Called after the device resumes
	return(TRUE);
}

//-----------------------------------------------------------------------------
// Device Request hooks
//   The following hooks are called by the end point 0 device request parser.
//-----------------------------------------------------------------------------
BOOL DR_GetDescriptor(void)
{
	return(TRUE);
}

BOOL DR_SetConfiguration(void)
{ // Called when a Set Configuration command is received

	if (EZUSB_HIGHSPEED())
	{ // ...FX2 in high speed mode

		SYNCDELAY;
		EP6AUTOINLENL = 0x03;
		SYNCDELAY;
		EP6AUTOINLENH = 0x00;
		SYNCDELAY;
	}
	else
	{ // ...FX2 in full speed mode
		EP6AUTOINLENH = 0x00;
		SYNCDELAY;
		EP8AUTOINLENH = 0x00;   // set core AUTO commit len = 64 bytes
		SYNCDELAY;
		EP6AUTOINLENL = 0x40;
		SYNCDELAY;
		EP8AUTOINLENL = 0x40;
	}

	Configuration = SETUPDAT[2];
	return(TRUE);        // Handled by user code
}

BOOL DR_GetConfiguration(void)
{ // Called when a Get Configuration command is received
	EP0BUF[0] = Configuration;
	EP0BCH = 0;
	EP0BCL = 1;
	return(TRUE);          // Handled by user code
}

BOOL DR_SetInterface(void)
{ // Called when a Set Interface command is received
	AlternateSetting = SETUPDAT[2];
	return(TRUE);        // Handled by user code
}

BOOL DR_GetInterface(void)
{ // Called when a Set Interface command is received
	EP0BUF[0] = AlternateSetting;
	EP0BCH = 0;
	EP0BCL = 1;
	return(TRUE);        // Handled by user code
}

BOOL DR_GetStatus(void)
{
	return(TRUE);
}

BOOL DR_ClearFeature(void)
{
	return(TRUE);
}

BOOL DR_SetFeature(void)
{
	return(TRUE);
}

BOOL DR_VendorCmnd(void)
{
	return(TRUE);
}

//-----------------------------------------------------------------------------
// USB Interrupt Handlers
//   The following functions are called by the USB interrupt jump table.
//-----------------------------------------------------------------------------

// Setup Data Available Interrupt Handler
void ISR_Sudav(void) interrupt 0
{
	GotSUD = TRUE;         // Set flag
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSUDAV;      // Clear SUDAV IRQ
}

// Setup Token Interrupt Handler
void ISR_Sutok(void) interrupt 0
{
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSUTOK;      // Clear SUTOK IRQ
}

void ISR_Sof(void) interrupt 0
{
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSOF;        // Clear SOF IRQ
}

void ISR_Ures(void) interrupt 0
{
	if (EZUSB_HIGHSPEED())
	{
		pConfigDscr = pHighSpeedConfigDscr;
		pOtherConfigDscr = pFullSpeedConfigDscr;
	}
	else
	{
		pConfigDscr = pFullSpeedConfigDscr;
		pOtherConfigDscr = pHighSpeedConfigDscr;
	}

	EZUSB_IRQ_CLEAR();
	USBIRQ = bmURES;       // Clear URES IRQ
}

void ISR_Susp(void) interrupt 0
{
	Sleep = TRUE;
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSUSP;
}

void ISR_Highspeed(void) interrupt 0
{
	if (EZUSB_HIGHSPEED())
	{
		pConfigDscr = pHighSpeedConfigDscr;
		pOtherConfigDscr = pFullSpeedConfigDscr;
	}
	else
	{
		pConfigDscr = pFullSpeedConfigDscr;
		pOtherConfigDscr = pHighSpeedConfigDscr;
	}

	EZUSB_IRQ_CLEAR();
	USBIRQ = bmHSGRANT;
}
void ISR_Ep0ack(void) interrupt 0
{
}
void ISR_Stub(void) interrupt 0
{
}
void ISR_Ep0in(void) interrupt 0
{
}
void ISR_Ep0out(void) interrupt 0
{
}
void ISR_Ep1in(void) interrupt 0
{
}
void ISR_Ep1out(void) interrupt 0
{
}
void ISR_Ep2inout(void) interrupt 0
{
}
void ISR_Ep4inout(void) interrupt 0
{
}
void ISR_Ep6inout(void) interrupt 0
{
}
void ISR_Ep8inout(void) interrupt 0
{
}
void ISR_Ibn(void) interrupt 0
{
}
void ISR_Ep0pingnak(void) interrupt 0
{
}
void ISR_Ep1pingnak(void) interrupt 0
{
}
void ISR_Ep2pingnak(void) interrupt 0
{
}
void ISR_Ep4pingnak(void) interrupt 0
{
}
void ISR_Ep6pingnak(void) interrupt 0
{
}
void ISR_Ep8pingnak(void) interrupt 0
{
}
void ISR_Errorlimit(void) interrupt 0
{
}
void ISR_Ep2piderror(void) interrupt 0
{
}
void ISR_Ep4piderror(void) interrupt 0
{
}
void ISR_Ep6piderror(void) interrupt 0
{
}
void ISR_Ep8piderror(void) interrupt 0
{
}
void ISR_Ep2pflag(void) interrupt 0
{
}
void ISR_Ep4pflag(void) interrupt 0
{
}
void ISR_Ep6pflag(void) interrupt 0
{
}
void ISR_Ep8pflag(void) interrupt 0
{
}
void ISR_Ep2eflag(void) interrupt 0
{

}
void ISR_Ep4eflag(void) interrupt 0
{
}
void ISR_Ep6eflag(void) interrupt 0
{
}
void ISR_Ep8eflag(void) interrupt 0
{
}
void ISR_Ep2fflag(void) interrupt 0
{
}
void ISR_Ep4fflag(void) interrupt 0
{
}
void ISR_Ep6fflag(void) interrupt 0
{
}
void ISR_Ep8fflag(void) interrupt 0
{
}
void ISR_GpifComplete(void) interrupt 0
{
}
void ISR_GpifWaveform(void) interrupt 0
{
}


