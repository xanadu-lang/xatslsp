#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "uriparse.h"
#include "uri_encode.h"

#include "file_system.h"

/*

file URI: @felixfbecker:
> The URI is passed from the client and either a URN or a URL. If it is a URL, it doesn't only uniquely identify the resource, but also includes information on how to retrieve (locate) it. So if the client passes you a file:// URL, you are free to read from the file system, if not, you should work with that URI and obtain the content in a different way (for example, it could be ftp://, in which case you could fetch it over FTP if you want to - but it is easier to let the client do that).

virtual file system
- multiple "roots" (= workspace roots), from which files may be opened
- open handles
- for each handle, we should know it its server-held or opened by the client
  - open_count
- ops:
  - get the text & feed it to the compiler
  - apply structured edit operations (only if it is client-opened)
  - refresh from disk (only if it is server-held)

set of files
- lookup file by document URI, returning a NULL if not found
- insert new file, it is client-opened: input is document URI and text, output is that we create a new file record

so we want a simple module, right?
- file uri:
  - to the actual path

document URI -> normalized URI -> file path (we do not support anything else?)
- how to normalize URIs? so we can hash them
- https://github.com/sjth/url-normalization-in-c/blob/master/src/cleanurl.c

we only support FILE uris!
 */

int file_uri_parse(const char *src, char output[FILE_URI_MAX]) {
  struct uri result;

  output[0] = '\0';
  memset(&result, 0, sizeof(result));

  int rc = uriparse(src, &result);
  if (rc != 0) {
    fprintf(stderr, "failed to parse\n");
    return 0; // failed to parse
  }

  if (result.scheme == NULL || strcmp(result.scheme, "file") // // we only support file URIs
      || (result.host != NULL ? strcmp(result.host, "localhost") : 0) // we only support local file system
      || result.path == NULL // there must be a path!
      ) {
    fprintf(stderr, "not a file uri or host is not localhost or path empty\n");
    return 0; 
  }

  // sanity check
  char *p;

  p = strchr(result.path, ' ');
  if (p != NULL) {
    fprintf(stderr, "URI path contains literal spaces\n");
    return 0;
  }
  p = strstr(result.path, "../");
  if (p != NULL) {
    fprintf(stderr, "URI path contains relative path\n");
    return 0;
  }
  p = strstr(result.path, "./");
  if (p != NULL) {
    fprintf(stderr, "URI path contains relative path\n");
    return 0;
  }
  
  size_t len = strlen(result.path);
  if (len + 1 >= FILE_URI_MAX) {
    fprintf(stderr, "too long\n");
    return 0; // not enough space!
  }

  uri_decode(result.path, len, output);

  return len;
}

static unsigned long hash_filename(const char *fname, int hash_size) {
  unsigned long hash = 5381;
  while (*fname != '\0') {
    int c = *fname;
    fname++;
    hash = hash * 33 + c;
  }
  hash &= hash_size - 1;
  return hash;
}

int file_path_of_uri(const char *uri, int hash_size_pow2, file_path_t *p) {
  assert(p != NULL);
  assert(hash_size_pow2 > 0 && (hash_size_pow2 & (hash_size_pow2 - 1)) == 0); // it is really a power of two

  memset(p, 0, sizeof(*p));
  int ret = file_uri_parse(uri, p->path);
  if (ret > 0) {
    p->path_hash = hash_filename(p->path, hash_size_pow2);
  }
  
  return ret;
}

file_t *file_system_remove(file_system_t *fs, file_t *file) {
  assert(fs != NULL);
  assert(file != NULL);
  
  unsigned long hash = file->fpath.path_hash;

  file_t *next = file->next;

  // unlink
  if (file->prev != NULL) {
    file->prev->next = file->next;
  } else {
    fs->files = file->next;
  }
  if (file->next != NULL) {
    file->next->prev = file->prev;
  }

  if (file->hash_prev != NULL) {
    file->hash_prev->hash_next = file->hash_next;
  } else {
    fs->files_hash_table[file->fpath.path_hash] = file->hash_next;
  }
  if (file->hash_next != NULL) {
    file->hash_next->hash_prev = file->hash_prev;
  }

  text_buffer_free(&file->text);

  free(file);

  return next;
}

void file_system_init(file_system_t *fs) {
  assert(fs != NULL);
  
  memset(fs, 0, sizeof(*fs));
}

void file_system_free(file_system_t *fs) {
  assert(fs != NULL);

  file_t *file = fs->files;

  while (file != NULL) {
    file = file_system_remove(fs, file);
  }
  memset(fs, 0, sizeof(*fs));
}

file_t *file_system_lookup(file_system_t *fs, const char *uri) {
  assert(fs != NULL);
  assert(uri != NULL);
  
  file_path_t filename;
  int uri_okay = file_path_of_uri(uri, FILE_HASH_SIZE, &filename);
  if (uri_okay == 0) {
    // unable to parse
    return NULL;
  }
  unsigned long hash = filename.path_hash;

  file_t *file = fs->files_hash_table[hash];
  while (file != NULL) {
    if (!strcmp(file->fpath.path, filename.path)) {
      return file;
    }
  }
  return NULL;
}

void file_system_open(file_system_t *fs, const char *uri, int version, const char *contents, size_t len) {
  assert(fs != NULL);
  assert(uri != NULL);
  assert(contents != NULL);
  
  file_path_t filename;
  int uri_okay = file_path_of_uri(uri, FILE_HASH_SIZE, &filename);
  if (uri_okay == 0) {
    // unable to parse
    return;
  }
  unsigned long hash = filename.path_hash;

  file_t *file = fs->files_hash_table[hash];
  while (file != NULL) {
    if (!strcmp(file->fpath.path, filename.path)) {
      text_buffer_clear(&file->text);
      text_buffer_insert(&file->text, contents, len);
      
      assert(file->open_count == 0); // it's a protocol breach otherwise!
      file->open_count++;
      file->version = version;
      return;
    }
  }
  assert(file == NULL);

  file = malloc(sizeof(file_t)); // TODO: handle failure
  memcpy(&file->fpath, &filename, sizeof(filename));
  file->version = version;
  file->open_count = 1;

  text_buffer_init(&file->text, TEXT_BUFFER_CHUNK_SIZE);
  text_buffer_insert(&file->text, contents, len);

  file->next = fs->files;
  if (fs->files != NULL) {
    fs->files->prev = file;
  }
  file->prev = NULL;
  fs->files = file;

  file->hash_next = fs->files_hash_table[hash];
  if (fs->files_hash_table[hash]) {
    fs->files_hash_table[hash]->hash_prev = file;
  }
  file->hash_prev = NULL;
  fs->files_hash_table[hash] = file;
}

static int
file_edit_range_not_empty(const file_edit_t *edit) {
  assert(edit != NULL);

  return edit->start_line < edit->end_line
    || edit->start_line == edit->end_line
    && edit->start_char < edit->end_char;
}

int file_system_change(file_system_t *fs, const char *uri, int version, const file_edit_t *edits, size_t num_edits) {
  assert(fs != NULL);
  assert(uri != NULL);

  file_t *file = file_system_lookup(fs, uri);
  if (file == NULL) {
    return 0; // FIXME! unknown file!
  }

  for (size_t i = 0; i < num_edits; i++) {
    const file_edit_t *edit = &edits[i];

    if (edit->start_line < 0 && edit->start_char < 0 && edit->end_line < 0 && edit->end_char < 0) {
      text_buffer_clear(&file->text);
    } else {
      text_position_t start_pos;
      start_pos.line_num = edit->start_line;
      start_pos.char_num = edit->start_char;

      // move the point
      if (!text_buffer_set_point(&file->text, &start_pos)) {
        // TODO: perhaps handle it in some way?
        fprintf(stderr, "file_system_change(%s): unable to locate the given position %ld,%ld!\n", uri, start_pos.line_num, start_pos.char_num);
        continue;
      }

      // delete the range
      if (file_edit_range_not_empty(edit)) {
        text_position_t end_pos;
        end_pos.line_num = edit->end_line;
        end_pos.char_num = edit->end_char;
        
        text_buffer_delete(&file->text, &end_pos);
      }
    }

    // insert, assuming we are already in position
    if (edit->text != NULL && edit->text_length > 0) {
      text_buffer_insert(&file->text, edit->text, edit->text_length);
    }
  }

  file->version = version;

  return 1;
}

void file_system_close(file_system_t *fs, const char *uri) {
  assert(fs != NULL);
  assert(uri != NULL);

  file_t *file = file_system_lookup(fs, uri);
  if (file == NULL) {
    return;
  }
  assert(file->open_count == 1); // it's a protocol breach otherwise!
  file->open_count--;

  file_system_remove(fs, file);
}
