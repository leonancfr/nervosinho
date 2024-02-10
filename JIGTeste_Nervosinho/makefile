TOOLCHAIN_PATH ?= ~/Documentos/charlinhos/openwrt/staging_dir/toolchain-mipsel_24kc_gcc-11.2.0_musl
ROOTFS_PATH ?= ~/Documentos/charlinhos/openwrt/staging_dir/target-mipsel_24kc_musl

CC := $(TOOLCHAIN_PATH)/bin/mipsel-openwrt-linux-gcc-11.2.0

LIB_DIR := $(ROOTFS_PATH)/usr/lib
INC_DIR := $(ROOTFS_PATH)/usr/include

CFLAGS := -Wall -Wextra -Werror -I$(INC_DIR) -Iinclude 
LDFLAGS := -L$(LIB_DIR) 

SRC := 	src/main.c \

OUTPUT := JIGTest

all: $(OUTPUT)

$(OUTPUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SRC) $(LDFLAGS)

clean:
	rm -f $(OUTPUT)
