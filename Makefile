# Laptop tests. The firmware itself builds with PlatformIO (`pio run`).
REF_REPO := https://github.com/Legion2/Somfy_Remote_Lib.git
REF_DIR := test/reference
# The reference only drives its pin when built for a real board.
CXXFLAGS := -DESP32 -std=c++17 -O2 -Wall -Wextra -Ilib/rts -Itest/stub -I$(REF_DIR)/src

.PHONY: test clean

test: build/test_rts
	./build/test_rts

$(REF_DIR)/src/SomfyRemote.cpp:
	git clone --quiet --depth 1 $(REF_REPO) $(REF_DIR)

build/test_rts: test/test_rts.cpp lib/rts/rts.cpp lib/rts/rts.h test/stub/Arduino.h $(REF_DIR)/src/SomfyRemote.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ test/test_rts.cpp lib/rts/rts.cpp $(REF_DIR)/src/SomfyRemote.cpp

clean:
	rm -rf build
