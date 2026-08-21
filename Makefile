.PHONY: all build clean run

BUILD_DIR ?= build

all: build

build:
	@cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	@cmake --build $(BUILD_DIR) -j

run: build
	@./$(BUILD_DIR)/mininginx

clean:
	@rm -rf $(BUILD_DIR)
