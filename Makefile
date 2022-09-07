all: __ssl_select client server client_stress
__ssl_select:
	git submodule update --init --recursive

client: client.c ./ssl_select/ssl_select.c
	${CC} $^ -o $@ -I ./ssl_select -lssl -lcrypto

server: server.c ./ssl_select/ssl_select.c
	${CC} $^ -o $@ -I ./ssl_select -lssl -lcrypto -lpthread

client_stress: client_stress.c ./ssl_select/ssl_select.c
	${CC} $^ -o $@ -I ./ssl_select -lssl -lcrypto -lpthread

clean:
	rm -f client
	rm -f server
	rm -f client_stress
