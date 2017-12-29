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

void removezombie(int signo) { while ( waitpid(-1 , NULL, WNOHANG) > 0 ) ; }//cout << "remove zombie!" << endl; }

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
    		unsigned char request[MAXBUF] = {0}, reply[MAXBUF] = {0};
    		int n = read(connfd, request, MAXBUF);
    		memcpy(reply, request, MAXBUF);
    		unsigned char VN = request[0], CD = request[1];
    		unsigned int DST_PORT = request[2] << 8 | request[3];
 			unsigned int DST_IP = request[4] << 24 | request[5] << 16 | request[6] << 8 | request[7];
    		
    		int maxfd = 0;
    		fd_set rset, result_rset;
			FD_ZERO(&rset);
    		
    		if(VN != 4) exit(0);
    		
    		FILE* fptr = fopen("socks.conf" , "r");
    		char line[MAXLINE];
    		
    		bool ac = 0;
    		while(fgets(line, MAXLINE, fptr)) {
    			char* tok = strtok(line, " ");
    			tok = strtok(NULL, " ");
				char* ip = strtok(NULL, "\r\n"), *token;
				int pos = 1;
				bool check = 1;
				while(true) {
					if(pos == 1) token = strtok(ip, ".");
					else token = strtok(NULL, ".");
					if(token == NULL) break;

					if((tok[0] == 'c' && CD == 2) || (tok[0] == 'b' && CD == 1)) {
						check = 0;
						break;
					}
					
					if(strcmp("*", token) == 0) ;
					else if(atoi(token) != (int)request[3+pos]) {
						check = 0;
						break;
					}
					pos++;				
				}
				ac |= check;
    		}
    		
    		memset(reply, 0, MAXBUF);
			reply[0] = 0;
			if(ac) reply[1] = 90;
			else reply[1] = 91;

    		//if(ac) cout << "PASS firewall" << endl;
    		//else cout << "BLOCK firewall" << endl;

    		struct sockaddr_in sin;
    		
    		cout << "<S_IP>:" << inet_ntoa(cli_addr.sin_addr)  << endl;
    		cout << "<S_PORT>:" <<  ntohs(cli_addr.sin_port) << endl;
			sin.sin_addr.s_addr = htonl(DST_IP);
			sin.sin_port = htons(DST_PORT);
    		cout << "<D_IP>:" <<  inet_ntoa(sin.sin_addr) << endl;
    		cout << "<D_PORT>:" <<  ntohs(sin.sin_port) << endl;


    		if(CD == 1) { 
    			cout << "<Command>:CONNECT" << endl;
    			
    			write(connfd, reply, 8); //accept

    			if(!ac) {
    				cout << "<Reply>:Reject" << endl;
    				exit(0);
    			}
    			else cout << "<Reply>:Accept" << endl;
    			
    			struct sockaddr_in client_sin;
    			
				if(connfd > maxfd) maxfd = connfd;
				FD_SET(connfd, &rset);
    			int dstfd = socket(AF_INET, SOCK_STREAM, 0);
		 		client_sin.sin_family = AF_INET;
				client_sin.sin_addr.s_addr = htonl(DST_IP);
				client_sin.sin_port = htons(DST_PORT);
				//cout << "VN: " << (int)VN  << " CD: " << (int)CD  << " DST_PORT: " << DST_PORT << " DST_IP: " << inet_ntoa(client_sin.sin_addr) << endl;

				if(connect(dstfd, (struct sockaddr *) &client_sin, sizeof(client_sin)) < 0) continue;
				else {
					if(dstfd > maxfd) maxfd = dstfd;
					FD_SET(dstfd, &rset);
				}
				int print = 1;
				while(true) {
					int n;
					result_rset = rset;
					select(maxfd+1, &result_rset, NULL, NULL, NULL);

					if(FD_ISSET(connfd, &result_rset)) {
						memset(request, 0, MAXBUF);
						if((n = read(connfd, request, MAXBUF)) > 0) write(dstfd, request, n);
						else break;
						if(print) {
							write(1,"<Content>:", 10);
							write(1, request, 12);
							write(1, "\n", 1);
							print = 0;
						}
					}
					if(FD_ISSET(dstfd, &result_rset)) {
						memset(request, 0, MAXBUF);
						if((n = read(dstfd, request, MAXBUF)) > 0) write(connfd, request, n);
						else break;
					}
				}
    		}
    		if(CD == 2) { //bind mode
    			cout << "<Command>:BIND" << endl;

    			struct sockaddr_in sv_addr;
    			socklen_t len = sizeof(sv_addr);
    			int fd = TcpListen(&sv_addr ,len , 0);
    			getsockname(fd, (struct sockaddr *)&sv_addr, &len);
    	
    			memcpy(reply+2, &sv_addr.sin_port, 2);
    			memcpy(reply+4, &sv_addr.sin_addr.s_addr, 4);
    			
    			write(connfd, reply, 8); //need?
    			if(!ac) {
    				cout << "<Reply>:Reject" << endl;
    				exit(0);
    			}
    			else cout << "<Reply>:Accept" << endl;
    			int dstfd = accept(fd, (struct sockaddr *) &cli_addr, &clilen);
    			
    			write(connfd, reply, 8); //accept

    			FD_SET(connfd, &rset);
    			if(connfd > maxfd) maxfd = connfd;
    			FD_SET(dstfd, &rset);
    			if(dstfd > maxfd) maxfd = dstfd;
    			int print = 1;
    			while(true) {
					int n;
					result_rset = rset;
					select(maxfd+1, &result_rset, NULL, NULL, NULL);

					if(FD_ISSET(connfd, &result_rset)) {
						memset(request, 0, MAXBUF);
						if((n = read(connfd, request, MAXBUF)) > 0) write(dstfd, request, n);
						else break;
						if(print) {
							write(1,"<Content>:", 10);
							write(1, request, 12);
							write(1, "\n", 1);
							print = 0;
						}
					}
					if(FD_ISSET(dstfd, &result_rset)) {
						memset(request, 0, MAXBUF);
						if((n = read(dstfd, request, MAXBUF)) > 0) write(connfd, request, n);
						else break;
					}
				}
    		}
    		exit(0);
    	}
    	else close(connfd);
    }
}
