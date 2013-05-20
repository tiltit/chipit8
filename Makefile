PRG		= chipit8


LIBS		= -lSDL

CC		= gcc

all: $(PRG)

SOURCES=main.c
OBJECTS=$(SOURCES:.c=.o)


.c.o:
	$(CC) -c -o $@ $< 


$(PRG): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(PRG) $(LIBS)

test: $(PRG)
	./$(PRG) ./programs/Chip8_Picture.ch8

clean:
	rm *.o $(PRG)
