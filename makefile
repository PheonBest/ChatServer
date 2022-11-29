main:
	gcc -c log/log.c -o log/log.o
	gcc -c server/db-utils.c -o server/db-utils.o
	gcc -c protocol/protocol.c -o protocol/protocol.o
	gcc -g -o server/server server/server2.c log/log.o server/db-utils.o protocol/protocol.o -lsqlite3
	gcc -g -o client/client client/chatclient.c log/log.o protocol/protocol.o

comp_run:
	gcc -c protocol/protocol.c -o protocol/protocol.o
	gcc -g -o server/server server/server2.c protocol/protocol.o
	gcc -g -o client/client client/chatclient.c protocol/protocol.o

serv:
	gcc -c log/log.c -o log/log.o
	gcc -c server/db-utils.c -o server/db-utils.o
	gcc -c protocol/protocol.c -o protocol/protocol.o
	gcc -g -o server/server server/server2.c log/log.o server/db-utils.o protocol/protocol.o -lsqlite3

cli:
	gcc -c protocol/protocol.c -o protocol/protocol.o log/log.o
	gcc -g -o client/client log/log.o client/chatclient.c protocol/protocol.o -lsqlite3

pro:
	gcc -c protocol/protocol.c -o protocol/protocol.o log/log.o
	gcc -o protocol/protocol_test log/log.o protocol/protocol_test.c protocol/protocol.o -lsqlite3

clean:
	rm */*.o
	rm protocol/protocol_test
	rm server/server
	rm client/client