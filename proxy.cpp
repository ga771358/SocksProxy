#include "config.h"

int TcpListen(struct sockaddr_in* servaddr,socklen_t servlen,int port){
    int listenfd,reuse = 1;
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) cout << "socket error" << endl;
    
    bzero(servaddr, servlen);
    servaddr->sin_family = AF_INET;
    servaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr->sin_port = htons(port);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    if(bind(listenfd, (struct sockaddr*) servaddr, servlen) < 0) cout << "bind error" << endl;
    if(listen(listenfd, MAXCONN) < 0) cout << "listen error" << endl; /*server listen port*/
    return listenfd;
}

void removezombie(int signo) { while ( waitpid(-1 , NULL, WNOHANG) > 0 ) cout << "remove zombie!" << endl; }

int readline(int fd,char* ptr) {
	char* now = ptr;
	memset(ptr, 0 , MAXLINE);
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
    		
    		
    		exit(0);
    	}
    	else close(connfd);
    }
}
