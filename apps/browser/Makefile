PS5_HOST ?= ps5
PS5_PORT ?= 9021
PS5_URL ?= https://v.qq.com

ifdef PS5_PAYLOAD_SDK
    include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
    $(error PS5_PAYLOAD_SDK is undefined; use `make` from the repository root)
endif

ELF := browser.elf
CFLAGS := -Wall -Werror -g -O2 -DPS5_URL='"$(PS5_URL)"'
LDADD := -lSceSystemService -lSceUserService

all: $(ELF)

$(ELF): main.c
	$(CC) $(CFLAGS) $(LDADD) -o $@ $^

deploy: $(ELF)
	$(PS5_DEPLOY) -h $(PS5_HOST) -p $(PS5_PORT) $^

clean:
	rm -f $(ELF)
