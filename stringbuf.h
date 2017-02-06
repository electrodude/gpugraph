#ifndef OBJSTORE_STRINGBUF_H
#define OBJSTORE_STRINGBUF_H

#include "stringslice.h"

#define STRINGBUF_DEBUG 0

#if STRINGBUF_DEBUG
#include <stdio.h>
#endif

// String builder class
// N.B.: The null terminator is not present on str->s except after calls to
// stringbuf_release, stringbuf_shrink, and stringbuf_get.

enum stringbuf_storage
{
	STRINGBUF_STORAGE_HEAP = 0,
	STRINGBUF_STORAGE_UNOWNED,
};

struct stringbuf
{
	char *s;          // pointer to buffer
	size_t n;         // current length of string
	                  //  (not counting null terminator)
	size_t maxn;      // allocated length of buffer

	enum stringbuf_storage storage;
	int bad     : 1;
	int fixed   : 1;

};

// you can append and such to this because realloc(NULL, size) === malloc(size)
#define STRINGBUF_EMPTY ((struct stringbuf){0})

#define STRINGBUF_ON_STACK(_name, _len) \
	char _name##_stringbuf_data[_len]; \
	struct stringbuf _name = {.s=_name##_stringbuf_data, .n=0, .maxn = _len, .storage = STRINGBUF_STORAGE_UNOWNED};

struct stringbuf *stringbuf_alloc_raw(void);

// Create a new string, given how many bytes to preallocate
// if str == NULL, malloc the string
// otherwise, put it at *str
#define stringbuf_new_prealloc(maxn) stringbuf_new_prealloc_at(stringbuf_alloc_raw(), maxn)
struct stringbuf *stringbuf_new_prealloc_at(struct stringbuf *str, size_t maxn);

// Create a new string
// if str == NULL, malloc the string
// otherwise, put it at *str
#define stringbuf_new() stringbuf_new_at(stringbuf_alloc_raw())
#define stringbuf_new_at(str) stringbuf_new_prealloc_at(str, 32)

// Create a new string
// Copy n chars starting at c into the string
#define stringbuf_new_array(n, s) stringbuf_new_array_at(stringbuf_alloc_raw(), n, s)
struct stringbuf *stringbuf_new_array_at(struct stringbuf *str, size_t n, const char *s);

// Create a new string from a string slice
#define stringbuf_new_array(n, s) stringbuf_new_array_at(stringbuf_alloc_raw(), n, s)
struct stringbuf *stringbuf_new_slice_at(struct stringbuf *str, const char *start, const char *end);

// Create a new string
// Copy a null-terminated string at s into the string
#define stringbuf_new_str(s) stringbuf_new_str_at(stringbuf_alloc_raw(), s)
struct stringbuf *stringbuf_new_str_at(struct stringbuf *str, const char *s);

// Clone a string
#define stringbuf_dup(orig) stringbuf_dup_at(stringbuf_alloc_raw(), orig)
struct stringbuf *stringbuf_dup_at(struct stringbuf *str, const struct stringbuf *orig);

// Destroy a string and its buffer
void stringbuf_free(struct stringbuf *str);

// Destruct a non-malloc'd string
void stringbuf_dtor(struct stringbuf *str);

// Destroy string, returning its internal buffer
// The caller assumes responsibilty for free()ing the returned buffer.
// Appends null terminator
char *stringbuf_release(struct stringbuf *str);

// Get a pointer to the end of a string
static inline char *stringbuf_end(struct stringbuf *str)
{
	return &str->s[str->n];
}

// Reset a string to zero length
// This does nothing but str->n = 0; it neither null-terminates it
// nor shrinks the allocated size of the internal buffer.
static inline void stringbuf_reset(struct stringbuf *str)
{
	str->n = 0;
	str->bad = 0;
}

// Ensure that at least len bytes are allocated
void stringbuf_grow(struct stringbuf *str, size_t maxn_new);
// Ensure that at least len + 1 bytes are available
static inline void stringbuf_reserve(struct stringbuf *str, size_t len);

// Return the number of available allocated bytes
static inline int stringbuf_available(struct stringbuf *str)
{
	return str->maxn - str->n - 1;
}

// realloc() internal buffer to be as small as possible
// Appends null terminator
// Returns pointer to internal buffer
// The pointer is only valid until the next call to stringbuf_put*.
// The null terminator is only there until the next time the string is modified
char *stringbuf_shrink(struct stringbuf *str);


// Append a character
static inline void stringbuf_putc(struct stringbuf *str, char c);

// Append a null-terminated string
static inline void stringbuf_puts(struct stringbuf *str, const char *s);

// Append a null-terminated string, but do not let the result exceed n characters
static inline void stringbuf_puts_limit(struct stringbuf *str, size_t len, const char *s);

// Append a string that is n characters long
static inline void stringbuf_putn(struct stringbuf *str, size_t n, const char *s);

// Append a hex byte
static inline void stringbuf_puthex(struct stringbuf *str, unsigned char byte);

// Append a number
static inline void stringbuf_putnum(struct stringbuf *str, int base, int num);

// Append printf-formatted text
#define stringbuf_putf(str, fmt, ...) stringbuf_printf(str, fmt, __VA_ARGS__)
#define stringbuf_append_printf(str, fmt, ...) stringbuf_printf(str, fmt, __VA_ARGS__)
void stringbuf_printf(struct stringbuf *str, const char *fmt, ...);

/*
// Ensure there are at least len available bytes,
// and return a pointer to the current end.
char *stringbuf_append_manual(struct stringbuf *str, size_t len);
*/

// Append a stringbuf
void stringbuf_append(struct stringbuf *str, const struct stringbuf *str2);

// Append a character, escaping it if necessary
void stringbuf_putq(struct stringbuf *str, char c);

// Append a stringbuf, escaping characters as necessary
void stringbuf_append_quote(struct stringbuf *str, const struct stringbuf *str2);

// Append a struct stringslice, escaping characters as necessary
void stringbuf_append_stringslice_quote(struct stringbuf *str, const struct stringslice *slice);

// Append a stringslice
static inline void stringbuf_append_stringslice(struct stringbuf *str, struct stringslice slice)
{
	stringbuf_putn(str, stringslice_len(&slice), slice.start);
}

// Append the data starting at start and ending at the byte before end
static inline void stringbuf_append_slice(struct stringbuf *str, const char *start, const char *end)
{
	stringbuf_append_stringslice(str, stringslice_new(start, end));
}

void stringbuf_pad(struct stringbuf *str, size_t len, char c);


// Set a string to a character, clearing it first
// Is equivalent to stringbuf_reset followed by stringbuf_putc.
// Deprecated.
static inline void stringbuf_setc(struct stringbuf *str, char c)
{
	stringbuf_reset(str);
	stringbuf_putc(str, c);
}

// Set a string to a null-terminated string, clearing it first
// Is equivalent to stringbuf_reset followed by stringbuf_puts.
// Deprecated.
static inline void stringbuf_sets(struct stringbuf *str, const char *s)
{
	stringbuf_reset(str);
	stringbuf_puts(str, s);
}

// Set a string to a string that is n characters long, clearing it first
// Is equivalent to stringbuf_reset followed by stringbuf_putn.
// Deprecated.
static inline void stringbuf_setn(struct stringbuf *str, size_t n, const char *s)
{
	stringbuf_reset(str);
	stringbuf_putn(str, n, s);
}

// Set a string to another string, clearing it first
// Is equivalent to stringbuf_reset followed by stringbuf_append.
// Deprecated.
static inline void stringbuf_setstr(struct stringbuf *str, const struct stringbuf *str2)
{
	stringbuf_reset(str);
	stringbuf_append(str, str2);
}


// Appends null terminator
// Returns pointer to internal buffer
// The pointer is only valid until the next call to stringbuf_reserve or stringbuf_shrink.
static inline char *stringbuf_get(struct stringbuf *str)
{
	if (str == NULL) return NULL;
	if (str->s == NULL) return NULL;
	if (str->bad) return NULL;

	str->s[str->n] = 0; // null-terminate the string
	                    //  (there is room already allocated)
	return str->s;
}

// Return the i-th character from the beginning of the struct stringbuf, or -1 if out of range
int stringbuf_index(struct stringbuf *str, size_t i);

// Set the i-th character from the beginning of the struct stringbuf
// Increase the size of the struct stringbuf if necessary
void stringbuf_assign(struct stringbuf *str, size_t i, char c);

int stringbuf_file_read(struct stringbuf *str, FILE *fp);

int stringbuf_file_write(const struct stringbuf *str, FILE *fp);



// inline implementations of frequently called functions

static inline void stringbuf_reserve(struct stringbuf *str, size_t len)
{
	size_t n = str->n + len;
	// make room for new stuff and null terminator
	if (str->maxn < n + 1)
	{
		stringbuf_grow(str, (n + 1)*2);
	}
}

static inline void stringbuf_putc(struct stringbuf *str, char c)
{
	if (str == NULL) return;

#if STRINGBUF_DEBUG
	fprintf(stderr, "putc(\"%s\", '%c')\n", stringbuf_get(str), c);
#endif

	stringbuf_reserve(str, 1);
	if (str->bad) return;

	str->s[str->n++] = c;
}

static inline void stringbuf_puts(struct stringbuf *restrict str, const char *restrict s)
{
	if (str == NULL) return;
	if (str->bad) return;

	if (s == NULL) return;

#if 0
	for (;*s;s++)
	{
		stringbuf_putc(str, *s);
	}
#else
	stringbuf_putn(str, strlen(s), s);
#endif
}

static inline void stringbuf_puts_limit(struct stringbuf *restrict str, size_t len, const char *restrict s)
{
	if (str == NULL) return;

	if (s == NULL) return;

	//stringbuf_reserve(len);
	if (str->bad) return;

	for (; *s && str->n < len; s++)
	{
		stringbuf_putc(str, *s);
	}
}

static inline void stringbuf_putn(struct stringbuf *restrict str, size_t n, const char *restrict s)
{
	if (str == NULL) return;

	if (s == NULL) return;

	stringbuf_reserve(str, n);
	if (str->bad) return;

	memcpy(stringbuf_end(str), s, n);

	str->n += n;
}

static const char stringbuf_putnum_digits[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

static inline void stringbuf_putnum(struct stringbuf *str, int base, int num)
{
	if (num < 0)
	{
		stringbuf_putc(str, '-');
		num = -num;
	}

	int top = num / base;
	if (top > 0)
	{
		stringbuf_putnum(str, top, base);
	}
	stringbuf_putc(str, stringbuf_putnum_digits[num % base]);
}

static char stringbuf_puthex_hexdigits[16] = "0123456789abcdef";

static inline void stringbuf_puthex(struct stringbuf *str, unsigned char byte)
{
	stringbuf_putc(str, stringbuf_puthex_hexdigits[(byte >> 4) & 0xF]);
	stringbuf_putc(str, stringbuf_puthex_hexdigits[(byte     ) & 0xF]);
}

#endif /* OBJSTORE_STRINGBUF_H */
