#ifndef __EPOLL_FUNCTIONS_H__
#define __EPOLL_FUNCTIONS_H__

#include <arpa/inet.h>

void co_sleep(int seconds);

int co_listen(char *server_address, short port);

int co_accept(int server_fd, struct sockaddr_in *client_addr);

ssize_t co_send(int fd, const void *buf, size_t n, int flags);

ssize_t co_recv(int fd, void *buf, size_t n, int flags);

#endif /* __EPOLL_FUNCTIONS_H__ */
