#include <stdio.h>
#include "fx2.h"
#include "fx2regs.h"

#include "fx2sdly.h"            // SYNCDELAY macro

extern unsigned char HaltProcessing;
extern unsigned char RecievedFirstPacket;

void ISR_EXTR0(void) interrupt 0                           
{ 

}
