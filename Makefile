.PHONY: all clean mandibule

ARCH ?= $(shell uname -m)

DEBUG ?= 1

SAMPLE_CFLAGS := -Wall
CFLAGS += -fpic -fpie -mno-sse
CFLAGS += -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables
CFLAGS += -fno-unwind-tables -fcf-protection=none
CFLAGS += -Wno-invalid-noreturn -Wno-unused-command-line-argument

ifneq ($(filter clang,$(CC)),)
CFLAGS += -Oz
else
CFLAGS += -Os
endif

# unable to compile arm without nostdlib (uidiv divmod ...)
ifneq ($(filter arm armhf armv7 armv7l,$(ARCH)),)
CFLAGS += -nostartfiles
else
CFLAGS += -nostdlib
endif

# add -m32 so we can compile it on amd64 as well
ifneq ($(filter i386 i486 i586 i686 x86,$(ARCH)),)
SAMPLE_CFLAGS += -m32
CFLAGS += -m32
endif

# as flag
ifneq ($(filter amd64 x86_64,$(ARCH)),)
AS_FLAGS += --64
else ifneq ($(filter i386 i486 i586 i686 x86,$(ARCH)),)
AS_FLAGS += --32
endif

# ld flag

ifneq ($(filter amd64 x86_64,$(ARCH)),)
LD_FLAGS += -m elf_x86_64
else ifneq ($(filter i386 i486 i586 i686 x86,$(ARCH)),)
LD_FLAGS += -m elf_i386
endif

ifeq ($(filter amd64 x86_64,$(ARCH)),)
$(error "only amd64 are tested")
endif

# automaticaly target current arch
all: clean toinject target mandibule mandibule.shellcode

mandibule.s: src/mandibule.c
	$(CC) $(CFLAGS) -DDEBUG=$(DEBUG) -S -I src/icrt/ -o $@ $<
	sed -i 's/\.addrsig//g' $@
	sed -i '/\.section/d' $@
	sed -i '/\.p2align/d' $@
	sed -i '/_sym /d' $@
# for x86, WIP
#	sed -i 's/_GLOBAL_OFFSET_TABLE_/_GOT_FAKE_/g' $@
#	echo ".globl _GLOBAL_OFFSET_TABLE_" >> $@
#	echo "_GLOBAL_OFFSET_TABLE_:" >> $@
#	echo ".zero	1" >> $@
#	sed -i 's/@GOTOFF/-_GLOBAL_OFFSET_TABLE_/g' $@
#	sed -E -i "s/_GLOBAL_OFFSET_TABLE_\+\((.*)-(.*)\)/_GLOBAL_OFFSET_TABLE_-\2/g" $@

mandibule.o: mandibule.s
	as $(AS_FLAGS) -o $@ $<
	rm -rf $<

mandibule: mandibule.o
	ld $(LD_FLAGS) --omagic -o mandibule.elf $<
	ld $(LD_FLAGS) --oformat binary --omagic -o mandibule.shellcode $<
	rm -rf $<

toinject: samples/toinject.c
	$(CC) $(SAMPLE_CFLAGS) -o $@ $<

target: samples/target.c
	$(CC) $(SAMPLE_CFLAGS) -o $@ $<

clean:
	rm -rf mandibule.s mandibule.o mandibule.elf mandibule.shellcode target toinject 
