#include "coroutine.h"
#include "error_util.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <ucontext.h>

void co_scheduler_insertCoroutine(coroutine_t *coroutine);

void co_scheduler_remove_coroutine(coroutine_t *coroutine);

void co_destroy(coroutine_t *coroutine);

void co_scheduler_main_function(void);

void co_entry_point(coroutine_t *coroutine);

void co_epoll_list_add(int fd, coroutine_t *coroutine);

void *co_epoll_list_remove(int fd);

void co_epoll_events_wait(void);

void co_wait(void);

static __thread co_scheduler_t *co_scheduler;

void co_scheduler_insertCoroutine(coroutine_t *coroutine) {
  struct list_t *element =
      list_append_element(&co_scheduler->coroutines, coroutine);

  if (co_scheduler->current == NULL) {
    co_scheduler->current = element;
  }

  co_scheduler->coroutines_size++;
}

void co_scheduler_remove_coroutine(coroutine_t *coroutine) {
  if (co_scheduler->current->elem == coroutine) {
    if (co_scheduler->current == co_scheduler->coroutines->list)
      co_scheduler->current = co_scheduler->current->next;
    else
      co_scheduler->current = co_scheduler->coroutines->list;
  }

  list_remove_element(co_scheduler->coroutines, coroutine);
  co_destroy(coroutine);

  co_scheduler->coroutines_size--;
}

void co_destroy(coroutine_t *coroutine) {
  if (coroutine->args)
    free(coroutine->args);
  free(coroutine->stack);
  free(coroutine);
}

void co_spawn(co_func func, void *args) {
  coroutine_t *coroutine = malloc(sizeof(coroutine_t));
  if (coroutine == NULL)
    err_exit("coroutine malloc");
  memset(coroutine, 0, sizeof(coroutine_t));

  coroutine->stack_size = STACK_SIZE;
  coroutine->stack = malloc(coroutine->stack_size);
  if (coroutine->stack == NULL)
    err_exit("coroutine stack malloc");

  getcontext(&(coroutine->uc));

  coroutine->uc.uc_stack.ss_sp = coroutine->stack;
  coroutine->uc.uc_stack.ss_size = coroutine->stack_size;
  coroutine->uc.uc_link = &(co_scheduler->sched_uc);
  makecontext(&(coroutine->uc), (void (*)(void))co_entry_point, 1, coroutine);

  coroutine->scheduler = co_scheduler;
  coroutine->status = COROUTINE_STATE_SUSPEND;
  coroutine->func = func;
  coroutine->args = args;

  coroutine_t *current_coroutine =
      (co_scheduler->current != NULL ? co_scheduler->current->elem : NULL);

  co_scheduler_insertCoroutine(coroutine);

  if (current_coroutine == NULL) {
    swapcontext(&(co_scheduler->main_uc), &(co_scheduler->sched_uc));
  } else {
    co_yield();
  }
}

void co_entry_point(coroutine_t *coroutine) {
  coroutine->func(coroutine->args);
  coroutine->status = COROUTINE_STATE_DONE;
  swapcontext(&(coroutine->uc), &(coroutine->scheduler->sched_uc));
}

void co_scheduler_main_function(void) {
  while (co_scheduler->current != NULL) {
    struct list_t *current_coroutine = co_scheduler->current;
    if (current_coroutine->next != NULL) {
      current_coroutine = current_coroutine->next;
    } else {
      current_coroutine = co_scheduler->coroutines->list;
    }
    co_scheduler->current = current_coroutine;

    coroutine_t *coroutine = current_coroutine->elem;
    if (coroutine->status == COROUTINE_STATE_SUSPEND) {
      coroutine->status = COROUTINE_STATE_RUNNING;
      swapcontext(&(co_scheduler->sched_uc), &(coroutine->uc));
    }

    if (coroutine->status == COROUTINE_STATE_DONE) {
      co_scheduler_remove_coroutine(current_coroutine->elem);
    }

    co_epoll_events_wait();
  }
  setcontext(&(co_scheduler->main_uc));
}

void co_epoll_events_wait(void) {
  if (co_scheduler->epoll_fd_list_size == 0 &&
      co_scheduler->coroutines_size == 0)
    return;

  int readyEvents = 0;
  if (co_scheduler->coroutines_size != 0 &&
      co_scheduler->coroutines_size == co_scheduler->epoll_fd_list_size) {
    // wait indefinitely
    readyEvents = epoll_wait(co_scheduler->epoll_fd, co_scheduler->events,
                             EPOLL_DEFAULT_EVENTS, -1);
  } else {
    // no wait
    readyEvents = epoll_wait(co_scheduler->epoll_fd, co_scheduler->events,
                             EPOLL_DEFAULT_EVENTS, 0);
  }

  for (; readyEvents > 0; readyEvents--) {
    co_epoll_remove(co_scheduler->events[readyEvents - 1].data.fd);
  }
}

void init_scheduler(void) {
  co_scheduler = calloc(1, sizeof(co_scheduler_t));
  if (co_scheduler == NULL)
    err_exit("scheduler malloc");

  getcontext(&(co_scheduler->sched_uc));

  co_scheduler->sched_uc.uc_stack.ss_sp = malloc(MINSIGSTKSZ);
  if (co_scheduler->sched_uc.uc_stack.ss_sp == NULL)
    err_exit("scheduler main_function malloc");

  memset(co_scheduler->sched_uc.uc_stack.ss_sp, 0, MINSIGSTKSZ);
  co_scheduler->sched_uc.uc_stack.ss_size = MINSIGSTKSZ;
  co_scheduler->sched_uc.uc_stack.ss_flags = 0;

  makecontext(&(co_scheduler->sched_uc),
              (void (*)(void))co_scheduler_main_function, 1, co_scheduler);

  co_scheduler->epoll_fd = epoll_create1(0);
  if (co_scheduler->epoll_fd == -1)
    err_exit("epoll_create1");

  co_scheduler->events =
      calloc(EPOLL_DEFAULT_EVENTS, sizeof(struct epoll_event));
  if (co_scheduler->events == NULL)
    err_exit("calloc epoll events");
}

void destroy_scheduler(void) {
  free(co_scheduler->sched_uc.uc_stack.ss_sp);
  free(co_scheduler);
}

void co_yield(void) {
  coroutine_t *current_coroutine = co_scheduler->current->elem;
  current_coroutine->status = COROUTINE_STATE_SUSPEND;
  swapcontext(&(current_coroutine->uc), &(co_scheduler->sched_uc));
}

void co_wait(void) {
  coroutine_t *current_coroutine = co_scheduler->current->elem;
  current_coroutine->status = COROUTINE_STATE_WAIT;
  swapcontext(&(current_coroutine->uc), &(co_scheduler->sched_uc));
}

void co_epoll_list_add(int fd, coroutine_t *coroutine) {
  struct co_epoll_fd_t *epoll_fd = calloc(1, sizeof(struct co_epoll_fd_t));
  if (epoll_fd == NULL)
    err_exit("calloc co_epoll_fd_t");

  epoll_fd->fd = fd;
  epoll_fd->coroutine = coroutine;

  list_append_element(&(co_scheduler->epoll_fds), epoll_fd);
  co_scheduler->epoll_fd_list_size++;
}

void *co_epoll_list_remove(int fd) {
  coroutine_t *coroutine = NULL;

  struct list_t *epoll_fd_list = co_scheduler->epoll_fds->list;
  struct co_epoll_fd_t *epoll_fd = epoll_fd_list->elem;
  do {
    if (epoll_fd->fd == fd)
      break;
  } while ((epoll_fd = list_next(&epoll_fd_list)) != NULL);

  if (epoll_fd == NULL)
    err_exit("Unable to remove epoll_fd with fd = %d", fd);

  coroutine = epoll_fd->coroutine;

  list_remove_element(co_scheduler->epoll_fds, epoll_fd);
  co_scheduler->epoll_fd_list_size--;

  free(epoll_fd);

  return coroutine;
}

void co_epoll_add(int fd, uint32_t events) {
  coroutine_t *current_coroutine = co_scheduler->current->elem;

  struct epoll_event epoll_event;
  epoll_event.events = events;
  epoll_event.data.fd = fd;

  if (epoll_ctl(co_scheduler->epoll_fd, EPOLL_CTL_ADD, fd, &epoll_event) == -1)
    err_exit("epoll_ctl add");

  if (events != 0) {
    co_epoll_list_add(fd, current_coroutine);
    co_wait();
  }
}

void co_epoll_wait(int fd) {
  coroutine_t *current_coroutine = co_scheduler->current->elem;

  co_epoll_list_add(fd, current_coroutine);

  co_wait();
}

void co_epoll_change(int fd, uint32_t events) {
  coroutine_t *current_coroutine = co_scheduler->current->elem;

  struct epoll_event epoll_event;
  epoll_event.events = events;
  epoll_event.data.fd = fd;

  if (epoll_ctl(co_scheduler->epoll_fd, EPOLL_CTL_MOD, fd, &epoll_event) == -1)
    err_exit("epoll_ctl mod");

  co_epoll_list_add(fd, current_coroutine);

  co_wait();
}

void co_epoll_remove(int fd) {
  coroutine_t *coroutine = (coroutine_t *)co_epoll_list_remove(fd);
  coroutine->status = COROUTINE_STATE_SUSPEND;
}

struct list_head_t *list_init_head(void) {
  /* {{{ */
  struct list_head_t *head =
      (struct list_head_t *)calloc(1, sizeof(struct list_head_t));
  if (head == NULL)
    perror("malloc list_head_t");

  return head;
}
/* }}} */

struct list_t *list_append_element(struct list_head_t **head, void *elem) {
  /* {{{ */
  if (*head == NULL) {
    *head = list_init_head();
  }

  if ((*head)->list == NULL) {
    (*head)->list = (struct list_t *)calloc(1, sizeof(struct list_t));
    if ((*head)->list == NULL)
      perror("malloc list_t");

    (*head)->list->elem = elem;
    return (*head)->list;
  }

  struct list_t *list = (*head)->list;
  struct list_t *current_element = list;
  while (current_element->next != NULL) {
    current_element = current_element->next;
  }

  current_element->next = (struct list_t *)calloc(1, sizeof(struct list_t));
  if (current_element->next == NULL)
    perror("malloc");

  current_element->next->elem = elem;

  return current_element->next;
}
/* }}} */

void list_remove_element(struct list_head_t *head, void *elem) {
  /* {{{ */
  if (head == NULL)
    return;

  struct list_t *list = head->list;
  if (list == NULL)
    return;

  struct list_t *previous_element = NULL;
  struct list_t *current_element = list;
  while (current_element != NULL) {
    if (current_element->elem == elem) {
      if (previous_element != NULL)
        previous_element->next = current_element->next;

      if (head->list == current_element)
        head->list = current_element->next;

      free(current_element);
      break;
    }

    previous_element = current_element;
    current_element = current_element->next;
  }
}
/* }}} */

void *list_next(struct list_t **list) {
  /* {{{ */
  if ((*list) == NULL)
    return NULL;

  void *element = (*list)->elem;
  (*list) = (*list)->next;

  return element;
}
/* }}} */
