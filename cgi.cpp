#include "config.h"

#include <fstream>
using namespace std;

ofstream ff("log.txt",ios::out);
typedef struct info {
	int connfd;
	FILE* fptr;
	char msg[MAXBUF];
	char* pos;
	int left_to_read;
	bool prompt;
	bool connected;
	bool proxy;
} Info;
Info server_info[5];

typedef struct data {
	char domain_name[100];
	int port;
	char proxy_domain_name[100];
	int proxy_port;
	char filename[100];
} Data;
Data server_data[5];

int hostname_to_ip(char* hostname, char* ip) {
	struct hostent *hst;
	struct in_addr **addr_list;
	memset(ip, 0 , 100);
	if((hst = gethostbyname(hostname)) == NULL) return 1;
	addr_list = (struct in_addr **) hst->h_addr_list;
	if(*addr_list == NULL) return 1;
	else strcpy(ip, inet_ntoa(*addr_list[0]));
	return 0;
}

void close_socket(Info& server) {
	close(server.connfd);
	server.connfd = -1;
}

void convert_html(int id, char* buffer) {
	bool line = 0;
	cout << "<script>document.all['m" << id << "'].innerHTML += \"<b>";
	for(int pos = 0,len = strlen(buffer); pos != len; pos++) {
		if(buffer[pos] == '>') cout << "&gt;";
		else if(buffer[pos] == '<') cout << "&lt;";
		else if(buffer[pos] == ' ') cout << "&nbsp;";
		else if(buffer[pos] == '\r') ;
		else if(buffer[pos] == '\n') {
			line = 1;
			cout << "</b><br>\";</script>" << endl;
		}
		else cout << buffer[pos];
	}
	if (!line) cout << "</b>\";</script>" << endl;
}

int total_read = 0, total_write = 0;
int main(int argc, char* argv[],char* envp[]) {
	 
	//char query[MAXBUF] = "h1=140.113.216.36&p1=1234&f1=t1.txt&h2=140.113.216.36&p2=1235&f2=t2.txt&h3=140.113.216.36&p3=1236&f3=t3.txt&h4=140.113.216.36&p4=1237&f4=t4.txt&h5=140.113.216.36&p5=1238&f5=t5.txt";
	char* parameter[30] = {0}, ip[100];
	char* query = getenv("QUERY_STRING");
	//ff << query << endl;
	int id = 0, para_num = 0;
	parameter[para_num++] = strtok(query, "&");
	while(parameter[para_num++] = strtok(NULL, "&"));
	//ff << para_num << endl;
	for(int k = 0; k < para_num - 1; k++) {
		strtok(parameter[k], "=");
		char* val = strtok(NULL, "&");
		if(val != NULL) {
		 	if(parameter[k][0] == 'h') strcpy(server_data[id].domain_name, val); 
		 	else if(parameter[k][0] == 'p') server_data[id].port = atoi(val); 
		 	else if(parameter[k][0] == 'f') sprintf(server_data[id].filename, "%s" , val);
		 	else if(strlen(parameter[k]) == 3 && strncmp(parameter[k],"sh", 2) == 0) strcpy(server_data[id].proxy_domain_name, val);
		 	else if(strlen(parameter[k]) == 3 && strncmp(parameter[k],"sp", 2) == 0) server_data[id].proxy_port = atoi(val);
		 	if(parameter[k][0] == 's') {
		 		server_info[id].proxy = 1;
		 	}
	 	}
	 	else if(parameter[k][0] == 's') server_info[id].proxy = 0;
	 	if(strlen(parameter[k]) == 3 && strncmp("sp",parameter[k], 2) == 0) id++;
	}
	//ff << id << endl;
	int server_num = id, maxfd = 0, conn_num = 0;
	cout << "Content-type: text/html\n\n" << endl;
	cout << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" /><title>Network Programming Homework 3</title></head>" << endl;
	cout << "<body bgcolor=#336699><font face=\"Courier New\" size=2 color=#FFFF99>" << endl;
	cout << "<table width=\"800\" border=\"1\">" << endl;
	cout << "<tr>";
	for(int id = 0; id < server_num; id++) cout << "<td>" << server_data[id].domain_name << "</td>";
	cout << "</tr><tr>";
	for(int id = 0; id < server_num; id++) cout << "<td valign=\"top\" id=\"m" << id << "\"></td>";
	cout << "</tr></table>";
			
	struct sockaddr_in client_sin;
	fd_set rset, wset, result_rset, result_wset;
	FD_ZERO(&rset);
	FD_ZERO(&wset);

	unsigned char request[MAXBUF],reply[MAXBUF];

	for(int id = 0; id < server_num; id++) {
	 	Info& server = server_info[id];
 		server.connfd = socket(AF_INET, SOCK_STREAM, 0);
 		client_sin.sin_family = AF_INET;

 		int flag = fcntl(server.connfd, F_GETFL, 0);
 		fcntl(server.connfd, F_SETFL, flag | O_NONBLOCK);

 		if(hostname_to_ip(server_data[id].domain_name, ip)) close_socket(server);
 		else {
			inet_pton(AF_INET, ip, &client_sin.sin_addr);
			client_sin.sin_port = htons(server_data[id].port);
			if(server.proxy) {

	 			request[0] = 4,request[1] = 1;
	 			memcpy(request+2, &client_sin.sin_port, 2);
	 			memcpy(request+4, &client_sin.sin_addr.s_addr, 4);
				
				hostname_to_ip(server_data[id].proxy_domain_name, ip);
				inet_pton(AF_INET, ip, &client_sin.sin_addr);
				client_sin.sin_port = htons(server_data[id].proxy_port);
			}
			
			if(connect(server.connfd, (struct sockaddr *) &client_sin, sizeof(client_sin)) < 0) {
				if(errno != EINPROGRESS) {
					close_socket(server);
					continue;
				}
				else server.connected = 0;
			}
			else {
				if(server.proxy) {
					write(server.connfd, request, 8);
					read(server.connfd, reply, MAXBUF);
					if(reply[0] == 0 && reply[1] == 90) ;
					else {
						close_socket(server);
						continue;
					}
				}
				server.connected = 1;
			}
			
			conn_num++;
			FD_SET(server.connfd, &rset);
			FD_SET(server.connfd, &wset);
			if(server.connfd > maxfd) maxfd = server.connfd;
    		server.fptr = fopen(server_data[id].filename , "r");
    		server.prompt = 0;
			server.left_to_read = 0;
		}
	}

	while(conn_num) {
		int err;
		socklen_t errlen;
	 	result_rset = rset, result_wset = wset;
	 	select(maxfd+1, &result_rset, &result_wset, NULL, NULL);

	 	for(int id = 0; id < server_num; id++) {
	 		Info& server = server_info[id];
	 		if(server.connfd > 0) {
		 		if(!server.connected) {
		 			if(FD_ISSET(server.connfd, &result_rset) || FD_ISSET(server.connfd, &result_wset)) {
		 				err = 0, errlen = sizeof(err);
		 				if (getsockopt(server.connfd, SOL_SOCKET, SO_ERROR, &err, &errlen) < 0 || err != 0) close_socket(server);
		 				else {
		 					if(server.proxy) {
								write(server.connfd, request, 8);
								read(server.connfd, reply, MAXBUF);
								if(reply[0] == 0 && reply[1] == 90) ;
								else {
									close_socket(server);
									continue;
								}
							}
		 					server.connected = 1;
		 				}
		 			}
		 		}
		 		else {
			 		if(FD_ISSET(server.connfd, &result_rset)) {

			 			char msg[MAXBUF] = {0};
			 			usleep(100000);
			 			if((err = read(server.connfd, msg, MAXBUF)) > 0) {
			 				cout << "<script>document.all['m" << id << "'].innerHTML += \"";
			 				for(int pos = 0,len = strlen(msg),first = 1; pos != len; pos++) {
			 					if(msg[pos] == '\n') cout << "<br>";
			 					else if(msg[pos] == '<') cout << "&lt;";
			 					else if(msg[pos] == '>') cout << "&gt;";
			 					else if(msg[pos] == '"') cout << "&quot;";
			 					else if(msg[pos] == ' ') {
			 						if(first && pos >=1 && msg[pos-1] == '%') first = 0;
									else if(pos >=1 && msg[pos-1] == '%') continue;
			 						cout << "&nbsp;";
			 					}
			 					else if(msg[pos] == '\r') ;
			 					else {
			 						if(msg[pos] == '%') {
			 							if(server.prompt) continue;
			 							server.prompt = 1;
			 						}
			 						cout << msg[pos];
			 					}
			 				}
			 				cout << "\";</script>" << endl;
			 			}
			 			else if(err <= 0){
			 				if(err == 0 || (err < 0 && errno != EWOULDBLOCK)) {
				 				FD_CLR(server.connfd,&rset);
				 				FD_CLR(server.connfd,&wset);
				 				close_socket(server);
				 				conn_num--;
			 				}
			 			}
			 		}

			 		if(FD_ISSET(server.connfd, &result_wset)) {
			 			if(server.left_to_read != 0) {
			 				int nwrite = write(server.connfd, server.pos, server.left_to_read);
			 				if(nwrite < 0) {
			 					if(errno != EWOULDBLOCK) return 1;
			 					else continue;
							}
						
							total_write+= nwrite;
							convert_html(id, server.pos);
			 				server.pos += nwrite;
			 				server.left_to_read -= nwrite;

			 			}
			 			else if(server.prompt){
			 				memset(server.msg, 0 , MAXBUF);
			 				int nread = 0;
			 				
			 				if(server.fptr != NULL) {
				 				if(fgets(server.msg, MAXBUF, server.fptr) != NULL) {
				 					strtok(server.msg, "\n");
				 					strcat(server.msg, "\n");
					 				nread = strlen(server.msg);
					 				total_read += nread;
					 				server.prompt = 0;
					
				 					int nwrite = write(server.connfd, server.msg, nread);
					 				
					 				if(nwrite < 0) {
				 						if(errno != EWOULDBLOCK) return 1;
				 						else continue;
									}
									total_write += nwrite;
									convert_html(id, server.msg);
					 				server.pos = server.msg + nwrite;
					 				server.left_to_read = nread - nwrite;

					 				if(strncmp("exit",server.msg, 4) == 0) {
					 					//fclose(server.fptr);
					 					shutdown(server.connfd, SHUT_WR); //total bytes
					 					FD_CLR(server.connfd, &wset);
					 				}
				 				}
			 				}
			 			}
			 		}
		 		}
	 		}
	 	}
	}
	//cout << "total_read = " << total_read << "<br>total_write = " << total_write << endl;
	cout << "</font></body></html>" << endl;
	return 0;
 }