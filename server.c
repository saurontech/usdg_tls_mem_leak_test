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

	do{

		if(ssl_recv_simple(ssl, rxbuf, sizeof(rxbuf), 1000, &ssl_errno) < 0){
			ssl_errno_str(ssl, ssl_errno, errstr, sizeof(errstr));
			printf("recv failed %s\n", errstr);
			break;
		}

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

	char txbuf[10240];
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

		//ssl->ctx = initialize_ctx(argv[2], argv[3], 0, 0/*&_pwddata*/);
		ssl->ctx = ctx;

		ssl->ssl = SSL_new(ssl->ctx);

		/*ssl->sk = socket(AF_INET, SOCK_STREAM, 0);
		printf("socket %d\n", ssl->sk);
		printf("connect to %s:%d\n", argv[1], port);
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = inet_addr(argv[1]);
		saddr.sin_port = htons(port);
		if (connect(ssl->sk, (struct sockaddr*)&saddr, sizeof(saddr)) != 0) {
			printf("connect error\n");
			return 0;
		}*/
		printf("ready to accept\n");
		ssl->sk = accept(server, (struct sockaddr *)&peeraddr, &len);


		if(SSL_set_fd(ssl->ssl, ssl->sk) == 0){
			printf("SSL_set_fd failed\n");
			return 0;
		}
		__set_nonblock(ssl->sk);

		pthread_t tid;
		pthread_create(&tid, 0, server_func, ssl);

		/*if(ssl_connect_simple(ssl, 1000, &ssl_errno) != 1){
			char ssl_errstr[256];

			printf("ssl_connect_simple_fail\n");
			ssl_errno_str(ssl, ssl_errno,
					ssl_errstr, sizeof(ssl_errstr));
			return 0;
		}
		
		int _errno;
		int txlen;
		txlen = ssl_send_simple(ssl, txbuf, sizeof(txbuf), 10, &_errno);
		int rxlen = 0;
		struct timeval to;
		to.tv_sec = 10;
		to.tv_usec = 0;
		do{
			printf("ready to recv %d|%d\n", rxlen, txlen);
			rxlen += ssl_recv_simple_tv(ssl, rxbuf, sizeof(rxbuf), &to, &_errno);
		}while(timerisset(&to) || rxlen == txlen);
		
		__set_block(ssl->sk);
		SSL_shutdown(ssl->ssl);
		SSL_free(ssl->ssl);
		close(ssl->sk);*/

	}while(1);

	//sleep(1000);
	return 0;
}
