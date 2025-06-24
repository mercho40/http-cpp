#include <cstring>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

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
  unordered_map<string, string> parameters;
  request(string raw_request) {
    int data_read = 0;
    string parameters_string;
    string full_path;
    for (int i = 0; i < raw_request.size(); i++) {
      // method
      if (raw_request[i] == ' ' && data_read == 0) {
        method = raw_request.substr(0, i);
        data_read++;
        continue;
      }

      // path
      if (raw_request[i] == ' ' && data_read == 1) {
        full_path =
            raw_request.substr(method.size() + 1, (i - method.size()) - 1);
        size_t query_start = full_path.find('?');
        if (query_start != string::npos) {
          path = full_path.substr(0, query_start);
          // parameters
          string parameters_string = full_path.substr(query_start + 1);
          size_t equalPos;
          size_t startPos = 0;
          for (size_t j = 0; j <= parameters_string.size(); j++) {
            if (j == parameters_string.size() || parameters_string[j] == '&') {
              if (equalPos > startPos) {
                string key =

                    parameters_string.substr(startPos, equalPos - startPos);
                string value =
                    parameters_string.substr(equalPos + 1, j - equalPos - 1);
                parameters[key] = value;
              }
              startPos = j + 1;
            } else if (parameters_string[j] == '=') {
              equalPos = j;
            }
          }

          data_read++;
        }
      }
      // version
      if (raw_request[i] == '\r' && data_read == 2) {
        version =
            raw_request.substr(method.size() + full_path.size() + 2,
                               i - (method.size() + full_path.size()) - 2);
        data_read++;
      }
      // headers
      if (raw_request[i] == '\r' && raw_request[i + 1] == '\n' &&
          raw_request[i + 2] == '\r' && raw_request[i + 3] == '\n' &&
          data_read == 3) {
        size_t headers_start =
            method.size() + 1 + full_path.size() + 1 + version.size() + 2;
        size_t headers_end = i;
        string headers_string =
            raw_request.substr(headers_start, headers_end - headers_start);
        size_t colPos;
        size_t startPos = 0;
        for (size_t j = 0; j <= headers_string.size(); j++) {
          if (j == headers_string.size() || headers_string[j] == '\r') {
            if (colPos > startPos) {
              string key = headers_string.substr(startPos, colPos - startPos);
              string value = headers_string.substr(colPos + 2, j - colPos - 2);
              headers[key] = value;
            }
            startPos = j + 2;
          } else if (headers_string[j] == ':') {
            bool found = false;
            for (int k = j - 1; k > 0; k--) {
              if (headers_string[k] == '\r') {
                break;
              } else if (headers_string[k] == ':') {
                found = true;
                break;
              }
            }
            if (!found) {
              colPos = j;
            }
          }
        }
        data_read++;
      }
      // body
      if (data_read == 4 && raw_request[i] == '\r' &&
          raw_request[i + 1] == '\n' && raw_request[i + 2] == '\r' &&
          raw_request[i + 3] == '\n') {
        size_t body_start = i + 4;
        if (body_start < raw_request.size()) {
          body = raw_request.substr(body_start);
        }
      }
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
      cout << "RAW REQUEST" << endl
           << raw_request << endl
           << "END RAW REQUEST" << endl;
      request request(raw_request);
      cout << endl << "Method: " << request.method << endl;
      cout << "Path: " << request.path << endl;
      cout << "Parameters:" << endl;
      for (auto parameter : request.parameters) {
        cout << parameter.first << " = " << parameter.second << endl;
      };
      cout << "Version: " << request.version << endl;
      cout << "Headers:" << endl;
      for (const auto &header : request.headers) {
        cout << header.first << ": " << header.second << endl;
      }
      cout << "Body: " << request.body << endl;
    }

    // create response
    response res = {200,
                    "OK",
                    "HTTP/1.1",
                    {{"Content-Type", "text/plain"}, {"Connection", "close"}},
                    "Hello Neighborhood!"};
    string response = res.build_response();
    char *message = &response[0];

    // send the response to the client
    ssize_t bytes_sent = send(newsockfd, message, strlen(message), 0);
    if (bytes_sent < 0)
      error("ERROR sending message");

    cout << endl << "Sent message to client: " << message << endl;

    // close the connection
    close(newsockfd);
  }
}
