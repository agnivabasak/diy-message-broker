# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -pthread -w -I/usr/include
CXXTESTFLAGS := -std=c++17 -isystem /usr/include/gtest -pthread -w
GTEST_LIBS := -lgtest -lgtest_main -lgmock

# Need nlohmann:json - sudo apt install nlohmann-json3-dev
# Need build-essential - sudo apt-get install build-essential.
# Need GoogleTest - sudo apt-get install libgtest-dev
# Need GoogleMock - sudo apt-get install libgmock-dev

# Folders
SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include
TARGET := $(BUILD_DIR)/nats

# Test Folders
TEST_SRC := tests/test_parser.cpp tests/test_sublist.cpp tests/test_client.cpp tests/test_server_integration.cpp
SRC := src/parser.cpp src/client.cpp src/server.cpp src/sublist.cpp
TEST_TARGET := $(BUILD_DIR)/test_nats

# Source and object files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# Default rule
all: $(TARGET)

# Compile .cpp files to .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link the final executable
$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build and link test executable
$(TEST_TARGET): $(TEST_SRC) $(SRC)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXTESTFLAGS) $^ $(GTEST_LIBS) -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Clean up
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean test