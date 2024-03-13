# possible architectures: armv6m rv32 wasm x86
TARGET_ARCH=x86
HOST_ARCH=x86

PUNYCC=../build/punycc_$(TARGET_ARCH).$(HOST_ARCH)
TARGET_LIB=../host_$(TARGET_ARCH).c

all: hello.$(TARGET_ARCH) diswasm.$(TARGET_ARCH)

$(PUNYCC): ../punycc.c
	../make.sh all

%.$(TARGET_ARCH): %.c $(TARGET_LIB)
	cat $(TARGET_LIB) $< | $(PUNYCC) > $@
	chmod +x $@

clean:
	rm -f *.armv6m *.rv32 *.wasm *.x86

.PHONY: all clean
