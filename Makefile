CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -O2 -I$(SRC_DIR) $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_image -lX11 -lXext -lm
TARGET = mousecat
SRC_DIR = src
BUILD_DIR = build
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

install-deps:
	@echo "Installing SDL2 dependencies..."
	@echo "For Ubuntu/Debian: sudo apt-get install libsdl2-dev libsdl2-image-dev"
	@echo "For Fedora: sudo dnf install SDL2-devel SDL2_image-devel"
	@echo "For Arch: sudo pacman -S sdl2 sdl2_image"

.PHONY: all clean run install-deps
