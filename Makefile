CFLAGS= -Wall -Wextra -DNDEBUG -O3
#CFLAGS= -Wall -Wextra -g

.PHONY: all clean

all: ringmaster player

ringmaster: ringmaster.o msg.o potato.o
	gcc ${CFLAGS} ringmaster.o msg.o potato.o -o ringmaster

ringmaster.o: ringmaster.c msg.h potato.h
	gcc ${CFLAGS} -c ringmaster.c -o ringmaster.o

msg.o: msg.c msg.h potato.h
	gcc ${CFLAGS} -c msg.c -o msg.o

player.o: player.c msg.h potato.h
	gcc ${CFLAGS} -c player.c -o player.o

potato.o: potato.c potato.h
	gcc ${CFLAGS} -c potato.c -o potato.o

player: player.o msg.o potato.o
	gcc ${CFLAGS} player.o msg.o potato.o -o player -lpthread


clean:
	rm *.o -f
	rm ringmaster player -f
