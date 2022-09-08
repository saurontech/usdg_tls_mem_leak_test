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

int main(int argc, char * argv[])
{
	//SSL_CTX ctx;
	struct sockaddr_in saddr;
	int ssl_errno;
	short port;
	port = atoi(argv[2]);
	//ssl_pwd_data _pwddata;

	ssl_info * ssl = sslinfo_alloc();

	ssl->ctx = initialize_ctx(argv[3], argv[4], 0, 0/*&_pwddata*/);

	printf("ssl ctx %p\n", ssl->ctx);

	unsigned char txbuf[10240];
	unsigned char rxbuf[10240];
	for(int i = 0; i < sizeof(txbuf); i++){
		txbuf[i] = i % 256;
	}



	ssl->ssl = SSL_new(ssl->ctx);

	ssl->sk = socket(AF_INET, SOCK_STREAM, 0);
	printf("socket %d\n", ssl->sk);
	printf("connect to %s:%d\n", argv[1], port);
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(argv[1]);
	saddr.sin_port = htons(port);
	if (connect(ssl->sk, (struct sockaddr*)&saddr, sizeof(saddr)) != 0) {
		printf("connect error\n");
		return 0;
	}


	if(SSL_set_fd(ssl->ssl, ssl->sk) == 0){
		printf("SSL_set_fd failed\n");
		return 0;
	}
	__set_nonblock(ssl->sk);

	if(ssl_connect_simple(ssl, 1000, &ssl_errno) != 1){
		char ssl_errstr[256];

		printf("ssl_connect_simple_fail\n");
		ssl_errno_str(ssl, ssl_errno,
				ssl_errstr, sizeof(ssl_errstr));
		return 0;
	}

	int _errno;
	int txlen;
	int rxlen;
	int txoffset;
	long long total_tx, total_rx;
	total_tx = total_rx = 0;
	txoffset = 0;
	unsigned char tmp = 0x0;
	/*txlen = ssl_send_simple(ssl, txbuf, sizeof(txbuf), 10, &_errno);
	int rxlen = 0;
	struct timeval to;
	to.tv_sec = 10;
	to.tv_usec = 0;
	do{
		printf("ready to recv %d|%d\n", rxlen, txlen);
		rxlen += ssl_recv_simple_tv(ssl, rxbuf, sizeof(rxbuf), &to, &_errno);
	}while(timerisset(&to) || rxlen == txlen);*/
	while(1){
		int need_select;
		struct timeval to, zero;
		to.tv_sec = 10;
		to.tv_usec = 0;
		zero.tv_sec = 0;
		zero.tv_usec = 0;
		need_select = 0;
		txlen = ssl_send_direct(ssl, &txbuf[txoffset], 10240 - txoffset, &_errno);
		if(txlen == SSL_OPS_SELECT){
			need_select = 1;
		}else if(txlen <= 0){
			char errstr[1024];
			ssl_errno_str(ssl, _errno, errstr, sizeof(errstr));
			printf("send error:%s\n", errstr);
			return 0;
		}else{
			txoffset += txlen;
			//txoffset %= 256;
			txoffset &= 0xff;
			total_tx += txlen;
		}

		rxlen = ssl_recv_direct(ssl, rxbuf, sizeof(rxbuf), &_errno);
		if(rxlen == SSL_OPS_SELECT){
			need_select = 1;
		}else if(rxlen <= 0){
			char errstr[1024];
			ssl_errno_str(ssl, _errno, errstr, sizeof(errstr));
			printf("recv error:%s\n", errstr);
			return 0;
		}else{
			total_rx  += rxlen;
			for(int i = 0; i < rxlen; i++){
				if(rxbuf[i] != (unsigned char)(tmp + i)){
					printf("rx data error i = %d data %hhx tmp %hhx",
					i, rxbuf[i], tmp);

					return 0;
				}
			}
			tmp = (rxbuf[(rxlen -1)]) + 1;
		}

		if(need_select){
			fd_set rfds, wfds;
			struct timeval * tv;

			if(SSL_pending(ssl->ssl)){
				tv = &zero;
			}else{
				tv = &to;
			}
			
			int maxfd = ssl_set_fds(ssl, 0, &rfds, &wfds);
			//printf("maxfd %d\n", maxfd);
			if(select(maxfd + 1, &rfds, &wfds, 0, tv) < 0){
				printf("select error\n");
				return 0;
			}
			//printf("select done\n");
		}
		printf("tx %lld rx %lld\r", total_tx, total_rx);
		fflush(stdout);
		//sleep(1);
	}

	__set_block(ssl->sk);
	SSL_shutdown(ssl->ssl);
	SSL_free(ssl->ssl);
	close(ssl->sk);


	//sleep(1000);
	return 0;
}
