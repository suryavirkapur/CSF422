ifeq ($(shell uname),Darwin) # Check if the OS is macOS (Darwin)
  CC = clang
  CFLAGS = -Wall -Xpreprocessor -fopenmp
  LDFLAGS = -L/opt/homebrew/opt/libomp/lib -lomp  
else 
  CC = gcc
  CFLAGS = -fopenmp -Wall -lm
  LDFLAGS =
endif

# Target executable name
TARGET = modified_jacobi

# Source files
SRCS = modified_jacobi.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $(TARGET)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean