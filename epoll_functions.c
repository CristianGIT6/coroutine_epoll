#define _GNU_SOURCE
#include "epoll_functions.h"
#include "coroutine.h"
#include "error_util.h"
#include <memory.h>
#include <stdio.h>
#include <sys/timerfd.h>
#include <unistd.h>

int timerfd_set(int seconds);
void timerfd_unset(int fd);

void co_sleep(int seconds) {
  int timer_fd = timerfd_set(seconds);
  co_epoll_add(timer_fd, EPOLLIN | EPOLLONESHOT);

  timerfd_unset(timer_fd);
}

int co_listen(char *server_address, short port) {
  int server_fd, optval;
  struct sockaddr_in echo_serv_addr;
  char server_addr_str[INET_ADDRSTRLEN];

  server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (server_fd == -1)
    err_exit("socket");

  optval = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                 sizeof(optval)) == -1)
    err_exit("setsockopt");

  memset(&echo_serv_addr, 0, sizeof(echo_serv_addr));

  echo_serv_addr.sin_family = AF_INET;
  if (inet_pton(AF_INET, server_address, &echo_serv_addr.sin_addr) == -1)
    err_exit("inet_pton");
  echo_serv_addr.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&echo_serv_addr,
           sizeof(echo_serv_addr)) == -1)
    err_exit("bind");

  if (listen(server_fd, 0) == -1)
    err_exit("listen");

  if (inet_ntop(AF_INET, &echo_serv_addr.sin_addr, server_addr_str,
                INET_ADDRSTRLEN) == NULL)
    err_exit("inet_ntop");

  /*   printf("Server started on address %s:%d\n", serverAddrStr, */
  /* ntohs(echoServAddr.sin_port)); */
  /*  */
  co_epoll_add(server_fd, EPOLLIN);

  return server_fd;
}

int co_accept(int server_fd, struct sockaddr_in *client_addr) {
  socklen_t len = sizeof(struct sockaddr_in);
  int client_fd = -1;
  do {
    client_fd =
        accept4(server_fd, (struct sockaddr *)client_addr, &len, SOCK_NONBLOCK);
    if (client_fd == -1) {
      if (errno != EAGAIN && errno != EWOULDBLOCK)
        break;

      co_epoll_wait(server_fd);
    }
  } while (client_fd < 0);

  co_epoll_add(client_fd, 0);

  return client_fd;
}

ssize_t co_send(int fd, const void *buf, size_t n, int flags) {
  ssize_t write_size = -1;
  do {
    write_size = send(fd, buf, n, flags);
    if (write_size == -1) {
      if (errno != EAGAIN && errno != EWOULDBLOCK)
        break;

      co_epoll_change(fd, EPOLLOUT | EPOLLONESHOT);
    }
  } while (write_size < 0);

  return write_size;
}

ssize_t co_recv(int fd, void *buf, size_t n, int flags) {
  ssize_t read_size = -1;
  do {
    read_size = recv(fd, buf, n, flags);
    if (read_size == -1) {
      if (errno != EAGAIN && errno != EWOULDBLOCK)
        break;

      co_epoll_change(fd, EPOLLIN | EPOLLONESHOT);
    }
  } while (read_size < 0);

  return read_size;
}

int timerfd_set(int seconds) {
  struct itimerspec timer_spec;
  memset(&timer_spec, 0, sizeof(struct itimerspec));
  timer_spec.it_value.tv_sec = seconds;

  int timer_fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
  if (timer_fd == -1)
    err_exit("timerfd_create");

  if (timerfd_settime(timer_fd, 0, &timer_spec, NULL) == -1)
    err_exit("timerfd_settime");

  return timer_fd;
}

void timerfd_unset(int fd) {
  if (close(fd) == -1)
    err_exit("close timerFd");
}
