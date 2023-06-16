CFLAGS = -g -pthread#-o2

all: dnsServer dnsCL dnsResolver
	
dnsServer: dnsServer.o cache.o queries.o utils.o
	gcc ${CFLAGS} -Wall -o dnsServer dnsServer.o cache.o queries.o utils.o

dnsResolver: dnsResolver.o cache.o queries.o utils.o
	gcc ${CFLAGS} -Wall -o dnsResolver dnsResolver.o queries.o utils.o cache.o

dnsCL: dnsCL.o queries.o utils.o
	gcc ${CFLAGS} -Wall -o dnsCL dnsCL.o queries.o utils.o

dnsServer.o:dnsServer.c dnsServer.h
	gcc -c ${CFLAGS} -Wall -o dnsServer.o dnsServer.c 


dnsCL.o:dnsCL.c
	gcc -c ${CFLAGS} -Wall -o dnsCL.o dnsCL.c

utils.o:utils.c utils.h
	gcc -c ${CFLAGS} -Wall -o utils.o utils.c

cache.o:cache.c
	gcc -c ${CFLAGS} -Wall -o cache.o cache.c 
	
queries.o:queries.c queries.h
	gcc -c ${CFLAGS} -Wall -o queries.o queries.c






 
clean:
	rm *.o dnsServer dnsCL dnsResolver
	
