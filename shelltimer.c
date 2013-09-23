#include <msp430.h>
#include <libemb/serial/serial.h>
#include <libemb/conio/conio.h>
#include <libemb/shell/shell.h>

/*******************************************
			PROTOTYPES & GLOBALS
*******************************************/
int shell_cmd_help(shell_cmd_args *args);
int shell_cmd_argt(shell_cmd_args *args);
int shell_cmd_stop(shell_cmd_args *args);
int shell_cmd_cont(shell_cmd_args *args);
int shell_cmd_setn(shell_cmd_args *args);
int shell_cmd_sped(shell_cmd_args *args);
int shell_cmd_send(shell_cmd_args *args);
void send_char(char c);

unsigned int i = 0;											// num mask assumes P1.0 - P1.6
const char num[] = { 0x7D, 0x60, 0x3E, 0x7A, 0x63, 0x5B, 0x5F, 0x71, 0x7F, 0x73, 0x77, 0x4F, 0x0E, 0x6E, 0x1F, 0x17};

/*******************************************
			SHELL COMMANDS STRUCT
*******************************************/
shell_cmds my_shell_cmds = {
	.count 	= 7,
	.cmds	= {
		{
			.cmd  = "help",
			.desc	= "list available commands",
			.func	= shell_cmd_help
		},
		{
			.cmd  = "args",
			.desc	= "print back given arguments",
			.func	= shell_cmd_argt
		},
		{
			.cmd  = "stop",
			.desc = "disable interrupts",
			.func = shell_cmd_stop
		},
		{
			.cmd  = "cont",
			.desc = "enable interrupts",
			.func = shell_cmd_cont
		},
		{
			.cmd  = "set",
			.desc = "set timer value",
			.func = shell_cmd_setn
		},
		{
			.cmd  = "speed",
			.desc = "set timer speed",
			.func = shell_cmd_sped
		},
		{
			.cmd  = "send",
			.desc = "IR send a char",
			.func = shell_cmd_send
		}
	}
};

/*******************************************
			SHELL CALLBACK HANDLERS
*******************************************/
int shell_cmd_help(shell_cmd_args *args) {
	int k;

	for(k = 0; k < my_shell_cmds.count; k++) {
		cio_printf("%s: %s\n\r", my_shell_cmds.cmds[k].cmd, my_shell_cmds.cmds[k].desc);
	}

	return 0;
}

int shell_cmd_argt(shell_cmd_args *args) {
	int k;

	cio_print((char *)"args given:\n\r");

	for(k = 0; k < args->count; k++) {
		cio_printf(" - %s\n\r", args->args[k].val);
	}

	return 0;
}

int shell_cmd_stop(shell_cmd_args *args) {
	__disable_interrupt();

	cio_printf("OK, timer stopped\n\r");

	return 0;
}

int shell_cmd_cont(shell_cmd_args *args) {
	__enable_interrupt();

	cio_printf("OK, timer resumed\n\r");

	return 0;
}

int shell_cmd_setn(shell_cmd_args *args) {
	i = shell_parse_int(args->args[0].val);

	if(i > 15) { i = 15; cio_printf("ERROR, maximum value is 15\n\r"); }

	P1OUT = num[i];
	P1OUT |= (num[i] << 6) & BIT7;							// Fix for using UART pins
	P2OUT = (num[i] >> 2);

	cio_printf("OK, timer set to %i\n\r", i);

	return 0;
}

int shell_cmd_sped(shell_cmd_args *args) {
	int speed = shell_parse_int(args->args[0].val);
	TA0CCR0 = speed;

	cio_printf("OK, speed set to %i\n\r", speed);

	return 0;
}

int shell_cmd_send(shell_cmd_args *args) {
	char *c = args->args[0].val;

	__enable_interrupt();

	int i;
	for(i = 0; c[i] != '\0'; i++) {
		send_char(c[i]);
		__delay_cycles(500000L);
	}

	cio_printf("OK, sent '%s'\n\r", c);

	__disable_interrupt();

	return 0;
}

int shell_process(char *cmd_line) {
     return shell_process_cmds(&my_shell_cmds, cmd_line);
}

/*******************************************
			INITIALIZE
*******************************************/
void main(void) {
	WDTCTL  = WDTPW + WDTHOLD;								// Stop watchdog timer
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL  = CALDCO_1MHZ;

	// BCSCTL3 = (BCSCTL3 & ~(BIT4+BIT5)) | LFXT1S_2;		// Disable xtal

	serial_init(9600);

	// P1DIR = 0xFF;											// Set P1 as output
	// P2DIR = BIT0;											// Set P2.0 as output
	// P1OUT = 0;												// Display initially blank
	// P2OUT = 0;

	// TA0CCR0 = 0x7FFF;										// Count limit 32767, 1 second

	// TA0CCTL0 = 0x10;										// Enable counter interrupts

	// TA0CTL = TASSEL_1 + MC_1;								// Timer A 0 with ACLK @ 32KHz, count UP

    P1OUT = 0;
	P1DIR |= BIT6;                                   // Enable PWM output on P1.6
    P1SEL |= BIT6;

    TACTL = TASSEL_2 | MC_1 /* | ID_3 */;           // Timer A config: SMCLK, count up
    TACCTL1 = OUTMOD_0;                             // Make sure IR is off

	__enable_interrupt();									// Enable global interrupts

/*******************************************
			MAIN LOOP
*******************************************/
	for(;;) {
		int j = 0;											// Char array counter
		char cmd_line[90] = {0};							// Init empty array

		cio_print((char *) "$ ");							// Display prompt
		char c = cio_getc();								// Wait for a character
		while(c != '\r') {									// until return sent then ...
			if(c == 0x08) {									//   was it the delete key?
				if(j != 0) {								//   cursor NOT at start?
					cmd_line[--j] = 0; cio_printc(0x08); cio_printc(' '); cio_printc(0x08);
				}											//   delete key logic
			} else {										// otherwise ...
				cmd_line[j++] = c; cio_printc(c);			//   echo received char
			}
			c = cio_getc();									// Wait for another
		}

		cio_print((char *) "\n\n\r");						// Delimit command result

		switch(shell_process(cmd_line))						// Execute specified shell command
		{													// and handle any errors
			case SHELL_PROCESS_ERR_CMD_UNKN:
				cio_print((char *) "ERROR, unknown command given\n\r");
				break;
			case SHELL_PROCESS_ERR_ARGS_LEN:
				cio_print((char *) "ERROR, an arguement is too lengthy\n\r");
				break;
			case SHELL_PROCESS_ERR_ARGS_MAX:
				cio_print((char *) "ERROR, too many arguements given\n\r");
				break;
			default:
				break;
		}

		cio_print((char *) "\n");							// Delimit before prompt
	}
}

/*******************************************
			INTERRUPTS
*******************************************/
// #pragma vector=TIMER0_A0_VECTOR
//   __interrupt void Timer0_A0 (void) {

// 	P1OUT = num[i];
// 	P1OUT |= (num[i] << 6) & BIT7;							// Fix for using UART pins
// 	P2OUT = (num[i++] >> 2);
// 	if(i > 15) { i = 0; }
// }