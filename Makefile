EXE = main

DEP_SOURCE = coroutine.c
DEP_LIB = lib.a
RM = rm -f

IMPL_CFLAGS = -std=c99 -D_XOPEN_SOURCE=600 \
	        -D_DEFAULT_SOURCE \
		-pedantic \
		-Wall \
		-W \
		-Wmissing-prototypes \
		-Wno-sign-compare \
		-Wimplicit-fallthrough \
		-Wno-unused-parameter


IMPL_THREAD_FLAGS = -pthread

CFLAGS = ${IMPL_CFLAGS} ${IMPL_THREAD_FLAGS}

all: ${EXE}

clean:
	rm -f ${EXE} ${DEP_LIB}

${EXE} : ${DEP_LIB}

${DEP_LIB}: ${DEP_SOURCE}
	${CC} -c -g ${CFLAGS} *.c
	${RM} ${DEP_LIB}
	${AR} rs ${DEP_LIB} *.o
	${RM} *.o
