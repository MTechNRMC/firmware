
//   MSP430G2xx3 MASTER Code
//
//   Description: Master receives 5 bytes of Data from Slave via I2C,
//   then Master transmits data over UART to another CPU
//   USCI_A0 RX interrupt triggers storage, then transmits over UART Buffer.
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
//   NEW FUNCTIONS convert & load_uart, variable set # of bytes by 1st slave byte
//
//
//******************************************************************************
#include <msp430.h>

void convert(int i2c_byte, int index);		// convert each i2c byte to string
void load_uart(int index);					// Load string into uart buffer

unsigned char *PRxData;                     // Pointer to RX data
unsigned char RXByteCtr;   				    // Initialized line 73
volatile unsigned char RxBuffer[128];       // Allocate 128 byte of RAM
volatile unsigned char i2c_storage[128];	// Allocate 128
volatile unsigned char uart_storage[6][6]; 	// Each data will have 'xxxxx' values
volatile unsigned int ii=0,slaveBytes=0;


int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT1
	if (CALBC1_1MHZ==0xFF){					  // If calibration constant erased
		while(1);                             // do not load, trap CPU!!
	}
	// Both UART & I2C
	P1SEL = BIT1 + BIT2 + BIT6 + BIT7;        // UART: P1.1 = RXD, P1.2=TXD
											  // & i2C: P1.6 = data line, P1.7 = clock
	P1SEL2 = BIT1 + BIT2 + BIT6 + BIT7;       // UART: P1.1 = RXD, P1.2=TXD
											  // & i2C: P1.6 = data line, P1.7 = clock
	//UART
	DCOCTL = 0;                               // Select lowest DCOx and MODx settings
	// To calculate Clock Settings See http://mspgcc.sourceforge.net/baudrate.html
	// TEST: Attempted to Set Clock to 16MHz, & Baud of 1M bps
	// Values could be seen on oscilloscope but not read out in PuTTY (even with Baud speed changed)
//	BCSCTL1 = CALBC1_16MHZ;                    // Set DCO
//	DCOCTL = CALDCO_16MHZ;
//	UCA0CTL1 |= UCSSEL_2;                      // SMCLK
//	UCA0BR0 = 16 OR 10???;                        // 16MHz 1M baud
//	UCA0BR1 = 00;                              // 16MHz 1M baud
	BCSCTL1 = CALBC1_1MHZ;                    // Set DCO
	DCOCTL = CALDCO_1MHZ;
	UCA0CTL1 |= UCSSEL_2;                     // SMCLK
	UCA0BR0 = 104;                            // 1MHz 9600
	UCA0BR1 = 0;                              // 1MHz 9600
	UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
	// i2C
	UCB0CTL1 |= UCSWRST;                      // Enable SW reset
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
	UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
	UCB0BR0 = 12;                             // fSCL = SMCLK/12 = ~100kHz
	UCB0BR1 = 0;
	UCB0I2CSA = 0x4A;                         // Slave Address initialization
	UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
	// Enable interrupts
	IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
	IE2 |= UCB0RXIE;                          // Enable USCI_B0 RX interrupt
	__bis_SR_register(GIE);       			  // interrupts enabled


	while (1){
		PRxData = (unsigned char *)RxBuffer;  // Start of RX buffer
		RXByteCtr =20;    					  // Initialize to MAX VALUE
		while (UCB0CTL1 & UCTXSTP);           // Ensure stop condition got sent
		UCB0CTL1 |= UCTXSTT;                  // I2C start condition
		__bis_SR_register(CPUOFF + GIE);      // Enter LPM0 w/ interrupts
											  // Remain in LPM0 until all data
											  // is RX'd //
		__no_operation();
	}
}
// Convert each byte to ASCII Char Values so it can be sent via UART
// Modified by Erin Wiles (Bryce Hill Instruction)
void convert(int i2c_val, int ii){

	volatile unsigned int jj=0,temp=0,bit_position=10000;
	for (jj=0;jj<slaveBytes;jj++){
		temp=i2c_val/bit_position;
		i2c_val-=temp*bit_position;
		bit_position/=10;
		temp+=0x30;
		uart_storage[ii][jj]=temp;
		//temp=0;
	}// MSB -> LSB
}
//Load data onto UART - Written by Erin Wiles
void load_uart(int ii){
	volatile unsigned int jj=0;
	for(jj=0;jj<slaveBytes;jj++){
		while (!(IFG2&UCA0TXIFG));                // UART TX buffer ready?
		//UCA0TXBUF = 'x';
		UCA0TXBUF = uart_storage[ii][jj];		  // load buffer one character at a time
	}// Load buffer character by character
}
//-------------------------------------------------------------------------------
// The USCI_B0 data ISR is used to move received data from the I2C slave
// to the MSP430 memory. It is structured such that it can be used to receive
// any 2+ number of bytes by reading the first byte from the slave which is the
// byte count.
// Written by Erin Wiles, Ross Mitchell, and Krista Stone.
//-------------------------------------------------------------------------------
#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
{
	RXByteCtr--;                              	// Initialized on line 73
	if (RXByteCtr){
		if (ii == 0){
			slaveBytes=UCB0RXBUF;				// Byte# fits 1st Slave Byte
			RXByteCtr=slaveBytes-1;
		}
		i2c_storage[ii]=UCB0RXBUF; 				// Move each I2C byte to storage
		convert(i2c_storage[ii],ii);			// Convert byte to string
		load_uart(ii);
		//		while (!(IFG2&UCA0TXIFG));      // UART TX buffer ready?
		//		UCA0TXBUF = 'x';                // Test Buffer
		ii++;
	}
	else{
		ii=slaveBytes-1;						// Unclear to team, why necessary
		i2c_storage[ii]=UCB0RXBUF;				// Move final RX data to PRxData
		convert(i2c_storage[ii],ii);
		load_uart(ii);
				while (!(IFG2&UCA0TXIFG));      // USCI_A0 TX buffer ready?
				UCA0TXBUF='\n';					// format easier reading
		UCB0CTL1 |= UCTXSTP;					// Generate I2C stop condition
		ii=0;
		__bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
	}
}



