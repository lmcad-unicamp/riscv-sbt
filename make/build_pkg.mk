###
### gen MAKEFILE
###
define RULE_MAKEFILE =
$$($(1)_MAKEFILE): $$($(1)_DEPS)
	@echo "*** $$@: $$? ***"
	if [ ! -d "$$($(1)_BUILD)" ]; then mkdir -p $$($(1)_BUILD); fi
	cd $$($(1)_BUILD) && \
	$$($(1)_CONFIGURE)
	touch $$@
endef


###
### build
###
define RULE_BUILD =
# unconditional build
.PHONY: $$($(1)_ALIAS)-build
$$($(1)_ALIAS)-build:
	@echo "*** $$@ ***"
	$(MAKE) -C $$($(1)_BUILD) $$($(1)_MAKE_FLAGS) $(MAKE_OPTS)

### serial build
.PHONY: $$($(1)_ALIAS)-build1
$$($(1)_ALIAS)-build1:
	$(MAKE) -C $$($(1)_BUILD)

#### build only when MAKEFILE changes
$$($(1)_OUT): $$($(1)_MAKEFILE)
	$(MAKE) $$($(1)_ALIAS)-build
	touch $$@
endef


###
### update files
###

ALL_FILES := find \! -type d | sed "s@^\./@@; /^pkg\//d;" | sort
DIFF_FILTER := grep "^< " | sed "s/^< //"
NEW_FILES := bash -c \
             'diff <($(ALL_FILES)) <(cat pkg/all.files) | $(DIFF_FILTER)'

define UPDATE_FILES
	cd $(TOOLCHAIN) && \
	cat pkg/$$($(1)_ALIAS).files >> pkg/all.files && \
	sort pkg/all.files -o pkg/all.files
endef


define RULE_UPDATE_FILELIST =
.PHONY: $$($(1)_ALIAS)-update-filelist
$$($(1)_ALIAS)-update-filelist:
	@echo "*** $$@ ***"
	echo "Updating file list..."
	if [ -f $$(TOOLCHAIN)/pkg/$$($(1)_ALIAS).files ]; then \
		echo "Skipped: file list already exists"; \
	else \
		if [ ! -f $$(TOOLCHAIN)/pkg/all.files ]; then \
			mkdir -p $$(TOOLCHAIN)/pkg; \
			touch $$(TOOLCHAIN)/pkg/all.files; \
		fi; \
		cd $$(TOOLCHAIN) && \
		$$(NEW_FILES) > pkg/$$($(1)_ALIAS).files && \
		$(call UPDATE_FILES,$(1)); \
	fi

endef

###
### install
###
define RULE_INSTALL =
# unconditional install
.PHONY: $$($(1)_ALIAS)-install
$$($(1)_ALIAS)-install:
	@echo "*** $$@ ***"
ifeq ($$($(1)_INSTALL),)
	$(MAKE) -C $$($(1)_BUILD) install
else
	$$($(1)_INSTALL)
endif
ifneq ($$($(1)_POSTINSTALL),)
	$$($(1)_POSTINSTALL)
endif
	$(MAKE) $$($(1)_ALIAS)-update-filelist

### install only when build OUTPUT changes
$$($(1)_TOOLCHAIN): $$($(1)_OUT)
	$(MAKE) $$($(1)_ALIAS)-install
	touch $$@
endef


###
### alias to invoke to build and install target
###
define RULE_ALIAS =
.PHONY: $$($(1)_ALIAS)
$$($(1)_ALIAS): $$($(1)_TOOLCHAIN)
endef

###
### rm pkg
###
define RULE_RMPKG =
$$($(1)_ALIAS)-rmpkg:
	cd $(TOOLCHAIN) && \
		if [ -f pkg/$$($(1)_ALIAS).files ]; then \
			rm -f `cat pkg/$$($(1)_ALIAS).files` && \
			rm -f pkg/$$($(1)_ALIAS).files && \
			$(ALL_FILES) > pkg/all.files; \
		fi || true
endef

###
### clean
###
define RULE_CLEAN =
$$($(1)_ALIAS)-clean: $$($(1)_ALIAS)-rmpkg
	rm -rf $$($(1)_BUILD)
endef


###
### all rules above
###
define RULE_ALL =
$(call RULE_MAKEFILE,$(1))
$(call RULE_BUILD,$(1))
$(call RULE_UPDATE_FILELIST,$(1))
$(call RULE_INSTALL,$(1))
$(call RULE_ALIAS,$(1))
$(call RULE_CLEAN,$(1))
endef


all_files:
	cd $(TOOLCHAIN) && \
	$(ALL_FILES)

new_files:
	cd $(TOOLCHAIN) && \
	$(NEW_FILES)

$(eval UPDATE_FILE := $(call UPDATE_FILES,$(PKG)))
update_files:
	f="$(TOOLCHAIN)/pkg/$($(PKG)_ALIAS).files" && \
	if [ ! -f "$$f" ]; then \
		echo "Invalid PKG: \"$(PKG)\""; \
		exit 1; \
	fi
	$(UPDATE_FILE)

