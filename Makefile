CFLAGS+=-std=c99 -Wextra -funsafe-math-optimizations
LDFLAGS+=-lglfw -lX11 -lXrandr -lXi -lXxf86vm -lm -lGL -lGLU -pthread
CC=gcc
LD=gcc
AR=ar rcu
RANLIB=ranlib

CFLAGS+=-g -O0
LDFLAGS+=-g
#CFLAGS+=-O3

SOURCES=$(shell echo main.c controls.c axes.c shader.c graphics.c nuklear.c stack.c stringbuf.c stringslice.c)
OBJECTS=$(patsubst %.c,%.o,${SOURCES})

all:		graph

clean:
		rm -vf ${OBJECTS} graph depends.inc

graph:		${OBJECTS}
		${LD} $^ ${LDFLAGS} -o $@

%.o:		%.c
		${CC} ${CFLAGS} -c $< -o $@

depends.inc:	${SOURCES}
		${CC} ${CFLAGS} -MM $^ > $@

.PHONY:		all clean depends.inc

include depends.inc
