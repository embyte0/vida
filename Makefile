LIBS = -lpthread -lncurses -lnet -lpcap
CFLAGS = -Wall -O2
VERSION = 0.8devel

all: compile


compile:
	@echo -e "\nWarning this is an unstable release, something could not work correctly!"
	@echo -e "\nCompiling, please wait... (version $(VERSION))\n"
	gcc $(CFLAGS) -c vida.c
	gcc $(CFLAGS) -c funz.c
	gcc $(CFLAGS) -c dnspoofing.c	
	gcc vida.o funz.o dnspoofing.o -o vida $(LIBS) 
	@echo -e "\nDone, execute with ./vida or use make install\n"      	
clean:  
	rm -f vida *~ *.o

install:
	cp vida /usr/bin/
	cp vida.1 /usr/man/man1/

uninstall:
	rm /usr/bin/vida

indent:
	indent -kr *.c *.h
	rm *~
