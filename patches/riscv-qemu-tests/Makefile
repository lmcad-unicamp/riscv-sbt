dirs64 = rv64i rv64m rv64f rv64d rv64a rv64c
dirs32 = rv32i rv32f
dirs = $(dirs64) $(dirs32)

default: all
all: all64
run: run64
both: run64 run32

all64: $(patsubst %,all-%,$(dirs64))
run64: $(patsubst %,run-%,$(dirs64))
all32: $(patsubst %,all-%,$(dirs32))
run32: $(patsubst %,run-%,$(dirs32))
clean: $(patsubst %,clr-%,$(dirs))

all-%:
	$(MAKE) -C $* all

run-%:
	$(MAKE) -C $* run

clr-%:
	$(MAKE) -C $* clean
