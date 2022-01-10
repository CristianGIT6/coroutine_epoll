
#include "error_util.h"
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static char *ename[] = {
    /*   0 */ "",
    /*   1 */ "EPERM",
    "ENOENT",
    "ESRCH",
    "EINTR",
    "EIO",
    "ENXIO",
    /*   7 */ "E2BIG",
    "ENOEXEC",
    "EBADF",
    "ECHILD",
    /*  11 */ "EAGAIN/EWOULDBLOCK",
    "ENOMEM",
    "EACCES",
    "EFAULT",
    /*  15 */ "ENOTBLK",
    "EBUSY",
    "EEXIST",
    "EXDEV",
    "ENODEV",
    /*  20 */ "ENOTDIR",
    "EISDIR",
    "EINVAL",
    "ENFILE",
    "EMFILE",
    /*  25 */ "ENOTTY",
    "ETXTBSY",
    "EFBIG",
    "ENOSPC",
    "ESPIPE",
    /*  30 */ "EROFS",
    "EMLINK",
    "EPIPE",
    "EDOM",
    "ERANGE",
    /*  35 */ "EDEADLK/EDEADLOCK",
    "ENAMETOOLONG",
    "ENOLCK",
    "ENOSYS",
    /*  39 */ "ENOTEMPTY",
    "ELOOP",
    "",
    "ENOMSG",
    "EIDRM",
    "ECHRNG",
    /*  45 */ "EL2NSYNC",
    "EL3HLT",
    "EL3RST",
    "ELNRNG",
    "EUNATCH",
    /*  50 */ "ENOCSI",
    "EL2HLT",
    "EBADE",
    "EBADR",
    "EXFULL",
    "ENOANO",
    /*  56 */ "EBADRQC",
    "EBADSLT",
    "",
    "EBFONT",
    "ENOSTR",
    "ENODATA",
    /*  62 */ "ETIME",
    "ENOSR",
    "ENONET",
    "ENOPKG",
    "EREMOTE",
    /*  67 */ "ENOLINK",
    "EADV",
    "ESRMNT",
    "ECOMM",
    "EPROTO",
    /*  72 */ "EMULTIHOP",
    "EDOTDOT",
    "EBADMSG",
    "EOVERFLOW",
    /*  76 */ "ENOTUNIQ",
    "EBADFD",
    "EREMCHG",
    "ELIBACC",
    "ELIBBAD",
    /*  81 */ "ELIBSCN",
    "ELIBMAX",
    "ELIBEXEC",
    "EILSEQ",
    "ERESTART",
    /*  86 */ "ESTRPIPE",
    "EUSERS",
    "ENOTSOCK",
    "EDESTADDRREQ",
    /*  90 */ "EMSGSIZE",
    "EPROTOTYPE",
    "ENOPROTOOPT",
    /*  93 */ "EPROTONOSUPPORT",
    "ESOCKTNOSUPPORT",
    /*  95 */ "EOPNOTSUPP/ENOTSUP",
    "EPFNOSUPPORT",
    "EAFNOSUPPORT",
    /*  98 */ "EADDRINUSE",
    "EADDRNOTAVAIL",
    "ENETDOWN",
    "ENETUNREACH",
    /* 102 */ "ENETRESET",
    "ECONNABORTED",
    "ECONNRESET",
    "ENOBUFS",
    /* 106 */ "EISCONN",
    "ENOTCONN",
    "ESHUTDOWN",
    "ETOOMANYREFS",
    /* 110 */ "ETIMEDOUT",
    "ECONNREFUSED",
    "EHOSTDOWN",
    "EHOSTUNREACH",
    /* 114 */ "EALREADY",
    "EINPROGRESS",
    "ESTALE",
    "EUCLEAN",
    /* 118 */ "ENOTNAM",
    "ENAVAIL",
    "EISNAM",
    "EREMOTEIO",
    "EDQUOT",
    /* 123 */ "ENOMEDIUM",
    "EMEDIUMTYPE",
    "ECANCELED",
    "ENOKEY",
    /* 127 */ "EKEYEXPIRED",
    "EKEYREVOKED",
    "EKEYREJECTED",
    /* 130 */ "EOWNERDEAD",
    "ENOTRECOVERABLE",
    "ERFKILL",
    "EHWPOISON"};

#define MAX_ENAME 133

static void output_error(int err, const char *format, va_list ap) {
#define BUF_SIZE 500
  char buf[1024], user_msg[BUF_SIZE], err_text[BUF_SIZE];

  vsnprintf(user_msg, BUF_SIZE, format, ap);

  snprintf(err_text, BUF_SIZE, " [%s %s]",
           (err > 0 && err <= MAX_ENAME) ? ename[err] : "?UNKNOWN?",
           strerror(err));

  snprintf(buf, 1024, "ERROR%s %s\n", err_text, user_msg);

  fflush(stdout);
  fputs(buf, stderr);
  fflush(stderr);
}

void err_exit(const char *format, ...) {
  va_list arg_list;

  va_start(arg_list, format);
  output_error(errno, format, arg_list);
  va_end(arg_list);

  exit(EXIT_FAILURE);
}
