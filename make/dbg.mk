###
### BEGIN debugging targets ###
###

TESTBIN := rv32-x86-jal
.PHONY: test-prep
test-prep: sbt qemu-tests-reset qemu-tests
	# clean

.PHONY: test
test: test-prep
	rm -f log.txt
	$(LOG) $(MAKE) $(TESTBIN)
	$(LOG) $(TESTBIN)

.PHONY: dbg
dbg:
	gdb $(TESTBIN)

###
### END debugging targets ###
###

