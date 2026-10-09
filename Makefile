# SPDX-License-Identifier: MPL-2.0
.PHONY: all test clean
BUILD ?= build/host
all:
	cmake -S . -B "$(BUILD)" -DCMAKE_BUILD_TYPE=Debug -DLIBESPNOW_SANITIZERS=ON
	cmake --build "$(BUILD)" --parallel
test: all
	ctest --test-dir "$(BUILD)" --output-on-failure
clean:
	cmake -E remove_directory "$(BUILD)"
