CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic -O2
DEBUG_FLAGS = -g -DDEBUG
LDFLAGS = 

# Source files and target
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
TARGET = dns-server
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Default build target
all: dirs $(BIN_DIR)/$(TARGET)

# Create necessary directories
dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(SRC_DIR)

# Copy main.c to src directory if it exists in root
init:
	@if [ -f "main.c" ] && [ ! -f "$(SRC_DIR)/main.c" ]; then \
		echo "Copying main.c to $(SRC_DIR)/"; \
		cp main.c $(SRC_DIR)/; \
	fi

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link the target
$(BIN_DIR)/$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	@echo "Build successful! Executable created at $@"
	@ln -sf $@ $(TARGET)

# Debug build with extra flags
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean all
	@echo "Debug build completed"

# Run the server with a default resolver
run: all
	./$(BIN_DIR)/$(TARGET) --resolver 8.8.8.8:53

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(TARGET)
	@echo "Clean completed"

# Deep clean (including potential src/main.c if copied from root)
distclean: clean
	@if [ -f "main.c" ] && [ -f "$(SRC_DIR)/main.c" ]; then \
		echo "Removing copied $(SRC_DIR)/main.c"; \
		rm -f $(SRC_DIR)/main.c; \
	fi

# Run tests (placeholder for future test implementation)
test:
	@echo "No tests implemented yet"

# Install the binary to system (requires root)
install: all
	install -m 755 $(BIN_DIR)/$(TARGET) /usr/local/bin/$(TARGET)
	@echo "Installed to /usr/local/bin/$(TARGET)"

# Uninstall from system
uninstall:
	rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstalled from /usr/local/bin/$(TARGET)"

# Generate a simple .gitignore if it doesn't exist
gitignore:
	@if [ ! -f ".gitignore" ]; then \
		echo "Creating .gitignore"; \
		echo "$(BUILD_DIR)/" > .gitignore; \
		echo "$(BIN_DIR)/" >> .gitignore; \
		echo "$(TARGET)" >> .gitignore; \
		echo "*.o" >> .gitignore; \
		echo "*.so" >> .gitignore; \
		echo "*.a" >> .gitignore; \
	fi

# Help target
help:
	@echo "DNS Forwarding Server Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build the server (default target)"
	@echo "  debug      - Build with debug flags"
	@echo "  run        - Build and run the server"
	@echo "  clean      - Remove build artifacts"
	@echo "  distclean  - Deep clean including copied source files"
	@echo "  test       - Run tests (placeholder)"
	@echo "  install    - Install to /usr/local/bin (requires root)"
	@echo "  uninstall  - Remove from /usr/local/bin"
	@echo "  gitignore  - Generate a basic .gitignore file"
	@echo "  help       - Display this help information"
	@echo ""
	@echo "Build options:"
	@echo "  CC         - Compiler (current: $(CC))"
	@echo "  CFLAGS     - Compiler flags (current: $(CFLAGS))"
	@echo "  LDFLAGS    - Linker flags (current: $(LDFLAGS))"

.PHONY: all dirs init debug run clean distclean test install uninstall gitignore help