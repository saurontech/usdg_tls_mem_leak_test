all: client server
ssl_select:
	git submodule update --init --recursive

client: client.c ./ssl_select/ssl_select.c ssl_select
	${CC} $^ -o $@ -I ./ssl_select -lssl -lcrypto

server: server.c ./ssl_select/ssl_select.c ssl_select
	${CC} $^ -o $@ -I ./ssl_select -lssl -lcrypto -lpthread

clean:
	rm -f client
	rm -f server
