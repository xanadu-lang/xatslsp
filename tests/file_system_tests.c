#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "file_system.h"

/* ****** ****** */

// defined in file_system.c
int file_uri_parse(const char *src, char output[FILE_URI_MAX]);

void file_uri_parses_okay(const char *string, const char *path) {
  char output[FILE_URI_MAX];

  //fprintf(stderr, "input: %s\n", string);

  int ret = file_uri_parse(string, output);

  assert(ret > 0);
  //fprintf(stderr, "output: %s\n", output);
  assert(!strcmp(path, output));
}

void file_uri_parses_nope(const char *string) {
  char output[FILE_URI_MAX];

  //fprintf(stderr, "input: %s\n", string);

  size_t ret = file_uri_parse(string, output);

  //fprintf(stderr, "output: %ld\n", ret);
  assert(ret == 0);
}

void file_uri_tests() {
  file_uri_parses_okay("file://localhost/etc/fstab", "/etc/fstab");
  file_uri_parses_okay("file:///etc/fstab", "/etc/fstab");
  file_uri_parses_okay("file:///home/someone/Projects%20Something/output.txt", "/home/someone/Projects Something/output.txt");
  file_uri_parses_okay("file://localhost/c:/WINDOWS/clock.avi", "/c:/WINDOWS/clock.avi");
  file_uri_parses_okay("file:///C:/Documents%20and%20Settings/davris/FileSchemeURIs.doc", "/C:/Documents and Settings/davris/FileSchemeURIs.doc");

  file_uri_parses_nope("file:///file/ with spaces/textfile"); // malformed
  file_uri_parses_nope("file://example.com/something.txt"); // non-local URI: unsupported!
  file_uri_parses_nope("file://example.com/something.txt"); // non-local URI: unsupported!
  file_uri_parses_nope("file://./file.txt"); // relative path
  file_uri_parses_nope("file:///some/dir/../../file.txt"); // relative path
  file_uri_parses_nope("file://file.txt"); // relative path
}

/* ****** ****** */

void file_system_simple_lifecycle() {
  const char *uri = "file:///bin/bash";
  const char *contents = "hello, world!";
  file_system_t fs;

  file_system_init(&fs);

  assert(file_system_lookup(&fs, uri) == NULL);

  file_system_open(&fs, uri, 1, contents, strlen(contents));

  file_t *file = file_system_lookup(&fs, uri);

  assert(file != NULL && file->version == 1 && file->open_count == 1);
  file_system_close(&fs, uri);

  assert(file_system_lookup(&fs, uri) == NULL);
  
  file_system_free(&fs);
}

typedef struct {
  const char *uri;
  const char *contents;
} file_example_t;

void file_system_multi_lifecycle() {
  file_example_t examples[] = {
    {"file:///foo/bar.txt", "bar.text"},
    {"file:///foo/baz.txt", "baz.text"},
    {"file:///foo/qbe.txt", "qbe.text"},
    {"file:///baz/qui.txt", "qui.text"},
    {NULL, NULL}
  };
  
  file_system_t fs;

  file_system_init(&fs);

  file_example_t *example;

  example = examples;
  while (example->uri != NULL) {
    file_system_open(&fs, example->uri, 1, example->contents, strlen(example->contents));
    example++;
  }

  example = examples;
  while (example->uri != NULL) {
    file_t *file = file_system_lookup(&fs, example->uri);
    assert(file != NULL && file->version == 1 && file->open_count == 1);
    file_system_close(&fs, example->uri);
    example++;
  }  

  example = examples;
  while (example->uri != NULL) {
    file_t *file = file_system_lookup(&fs, example->uri);
    assert(file == NULL);
    example++;
  }  

  file_system_free(&fs);
}

/* ****** ****** */

int main(int argc, char **argv) {
  file_uri_tests();
  file_system_simple_lifecycle();
  file_system_multi_lifecycle();

  return 0;
}
