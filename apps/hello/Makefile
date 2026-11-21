PS5_HOST ?= ps5
PS5_PORT ?= 9021

ifdef PS5_PAYLOAD_SDK
    include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
    $(error PS5_PAYLOAD_SDK is undefined; use `make` from the repository root)
endif

ELF := hello.elf
CFLAGS := -Wall -Werror -g -O2

all: $(ELF)

$(ELF): main.c
	$(CC) $(CFLAGS) -o $@ $^

deploy: $(ELF)
	$(PS5_DEPLOY) -h $(PS5_HOST) -p $(PS5_PORT) $^

clean:
	rm -f $(ELF)
