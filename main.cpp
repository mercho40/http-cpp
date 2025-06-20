#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

int main() {
  // create a socket
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  // bind the socket to an address
  bind(socket_fd, (struct sockaddr *)&(struct sockaddr_in){
    .sin_family = AF_INET, .sin_port = htons(8080), .sin_addr = { INADDR_ANY }
   }
}
