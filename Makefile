# the compiler to use.
CC = gcc

# compiler compilation options.
CFLAGS = -std=c11 -pedantic-errors -Wall -Wextra

# libraries to link against.
LFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_net

# the path to object files and executable.
BUILD_PATH = build

# the path to source files.
SRC_PATH = src

# a set of source files from the source file folder.
SRC = $(wildcard $(SRC_PATH)/*.c)

# a set of object files based on the resolved source files.
OBJ = $(SRC:$(SRC_PATH)/%.c=$(BUILD_PATH)/%.o)

# rule to compile from source to object files.
$(BUILD_PATH)/%.o: $(SRC_PATH)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

# rule to compile the executable.
all: $(OBJ)
	$(CC) -o $(BUILD_PATH)/pong.exe $(OBJ) $(CFLAGS) $(LFLAGS)
