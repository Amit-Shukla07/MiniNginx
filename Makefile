.PHONY: all build release debug clean test test-integration benchmark format tidy sanitize help

BUILD_DIR ?= build

all: build

build:
	@cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	@cmake --build $(BUILD_DIR) -j

debug:
	@cmake -B $(BUILD_DIR)/debug -DCMAKE_BUILD_TYPE=Debug
	@cmake --build $(BUILD_DIR)/debug -j

sanitize:
	@cmake -B $(BUILD_DIR)/asan -DCMAKE_BUILD_TYPE=Debug -DMININGINX_ENABLE_ASAN=ON
	@cmake --build $(BUILD_DIR)/asan -j

test: build
	@ctest --test-dir $(BUILD_DIR) --output-on-failure

test-integration: build
	@python3 tests/integration/test_server_integration.py

benchmark: build
	@bash scripts/benchmark.sh

format:
	@find include src tests -name "*.hpp" -o -name "*.cpp" | xargs clang-format -i

tidy:
	@clang-tidy -p $(BUILD_DIR) src/**/*.cpp

clean:
	@rm -rf $(BUILD_DIR) bin lib out

help:
	@echo "MiniNginx Build Targets:"
	@echo "  make build             - Build release binary in $(BUILD_DIR)"
	@echo "  make debug             - Build debug binary in $(BUILD_DIR)/debug"
	@echo "  make sanitize          - Build with ASan & UBSan in $(BUILD_DIR)/asan"
	@echo "  make test              - Run unit tests with CTest"
	@echo "  make test-integration  - Run Python integration test suite"
	@echo "  make benchmark         - Run performance benchmark suite"
	@echo "  make format            - Run clang-format on all source/headers"
	@echo "  make clean             - Remove all build artifacts"
