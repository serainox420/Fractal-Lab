# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -I./imgui -I./imgui/backends
LDFLAGS = -lGL -lglfw -lGLEW -lm

# Executable name
TARGET = fractal_lab

# Source files
SOURCES = main.cpp \
          imgui/imgui.cpp \
          imgui/imgui_demo.cpp \
          imgui/imgui_draw.cpp \
          imgui/imgui_tables.cpp \
          imgui/imgui_widgets.cpp \
          imgui/backends/imgui_impl_glfw.cpp \
          imgui/backends/imgui_impl_opengl3.cpp

# Object files (compiled in the same directories as source files)
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Linking the executable
$(TARGET): $(OBJECTS)
	@echo "Linking $@"
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete! Run with 'make run'"

# Compiling source files to object files
%.o: %.cpp
	@echo "Compiling $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target to remove compiled files
clean:
	@echo "Cleaning up..."
	@rm -f $(OBJECTS) $(TARGET)

# Run target (Includes the Wayland workaround for GLEW)
run: $(TARGET)
	@echo "Launching $(TARGET)..."
	env -u WAYLAND_DISPLAY ./$(TARGET)

.PHONY: all clean run
