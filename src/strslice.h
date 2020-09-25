#include <stddef.h>
#include <string.h>

// a string slice is a reference to a portion of a C string;
// it does NOT own the memory it points to, hence you should
// always ensure that any slices taken out of a string do not
// outlive the string

typedef struct strslice_s {
  char *base;
  size_t length;
} strslice_t;

static inline
int strslice_is_empty(strslice_t str) {
  return str.length <= 0;
}

static inline
strslice_t strslice_of_string(char *string) {
  assert(string != NULL);

  strslice_t res = { string, strlen(string) };
  return res;
}

static inline
strslice_t strslice_of_strlen(char *string, size_t length) {
  assert(string != NULL);

  strslice_t res = { string, length };
  return res;
}

static inline
size_t strslice_len(strslice_t str) {
  return str.length;
}

static inline
strslice_t strslice_substring(strslice_t str, size_t start, size_t len) {
  strslice_t res;
  
  char *base = str.base;

  // silently truncate
  start = start < str.length ? start : (str.length > 0 ? str.length - 1 : 0);
  size_t max_len = str.length - start;
  len = len <= max_len ? len : max_len;

  res.base = base + start;
  res.length = len;
  return res;
}

static inline
int strslice_count(strslice_t str, int chr) {
  char *s = str.base;
  char *t = str.base + str.length;
  int count = 0;
  while (s < t) {
    int c = *s;
    if (c == chr) {
      count++;
    }
    s++;
  }
  return count;
}

static inline
char *strslice_strrchr(strslice_t str, int chr) {
  char *b = str.base;
  char *s = str.base + str.length - 1;

  while (b <= s) {
    int c = *s;
    if (c == chr) {
      return s;
    }
    s--;
  }
  return NULL;
}

static inline
char *strslice_strchr(strslice_t str, int chr) {
  char *s = str.base;
  char *t = str.base + str.length;
  while (s < t) {
    int c = *s;
    if (c == chr) {
      return s;
    }
    s++;
  }
  return NULL;
}

static inline
int strslice_cmp(strslice_t str1, strslice_t str2) {
  return str1.length < str2.length ? -1 :
    str1.length > str2.length ? 1 :
    strncmp(str1.base, str2.base, str1.length);
}
