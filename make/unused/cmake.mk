###
### cmake
###

CMAKE_PKG_FILE := cmake-3.5.2-Linux-x86_64.tar.gz
CMAKE_URL := http://www.cmake.org/files/v3.5/$(CMAKE_PKG_FILE)
CMAKE_PKG := $(REMOTE_DIR)/$(CMAKE_PKG_FILE)
CMAKE_BUILD := $(BUILD_DIR)/cmake
CMAKE := $(CMAKE_BUILD)/bin/cmake

$(CMAKE_PKG):
	mkdir -p $(dir $@)
	wget $(CMAKE_URL) -O $@

$(CMAKE): $(CMAKE_PKG)
	mkdir -p $(CMAKE_BUILD)
	tar --strip-components=1 -xvf $(CMAKE_PKG) -C $(CMAKE_BUILD)
	touch $(CMAKE)

.PHONY: cmake
cmake: $(CMAKE)

