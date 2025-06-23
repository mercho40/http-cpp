#include <cstring>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define ll long long

using namespace std;

// struct sockaddr_in {
//     sa_family_t    sin_family;    // Address family (AF_INET for IPv4)
//     in_port_t      sin_port;      // Port number (16 bits)
//     struct in_addr sin_addr;      // IP address (32 bits)
//     char           sin_zero[8];   // Padding to make structure same size as
//     sockaddr
// };
// struct in_addr {
//     uint32_t       s_addr;        // IPv4 address in network byte order
// };

void error(const char *msg) {
  perror(msg);
  exit(1);
}
// Request structure
struct request {
  string method;  // HTTP method (e.g., GET, POST)
  string path;    // Request path (e.g., /index.html)
  string version; // HTTP version (e.g., HTTP/1.1)
  unordered_map<string, string>
      headers; // HTTP headers (e.g., Content-Type, User-Agent)
  string body;
  string parameters;
  request(string raw_request) {
    char buffer[4096] = {0};
    for (int i = 0; i < raw_request.size(); i++) {
      buffer[i] = raw_request[i];
    }
  }
};

// Response structure
struct response {
  int status;     // HTTP status code
  string message; // HTTP response message
  string version; // HTTP version
  unordered_map<string, string>
      headers; // HTTP headers (e.g., Content-Type, User-Agent)
  string body; // HTTP response body
  std::map<string, string> content_length; // Length of the response body
  string build_response() {
    string raw_response;

    // Build the response line
    raw_response += version + " " + to_string(status) + " " + message + "\r\n";
    for (auto header : headers) {
      raw_response += header.first + ": " + header.second + "\r\n";
    }
    raw_response += "Content-Length: " + to_string(body.size()) + "\r\n";
    raw_response += "\r\n"; // End of headers
    raw_response += body;   // Add the body

    return raw_response;
  }
};

int main() {
  // create a socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  // crate a variable server_addr of type sockaddr_in
  sockaddr_in server_addr = {0};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  // bind the socket to the address and port
  if (::bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    error("ERROR on binding");

  // listen for incoming connections
  if (listen(sockfd, 5) < 0)
    error("ERROR on listen");

  cout << "Server listening on port: " << PORT << endl;
  while (true) {
    // accept a connection
    sockaddr_in client_addr = {0};
    socklen_t client_len = sizeof(client_addr);
    int newsockfd =
        accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
    if (newsockfd < 0)
      error("ERROR on accept");

    cout << "Accepted connection from client." << endl;

    // read the request from the client
    char buffer[4096] = {0};

    // Read from socket into buffer
    ssize_t bytes_received = recv(newsockfd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received > 0) {
      string raw_request(buffer, bytes_received);
      // Parse this string with your parse_request function
      cout << raw_request << endl;
      request request(raw_request);
      cout << "Method: " << request.method << endl;
      cout << "last: " << request.method[request.method.size() - 1] << endl;
    }

    // create response
    response res = {200,
                    "OK",
                    "HTTP/1.1",
                    {{"Content-Type", "text/plain"}, {"Connection", "close"}},
                    "panfleto cacerola"};
    string response = res.build_response();
    char *message = &response[0];

    // send the response to the client
    ssize_t bytes_sent = send(newsockfd, message, strlen(message), 0);
    if (bytes_sent < 0)
      error("ERROR sending message");

    cout << "Sent message to client: " << endl << message << endl;

    // close the connection
    close(newsockfd);
  }
}
