# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -std=c11
LDFLAGS = -pthread -lncurses

# Target executable
TARGET = runway_simulator

# Source files
SRCS = main.c runway.c plane.c queue.c gui.c

# Object files
OBJS = $(SRCS:.c=.o)

# Header files
HEADERS = runway.h plane.h queue.h gui.h

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Build successful! Run with: ./$(TARGET)"

# Compile source files to object files
%.o: %.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(OBJS) $(TARGET)
	@echo "Clean complete."

# Run the simulator with default parameters
run: $(TARGET)
	@echo "Running simulator with default parameters..."
	./$(TARGET)

# Run with custom parameters (example)
run-demo: $(TARGET)
	@echo "Running simulator with demo parameters (15 planes, 25% emergency)..."
	./$(TARGET) -n 15 -e 25 -l 5 -t 4

# Run with GUI mode
run-gui: $(TARGET)
	@echo "Running simulator with GUI mode..."
	./$(TARGET) -g -n 10 -e 20 -l 4 -t 3

# Display help
help:
	@echo "Airport Runway Management System - Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  make          - Build the simulator"
	@echo "  make all      - Build the simulator (same as make)"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make run      - Build and run with default parameters"
	@echo "  make run-demo - Build and run with demo parameters"
	@echo "  make run-gui  - Build and run with GUI mode"
	@echo "  make help     - Display this help message"
	@echo ""
	@echo "Manual execution:"
	@echo "  ./$(TARGET) -h    - Display program usage and options"

.PHONY: all clean run run-demo run-gui help
