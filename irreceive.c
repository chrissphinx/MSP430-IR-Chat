#include <msp430.h>
#include <libemb/serial/serial.h>
#include <libemb/conio/conio.h>

#define T05	300
#define T65	T05*13
#define T2	T05*4
#define T3	T05*6

unsigned int rxData = 0;					// received data: A4-A0 and C6-C0 0000 AAAA ACCC CCCC
unsigned int bitCounter = 0x80;

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;				// stop WDT
	BCSCTL1 = CALBC1_1MHZ;					// load calibrated data
	DCOCTL = CALDCO_1MHZ;

	P2DIR &= ~BIT0;							// P2.0 input  
    P2SEL |= BIT0;							// P1.1 Timer_A CCI0A
    P2SEL2 |= BIT0;
    // P1OUT &= ~BIT0;

	P1OUT &= ~(BIT0 + BIT4 + BIT6);			// P1.3-1.5 out
	P1DIR |= (BIT0 + BIT4 + BIT6); 

    TA1CTL = TASSEL_2 | MC_2;                // SMCLK, continuous mode
    TA1CCTL0 = CM_2 | CCIS_0 | CAP | CCIE;     // falling edge capture mode, CCI0A, enable IE

    serial_init(9600);

	// __bis_SR_register(LPM0_bits + GIE);		// switch to LPM0 with interrupts
	for(;;) {
		_BIS_SR(LPM0_bits + GIE);
		// cio_print((char *) "0xA90\n\r");
		__disable_interrupt();
		// cio_printf("0x%i\n\r", rxData);
		rxData = 0;
	}
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer_A (void)
{
	if(TA1CCTL0 & CAP) {						// start bit                          
		TA1CCR0 += T65;						// add 6.5 bits to counter
		TA1CCTL0 &= ~CAP;						// compare mode
	} else {
		switch (bitCounter) {
			case 0:					// received all bits
				bitCounter = 0x80;				// reset counter
                // process received data, for example toggle LEDs
                switch (rxData & 0xFE0) {	// mask device number
                	case 19:			//	Volume -	0010011 = 19
                		P1OUT ^= BIT0;
                		break;
                	case 18:			//	Volume +	0010010 = 18
                		P1OUT ^= BIT4;
                		break;
                	case 4064:			//	Power		0010101 = 21
                		P1OUT ^= BIT6;
                		break;
                }
                // cio_printf("0x%x", rxData);
                cio_printf("0x%x\n\r", rxData);
                // cio_print((char *)"\n\r");
                // rxData = 0;
                // end process received data
				TA1CCTL0 |= CAP;				// capture mode
				_BIC_SR_IRQ(LPM0_bits);
				break;   
			default:						// data bit
				if (TA1CCTL0 & SCCI) {			// bit = 1
					TA1CCR0 += T2;				// add 2 bits to counter
				} else {					// bit = 0
					rxData |= bitCounter;	// set proper bit of rxData
					TA1CCR0 += T3;				// add 3 bits to counter
				}
				bitCounter >>= 1;			// increase (shift) bit counter
				break;
		}
	}
}
