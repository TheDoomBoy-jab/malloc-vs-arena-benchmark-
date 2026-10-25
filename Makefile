CC = clang
CFLAGS = -Wall -Wextra -O3 -std=c11

TARGET = test

.PHONY: all run clean

all: $(TARGET)

$(TARGET): test.c
	$(CC) $(CFLAGS) $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)