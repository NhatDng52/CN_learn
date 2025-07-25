CC = gcc
CFLAGS = -Wall -pthread
TARGET = udp_simulator

SRC = main.c sender.c receiver.c network_services.c

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run:
	sudo ./$(TARGET)

clean:
	rm -f $(TARGET)
