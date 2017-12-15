#include <iostream>
#include <sstream>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>
#include <netdb.h>

using namespace std;
#define MAXLINE 15000
#define MAXCONN 1000
#define MAXCLI 35
#define MAXNUM 10
#define MAXBUF 65535