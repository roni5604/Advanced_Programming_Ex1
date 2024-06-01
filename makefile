# Define the compiler to use
CC=gcc

# Define any compile-time flags
CTYPE=-Wall -g

# Define the C source files
SRCS=shell2.c

# Define the C object files 
OBJS=$(SRCS:.c=.o)

# Define the executable file 
MAIN=myshell

# The following part of the Makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'

all:    $(MAIN)
	@echo  Compiling $(MAIN) completed

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)
