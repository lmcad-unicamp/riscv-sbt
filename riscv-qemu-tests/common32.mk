CROSS = riscv64-unknown-linux-gnu-
AS = $(CROSS)as -march=rv32g
LD = $(CROSS)ld -m elf32lriscv
ASFLAGS = -g -I..
LDFLAGS = -g
QEMU = qemu-riscv32

.SUFFIXES:

run = $(patsubst %,run-%,$(all))

all: $(all)

run: $(all) $(run)

$(all): %: %.o
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

run-%: %
	$(QEMU) $<

clean:
	rm -f *.o $(all)
