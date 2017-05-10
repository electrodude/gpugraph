#ifndef SESSION_SAVE_H
#define SESSION_SAVE_H

#include <stdio.h>

#include "stringbuf.h"

void session_save_serialize(struct stringbuf *str);

void session_save_file(FILE *fp);

int session_save_path(const char *path);

extern struct stringbuf session_path;

int session_save_curr(void);

#endif /* SESSION_SAVE_H */
