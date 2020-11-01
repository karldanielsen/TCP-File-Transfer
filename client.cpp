//Karl Danielsen
//204983147

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

#include <csignal>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

const uint64_t m_poly = 0x42F0E1EBA9EA3693;

uint64_t crcTable[256];

void buildCRCTable();

uint64_t calcCRC(uint8_t *message, int size);

int main(int argc, char *argv[])
{
  //Populate the crctable
  buildCRCTable();
  
  //First process the inputs
  if(argc != 4){
    cerr << "ERROR: Incorrect number of arguments" << endl;
    exit(1);
  }
  
  char* hostname = argv[1];
  unsigned int port;
  string filename = argv[3];
  
  if(!sscanf(argv[2],"%i", &port)){
    cerr << "ERROR: Port input must be a number" << endl;
    exit(1);
  }

  if(port < 1024 || port > 65535){
    cerr << "ERROR: port value must be between 1024 and 65535" << endl;
    exit(1);
  }
  
  // create a socket using TCP IP
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  //retrieve ip from hostname
  hostent* record = gethostbyname(hostname);
  if(record == NULL){
    cerr << "ERROR: Invalid hostname." << endl;
    exit(1);
  }
  in_addr *address = (in_addr*)(record->h_addr);
  
  //make socket non-blocking
  fcntl(sockfd, F_SETFL, F_GETFL | O_NONBLOCK);
  
  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);     // short, network byte order
  serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*address));
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

  // connect to the server asynchronously
  connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

  if(errno == EINVAL){
    cerr << "ERROR: Problem establishing connection." << endl;
    exit(1);
  }
  
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(sockfd,&fds);
  
  struct timeval timeout;
  timeout.tv_sec = 10;
  timeout.tv_usec = 0;

  //Status detects if a connection is established or not as connect runs async
  int status = select(sockfd+1,NULL,&fds,NULL,&timeout);
  if(status < 1){
    cerr << "ERROR: Timeout connecting to server" << endl;
    return 1;
  }
  
  //check status to make sure a connection was made and sockfd isn't in write
  //because it's empty.
  int serr;
  socklen_t socklen = sizeof status;
  getsockopt(sockfd, SOL_SOCKET,SO_ERROR, &serr, &socklen);
  if(serr != 0){
    perror("connect");
    return 2;
  }
  
  fcntl(sockfd, F_SETFL, 0);
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

  // send/receive data to/from connection
  string input;
  size_t bufLen = 1024;
  char buf[1024];
  std::stringstream ss;
  ifstream f(filename,ios::in | ios::binary); 

  struct pollfd pfd;
  pfd.fd = sockfd;
  pfd.events = POLLOUT;
  
  while (true) {
    memset(buf, '\0', sizeof(buf));

    f.read(buf,1016);
    bufLen = f.gcount();
    ss << buf;
    //need to audit size when near max
    if(ss.str().size() > 1016)
      input = ss.str().substr(0,1016);
    else
      input = ss.str();

    uint64_t crc = htobe64(calcCRC((uint8_t*)buf, bufLen));
    for(int i = 7; i > -1; i--){
      input += (char)crc;
      crc = crc >> 8;
    }

    //This does not work if the pipe opens in the instant between poll() and send().
    if(poll(&pfd,1,10000) == 0){
      cerr << "ERROR: Timeout waiting for send" << endl;
      exit(1);
    }
    if (send(sockfd, input.c_str(), input.size(), 0) == -1) {
      perror("send");
      return 4;
    }

    if (recv(sockfd, buf, bufLen, 0) == -1) {
      perror("recv");
      return 5;
    }
    //cout << buf;
    ss.str("");
    //catch when the file is empty
    if(!f)
      break;
  }
  
  close(sockfd);

  return 0;
}

void buildCRCTable(){
  uint64_t crc = 0x8000000000000000;
  int i = 1;
  do{
    if(crc & 0x8000000000000000){
      crc = (crc << 1) ^ m_poly;
    }
    else{
      crc = crc << 1;
    }
    for(int j = 0; j < i-1; j++){
      crcTable[i+j] = crc ^ crcTable[j];
    }
    i = i << 1;
  } while(i < 256);
}

uint64_t calcCRC(uint8_t *message, int size){
  uint64_t rem = 0;
  for(int i = 0; i < size; i++)
    rem = (rem >> 8) ^ (crcTable[(uint8_t)(message[i] ^ rem)]);
  return rem;
}
