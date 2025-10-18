#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 9090
#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096
#define BACKLOG 5

typedef enum { STATE_NEW, STATE_CONNECTED, STATE_DISCONNECTED } state_e;

typedef struct {
  int sockfd;
  state_e state;
  char buffer[BUFFER_SIZE];
} clientstate_t;

clientstate_t clients[MAX_CLIENTS];

void init_clients() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    clients[i].sockfd = -1;
    clients[i].state = STATE_NEW;
    memset(&clients[i].buffer, '\0', BUFFER_SIZE);
  }
}

int find_free_slot() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].sockfd == -1) {
      return i;
    }
  }
  return -1;
}

int start_multi_server(void) {
  int listen_fd, conn_fd, nfds, free_slot;
  struct sockaddr_in server_addr, client_addr = {0};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  socklen_t client_len = sizeof(client_addr);
  fd_set read_fds, write_fds, except_fds;
  init_clients();

  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd == -1) {
    perror("socket");
    return -1;
  }

  if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("bind");
    return -1;
  }

  if (listen(listen_fd, BACKLOG) == -1) {
    perror("listen");
    return 0;
  }

  printf("Listening on port %d\n", PORT);

  while (1) {
    // Zero
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);

    // Make sure listen fd is available
    FD_SET(listen_fd, &read_fds);
    nfds = listen_fd + 1; // a max value, socket always increases

    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clients[i].sockfd != -1) {
        FD_SET(clients[i].sockfd, &read_fds);
        if (clients[i].sockfd >= nfds)
          nfds = clients[i].sockfd + 1;
      }
    }

    if (select(nfds, &read_fds, &write_fds, &except_fds, NULL) == -1) {
      perror("select");
      return -1;
    }

    if (FD_ISSET(listen_fd, &read_fds)) {
      int client_sock =
          accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
      if (client_sock == -1) {
        perror("accept");
        continue;
      }
      printf("new connection from %s:%d\n", inet_ntoa(client_addr.sin_addr),
             ntohs(client_addr.sin_port));

      free_slot = find_free_slot();
      if (free_slot != -1) {
        clients[free_slot].sockfd = client_sock;
        clients[free_slot].state = STATE_CONNECTED;
      } else {
        fprintf(stderr, "Server full, rejecting connection\n");
        close(client_sock);
      }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clients[i].sockfd == -1 || !FD_ISSET(clients[i].sockfd, &read_fds))
        continue;
      ssize_t bytes_read =
          read(clients[i].sockfd, clients[i].buffer, BUFFER_SIZE);

      if (bytes_read <= 0) { // error reading close
        close(clients[i].sockfd);
        clients[i].state = STATE_DISCONNECTED;
        clients[i].sockfd = -1;
        printf("Client disconnected on error\n");
      }
      write(clients[i].sockfd, clients[i].buffer, bytes_read);
    }
  }
  close(listen_fd);
  return 0;
}

int start_server(void) {
  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1)
    return -1;

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
  int s = start_multi_server();
  return s;
}
