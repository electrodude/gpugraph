CFLAGS+=-std=c99 -Wextra -funsafe-math-optimizations
LDFLAGS+=-lglfw -lX11 -lXrandr -lXi -lXxf86vm -lm -lGL -lGLU -pthread
CC=gcc
LD=gcc
AR=ar rcu
RANLIB=ranlib

#CFLAGS+=-g -O0
#LDFLAGS+=-g
CFLAGS+=-O3

LIBAEM_DIR=aem

LIBS=${LIBAEM_DIR}/libaem.a

SOURCES=main.c session_load.c session_save.c controls.c axes.c shader.c graphics.c nuklear.c
OBJECTS=$(patsubst %.c,%.o,${SOURCES})

DEPDIR=.deps
DEPFLAGS=-MD -MP -MF ${DEPDIR}/$*.d

$(shell mkdir -p ${DEPDIR})

all:		graph

graph:		${OBJECTS} ${LIBS}
		${LD} $^ ${LDFLAGS} -o $@

%.o:		%.c
		${CC} ${CFLAGS} ${DEPFLAGS} -c $< -o $@

clean:
		rm -vf ${DEPDIR}/*.d ${OBJECTS} graph
		${MAKE} -C ${LIBAEM_DIR} -w clean

${LIBAEM_DIR}/libaem.a:
		${MAKE} -C ${LIBAEM_DIR} -w libaem.a

.PHONY:		${LIBAEM_DIR}/libaem.a

.PHONY:		all clean

include $(wildcard ${DEPDIR}/*.d)
