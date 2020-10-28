#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <csignal>
using namespace std;

//sockfd is constant once the threads start running, so making it global is fine.
int sockfd;
string dir;

void signalHandler(int signum);

void *handleClient(void *threadid);

int main(int argc, char *argv[])
{
  //First process the inputs
  if(argc != 3){
    cerr << "Error: Incorrect number of arguments" << endl;
    exit(1);
  }
  unsigned int port;
  string dir = argv[2];

  if(!sscanf(argv[1],"%i", &port)){
    cerr << "Error: port input must be a number" << endl;
    exit(1);
  }

  if(port < 1024 || port > 65535){
    cerr << "Error: port value must be between 1024 and 65535" << endl;
    exit(1);
  }
  
  //Activate signal handlers
  signal(SIGTERM, signalHandler);
  signal(SIGQUIT, signalHandler);

  // create a socket using TCP IP
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  // allow others to reuse the address
  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    return 1;
  }

  // bind address to socket
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);     // short, network byte order
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind");
    return 2;
  }

  // set socket to listen status
  if (listen(sockfd, 1) == -1) {
    perror("listen");
    return 3;
  }


  //TODO: this is probably where the multithreading should start.
  // accept a new connection

  struct pollfd pfd;
  pfd.fd = sockfd;
  pfd.events = POLLOUT | POLLPRI | POLLIN;
  pfd.revents = POLLRDHUP | POLLERR;
  int i = 0;
  
  //poll returns 1 if a client connects, 0 otherwise.
  while(true){
    if(poll(&pfd,1,10000)){
      pthread_t thread;
      int rc;
      i++;
      rc = pthread_create(&thread, NULL, &handleClient, (void *)(intptr_t)i);
      if(rc){
	cout << "Error: No thread created, code: " << rc << endl;
	exit(7);
      }
    }
    else
      cout << "Poll Timeout, resuming..." << endl;
    pthread_exit(NULL);
  }
  return 0;
}

void *handleClient(void *threadid){
  long tid;
  tid = (long)threadid;
  cout << tid << endl;
  struct sockaddr_in clientAddr;
  socklen_t clientAddrSize = sizeof(clientAddr);
  int clientSockfd = accept(sockfd, (struct sockaddr*)&clientAddr, &clientAddrSize);
  if (clientSockfd == -1) {
    perror("accept");
    exit(4);
  }

  char ipstr[INET_ADDRSTRLEN] = {'\0'};
  inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
  std::cout << "Accept a connection from: " << ipstr << ":" <<
    ntohs(clientAddr.sin_port) << std::endl;

  // read/write data from/into the connection
  bool isEnd = false;
  char buf[1024] = {0};
  std::stringstream ss;
  ofstream output(to_string(tid) + ".file");

  while (!isEnd) {
    memset(buf, '\0', sizeof(buf));
    if (recv(clientSockfd, buf, 20, 0) == -1) {
      perror("recv");
      exit(5);
    }

    ss << buf << std::endl;
    output << buf;

    if (send(clientSockfd, buf, 20, 0) == -1) {
      perror("send");
      exit(6);
    }

    if (ss.str() == "close\n")
      break;

    ss.str("");
  }
  output.close();
  close(clientSockfd);
  pthread_exit(NULL);
}

void signalHandler(int signum){
  cout << "Process exiting with SIGQUIT/SIGTERM signal, signum = " << signum <<"..." << endl;
  exit(1);
}
