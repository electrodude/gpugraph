#ifndef OBJSTORE_STRINGSLICE_H
#define OBJSTORE_STRINGSLICE_H

#include <stdio.h>
#include <string.h>

struct stringslice
{
	const char *start;
	const char *end;
};

#define stringslice_new(p, pe) ((struct stringslice){.start = p, .end = pe})
#define STRINGSLICE_EMPTY stringslice_new(NULL, NULL)

static inline struct stringslice stringslice_new_len(const char *p, size_t n)
{
	return stringslice_new(p, &p[n]);
}

static inline struct stringslice stringslice_new_cstr(const char *p)
{
	return stringslice_new_len(p, strlen(p));
}

#define stringslice_new_str(str) stringslice_new_len((str).s, (str).n)

int stringslice_file_write(const struct stringslice *slice, FILE *fp);

static inline int stringslice_ok(const struct stringslice *slice)
{
	return slice->start != slice->end;
}

static inline size_t stringslice_len(const struct stringslice *slice)
{
	return slice->end - slice->start;
}

int stringslice_match(struct stringslice *slice, const char *s);

int stringslice_match_hexbyte(struct stringslice *slice);

#endif /* OBJSTORE_STRINGSLICE_H */
