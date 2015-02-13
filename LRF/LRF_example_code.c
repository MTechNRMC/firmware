//Code to read laser range finder values over UART


#include <msp430.h>


volatile char vec[200];                                 //Vector to store strings of length 200
volatile int vec_ptr = 0;                               //Pointer

//Function that converts string coming from serial to a number
unsigned int str2num (int stval, int etval)
{
        unsigned int outval = 0, multval = 10000, k;
        char temp_val;                                          //Not used?
        for (k=0; k<(etval-stval+1);k++)
        {
                temp_val = vec[k+stval];
                outval+=(vec[k+stval]-0x30)*multval;
                multval/=10;

        }
        return outval;
}

//Variables used
volatile unsigned int state=0, UART_EOS=0, dist_rem, stop_dist;

//Function that burns clock cycles. total = t1*t2
void burn_time (unsigned int t1, unsigned int t2)
{
        volatile unsigned int k, l;
        for (k=t1; k>0; k--)
        {
                for (l=t2;l>0;l--);
        }
}

//function the sends the signal to the lrf to gather data
void lrf_send (void)
{
        char read_vec[]="*00004#";
        volatile int k;
        for (k=0;k<7;k++)
        {
                while (!(IFG2&UCA0TXIFG));
                UCA0TXBUF=read_vec[k];

        }
}

//Main Function
int main(void)
{
        char dist_vec[5];                                               //Not used? Not needed?
        volatile unsigned int dist_val, k;
        WDTCTL = WDTPW + WDTHOLD;                // Stop WDT
        P1DIR |= 0x10;                                                  //P1.3 set to output (Connected to ON pin on the LRF)
        burn_time(50000,3);                                             //Wait..(I dont think this needs to be so long)
        P1OUT |= 0x10;                                                  //Set P1.3 high (Turns on the laser range finder)
        burn_time(30000,0);                                             //Wait
        P1OUT &=~ 0x10;                                                 //Set P1.3 low


        //UART Communication
        if (CALBC1_1MHZ==0xFF)                                  // If calibration constant erased
        {
                while(1)
                {

                        // do not load, trap CPU!!
                }
        }
        DCOCTL = 0;                               // Select lowest DCOx and MODx settings
        BCSCTL1 = CALBC1_8MHZ;                    // Set DCO
        DCOCTL = CALDCO_8MHZ;
        P1SEL = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
        P1SEL2 = BIT1 + BIT2 ;                    // P1.1 = RXD, P1.2=TXD
        UCA0CTL1 |= UCSSEL_2;                     // SMCLK
        UCA0BR0 = 69;                             // 1MHz 9600
        UCA0BR1 = 00;                             // 1MHz 9600
        UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
        UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
        IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt


        //PWM Setup (Under refinement)
        P2DIR |= BIT1 | BIT4; // P2.1 y P2.4 como salida

        P2SEL |= BIT1 | BIT4; // Asociado al Timer_A1



        TA1CCR0 = 0xFFFE; // Cargamos el periodo PWM

        TA1CCR1 = 1500*8; // PWM duty cycle, 10% CCR1/(CCR0+1) * 100

        TA1CCR2 = 1500*8; // PWM duty cycle, 50% CCR2/(CCR0+1) * 100

        TA1CCTL1 = OUTMOD_7; //Modo7 reset/set

        TA1CCTL2 = OUTMOD_7; //Modo7 reset/set

        TA1CTL = TASSEL_2 + MC_1; // Timer SMCLK Modo UP

        __bis_SR_register(GIE);       // Enter LPM0, interrupts enabled

        //Forever Loop:
        while(1)
        {
                lrf_send();                                                             //Send command to lrf
                burn_time(50000,3);                                             //Wait for response
                if (UART_EOS==1)                                                //If the UART Flag is set
                {
                        UART_EOS=0;                                                     //Reset UART Flag

                        //                                      for (k=0;k<5;k++)
                        //                                      {
                        //                                              dist_vec[k]=vec[k+10];
                        //                                      }


                        dist_val=str2num(10, 14);                       //Convert string from lrf to distance
                        burn_time(5,1);                                         //Wait
                        stop_dist = 400;                                        //Stop .4 m from wall
                        dist_rem = dist_val - stop_dist;        //Distance remaining = Actual distance from wall - Desired distance from wall
                        TA1CCR1 = 1500*8 ;                                      //Set left? motor pwm
                        TA1CCR2 = 1500*8;                                       //Set right? motor pwm
                        burn_time(50000,10);                            //Really long wait. Should be lowered

                        //              if (dist_rem > 5000)
                        //              {
                        //                      TA1CCR1 = 1500*16;
                        //                      TA1CCR2 = 1500*16;
                        //              }
                        //              if (dist_rem < 5000 && dist_rem > 1000)
                        //              {
                        //                      TA1CCR1 = (-dist_rem/10+1500)*16;
                        //                      TA1CCR2 = (dist_rem/10+1500)*16;
                        //              }
                        //              if (dist_rem <1000)
                        //              {
                        //                      TA1CCR1 = (-dist_rem/1.5+1500)*16;
                        //                      TA1CCR2 = (dist_rem/1.5+1500)*16;
                        //              }


                }
        }
}


//  Return back RXed character, confirm TX buffer is ready first
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCI0RX_ISR (void)
#else
#error Compiler not supported!
#endif
{
        char inval;

        while (!(IFG2&UCA0TXIFG));                // USCI_A0 TX buffer ready?
        inval=UCA0RXBUF;                                                //inval equals RX string
        if (inval=='#')                                                 //Is inval a '#'? Start of string
        {
                if ((vec[4]=='6')&&(vec[5]=='4'))       //Is the fifth character a 6 and the sixth character a 4? The specific string we want
                {
                        UART_EOS=1;                                             //Set UART flag
                }
                state=0;                                                        //Set state to 0
        }

        if (inval=='*')                                                 //Is inval a '*'? End of string
        {
                state++;                                                        //State ++
                vec_ptr=0;                                                      //vec_ptr = 0

        }

        if (state)                                                              //loop If state = 1
        {
                vec[vec_ptr]=inval;                                     //Vector of pointer 0 (then 1, 2, 3,) = inval
                vec_ptr++;                                                      //Increment pointer
        }

        if (vec_ptr>200)                                                //If pointer gets over 200
        {         vec_ptr = 0;                                           // Reset pointer to 0
        }
}
