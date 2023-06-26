CFLAGS+=-std=c99 -Wall -Wextra -funsafe-math-optimizations
LDFLAGS+=-lglfw -lX11 -lXrandr -lXi -lXxf86vm -lm -lGL -lGLU -pthread
CC=gcc
LD=gcc
AR=ar rcu
RANLIB=ranlib

#CFLAGS+=-g -Og
#LDFLAGS+=-g
CFLAGS+=-O3
CFLAGS+=-Wno-unused-variable -Wno-unused-parameter


SOURCES=main.c session_load.c session_save.c controls.c axes.c shader.c graphics.c nuklear.c
OBJECTS=$(patsubst %.c,%.o,${SOURCES})

all:		graph

include aem/make.inc

graph:		${OBJECTS} ${LIBS}
		${LD} $^ ${LDFLAGS} -o $@

clean:	libaem_clean
		rm -vf ${OBJECTS} graph
		${MAKE} -C ${LIBAEM_DIR} -w clean

.PHONY:		all clean
