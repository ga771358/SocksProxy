#include "config.h"

int TcpListen(struct sockaddr_in* servaddr,socklen_t servlen,int port){
    int listenfd,reuse = 1;
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) cout << "socket error" << endl;
    
    bzero(servaddr, servlen);
    servaddr->sin_family = AF_INET;
    servaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    if(port) servaddr->sin_port = htons(port);
    else servaddr->sin_port = htons(INADDR_ANY);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    if(bind(listenfd, (struct sockaddr*) servaddr, servlen) < 0) cout << "bind error" << endl;
    if(listen(listenfd, MAXCONN) < 0) cout << "listen error" << endl; /*server listen port*/
    return listenfd;
}

void removezombie(int signo) { while ( waitpid(-1 , NULL, WNOHANG) > 0 ) cout << "remove zombie!" << endl; }

int readline(int fd,char* ptr) {
	char* now = ptr;
	memset(ptr, 0 , MAXBUF);
    while(read(fd, now, 1) > 0) {
        if(*now !='\n') ++now;
        else return now-ptr+1;
    }
    return now-ptr;
}

int main(int argc, char* argv[]){

	struct sockaddr_in cli_addr, serv_addr;
	socklen_t clilen = sizeof(cli_addr);
    if(argv[1] == NULL) return 0;
 
    signal(SIGCHLD, removezombie);
    int listenfd = TcpListen(&serv_addr, sizeof(serv_addr), atoi(argv[1]));

    while(true) {
    	int connfd = accept(listenfd, (struct sockaddr *) &cli_addr, &clilen);

    	int pid = fork();
    	if(pid == 0) {
    		unsigned char request[MAXBUF] = {0},reply[MAXBUF] = {0};
    		int n = read(connfd, request, MAXBUF);
    		memcpy(reply, request, MAXBUF);
    		unsigned char VN = request[0], CD = request[1];
    		unsigned int DST_PORT = request[2] << 8 | request[3];
 			unsigned int DST_IP = request[4] << 24 | request[5] << 16 | request[6] << 8 | request[7];
    		unsigned char* USER_ID = request + 8;
    		int maxfd = 0;
    		fd_set rset, wset, result_rset, result_wset;
			FD_ZERO(&rset);
			FD_ZERO(&wset);
    					   		
    		if(CD == 1) { //connect mode
    			cout << "connect" << endl;
    			reply[0] = 0,reply[1] = 90;
    			write(connfd, reply, 8); //accept

    			struct sockaddr_in client_sin;
    			
				if(connfd > maxfd) maxfd = connfd;
				FD_SET(connfd, &rset);
				FD_SET(connfd, &wset); 
				cout << "VN:" << VN  << " CD:" << CD  << " DST_PORT:" << DST_PORT << "DST_IP:" << DST_IP << endl;
    			int dstfd = socket(AF_INET, SOCK_STREAM, 0);
		 		client_sin.sin_family = AF_INET;
				client_sin.sin_addr.s_addr = htonl(DST_IP);
				client_sin.sin_port = htons(DST_PORT);
				cout << "IP:" << inet_ntoa(client_sin.sin_addr) << endl;

				if(connect(dstfd, (struct sockaddr *) &client_sin, sizeof(client_sin)) < 0) continue;
				else {
					if(dstfd > maxfd) maxfd = dstfd;
					FD_SET(dstfd, &rset);
					FD_SET(dstfd, &wset);
					cout << "connect ok!" << endl;
				}
			
				while(true) {
					int n;
					result_rset = rset, result_wset = wset;
					select(maxfd+1, &result_rset, &result_wset, NULL, NULL);

					if(FD_ISSET(connfd, &result_rset)) {
						memset(request, 0, MAXBUF);
						if((n = read(connfd, request, MAXBUF)) > 0) write(dstfd, request, n);
						else {
							close(connfd);
							FD_CLR(connfd, &rset);
							FD_CLR(connfd, &wset);
							cout << "client close" << endl;
							break;
						}
					}
					if(FD_ISSET(dstfd, &result_rset)) {
						memset(request, 0, MAXBUF);
						if((n = read(dstfd, request, MAXBUF)) > 0) write(connfd, request, n);
						else {
							cout << "server close" << endl;
							break;
						}
					}

				}

				

    		}
    		else if(CD == 2) { //bind mode
    			cout << "bind" << endl;

    			struct sockaddr_in sv_addr;
    			int fd = socket(AF_INET, SOCK_STREAM, 0);
    			socklen_t len = sizeof(sv_addr);
    			TcpListen(&sv_addr ,len , 0);
    			getsockname(fd, (struct sockaddr *)&sv_addr, &len);

    			reply[0] = 0,reply[1] = 90;
    			write(connfd, reply, 8); //accept
    			memset(reply, 0, MAXBUF);
    			reply[0] = 0,reply[1] = 90;
    			if(sv_addr.sin_port == ntohs(sv_addr.sin_port)) {
    				reply[2] = sv_addr.sin_port%256;
    				reply[3] = sv_addr.sin_port/256;
    			}
    			else {
    				reply[2] = sv_addr.sin_port/256;
    				reply[3] = sv_addr.sin_port%256;
    			}
    			int dstfd = accept(fd, (struct sockaddr *) &cli_addr, &clilen);
    			
    			write(connfd, reply, 8); //accept

    			FD_SET(connfd, &rset);
    			FD_SET(connfd, &wset);
    			if(connfd > maxfd) maxfd = connfd;
    			FD_SET(dstfd, &rset);
    			FD_SET(dstfd, &wset);
    			if(dstfd > maxfd) maxfd = dstfd;

    			while(true) {
					int n;
					result_rset = rset, result_wset = wset;
					select(maxfd+1, &result_rset, &result_wset, NULL, NULL);

					if(FD_ISSET(connfd, &result_rset)) {
						memset(request, 0, MAXBUF);
						if((n = read(connfd, request, MAXBUF)) > 0) write(dstfd, request, n);
						else {
							close(connfd);
							FD_CLR(connfd, &rset);
							FD_CLR(connfd, &wset);
							cout << "client close" << endl;
							break;
						}
					}
					if(FD_ISSET(dstfd, &result_rset)) {
						memset(request, 0, MAXBUF);
						if((n = read(dstfd, request, MAXBUF)) > 0) write(connfd, request, n);
						else {
							cout << "server close" << endl;
							break;
						}
					}

				}
    		}

    		
    		exit(0);
    	}
    	else close(connfd);
    }
}
