OBJ=demo.o
TARGET=demo
CFLAGS=-O -w -fstack-protector -lm -lncurses

all: $(TARGET)

$(TARGET): $(OBJ)
	sudo cc -o $(TARGET) $(CFLAGS) $(OBJ)

demo.o: demo.c

