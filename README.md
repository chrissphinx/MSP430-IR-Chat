# MSP430 IR Chat Program #

Program using IR to communicate between two MSP430's connected via USB.

### To Do ###
- Clean up and separate send/receive
- Rework shell code, only need 'send' & 'help'
- Use only 8 bits, consecutive until 'null' transmitted
- Receive code should build string and return it

### Architecture ###
- Receive using **TIMER1_A0**
- Receive on **P2.0**
- Send using **TIMER0_A0**
- Send on **P1.6** (PWM)
