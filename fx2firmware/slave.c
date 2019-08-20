#pragma NOIV                    // Do not generate interrupt vectors
//-----------------------------------------------------------------------------
//   File:      slave.c
//   Contents:  Hooks required to implement USB peripheral function.
//              Code written for FX2 REVE 56-pin and above.
//              This firmware is used to demonstrate FX2 Slave FIF
//              operation.
//   Copyright (c) 2003 Cypress Semiconductor All rights reserved
//-----------------------------------------------------------------------------
#include "fx2.h"
#include "fx2regs.h"
#include "fx2sdly.h"            // SYNCDELAY macro

extern BOOL GotSUD;             // Received setup data flag
extern BOOL Sleep;
extern BOOL Rwuen;
extern BOOL Selfpwr;

BYTE Configuration;             // Current configuration
BYTE AlternateSetting;          // Alternate settings

//-----------------------------------------------------------------------------
// Task Dispatcher hooks
//   The following hooks are called by the task dispatcher.
//-----------------------------------------------------------------------------
void TD_Init( void )
{ // Called once at startup
	
	CPUCS = 0x12; // CLKSPD[1:0]=10, for 48MHz operation, output CLKOUT
	
	PINFLAGSAB = 0x00;
	SYNCDELAY;
	PINFLAGSCD = 0x00;
	SYNCDELAY;
	PORTACFG |= 0x80; // enable flagD and INT0
	SYNCDELAY;
	IFCONFIG = 0xCB; //Internal clock, 48 MHz, Slave FIFO interface
	SYNCDELAY;
	
	REVCTL = 0x03;SYNCDELAY;
	
	EP2CFG = 0xA0;                //out 512 bytes, 4x, bulk
	SYNCDELAY;
	EP6CFG = 0xE0;	// Valid, BULK-IN, 512 byte buffer, double-buffered
	SYNCDELAY;
	EP4CFG = 0x02;                //clear valid bit
	SYNCDELAY;
	EP8CFG = 0x02;                //clear valid bit
	SYNCDELAY;
	FIFOPINPOLAR = 0x03;
	SYNCDELAY;
	
	SYNCDELAY;
	FIFORESET = 0x80;             // activate NAK-ALL to avoid race conditions
	SYNCDELAY;                    // see TRM section 15.14
	FIFORESET = 0x02;             // reset, FIFO 2
	SYNCDELAY;                    //
	FIFORESET = 0x04;             // reset, FIFO 4
	SYNCDELAY;                    //
	FIFORESET = 0x06;             // reset, FIFO 6
	SYNCDELAY;                    //
	FIFORESET = 0x08;             // reset, FIFO 8
	SYNCDELAY;                    //
	FIFORESET = 0x00;             // deactivate NAK-ALL
	
	
	
	
	// handle the case where we were already in AUTO mode...
	// ...for example: back to back firmware downloads...
	SYNCDELAY;                    //
	EP2FIFOCFG = 0x00;            // AUTOOUT=0, WORDWIDE=1
	
	// core needs to see AUTOOUT=0 to AUTOOUT=1 switch to arm endp's
	
	SYNCDELAY;                    //
	EP2FIFOCFG = 0x00;            // AUTOOUT=0, WORDWIDE=0
	
	SYNCDELAY;                    //
	EP6FIFOCFG = 0x0C;            // AUTOIN=1, ZEROLENIN=1, WORDWIDE=0
	//EP6FIFOCFG = 0x00;            // AUTOIN=0, ZEROLENIN=0, WORDWIDE=0
	SYNCDELAY;
	EP6AUTOINLENL = 0x03;
	SYNCDELAY;
	EP6AUTOINLENH = 0x00;
	SYNCDELAY;
	
	SYNCDELAY;
	
	SYNCDELAY;
	OUTPKTEND =0x82;   //arming the EP2 OUT quadruple times, as it's quad buffered.
	SYNCDELAY;
	OUTPKTEND =0x82;
	SYNCDELAY;
	OUTPKTEND =0x82;
	SYNCDELAY;
	OUTPKTEND =0x82;
	SYNCDELAY;
	
	
	
	
	OEA  = 0x08;                // PORTA.3  is an output, all others in
	//IOC = 0xFF;                 // Initialize PORTC to all LOW
	
	TCON = 0x00;               // INT0 configured as Edge triggered interrupts.
	
	IE  |= 0x01;			    // Enable External Interrupts 0
	EA   = 1;				    // Enable Global Interrupt
	
}

unsigned char HaltProcessing = 1;
unsigned char RecievedFirstPacket = 0;
unsigned char countUp = 0;

void TD_Poll( void )
{ // Called repeatedly while the device is idle
	
	
	if( !( EP2468STAT & 0x01 ) )   //if EP2 is not empty, modify the packet and commit it to the peripheral domain
	{
		
		
		if (!HaltProcessing){
			if (EP2BCH == 0x00 && EP2BCL == 42){ // Todo - make this count the number of packets rather than looking for a 42byte one, and send 80 512byte packets (40960).
				
				HaltProcessing = 1;
				IOA ^= 0x08;
				SYNCDELAY;
				OUTPKTEND = 0x02;//OUTPKTEND = 0x82; rehect
				SYNCDELAY;
			}
			else {
				SYNCDELAY;
				OUTPKTEND = 0x02;
				SYNCDELAY;
				
			}
		}
		
	}
	
	if (HaltProcessing){
		
		// Count the amount of time since RD last went low
		if (IOA & 0x02){
			countUp++;
			
			// More than 50 cycles, that means we're in VBLANK, so we can restart processing
			if (countUp > 50){
				IOA ^= 0x08;
				HaltProcessing = 0;
				countUp = 0;
			}
		}
		else
		countUp=0;
	}
	
	SYNCDELAY;
	EP6AUTOINLENL = 0x03;
	SYNCDELAY;
	EP6AUTOINLENH = 0x00;
	SYNCDELAY;
	
}

BOOL TD_Suspend( void )
{ // Called before the device goes into suspend mode
	return( TRUE );
}

BOOL TD_Resume( void )
{ // Called after the device resumes
	return( TRUE );
}

//-----------------------------------------------------------------------------
// Device Request hooks
//   The following hooks are called by the end point 0 device request parser.
//-----------------------------------------------------------------------------
BOOL DR_GetDescriptor( void )
{
	return( TRUE );
}

BOOL DR_SetConfiguration( void )
{ // Called when a Set Configuration command is received
	
	if( EZUSB_HIGHSPEED( ) )
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
	
	Configuration = SETUPDAT[ 2 ];
	return( TRUE );        // Handled by user code
}

BOOL DR_GetConfiguration( void )
{ // Called when a Get Configuration command is received
	EP0BUF[ 0 ] = Configuration;
	EP0BCH = 0;
	EP0BCL = 1;
	return(TRUE);          // Handled by user code
}

BOOL DR_SetInterface( void )
{ // Called when a Set Interface command is received
	AlternateSetting = SETUPDAT[ 2 ];
	return( TRUE );        // Handled by user code
}

BOOL DR_GetInterface( void )
{ // Called when a Set Interface command is received
	EP0BUF[ 0 ] = AlternateSetting;
	EP0BCH = 0;
	EP0BCL = 1;
	return( TRUE );        // Handled by user code
}

BOOL DR_GetStatus( void )
{
	return( TRUE );
}

BOOL DR_ClearFeature( void )
{
	return( TRUE );
}

BOOL DR_SetFeature( void )
{
	return( TRUE );
}

BOOL DR_VendorCmnd( void )
{
	return( TRUE );
}

//-----------------------------------------------------------------------------
// USB Interrupt Handlers
//   The following functions are called by the USB interrupt jump table.
//-----------------------------------------------------------------------------

// Setup Data Available Interrupt Handler
void ISR_Sudav( void ) interrupt 0
{
	GotSUD = TRUE;         // Set flag
	EZUSB_IRQ_CLEAR( );
	USBIRQ = bmSUDAV;      // Clear SUDAV IRQ
}

// Setup Token Interrupt Handler
void ISR_Sutok( void ) interrupt 0
{
	EZUSB_IRQ_CLEAR( );
	USBIRQ = bmSUTOK;      // Clear SUTOK IRQ
}

void ISR_Sof( void ) interrupt 0
{
	EZUSB_IRQ_CLEAR( );
	USBIRQ = bmSOF;        // Clear SOF IRQ
}

void ISR_Ures( void ) interrupt 0
{
	if ( EZUSB_HIGHSPEED( ) )
	{
		pConfigDscr = pHighSpeedConfigDscr;
		pOtherConfigDscr = pFullSpeedConfigDscr;
	}
	else
	{
		pConfigDscr = pFullSpeedConfigDscr;
		pOtherConfigDscr = pHighSpeedConfigDscr;
	}
	
	EZUSB_IRQ_CLEAR( );
	USBIRQ = bmURES;       // Clear URES IRQ
}

void ISR_Susp( void ) interrupt 0
{
	Sleep = TRUE;
	EZUSB_IRQ_CLEAR( );
	USBIRQ = bmSUSP;
}

void ISR_Highspeed( void ) interrupt 0
{
	if ( EZUSB_HIGHSPEED( ) )
	{
		pConfigDscr = pHighSpeedConfigDscr;
		pOtherConfigDscr = pFullSpeedConfigDscr;
	}
	else
	{
		pConfigDscr = pFullSpeedConfigDscr;
		pOtherConfigDscr = pHighSpeedConfigDscr;
	}
	
	EZUSB_IRQ_CLEAR( );
	USBIRQ = bmHSGRANT;
}
void ISR_Ep0ack( void ) interrupt 0
{
}
void ISR_Stub( void ) interrupt 0
{
}
void ISR_Ep0in( void ) interrupt 0
{
}
void ISR_Ep0out( void ) interrupt 0
{
}
void ISR_Ep1in( void ) interrupt 0
{
}
void ISR_Ep1out( void ) interrupt 0
{
}
void ISR_Ep2inout( void ) interrupt 0
{
}
void ISR_Ep4inout( void ) interrupt 0
{
}
void ISR_Ep6inout( void ) interrupt 0
{
}
void ISR_Ep8inout( void ) interrupt 0
{
}
void ISR_Ibn( void ) interrupt 0
{
}
void ISR_Ep0pingnak( void ) interrupt 0
{
}
void ISR_Ep1pingnak( void ) interrupt 0
{
}
void ISR_Ep2pingnak( void ) interrupt 0
{
}
void ISR_Ep4pingnak( void ) interrupt 0
{
}
void ISR_Ep6pingnak( void ) interrupt 0
{
}
void ISR_Ep8pingnak( void ) interrupt 0
{
}
void ISR_Errorlimit( void ) interrupt 0
{
}
void ISR_Ep2piderror( void ) interrupt 0
{
}
void ISR_Ep4piderror( void ) interrupt 0
{
}
void ISR_Ep6piderror( void ) interrupt 0
{
}
void ISR_Ep8piderror( void ) interrupt 0
{
}
void ISR_Ep2pflag( void ) interrupt 0
{
}
void ISR_Ep4pflag( void ) interrupt 0
{
}
void ISR_Ep6pflag( void ) interrupt 0
{
}
void ISR_Ep8pflag( void ) interrupt 0
{
}
void ISR_Ep2eflag( void ) interrupt 0
{
	
}
void ISR_Ep4eflag( void ) interrupt 0
{
}
void ISR_Ep6eflag( void ) interrupt 0
{
}
void ISR_Ep8eflag( void ) interrupt 0
{
}
void ISR_Ep2fflag( void ) interrupt 0
{
}
void ISR_Ep4fflag( void ) interrupt 0
{
}
void ISR_Ep6fflag( void ) interrupt 0
{
}
void ISR_Ep8fflag( void ) interrupt 0
{
}
void ISR_GpifComplete( void ) interrupt 0
{
}
void ISR_GpifWaveform( void ) interrupt 0
{
}


