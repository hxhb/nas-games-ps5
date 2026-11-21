PS5_HOST ?= ps5
PS5_PORT ?= 9021

ifdef PS5_PAYLOAD_SDK
    include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
endif

APP_DIR := $(abspath .)
ROOT := $(abspath ../..)
BUILD ?= $(APP_DIR)/build
UI_DIR := $(APP_DIR)/ui
UI_GEN := $(APP_DIR)/tools/embed_assets.py
UI_HEADER := $(APP_DIR)/src/ui_assets.h
UI_SOURCE := $(APP_DIR)/src/ui_assets.c
UI_FILES := $(sort $(wildcard $(UI_DIR)/*))
HEADERS := $(wildcard src/*.h third_party/stb/*.h)

SOURCES := \
	src/app.c \
	src/art_thumb.c \
	src/config.c \
	src/http_server.c \
	src/install_state.c \
	src/installer.c \
	src/json.c \
	src/keep_awake.c \
	src/launcher.c \
	src/log.c \
	src/main.c \
	src/pkg_meta.c \
	src/scanner.c \
	src/source.c \
	src/source_ftp.c \
	src/source_local.c \
	src/source_prefetch.c \
	src/source_smb.c \
	src/storage.c \
	src/util.c \
	$(UI_SOURCE)

CFLAGS := -Wall -Wextra -Werror -g -O2 -std=gnu11 -D_GNU_SOURCE \
	-DNASGAMES_WITH_SMB -I$(APP_DIR)/third_party/stb
LDADD := -lSceUserService -lSceSystemService
ELF := nasgames.elf
HOST_ELF := $(BUILD)/nasgames-host
HOST_SOURCES := $(filter-out src/source_smb.c $(UI_SOURCE),$(SOURCES))
HOST_OBJECTS := $(patsubst src/%.c,$(BUILD)/host-obj/%.o,$(HOST_SOURCES)) \
	$(BUILD)/host-obj/ui_assets.o
SMB2_LIB := $(BUILD)/libsmb2/libsmb2.a
RELEASE_PREPARE_ELF := $(BUILD)/release-prepare.elf

all: $(ELF)

$(UI_SOURCE) $(UI_HEADER) &: $(UI_FILES) $(UI_GEN)
	python3 $(UI_GEN) $(UI_DIR) $(UI_HEADER) $(UI_SOURCE)

$(SMB2_LIB): $(APP_DIR)/third_party/libsmb2/config.h $(ROOT)/scripts/build-libsmb2.sh
	bash $(ROOT)/scripts/build-libsmb2.sh $(dir $(SMB2_LIB))

$(ELF): $(SOURCES) $(UI_HEADER) $(HEADERS) $(SMB2_LIB) Makefile
	$(CC) $(CFLAGS) -I$(APP_DIR)/src \
		-I$(BUILD)/libsmb2/include \
		-o $@ $(filter %.c,$^) $(SMB2_LIB) $(LDADD)

release-prepare: $(RELEASE_PREPARE_ELF)

$(RELEASE_PREPARE_ELF): tools/release-prepare.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD)/host-obj/%.o: src/%.c $(UI_HEADER) $(HEADERS) Makefile
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) -Isrc -c -o $@ $<

$(BUILD)/host-obj/ui_assets.o: $(UI_SOURCE) $(UI_HEADER) Makefile
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) -Isrc -c -o $@ $<

host: $(HOST_ELF)

$(HOST_ELF): $(HOST_OBJECTS)
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) -o $@ $^ -lpthread -lm

HOST_CC ?= cc
HOST_CFLAGS ?= -Wall -Wextra -Werror -g -O2 -std=gnu11 -D_GNU_SOURCE -DNASGAMES_HOST \
	-I$(APP_DIR)/third_party/stb

deploy: $(ELF)
	$(PS5_DEPLOY) -h $(PS5_HOST) -p $(PS5_PORT) $(ELF)

clean:
	rm -rf $(BUILD) $(ELF) $(UI_HEADER) $(UI_SOURCE)

.PHONY: all release-prepare host deploy clean
