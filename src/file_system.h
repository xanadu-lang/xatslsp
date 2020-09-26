#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include <stdlib.h>
#include "text_buffer.h"

/*
 * according to LSP, all documents are identified by URIs.
 *
 * for us it is much easier to only deal with file:// URIs;
 * they can be easily canonicalized, which we use for hashing
 * and comparison.
 */

#define FILE_URI_MAX 1024

typedef struct file_path_t {
  char           path[FILE_URI_MAX];
  unsigned long  path_hash;
} file_path_t;

// returns length of path if successful, zero if failed;
// all arguments are required
int file_path_of_uri(const char *uri, int hash_size_pow2, file_path_t *path);

typedef struct file_s {
  file_path_t fpath;
  
  int   version;
  int   open_count;
  text_buffer_t text;

  struct file_s *next, *prev;
  struct file_s *hash_next, *hash_prev;
} file_t;

#define FILE_HASH_SIZE 256

typedef struct file_system_s {
  file_t *files;
  file_t *files_hash_table[FILE_HASH_SIZE];  
} file_system_t;

// edit kinds:
// - replace all with the new text
// - given range, and new text: delete the range, and then insert the new text at the start of the range
typedef struct file_edit_s {
  // range to delete: start offset, end offset (start < end; or if empty: start=end)
  // - this may be invalid range: -1 everywhere (in this case, the text is meant to replace the whole source code)
  int start_line;
  int start_char;
  int end_line;
  int end_char;
  
  const char *text; // text to insert at start offset after deletion (may be NULL)
  size_t text_length;
} file_edit_t;

void file_system_init(file_system_t *fs);
void file_system_free(file_system_t *fs);

file_t *file_system_lookup(file_system_t *fs, const char *uri);
void file_system_open(file_system_t *fs, const char *uri, int version, const char *contents, size_t len);
int file_system_change(file_system_t *fs, const char *uri, int version, const file_edit_t *edits, size_t num_edits);
void file_system_close(file_system_t *fs, const char *uri);

#endif /* !__FILE_SYSTEM_H__ */
