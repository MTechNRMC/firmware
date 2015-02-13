
//   MSP430G2xx3 MASTER Code
//
//   Description: Master receives sequence of Characters from Main CPU via UART,
//
//	 TEST: Attempted to convert string to integer, variable type disagreement
//   Unable to change slave address outside of initialized settings.

//   USCI_A0 RX interrupt triggers storage
//   Baud rate divider with 1MHz = 1MHz/9600 = ~104.2
//   ACLK = n/a, MCLK = SMCLK = CALxxx_1MHZ = 1MHz
//
//   Modified version of TI Example Code <msp430g2xx3_uscib0_i2c_05.c> by D. Dang
//	 Modified version of TI Example Code <msp430g2xx3_uscib0_i2c_10.c> by D. Dang
//   Texas Instruments Inc.
//   February 2011
//   Built with CCS Version 4.2.0 and IAR Embedded Workbench Version: 5.10
//
//   Modified for CSCI 255 by Erin Wiles, Krista Stone & Ross Mitchell
//	 Version 1.6 Dec. 12th, 2014 9:30pm
//	 Merged I2C and UART Settings
//
//******************************************************************************
#include <msp430.h>

//**** TESTING ***//
// int  convert2hex(int index);
volatile unsigned int ii=0,kk=0;
//**** TESTING ***//
// volatile unsigned int slave_flag=0;
volatile unsigned char slave_address[20];	// May make sense to change size


int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
	if (CALBC1_1MHZ==0xFF)					  // If calibration constant erased
	{
		while(1);                             // do not load, trap CPU!!
	}
	DCOCTL = 0;                               // Select lowest DCOx and MODx settings
	BCSCTL1 = CALBC1_1MHZ;                    // Set DCO
	DCOCTL = CALDCO_1MHZ;
	P1SEL = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
	P1SEL2 = BIT1 + BIT2 ;                    // P1.1 = RXD, P1.2=TXD
	UCA0CTL1 |= UCSSEL_2;                     // SMCLK
	UCA0BR0 = 104;                            // 1MHz 9600
	UCA0BR1 = 0;                              // 1MHz 9600
	UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
	IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt

	__bis_SR_register(GIE);
	//	while(1){
	//		if(slave_flag == 1){
	// 			check valid hex address?
	//			initialize I2C settings?
	//			slave_hex=convert2hex(kk);
	//			UCB0I2CSA = slave_hex; // set slave address
	//			__bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, enable I2C interrupts
	//
	//		}
	//	}
}
//void convert2hex(int index){
// Have not worked out variable types! Some sort of mis-match
// test char?
// temp int?
// strcpy(test,slave_address);
// slave_hex=strtol(test,index,16);
//}

//CHARACTER WISE ADDRESSING - Written by Erin Wiles, Ross Mitchell, Krista Stone
//Interrupt activated by keyboard presses via UART
// Sequence '#xxxx\n', variable # of bytes upto 19.
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{

	if (UCA0RXBUF == '#')							// Current character '#'?
	{
		slave_address[0]=UCA0RXBUF;					// Set '#' as 1st Character
		kk=1;										// index read for 2nd char
	}
	if (slave_address[0] == '#' && UCA0RXBUF != '#')// '#' 1st Char & current not '#'?
	{
		if (UCA0RXBUF != '\n')						// Current not '\n'?
		{	slave_address[kk]=UCA0RXBUF;			// Store current character
		kk++;										// Next index
		}
		else{
			// store '\n'? no?
			// slave_flag=1;// set flag when complete address entered
		}

	}
	else;
}



