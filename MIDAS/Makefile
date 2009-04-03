### Compiler/linker options
CFLAGS = -g -Wall
LDFLAGS = -lm

### The targets of this project
program = midas
bin = ./
src = src/
headers = includes/

### Objects used in this Makefile
default_objects = $(src)$(program).o


all: $(program)

$(program): $(default_objects)
	@echo Compilating...
	gcc $(CFLAGS) $(src)*.o -o $(bin)$(program)
	@echo Compilation successful!

clean: 
	@echo Cleaning target...
	rm -f $(bin)$(program) $(src)*.o
	@echo Cleaning done.

%.o: %.c
	gcc -c $(CFLAGS) -I $(headers) $< -o $*.o