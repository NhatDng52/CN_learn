CC = gcc
CFLAGS = -Wall -pthread
TARGET = udp_simulator
SRCS = main.c sender.c receiver.c network_services.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
