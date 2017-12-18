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
    		cout << "firewall" << endl;
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
					pos++;
					if(strcmp("*", token) == 0) ;
					else if(atoi(token) != (int)request[3+pos]) {
						check = 0;
						break;
					}					
				}
				ac |= check;
    		}
    		
    		memset(reply, 0, MAXBUF);
			reply[0] = 0;
			if(ac) reply[1] = 90;
			else reply[1] = 91;

    		if(ac) cout << "PASS firewall" << endl;
    		else cout << "BLOCK firewall" << endl;

    		if(CD == 1) { 
    			cout << "connect mode" << endl;
    			
    			write(connfd, reply, 8); //accept

    			if(!ac) exit(0);
    			
    			struct sockaddr_in client_sin;
    			
				if(connfd > maxfd) maxfd = connfd;
				FD_SET(connfd, &rset);
    			int dstfd = socket(AF_INET, SOCK_STREAM, 0);
		 		client_sin.sin_family = AF_INET;
				client_sin.sin_addr.s_addr = htonl(DST_IP);
				client_sin.sin_port = htons(DST_PORT);
				cout << "VN: " << (int)VN  << " CD: " << (int)CD  << " DST_PORT: " << DST_PORT << " DST_IP: " << inet_ntoa(client_sin.sin_addr) << endl;

				if(connect(dstfd, (struct sockaddr *) &client_sin, sizeof(client_sin)) < 0) continue;
				else {
					if(dstfd > maxfd) maxfd = dstfd;
					FD_SET(dstfd, &rset);
				}
			
				while(true) {
					int n;
					result_rset = rset;
					select(maxfd+1, &result_rset, NULL, NULL, NULL);

					if(FD_ISSET(connfd, &result_rset)) {
						memset(request, 0, MAXBUF);
						if((n = read(connfd, request, MAXBUF)) > 0) write(dstfd, request, n);
						else break;
					}
					if(FD_ISSET(dstfd, &result_rset)) {
						memset(request, 0, MAXBUF);
						if((n = read(dstfd, request, MAXBUF)) > 0) write(connfd, request, n);
						else break;
					}
				}
    		}
    		if(CD == 2) { //bind mode
    			cout << "bind mode" << endl;

    			struct sockaddr_in sv_addr;
    			socklen_t len = sizeof(sv_addr);
    			int fd = TcpListen(&sv_addr ,len , 0);
    			getsockname(fd, (struct sockaddr *)&sv_addr, &len);
    	
    			memcpy(reply+2, &sv_addr.sin_port, 2);
    			memcpy(reply+4, &sv_addr.sin_addr.s_addr, 4);
    			
    			//write(connfd, reply, 8); //need?
    			if(!ac) exit(0);
    			int dstfd = accept(fd, (struct sockaddr *) &cli_addr, &clilen);
    			
    			write(connfd, reply, 8); //accept

    			FD_SET(connfd, &rset);
    			if(connfd > maxfd) maxfd = connfd;
    			FD_SET(dstfd, &rset);
    			if(dstfd > maxfd) maxfd = dstfd;

    			while(true) {
					int n;
					result_rset = rset;
					select(maxfd+1, &result_rset, NULL, NULL, NULL);

					if(FD_ISSET(connfd, &result_rset)) {
						memset(request, 0, MAXBUF);
						if((n = read(connfd, request, MAXBUF)) > 0) write(dstfd, request, n);
						else break;
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
