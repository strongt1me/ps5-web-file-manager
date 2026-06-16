ifdef PS5_PAYLOAD_SDK
    include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
    $(error PS5_PAYLOAD_SDK is undefined)
endif

VERSION_TAG := v0.1
TITLE_ID    := FMGR88888
PYTHON      ?= python3
STRIP       ?= $(PS5_PAYLOAD_SDK)/bin/prospero-strip
PKG_CONFIG  ?= $(PS5_PAYLOAD_SDK)/bin/prospero-pkg-config

BIN      := web-file-mgr.elf
SRCS     := src/main.c src/websrv.c src/filemgr.c src/asset.c src/mime.c
SRCS     += src/app_installer.c src/notify.c
ASSETS   := $(wildcard assets/*)
GEN_SRCS := $(patsubst assets/%,gen/%, $(ASSETS:=.c))

CFLAGS := -Oz -fno-asynchronous-unwind-tables -fno-unwind-tables -Wall -Werror -ffunction-sections -fdata-sections -Isrc -DVERSION_TAG=\"$(VERSION_TAG)\" -DTITLE_ID=\"$(TITLE_ID)\"
CFLAGS += `$(PKG_CONFIG) libmicrohttpd --cflags`
LDFLAGS := -Wl,--gc-sections
LDADD  := `$(PKG_CONFIG) libmicrohttpd --libs`
LDADD  += -lSceIpmi -lSceAppInstUtil

.PHONY: all deps clean

all: deps $(BIN)

deps:
	@$(PKG_CONFIG) --exists libmicrohttpd || ./install-libmicrohttpd.sh

gen:
	mkdir gen

clean:
	rm -rf $(BIN) gen

gen/%.c: assets/% gen-asset-module.py gen
	$(PYTHON) gen-asset-module.py --path $* $< > $@

$(BIN): $(SRCS) $(GEN_SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDADD)
	$(STRIP) $@
