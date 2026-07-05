CC = gcc
CFLAGS = -Wall -Wextra -Ilexer -Iparser -I./include 

SRC = src/main.c \
      src/lexer.c \
      src/parser.c \
      src/shell.c \
      src/execute.c \
      src/builtin.c \

OBJ = $(SRC:.c=.o)

TARGET = eternal

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	@./$(TARGET)

clean:
	@echo "CLEANED"
	@rm -f $(OBJ) $(TARGET)

.PHONY: all run clean