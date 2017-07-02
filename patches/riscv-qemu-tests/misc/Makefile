all = pass fail

include ../common64.mk

run-fail:
	$(QEMU) fail 2>/dev/null; test $$? = 17
