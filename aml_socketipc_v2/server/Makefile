# linux envs
CC = gcc

CFLAGS = -I. -I../common -pthread -g
LDFLAGS = -L..
# Assuming the .so file is in the .. directory. Adjust as necessary.
# when run, need export LD_LIBRARY_PATH=..:$LD_LIBRARY_PATH
# or install default lib

SRCS = TestServer.c
OBJS = $(SRCS:.c=.o)

TARGET = server
LIBS = -laml_socketipc_v2 -laml_log

.PHONY: all clean

all: $(TARGET)
	rm -f $(OBJS)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -o $(TARGET)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
