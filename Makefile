CC = gcc
CFLAGS = -Wall -Wextra -Ilexer -Iparser -I./include 

SRC = src/main.c \
      src/parse/lexer.c \
      src/parse/parser.c \
      src/shell/shell.c \
      src/shell/variable.c \
      src/shell/history.c \
      src/exec/execute.c \
      src/exec/builtin.c 

OBJ = $(SRC:.c=.o)

TARGET = eternal

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	@sudo cp eternal /usr/local/bin/

clean:
	@echo "CLEANED"
	@sudo rm /usr/local/bin/$(TARGET)
	@rm -f $(OBJ) $(TARGET)

.PHONY: all run clean