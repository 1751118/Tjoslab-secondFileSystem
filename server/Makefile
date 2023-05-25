.PHONY: all clean

CC = g++ -g
CFLAG = -Wall -fpermissive
INC_PATH = ./include/
SOURCE=$(wildcard *.cpp)

HEADER = $(wildcard $(INC_PATH)*.h)
TAR= secondFileSystem

all: $(TAR)

$(TAR): $(SOURCE) $(HEADER)
	$(CC) $(CFLAG) -I$(INC_PATH) -o $@ $^ -lpthread

clean:
	rm -f $(TAR)
