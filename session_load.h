#ifndef SESSION_LOAD_H
#define SESSION_LOAD_H

#include <stdio.h>

#include <aem/stringslice.h>

int session_load_parse(struct aem_stringslice *p);

int session_load_file(FILE *fp);

extern struct aem_stringbuf session_path;

int session_load_path(const char *path);

int session_load_curr(void);

#endif /* SESSION_LOAD_H */
