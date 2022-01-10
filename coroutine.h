#ifndef __COROUTINE_H__
#define __COROUTINE_H__

#include <search.h>
#include <sys/epoll.h>
#include <ucontext.h>

#define STACK_SIZE 4 * 1024
#define EPOLL_DEFAULT_EVENTS 10

typedef void (*co_func)(void *);

enum coroutine_status {
  COROUTINE_STATE_RUNNING = 0,
  COROUTINE_STATE_SUSPEND = 1,
  COROUTINE_STATE_DONE = 2,
  COROUTINE_STATE_WAIT = 3,
};

struct co_scheduler_t;
struct coroutine_t;

struct list_head_t {
  struct list_t *list;
};
struct list_t {
  void *elem;
  struct list_t *next;
};

typedef struct coroutine_t {
  void *stack;
  size_t stack_size;
  ucontext_t uc;

  co_func func;
  void *args;

  struct co_scheduler_t *scheduler;
  enum coroutine_status status;

} coroutine_t;

struct co_epoll_fd_t {
  int fd;
  coroutine_t *coroutine;
  struct co_epoll_fd_t *previous;
};

typedef struct co_scheduler_t {
  ucontext_t sched_uc;
  ucontext_t main_uc;

  struct list_t *current;
  struct list_head_t *coroutines;
  int coroutines_size;

  int epoll_fd;
  struct epoll_event *events;
  struct list_head_t *epoll_fds;
  int epoll_fd_list_size;
} co_scheduler_t;

void init_scheduler(void);
void destroy_scheduler(void);
void co_spawn(co_func func, void *args);
void co_yield(void);

void co_epoll_add(int fd, uint32_t events);
void co_epoll_change(int fd, uint32_t events);
void co_epoll_wait(int fd);
void co_epoll_remove(int fd);

struct list_head_t *list_init_head(void);
struct list_t *list_append_element(struct list_head_t **head, void *elem);
void list_remove_element(struct list_head_t *head, void *elem);
void *list_next(struct list_t **list);

#endif /* __COROUTINE_H__ */
