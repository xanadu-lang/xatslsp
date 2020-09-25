#ifndef __TEXT_BUFFER_H__
#define __TEXT_BUFFER_H__

// NOTE: based on https://www.cs.cmu.edu/~fp/courses/15122-f12/assignments/15-122-prog4-2.pdf

/* ****** ****** */

typedef struct gapbuf_s {
  size_t limit;
  char *buffer;
  size_t gap_start;
  size_t gap_end;

  struct gapbuf_s *next, *prev;
} gapbuf_t;

int gapbuf_init(size_t limit, gapbuf_t *gb);
void gapbuf_free(gapbuf_t *gb);

int is_gapbuf(gapbuf_t *gb);
size_t gapbuf_point(gapbuf_t *gb);
int gapbuf_empty(gapbuf_t *gb);
int gapbuf_full(gapbuf_t *gb);
size_t gapbuf_gap_size(gapbuf_t *gb);
size_t gapbuf_length(gapbuf_t *gb);
void gapbuf_clear(gapbuf_t *gb);
int gapbuf_at_left(gapbuf_t *gb);
int gapbuf_at_right(gapbuf_t *gb);
int gapbuf_forward(gapbuf_t *gb, size_t num);
int gapbuf_backward(gapbuf_t *gb, size_t num);
int gapbuf_insert(gapbuf_t *gb, const char *str, size_t length);
int gapbuf_delete(gapbuf_t *gb, size_t length);
int gapbuf_getc(gapbuf_t *gb, unsigned char *res);
int gapbuf_strncmp(gapbuf_t *gb, const char *str, size_t length);

/* ****** ****** */

typedef struct text_position_s {
  size_t line_num;
  size_t char_num;
} text_position_t;

#define TEXT_BUFFER_CHUNK_SIZE 16384

typedef struct text_buffer_s {
  gapbuf_t start, *point, end;
  size_t chunk_size;

  // derived & stored info: line/char number of the point (only valid if moving forward!)
  text_position_t point_position;
} text_buffer_t;

int is_tbuf(text_buffer_t *tb);

void text_buffer_init(text_buffer_t *tb, size_t chunk_size);
void text_buffer_free(text_buffer_t *tb);

// all editing operations are performed relative to the "point", which is the current position *between*
// two characters.

// move the cursor forward, to the right;
// returns how many codepoints actually skipped
int forward_chars(text_buffer_t *tb, size_t length);
// move the cursor backward, to the left;
// returns how many codepoints actually skipped
// NOTE: does not maintain line/char information properly!
int backward_chars(text_buffer_t *tb, size_t length);

// insert the string after the cursor (length is bytes!)
void insert_string(text_buffer_t *tb, const char *str, size_t length);
// delete the string after the cursor (length is bytes!)
void delete_string(text_buffer_t *tb, size_t length);

// set the point to the specified location (returns non-zero if succeeded)
int text_buffer_set_point(text_buffer_t *tb, text_position_t *pos);
// get the location of the point
void text_buffer_get_point(text_buffer_t *tb, text_position_t *pos);

// clear all text in the buffer
void text_buffer_clear(text_buffer_t *tb);

// insert text at point (length is bytes!)
void text_buffer_insert(text_buffer_t *tb, const char *text, size_t length);
// delete from point until the given position
void text_buffer_delete(text_buffer_t *tb, text_position_t *pos);

// return 0 to stop iteration
typedef
int (*text_buffer_read_t)(char *buffer, size_t length, void *state);

// sequential reading from the buffer
void text_buffer_read(text_buffer_t *tb, text_buffer_read_t read, void *state);

#endif /* !__TEXT_BUFFER_H__ */
