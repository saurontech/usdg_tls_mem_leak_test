#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h> 
#include <ctype.h>
#include <dirent.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <syslog.h>
#include <netinet/tcp.h>
#include <sys/mman.h>
#include <linux/limits.h>

#include "ssl_select.h"

char txbuf[10240];


void * server_func(void * input)
{
	ssl_info * ssl = (ssl_info *)input;
	char rxbuf[1024];
	char errstr[1024];
	int ssl_errno;
	if(ssl_accept_simple(ssl, 1000, &ssl_errno) <= 0){
		printf("ssl accept failed\n");
		__set_block(ssl->sk);
		SSL_shutdown(ssl->ssl);
		SSL_free(ssl->ssl);
		close(ssl->sk);
		free(ssl);
		return 0;
	}
	int firstRun = 1;
	int rxlen = 0;

	do{
		int len = ssl_recv_simple(ssl, rxbuf, sizeof(rxbuf), 3000, &ssl_errno);
		if( len < 0){
			ssl_errno_str(ssl, ssl_errno, errstr, sizeof(errstr));
			printf("recv failed %s\n", errstr);
			if(!firstRun){
				break;
			}	
			firstRun = 0;
		}else if(len == 0){
			printf("socket closed\n");
			break;
		}else{
			rxlen += len;
			printf("recv(%d)\n", rxlen);
			if(rxlen < sizeof(txbuf)){
				
				continue;
			}
		}
		len = ssl_send_simple(ssl, txbuf, sizeof(txbuf), 3000, &ssl_errno);
		if( len < 0){
			ssl_errno_str(ssl, ssl_errno, errstr, sizeof(errstr));
			printf("write failed %s\n", errstr);
			break;
		}
		printf("tx %d", len);
		rxlen = 0;
		
	}while(1);

	__set_block(ssl->sk);
	SSL_shutdown(ssl->ssl);
	SSL_free(ssl->ssl);
	close(ssl->sk);
	free(ssl);

	return 0;
}

int main(int argc, char * argv[])
{
	//SSL_CTX ctx;
	struct sockaddr_in saddr;
	int ssl_errno;
	short port;
	port = atoi(argv[1]);
	//ssl_pwd_data _pwddata;


	SSL_CTX * ctx = initialize_ctx(argv[2], argv[3], 0, 0/*&_pwddata*/);
	

	//printf("ssl ctx %p\n", ssl->ctx);

	
	char rxbuf[10240];
	for(int i; i < sizeof(txbuf); i++){
		txbuf[i] = i % 256;
	}

	int server = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in servaddr, peeraddr;

	servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
	if ((bind(server, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) {
		printf("bind failed\n");
		return 0;
	}

	if(listen(server, 32) != 0){
		printf("listen failed\n");
		return 0;
	}


	do{
		int len;
		ssl_info * ssl = sslinfo_alloc();

		ssl->ctx = ctx;

		ssl->ssl = SSL_new(ssl->ctx);
		
		printf("ready to accept\n");
		ssl->sk = accept(server, (struct sockaddr *)&peeraddr, &len);


		if(SSL_set_fd(ssl->ssl, ssl->sk) == 0){
			printf("SSL_set_fd failed\n");
			return 0;
		}
		__set_nonblock(ssl->sk);

		pthread_t tid;
		pthread_create(&tid, 0, server_func, ssl);


	}while(1);

	return 0;
}
