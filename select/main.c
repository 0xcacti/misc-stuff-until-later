#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 9090
#define BACKLOG 5
#define BUFFER_SIZE 8

int start_server(void) {
  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1)
    return -1;

  fd_set read_fds, write_fds, except_fds;
  FD_ZERO(&read_fds);
  FD_ZERO(&write_fds);
  FD_ZERO(&except_fds);

  struct sockaddr_in server_info = {0};
  server_info.sin_family = AF_INET;
  server_info.sin_addr.s_addr = INADDR_ANY;
  server_info.sin_port = htons(PORT);

  if (bind(sfd, (struct sockaddr *)&server_info, sizeof(server_info)) == -1) {
    perror("bind");
    return -1;
  }

  if (listen(sfd, BACKLOG) == -1) {
    perror("listen");
    return 0;
  }

  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_sock =
        accept(sfd, (struct sockaddr *)&client_addr, &client_addr_len);

    if (client_sock == -1) {
      perror("accept");
      continue;
    }
    printf("Client connected\n");
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(client_sock, buffer, BUFFER_SIZE)) > 0) {
      write(client_sock, buffer, bytes_read);
    }
    printf("Client disconnected\n");
    close(client_sock);
  }
  close(sfd);
  return 0;
}

int main() {
  start_server();
  return 0;
}
