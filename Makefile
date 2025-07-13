# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -pthread -w -I/usr/include

# Need nlohmann:json - sudo apt install nlohmann-json3-dev
# Need build-essential - sudo apt-get install build-essential.

# Folders
SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include
TARGET := $(BUILD_DIR)/nats

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

# Clean up
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean