/* TODO: Remove cin-based file transfer and replace it with a file that transfers in 1024 byte sections.*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <csignal>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

void signalHandler(int signum);

int main(int argc, char *argv[])
{
  //First process the inputs
  if(argc != 4){
    cerr << "Error: Incorrect number of arguments" << endl;
    exit(1);
  }
  string hostname = argv[1];
  unsigned int port;
  string filename = argv[3];
  
  if(!sscanf(argv[2],"%i", &port)){
    cerr << "Error: port input must be a number" << endl;
    exit(1);
  }

  if(port < 1024 || port > 65535){
    cerr << "Error: port value must be between 1024 and 65535" << endl;
    exit(1);
  }
  
  // create a socket using TCP IP
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  // struct sockaddr_in addr;
  // addr.sin_family = AF_INET;
  // addr.sin_port = htons(40001);     // short, network byte order
  // addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  // memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));
  // if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
  //   perror("bind");
  //   return 1;
  // }

  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);     // short, network byte order
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

  // connect to the server
  if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
    perror("connect");
    return 2;
  }

  struct sockaddr_in clientAddr;
  socklen_t clientAddrLen = sizeof(clientAddr);
  if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1) {
    perror("getsockname");
    return 3;
  }

  char ipstr[INET_ADDRSTRLEN] = {'\0'};
  inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
  std::cout << "Set up a connection from: " << ipstr << ":" <<
    ntohs(clientAddr.sin_port) << std::endl;

  //Activate signal handlers
  signal(SIGTERM, signalHandler);
  signal(SIGQUIT, signalHandler);

  // send/receive data to/from connection
  bool isEnd = false;
  string input;
  char buf[1024];
  std::stringstream ss;
  ifstream f(filename,ios::in | ios::binary); 
  
  while (!isEnd) {
    memset(buf, '\0', sizeof(buf));

    f.read(buf,1024);
    if(!f){
      cout << "Bytes read: " <<f.gcount() << endl;
      cerr << "Error: Failed to place section of input file into buffer." << endl;
      exit(1);
    }
    ss << buf << endl;
    input = ss.str();
    if (send(sockfd, input.c_str(), input.size(), 0) == -1) {
      perror("send");
      return 4;
    }


    if (recv(sockfd, buf, 20, 0) == -1) {
      perror("recv");
      return 5;
    }
    std::cout << "echo: ";
    std::cout << buf << std::endl;

    if (ss.str() == "close\n")
      break;

    ss.str("");
  }

  close(sockfd);

  return 0;
}

void signalHandler(int signum){
  cout << "Process exiting with SIGQUIT/SIGTERM signal, signum = " << signum <<"..." << endl;
  exit(1);
}
