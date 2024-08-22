TARGET=client
CC = gcc
OFLAGS = -o
CFLAGS = -c
OBJS = main.o

$(TARGET): $(OBJS)
	$(CC) $(OFLAGS) $(TARGET) $(OBJS) -lpthread

%.o: %.c
	$(CC) -std=c99 $(CFLAGS) $<

all: $(TARGET)

clean:
	rm $(TARGET) $(OBJS)

cleanobj:
	rm *.o

