#include "coroutine.h"
#include "epoll_functions.h"
#include "error_util.h"
#include <arpa/inet.h>
#include <memory.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_SIZE 32

void handle_client(void *arg);
void run_server(void *arg);
void sig_handler(int signal);

struct client_socket {
  int fd;
  struct sockaddr_in address;
};

void handle_client(void *arg) {
  int cfd;
  struct sockaddr_in client_addr;
  char client_address[INET_ADDRSTRLEN];
  char echo_buf[BUF_SIZE];
  size_t receive_msg_size;

  struct client_socket *client_socket = (struct client_socket *)arg;
  memcpy(&cfd, &client_socket->fd, sizeof(int));
  memcpy(&client_addr, &client_socket->address, sizeof(struct sockaddr_in));

  if (inet_ntop(AF_INET, &client_addr.sin_addr, client_address,
                INET_ADDRSTRLEN) == NULL)
      err_exit("inet_ntop");

  /* printf("Connected user %s:%d\n", clientAddress,
   * ntohs(clientAddr.sin_port)); */

  memset(echo_buf, 0, BUF_SIZE);
  if ((receive_msg_size = co_recv(cfd, echo_buf, BUF_SIZE, 0)) == -1)
      err_exit("co_tcp_recv");

  while (receive_msg_size > 0) {
    if (co_send(cfd, echo_buf, receive_msg_size, 0) == -1)
        err_exit("co_tcp_send");

    if ((receive_msg_size = co_recv(cfd, echo_buf, BUF_SIZE, 0)) == -1)
        err_exit("co_tcp_recv");
  }

  close(cfd);
  /*   printf("Disconnected user %s:%d\n", clientAddress, */
  /* ntohs(clientAddr.sin_port)); */
}

void run_server(void *arg) {
  struct sockaddr_in client_addr;
  int serverFd;

  serverFd = co_listen("127.0.0.1", 8080);

  for (;;) {
    int cfd = co_accept(serverFd, &client_addr);
    if (cfd == -1)
        err_exit("co_tcp_accept");

    struct client_socket *client_data =
        (struct client_socket *)malloc(sizeof(struct client_socket));
    client_data->fd = cfd;
    client_data->address = client_addr;

    co_spawn(handle_client, client_data);
  }
}

void sig_handler(int signal) {
  printf("Handled %d\n", signal);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  /* Disable user-space buffer, memory leaks detected when enabled */
  setbuf(stdout, NULL);

  signal(SIGSEGV, sig_handler);
    init_scheduler();

  co_spawn(run_server, NULL);

  printf("End of coroutines\n");

    destroy_scheduler();

  exit(EXIT_SUCCESS);
}
