#include <msp430.h>
#include <libemb/serial/serial.h>
#include <libemb/conio/conio.h>

#define T05	300
#define T25 T05*5
#define T35 T05*7
#define T50 T05*10
#define TMAX 65000

#define IR_DETECTOR_PIN BIT1

void reset();

unsigned int rxData = 0; // received data: A4-A0 and C6-C0 0000 AAAA ACCC CCCC
unsigned int bitCounter = 0;

void main(void) {
	WDTCTL = WDTPW + WDTHOLD; // stop WDT
	BCSCTL1 = CALBC1_1MHZ; // load calibrated data
	DCOCTL = CALDCO_1MHZ;

	// serial_init(9600);

	P1OUT &= ~(BIT0 + BIT6); // P1.0 & P1.6 out (LP LEDs)
	P1DIR |= (BIT0 + BIT6);

	// cio_printf("sodfsdhkjh\n\r");
	// cio_print((char *) "skfjglkfjg");

	P1REN |= IR_DETECTOR_PIN; // P1.1 pull-up
	P1OUT |= IR_DETECTOR_PIN; // P1.1 pull-up
	P1IE |= IR_DETECTOR_PIN; // P1.1 interrupt enabled
	P1IES |= IR_DETECTOR_PIN; // P1.1 high/low edge
	P1IFG &= ~IR_DETECTOR_PIN; // P1.1 IFG cleared

	CCR0 |= TMAX; // interrupt if no edge for T32
	TACTL |= TASSEL_2 + MC_1; // SMCLK, up mode

	serial_init(9600);

	__bis_SR_register(LPM0_bits + GIE);
	// switch to LPM0 with interrupts
}

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void) {

	cio_printf((char *) "INTERRUPTED\n\r");

	if (P1IFG & IR_DETECTOR_PIN) {
		P1IE &= ~IR_DETECTOR_PIN;

		if (bitCounter == 0) {
			P1IES &= ~IR_DETECTOR_PIN; // P1.1 low/high edge
			bitCounter++;
			TACTL |= TACLR;
			TACTL |= MC_1;
			CCTL0 = CCIE;
		} else {
			switch (bitCounter) {
			case 14: // received all bits
				// process received data, for example toggle LEDs
				switch (rxData & 0x001F) { // mask device number
				case 19: //	Volume -	0010011 = 19
					P1OUT ^= BIT0;
					break;
				case 18: //	Volume +	0010010 = 18
					P1OUT ^= BIT6;
					break;
				case 21: //	Power	0010101 = 21
					P1OUT ^= BIT6;
					P1OUT ^= BIT0;
					break;
				case 20: //	Mute	0010100 = 20
					P1OUT &= ~BIT6;
					P1OUT &= ~BIT0;
					break;
				}
				reset();
				break;
			case 1: // start bit?
				if (TA0R < T35) { // could also add || TA0R > T50
					reset();
				} else {
					TACTL |= TACLR;
					TACTL |= MC_1;
					bitCounter++;
				}
				break;
			default: // data bit
				rxData >>= 1;
				if (TA0R > T25) {
					rxData |= 0x0800; // set bit 12 of rxData
				}
				TACTL |= TACLR;
				TACTL |= MC_1;
				bitCounter++; // increase bit counter
				break;
			}
		}
		P1IFG &= ~IR_DETECTOR_PIN;
		P1IE |= IR_DETECTOR_PIN;
	}
}

void reset() {
	CCTL0 &= ~CCIE;
	P1IES |= IR_DETECTOR_PIN; // P1.1 high/low edge
	rxData = 0;
	bitCounter = 0;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A(void) {
	// reset
	P1IE &= ~IR_DETECTOR_PIN;
	reset();
	P1IFG &= ~IR_DETECTOR_PIN;
	P1IE |= IR_DETECTOR_PIN;
}
