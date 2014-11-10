CC 		= gcc
SOURCES		= main.c
OBJECTS 	= $(SOURCES:.c=.o)
PROJECTNAME	= remote

all: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(PROJECTNAME)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(patsubst %,%.o,$(SOURCES:.c=)) $(PROJECTNAME)
