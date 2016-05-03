CC ?= gcc
LD ?= ld

CFLAGS := -g -m32 -mno-mmx -mno-sse -mno-sse2 -ffreestanding \
	-Wall -Wextra -Werror -pedantic -std=c99 \
	-Wframe-larger-than=4096 -Wstack-usage=4096 -Wno-unknown-warning-option
AFLAGS := -g -m32 -D__ASM_FILE__
LFLAGS := -nostdlib -z max-page-size=0x1000

SRC := main.c list.c serial.c console.c vsinkprintf.c stdio.c ctype.c \
	string.c stdlib.c acpi.c pci.c
OBJ := $(SRC:.c=.o)
DEP := $(SRC:.c=.d)

ASM := boot.S
AOBJ:= $(ASM:.S=.o)
ADEP:= $(ASM:.S=.d)

all: kernel

kernel: $(AOBJ) $(OBJ) kernel.ld
	$(LD) $(LFLAGS) -T kernel.ld -o $@ $(AOBJ) $(OBJ)

%.o: %.S
	$(CC) $(AFLAGS) -MMD -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@

-include $(DEP)
-include $(ADEP)

.PHONY: clean
clean:
	rm -f kernel $(AOBJ) $(OBJ) $(DEP) $(ADEP)
