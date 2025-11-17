CC = gcc
FLAGS = -Wall -Wextra -Wno-override-init -Wno-unused-value -ggdb

TARGET = main
BUILD_DIR = ./build

SRCS = main.c
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clear

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CC) $(FLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@

clear:
	rm -rf $(BUILD_DIR) $(TARGET)
