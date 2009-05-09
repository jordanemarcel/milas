### Compiler/linker options
CFLAGS = -g -Wall
LDFLAGS = -lm

### The targets of this project
program = midas
bin = ./
src = src/
headers = includes/

### Objects used in this Makefile
default_objects_midas = $(src)midas.o $(src)tcp_server.o $(src)MIDAS_http_server.o $(src)daemon.o $(src)dictionary.o $(src)log.o $(src)midnet.o $(src)midas_utils.o $(src)tcp.o
default_objects_list = $(src)list.o $(src)tcp.o $(src)tcp_client.o $(src)log.o

all: clean $(program) list

$(program): $(default_objects_midas)
	@echo Compilating...
	gcc $(CFLAGS) $(default_objects_midas) -o $(bin)$(program)
	@echo Compilation successful!
	
list: $(default_objects_list)
	@echo Compilating...
	gcc $(CFLAGS) $(default_objects_list) -o $(bin)list
	@echo Compilation successful!

clean: 
	@echo Cleaning target...
	rm -f $(bin)$(program) $(src)*.o
	@echo Cleaning done.

%.o: %.c
	gcc -c $(CFLAGS) -I $(headers) $< -o $*.o