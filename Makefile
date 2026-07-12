CC = gcc
CFLAGS = -Wall -Wextra -Ilexer -Iparser -I./include 

SRC = src/main.c \
      src/parse/lexer.c \
      src/parse/parser.c \
      src/shell/shell.c \
      src/shell/variable.c \
      src/shell/history.c \
      src/shell/signal.c \
      src/exec/execute.c \
      src/exec/builtin.c 

OBJ = $(SRC:.c=.o)

TARGET = lsh

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	@sudo mv $(TARGET) /usr/local/bin/
	@sudo chmod +x /usr/local/bin/lsh

clean:
	@echo "CLEANED"
	@rm -f $(OBJ) $(TARGET)
	@sudo rm /usr/local/bin/$(TARGET)

.PHONY: all run clean