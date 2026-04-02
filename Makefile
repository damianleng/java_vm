CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g -Iinclude
TARGET  = jvm

SRCS = src/main.c \
       src/classfile/classfile.c \
       src/interpreter/interpreter.c \
       src/memory/heap.c \
       src/gc/gc.c \
       src/runtime/runtime.c \
       src/threads/threads.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
