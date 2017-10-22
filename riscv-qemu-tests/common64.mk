CROSS = riscv64-unknown-elf-
AS = $(CROSS)as
LD = $(CROSS)ld
ASFLAGS = -g -I..
LDFLAGS = -g
QEMU = qemu-riscv64

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
