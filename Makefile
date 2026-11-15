.RECIPEPREFIX := >
PREFIX ?= arm-none-eabi-
CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
CFLAGS := -mthumb -mthumb-interwork -O2 -Wall -fno-strict-aliasing -std=gnu99
LDFLAGS := -specs=gba.specs -mthumb -mthumb-interwork

all: canal0.gba

canal0.elf: src/main.c src/game.c src/hw.h src/game.h
> $(CC) $(CFLAGS) src/main.c src/game.c $(LDFLAGS) -o $@

canal0.gba: canal0.elf
> $(OBJCOPY) -O binary $< $@
> -gbafix $@ -tCANAL0 -cCAN0

clean:
> rm -f canal0.elf canal0.gba
