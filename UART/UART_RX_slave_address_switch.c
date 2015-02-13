
//   MSP430G2xx3 MASTER Code
//
//   Description: Master 1 character from Main CPU via UART, character which
//   designates slave address ,
//
//	 TEST: Unable to change slave address outside of initialized settings
//   MSP430G2xx3 MASTER Code I2C UART TX
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

volatile unsigned int ii=0,slave_flag=0;
volatile unsigned int slave_addresses[3]={0x48,0x4A,0x4C};


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
	//	while(slave_flag){
	//			initialize I2C settings?
	//			set_slave(ii)// set slave address
	//			__bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, enable I2C interrupts
	//	}
}


//SWICH CASE ADDRESSING - Written by Erin Wiles, Ross Mitchell, Krista Stone
//Interrupt activated by keyboard presses via UART
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
	volatile unsigned int invalid=0;
	switch(UCA0RXBUF){
	case '1' :
		ii=0;
		break;
	case '2' :
		ii=1;
		break;
	case '3' :
		ii=2;
		break;
	default:
		invalid=1;
		break;//invalid
	}// Interrupts each time user enters different character
	if (invalid != 1){
		while (!(IFG2&UCA0TXIFG));                // UART TX Buffer ready?
		UCA0TXBUF=slave_addresses[ii];			  // SEND BACK slave address char
		slave_flag=1;
	}// only load if valid
}



