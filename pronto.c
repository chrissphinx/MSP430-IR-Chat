#include <msp430.h>

unsigned code[] = {
    0,
    109,
    13,
    0,
    79, 20,
    20, 20,
    20, 20,
    20, 20,
    20, 20,
    0, 0,
    0, 0,
    0, 0,
    0, 0,
    0, 0,
    0, 0,
    0, 0,
    0, 0
};
                                                    
static volatile int cycle_count;                    // IR carrier cycle count

#if 1
__attribute__((interrupt(TIMER0_A0_VECTOR), naked))
void timera0_isr(void)                              // This ISR must be very simple because there may
                                                    // be as few as 16 cycles available for it to complete
{                                                   // 6 cycles to here
  __asm__ volatile(
    " dec %0\n"                                     // --cycle_count 4 cycles
    " jnz 1f\n"                                     // if zero 2 cycles
    " bic %1, 0(r1)\n"                              // wake up main code, set stack SR to LPM0_EXIT 5 cycles
    "1:\n"
    " reti\n"                                       // 5 cycles
    : "+m" (cycle_count) : "i" (LPM0_bits)
  );
}

#else
#pragma vector = TIMER0_A0_VECTOR                   // Timer A Capture/Compare 0 interrupt
__interrupt void timera0_isr(void)                  // This ISR must be very simple because there may
                                                    // be as few as 16 cycles available for it to complete

    if(--cycle_count == 0)                          // Decrement cycle count
        _BIC_SR_IQR(LPM0_bits);                     // Wakeup main code when count reaches 0

#endif

inline void send_chunk(const unsigned *code, unsigned len)
{
    do {                                            // Do all pairs
        _BIS_SR(LPM0_bits + GIE);                   // Sleep until cycle count is zero
        TACCTL1 = OUTMOD_7;                         // Turn on IR
        cycle_count += *code++;                     // Set on cycle count
        _BIS_SR(LPM0_bits + GIE);                   // Sleep until cycle count is zero
        TACCTL1 = OUTMOD_0;                         // Turn off IR
        cycle_count += *code++;                     // Set off cycle count                                                  
    } while(--len);                                 // Decrement pair count, do next pair if not zero
}

void send_code(const unsigned *code, unsigned repeat)
{
    if(*code++) return;                             // First word must be 0 (0 = Modulated code)
                                                    // Pronto carrier freq == 4,145,152 / N
                                                    // ( 4,145,152 == 32,768 * 506 / 4 )
                                                    // 1,000,000 / 4,145,152 == 0.241246
                                                    // 0.241246 << 16 == 15,810
    const unsigned period = (((unsigned long)*code++ * 15810) + 7905) >> 16; // For 1 MHz MPU clock
    const unsigned one_len = *code++;               // One time length
    const unsigned repeat_len = *code++;            // Repeat length

    TACCR0 = period - 1 ;                           // Set timer period
    TACCR1 = TACCR0 >> 1;                           // Set timer on duration - 50% duty cycle
    cycle_count = 16;                               // Setup some initial IR off lead in
    TACCTL0 = CCIE;                                 // Enable Timer A CC0 interrupt

    if(one_len) send_chunk(code, one_len);          // Send one time chunk

    if(repeat_len) {                                // Send repeat chunk
        code += (one_len << 1);                     // Move pointer to repeat chunk
        while(repeat) {                             // Do it repeat times
            send_chunk(code, repeat_len);           // Send it
            --repeat;                               // Decrement repeat count
        }
    }

    _BIS_SR(LPM0_bits + GIE);                       // Sleep until cycle count is zero
    TACCTL0 = 0;                                    // Turn off Timer A CC0 interrupt
}

void send_char(char c)
{
    int i = 0;
    for(i = 0; i < 8; i++) {
        if(c & 0x80) {
            code[14 + 2 * i] = 39;
            code[15 + 2 * i] = 20;
        } else {
            code[14 + 2 * i] = 20;
            code[15 + 2 * i] = 20;
        }
        c <<= 1;
    }
    send_code(code, 1);
}

// int main(void)
// {
//     WDTCTL = WDTPW | WDTHOLD;                       // Disable watchdog reset
//                                                     // Run at 8 MHz
//     BCSCTL1 = CALBC1_1MHZ;                          // Note: The ideal clock freq for this code
//     DCOCTL = CALDCO_1MHZ;                           //  is 4.145152 MHz - calibrate the DCO for
//                                                     //  that freq for optimal timing - the second
//                                                     //  word of the pronto code can then be used
//                                                     //  without scaling
//     P1OUT = 0;
//     P1DIR = BIT6;                                   // Enable PWM output on P1.6
//     P1SEL = BIT6;

//     TACTL = TASSEL_2 | MC_1 /* | ID_3 */;           // Timer A config: SMCLK, count up
//     TACCTL1 = OUTMOD_0;                             // Make sure IR is off

//     __enable_interrupt();                           // Enable global interrupts

//     send_char('A');
//     return 0;
// }