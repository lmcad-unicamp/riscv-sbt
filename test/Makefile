### config ###

CFLAGS := -Wall -Werror -O2

AS32 := riscv32-unknown-elf-as
LD32 := riscv32-unknown-elf-ld
GCC32 := riscv32-unknown-elf-gcc

AS64 := riscv64-unknown-elf-as
LD64 := riscv64-unknown-elf-ld
GCC64 := riscv64-unknown-elf-gcc

X86_AS := i386-unknown-elf-as
X86_LD := i386-unknown-elf-ld

CLANG := clang
SYSROOT := $(TOPDIR)/toolchain/release/riscv32-unknown-elf
SYSROOT_FLAG := -isysroot $(SYSROOT) -isystem $(SYSROOT)/include
CLANG_FLAGS := --target=riscv32 -mriscv=RV32IAMFD $(SYSROOT_FLAG)
TOOLCHAIN := $(TOPDIR)/toolchain
TOOLCHAIN_RELEASE := $(TOOLCHAIN)/release
LIB := $(TOOLCHAIN_RELEASE)/riscv32-unknown-elf/lib
CRT0 := $(LIB)/crt0.o

X86_CLANG_FLAGS := --target=i386
X86_LIB := $(TOOLCHAIN)/x86/i386-unknown-elf/lib

#X86_LIB_GCC := /usr/lib/gcc/x86_64-linux-gnu/4.9.2/32
X86_LIB_GCC := $(TOOLCHAIN)/x86/lib/gcc/i386-unknown-elf/7.0.0
X86_LIBC := /usr/lib32
X86_CRT1 := $(X86_LIBC)/crt1.o
X86_CRTI := $(X86_LIBC)/crti.o
X86_CRTBEGIN := $(X86_LIB_GCC)/crtbegin.o
X86_CRTEND := $(X86_LIB_GCC)/crtend.o
X86_CRTN := $(X86_LIBC)/crtn.o
X86_LGCC := $(X86_LIB_GCC)/libgcc.a
X86_LC := $(X86_LIBC)/libc.a

### rules ###

BINS := rv32-gcc-hello rv32-as-hello rv32-clang-hello x86-clang-hello

all: $(BINS)

#
# RV32
#

# GCC
define RV32_GCC
$(1).s: $(2).c
	$(GCC32) $(CFLAGS) -o $$@ -S $$<

$(1).o: $(1).s
	$(GCC32) $(CFLAGS) -o $$@ -c $$<

$(1): $(1).o
	$(GCC32) $(CFLAGS) -o $$@ $$<

run-$(1): $(1)
	spike $(PK32) $(1)

clean-$(1):
	rm -f $(1) $(1).o $(1).s
endef

# AS
define RV32_AS
$(1).o: $(2).s
	$(AS32) -o $$@ -c $$<

$(1): $(1).o
	$(LD32) -o $$@ $$<

run-$(1): $(1)
	spike $(PK32) $(1)

clean-$(1):
	rm -f $(1) $(1).o
endef

# CLANG
define RV32_CLANG
$(1).s: $(2).c
	$(CLANG) $(CLANG_FLAGS) $(CFLAGS) -o $$@ -S $$<

$(1).o: $(1).s
	$(AS32) -o $$@ $$<

$(1): $(1).o
	$(LD32) -o $$@ -dT ldscripts/elf32lriscv.x $(CRT0) $$< -lc -lgloss -lc

run-$(1): $(1)
	spike $(PK32) $(1)

clean-$(1):
	rm -f $(1) $(1).o $(1).s
endef

#
# RV64
#

# GCC
define RV64_GCC
$(1).s: $(2).c
	$(GCC64) $(CFLAGS) -o $$@ -S $$<

$(1).o: $(1).s
	$(GCC64) $(CFLAGS) -o $$@ -c $$<

$(1): $(1).o
	$(GCC64) $(CFLAGS) -o $$@ $$<

run-$(1): $(1)
	spike --isa=RV64IMAFDC pk $(1)

clean-$(1):
	rm -f $(1) $(1).o $(1).s
endef

#
# X86
#

# CLANG
define X86_CLANG
$(1).s: $(2).c
	$(CLANG) $(X86_CLANG_FLAGS) $(CFLAGS) -fno-exceptions -fno-rtti -o $$@ -S $$<

$(1).o: $(1).s
	$(X86_AS) -o $$@ $$<

$(1): $(1).o
	$(X86_LD) -o $$@ \
		$(X86_CRT1) $(X86_CRTI) \
		$$< \
		$(X86_LC) $(X86_LGCC) $(X86_LC) \
		$(X86_CRTN)

run-$(1): $(1)
	./$(1)

clean-$(1):
	rm -f $(1) $(1).o $(1).s
endef


$(eval $(call RV32_GCC,rv32-gcc-hello,hello))
$(eval $(call RV64_GCC,rv64-gcc-hello,hello))
$(eval $(call RV32_AS,rv32-as-hello,rv32-hello))
$(eval $(call RV32_CLANG,rv32-clang-hello,hello))
$(eval $(call X86_CLANG,x86-clang-hello,hello))

run: $(foreach prog,$(BINS),run-$(prog))

clean: $(foreach prog,$(BINS),clean-$(prog))
