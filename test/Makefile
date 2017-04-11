include $(TOPDIR)/make/config.mk

BINS := rv32-hello rv32-gcc-hello rv32-clang-hello x86-clang-hello
EXTRA := rv64-hello rv64-gcc-hello rv64-clang-hello \
         rv32-gcc-linux-hello rv64-gcc-linux-hello

all: $(BINS)
extra: $(EXTRA)

include $(TOPDIR)/make/rules.mk

$(eval $(call BUILD,RV32,rv32-hello))
$(eval $(call BUILD,RV64,rv64-hello))
$(eval $(call BUILD,RV32,rv32-gcc-hello,GCC,hello))
$(eval $(call BUILD,RV64,rv64-gcc-hello,GCC,hello))
$(eval $(call BUILD,RV32,rv32-clang-hello,CLANG,hello))
$(eval $(call BUILD,RV64,rv64-clang-hello,CLANG,hello))
$(eval $(call BUILD,X86,x86-clang-hello,CLANG,hello))


# linux 32

M32 := -march=rv32g -mabi=ilp32d
CFLAGS := -Wall -Werror -O2

rv32-gcc-linux-hello: rv32-gcc-linux-hello.o
	$(RV64_LINUX_TRIPLE)-gcc $(M32) -static -o $@ $<

rv32-gcc-linux-hello.o: hello.c
	$(RV64_LINUX_TRIPLE)-gcc $(M32) $(CFLAGS) -o $@ -c $<

clean-rv32-gcc-linux-hello:
	rm -f rv32-gcc-linux-hello rv32-gcc-linux-hello.o

SPIKE32 := spike $(PK32)
run-rv32-gcc-linux-hello:
	$(SPIKE32) rv32-gcc-linux-hello

# linux 64

rv64-gcc-linux-hello: rv64-gcc-linux-hello.o
	$(RV64_LINUX_TRIPLE)-gcc -static -o $@ $<

rv64-gcc-linux-hello.o: hello.c
	$(RV64_LINUX_TRIPLE)-gcc $(CFLAGS) -o $@ -c $<

clean-rv64-gcc-linux-hello:
	rm -f rv64-gcc-linux-hello rv64-gcc-linux-hello.o

SPIKE64 := spike --isa=RV64IMAFD pk
run-rv64-gcc-linux-hello:
	$(SPIKE64) rv64-gcc-linux-hello


# run/clean

run: $(foreach prog,$(BINS),run-$(prog))

run-extra: $(foreach prog,$(EXTRA),run-$(prog))
clean-extra: $(foreach prog,$(EXTRA),clean-$(prog))

clean: $(foreach prog,$(BINS),clean-$(prog)) clean-extra
