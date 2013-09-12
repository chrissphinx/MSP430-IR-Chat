# Project name
NAME            = shelltimer
# MSP430 MCU to compile for
CPU             = msp430g2553
# Optimisation level
OPTIMIZATION	= -Os
# Extra variables
CFLAGS          = -mmcu=$(CPU) $(OPTIMIZATION) -Wall -g
LIBEMB			= -lshell -lconio -lserial
SOURCES			= $(NAME).c pronto.c
CC              = msp430-gcc

# Build and link executable
$(NAME).elf: $(SOURCES)
ifeq (,$(findstring libemb,$(shell cat $(NAME).c)))
	$(CC) $(CFLAGS) -o $@ $(SOURCES)
else
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LIBEMB)
endif

# Flash to board with mspdebug
flash: $(NAME).elf
	mspdebug rf2500 "prog $(NAME).elf"

# Erase board
erase:
	mspdebug rf2500 erase

# Clean up temporary files
clean:
	rm -f *.elf