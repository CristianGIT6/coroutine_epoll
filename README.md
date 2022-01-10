### Simple Coroutine Epoll
Functions implemented

Time
- sleep(seconds) -> co_sleep

TCP/UDP
- accept -> co_accept
- recv -> co_recv
- send -> co_send

#### Usage:
##### Instalation
```
git clone https://github.com/vitdevelop/coroutine_epoll.git
cd coroutine_epoll
```

##### Build:
`make`

##### Clean:
`make clean`

##### Run:
`./main`

##### Connect:
`telnet localhost 8080`

##### Library functions
See `coroutine.h` file

> Notes:
> - Userspace IO stdout buffer should be disabled `setbuf(stdout, NULL)` because memory leaks detected. (Unknown why)
> - If userspace IO stdout buffer is disabled, make sure that `\n` is put at end of line to prevent _segmentation fault_.
