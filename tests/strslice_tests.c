#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "strslice.h"

int main(int argc, char **argv) {
  char *hello_world = "hello, world!";
  
  strslice_t nil = strslice_of_string("");
  strslice_t hello = strslice_of_string(hello_world);

  assert(strslice_is_empty(nil));
  assert(!strslice_is_empty(hello));
  assert(strslice_len(nil) == 0);

  assert(strslice_len(hello) == strlen(hello_world));
  assert(strslice_count(hello, 'h') == 1);

  assert(strslice_strchr(hello, 'w') == strchr(hello_world, 'w'));
  assert(strslice_strchr(hello, 'o') == strchr(hello_world, 'o'));
  assert(strslice_strchr(hello, 'h') == strchr(hello_world, 'h'));
  assert(strslice_strchr(hello, '-') == strchr(hello_world, '-'));

  assert(strslice_strrchr(hello, 'w') == strrchr(hello_world, 'w'));
  assert(strslice_strrchr(hello, 'o') == strrchr(hello_world, 'o'));
  assert(strslice_strrchr(hello, 'h') == strrchr(hello_world, 'h'));
  assert(strslice_strrchr(hello, '-') == strrchr(hello_world, '-'));

  strslice_t sub1 = strslice_substring(hello, 7, 6);
  assert(!strcmp(sub1.base, "world!"));
  strslice_t sub2 = strslice_substring(hello, 7, 8);
  assert(!strcmp(sub2.base, "world!"));
  strslice_t sub3 = strslice_substring(hello, 0, 6);
  assert(!strslice_cmp(sub3, strslice_of_string("hello,")));
  strslice_t sub4 = strslice_substring(hello, 3, 4);
  assert(!strslice_cmp(sub4, strslice_of_string("lo, ")));

  return 0;
}
