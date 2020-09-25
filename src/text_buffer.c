#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "text_buffer.h"

int is_gapbuf(gapbuf_t *tb) {
  return tb != NULL
    && tb->limit > 0 && tb->buffer != NULL &&
    0 <= tb->gap_start &&
    tb->gap_start <= tb->gap_end &&
    tb->gap_end <= tb->limit;
}

size_t gapbuf_point(gapbuf_t *gb) { assert(is_gapbuf(gb)); return gb->gap_start; }
int gapbuf_empty(gapbuf_t *gb) { assert(is_gapbuf(gb)); return gb->gap_start == 0 && gb->gap_end == gb->limit; }
int gapbuf_full(gapbuf_t *gb) { assert(is_gapbuf(gb)); return gb->gap_start == gb->gap_end; }
size_t gapbuf_gap_size(gapbuf_t *gb) { assert(is_gapbuf(gb)); return gb->gap_end - gb->gap_start; }
size_t gapbuf_length(gapbuf_t *gb) { assert(is_gapbuf(gb)); return gb->limit - gapbuf_gap_size(gb); }
int gapbuf_at_left(gapbuf_t *gb) { assert(is_gapbuf(gb)); return gb->gap_start == 0; }
int gapbuf_at_right(gapbuf_t *gb) { assert(is_gapbuf(gb)); return gb->gap_end == gb->limit; }

int gapbuf_init(size_t limit, gapbuf_t *gb) {
  assert(gb != NULL);
  assert(limit > 0);

  char *buf = malloc(limit);
  if (buf == NULL) {
    return 0;
  }
  memset(buf, 0, limit);
  gb->buffer = buf;
  gb->limit = limit;
  gb->gap_start = 0;
  gb->gap_end = limit;
  gb->next = gb->prev = NULL;
  
  assert(is_gapbuf(gb));
  assert(gapbuf_empty(gb));
  return 1;
}

void gapbuf_free(gapbuf_t *gb) {
  assert(is_gapbuf(gb));
  free(gb->buffer);
  gb->buffer = NULL;
  gb->gap_start = gb->gap_end = gb->limit = 0;
  gb->next = gb->prev = NULL;
}

void gapbuf_clear(gapbuf_t *gb) {
  assert(is_gapbuf(gb));

  gb->gap_start = 0;
  gb->gap_end = gb->limit;
  
  assert(is_gapbuf(gb));
}

int gapbuf_forward(gapbuf_t *gb, size_t num) {
  assert(is_gapbuf(gb));

  if (gapbuf_at_right(gb) || gb->gap_end+num > gb->limit) {
    return 0;
  }

  // copy [num] characters from after the gap to before the gap
  memmove(gb->buffer + gb->gap_start, gb->buffer + gb->gap_end, num);
  
  gb->gap_start += num;
  gb->gap_end += num;
  
  assert(is_gapbuf(gb));
  return 1;
}
int gapbuf_backward(gapbuf_t *gb, size_t num) {
  assert(is_gapbuf(gb));

  if (gapbuf_at_left(gb) || gb->gap_start < num) {
    return 0;
  }

  // copy [num] characters from before the gap to after the gap
  memmove(gb->buffer + gb->gap_end - num, gb->buffer + gb->gap_start - num, num);
  
  gb->gap_start -= num;
  gb->gap_end -= num;
  
  assert(is_gapbuf(gb));
  return 1;
}
int gapbuf_insert(gapbuf_t *gb, const char *str, size_t length) {
  assert(is_gapbuf(gb));

  // should be enough room in the gap
  if (gb->gap_end - gb->gap_start < length) {
    return 0;
  }
  memcpy(gb->buffer + gb->gap_start, str, length);
  gb->gap_start += length;
  
  assert(is_gapbuf(gb));
  return 1;
}
int gapbuf_delete(gapbuf_t *gb, size_t length) {
  assert(is_gapbuf(gb));

  // should be enough characters to delete
  if (gb->gap_end + length > gb->limit) {
    return 0;
  }
  gb->gap_end += length;
  
  assert(is_gapbuf(gb));

  return 1;
}
int gapbuf_getc(gapbuf_t *gb, unsigned char *res) {
  assert(is_gapbuf(gb));

  // the character immediately following the gap
  if (gb->gap_end < gb->limit) {
    *res = gb->buffer[gb->gap_end];
    return 1;
  }

  return 0;
}
int gapbuf_strncmp(gapbuf_t *gb, const char *str, size_t length) {
  assert(is_gapbuf(gb));

  size_t content_length = gapbuf_length(gb);
  char *buffer = gb->buffer;

  if (content_length < length) {
    return -1;
  } else if (content_length > length) {
    return 1;
  }

  if (length > gb->gap_start) {
    int ret = memcmp(buffer, str, gb->gap_start);

    if (ret != 0) {
      return ret;
    }
    assert(gb->gap_end < gb->limit);
    ret = memcmp(buffer + gb->gap_end, str + gb->gap_start, length - gb->gap_start);
    return ret;
  } else {
    return memcmp(buffer, str, length);
  }
}

/* ****** ****** */

int is_linked(text_buffer_t *tb) {
  assert(tb != NULL);

  // next links proceed from the [start] node to the [end] node, passing [point] along the way
  // prev links mirror the [next] links
  if (tb->start.prev != NULL || tb->end.next != NULL) {
    return 0;
  }

  int point_passed = 0;
  gapbuf_t *rover = tb->start.next;
  while (rover != &tb->end) {
    if (!is_gapbuf(rover)) {
      return 0;
    }
    
    gapbuf_t *next = rover->next, *prev = rover->prev;
    if (next == NULL || prev == NULL) {
      return 0;
    }
    if (next->prev != rover || prev->next != rover) {
      return 0;
    }
    if (rover == tb->point) {
      if (point_passed) {
        return 0;
      }
      point_passed = 1;
    }
    rover = next;
  }
  if (!point_passed) {
    return 0;
  }

  // point is distinct from both the start and the end nodes: the list must be non-empty
  int point_distinct = tb->point != &tb->start && tb->point != &tb->end;

  return point_distinct;
}

static int is_tbuf_empty_or_nonempty(text_buffer_t *tb) {
  assert(tb != NULL && tb->point != NULL);

  // FIXME: not sure what they mean?
  // "... a text buffer must either be the empty text buffer, or else all gap buffers must be non-empty"
  // - which text buffer is that? I think it's the one containing the point
  // - and the "**" in the diagram stand for sentinel nodes of the doubly-linked list
  if (tb->start.next == tb->point && tb->end.prev == tb->point) {
    return 1;
  } else {
    // all gapbufs must be non-empty
    int good = 1;
    gapbuf_t *rover = tb->start.next;
    while (rover != &tb->end && good) {
      good = good && !gapbuf_empty(rover);
      rover = rover->next;
    }
    return good;
  }
}

static int is_tbuf_aligned(text_buffer_t *tb) {
  gapbuf_t *rover;
  
  // for all gap buffers to the left of the point, the gap is on the right
  rover = tb->start.next;
  while (rover != tb->point) {
    if (!gapbuf_at_right(rover)) {
      return 0;
    }
    rover = rover->next;
  }
  
  // for all gap buffers to the right of the point, the gap is on the left
  rover = tb->end.prev;
  while (rover != tb->point) {
    if (!gapbuf_at_left(rover)) {
      return 0;
    }
    rover = rover->prev;
  }
  
  return 1;
}

int is_tbuf_on_codepoint(text_buffer_t *tb) {
  unsigned char ch = 0;

  // the cursor is always on the leading byte of a UTF-8 codepoint representation
  if (!gapbuf_getc(tb->point, &ch)) {
    return (!((ch & 0xC0) == 0x80));
  } else {
    return 1;
  }
}

int is_tbuf(text_buffer_t *tb) {
  assert(tb != NULL);

  if (!is_linked(tb)) {
    return 0;
  }
  if (!is_tbuf_empty_or_nonempty(tb)) {
    return 0;
  }
  if (!is_tbuf_aligned(tb)) {
    return 0;
  }

  if (!is_tbuf_on_codepoint(tb)) {
    return 0;
  }
  return 1;
}

void text_buffer_init(text_buffer_t *tb, size_t chunk_size) {
  assert(chunk_size > 0 && (chunk_size & (chunk_size - 1)) == 0); // it is really a power of two
  
  gapbuf_t *point = malloc(sizeof(gapbuf_t));
  assert(point != NULL); // TODO: handle this case
  gapbuf_init(chunk_size, point);
  
  memset(&tb->start, 0, sizeof(tb->start));
  memset(&tb->end, 0, sizeof(tb->end));

  point->prev = &tb->start;
  point->next = &tb->end;

  tb->chunk_size = chunk_size;
  tb->start.prev = NULL;
  tb->start.next = point;
  tb->end.prev = point;
  tb->end.next = NULL;

  tb->point = point;

  tb->point_position.line_num = 0;
  tb->point_position.char_num = 0;

  assert(is_tbuf(tb));
}

void text_buffer_free(text_buffer_t *tb) {
  assert(is_tbuf(tb));

  // go over the list and dealloc all nodes
  tb->point = NULL;
  gapbuf_t *rover = tb->start.next;
  while (rover != &tb->end) {
    gapbuf_t *next = rover->next;

    gapbuf_free(rover);
    free(rover);
    
    rover = next;
  }
  tb->start.next = &tb->end;
  tb->end.prev = &tb->start;
}

// returns true iff the text buffer is empty
int tbuf_empty(text_buffer_t *tb) {
  assert(is_tbuf(tb));
  // check if point is empty... if it is, then we are empty!
  // otherwise, there exist non-empty gapbufs to the left or
  // to the right of the point (due to is_tbuf_empty_or_nonempty):
  // then we are non-empty!
  return gapbuf_empty(tb->point);
}

// takes a text buffer whose point is a full gapbuf, and turns it into
// a text buffer whose point is not full
int split_point(text_buffer_t *tb) {
  assert(is_tbuf(tb));
  assert(gapbuf_full(tb->point));

  gapbuf_t *point = tb->point;

  gapbuf_t *gb = malloc(sizeof(gapbuf_t));
  gapbuf_init(tb->chunk_size, gb);

  size_t half = point->limit / 2;
  
  size_t point_start = gapbuf_point(point);
  if (point_start <= half) {
    // the point is within the first half of the buffer:
    // - move the second half to the other buffer;
    // - shift bytes around to make room for the gap
    gapbuf_insert(gb, point->buffer + half, half);
    gapbuf_backward(gb, half);

    memcpy(point->buffer + half + point_start, point->buffer + point_start, half - point_start);
    point->gap_end += half;

    assert(is_gapbuf(point));
    
    // gb is after point
    gb->prev = point;
    gb->next = point->next;
    point->next->prev = gb;
    point->next = gb;
  } else {
    // the point is within the second half of the buffer:
    // - move the first half to the other buffer;
    // - shift bytes around to make room for the gap
    gapbuf_insert(gb, point->buffer, half);
    
    char *buffer = point->buffer;

    memcpy(buffer, buffer+half, point_start - half);
    point->gap_start -= half;
    assert(is_gapbuf(point));

    // gb is before point
    gb->next = point;
    gb->prev = point->prev;
    point->prev->next = gb;
    point->prev = gb;
  }

  assert(is_tbuf(tb));
  assert(!gapbuf_full(point));
}

static void textbuf_rewind(text_buffer_t *tb) {
  assert(is_tbuf(tb));

  tb->point_position.line_num = 0;
  tb->point_position.char_num = 0;

  gapbuf_t *rover = tb->point;
  while (rover != &tb->start) {
    // move the gap to the end of the buffer (to preserve the alignment invariant)
    gapbuf_backward(rover, gapbuf_point(rover));
    
    rover = rover->prev;
  }
  tb->point = tb->start.next;

  assert(gapbuf_at_left(tb->point));
  assert(is_tbuf(tb));
}

static
int text_position_cmp(text_position_t *a, text_position_t *b) {
  if (a->line_num < b->line_num) {
    return -1;
  }
  if (a->line_num > b->line_num) {
    return 1;
  }
  // NOTE: a->line_num == b->line_num
  if (a->char_num < b->char_num) {
    return -1;
  }
  if (a->char_num > b->char_num) {
    return 1;
  }
  // NOTE: a->char_num == b->char_num
  return 0;
}

// AS: adapted from librope by Joseph Gentle: https://github.com/josephg/librope
//
// Find out how many bytes the unicode character which starts with the specified byte
// will occupy in memory.
// Returns the number of bytes, or SIZE_MAX if the byte is invalid.
static inline size_t codepoint_size(int byte) {
  if (byte == 0) { assert(0); return 0; } // NULL byte.
  else if (byte <= 0x7f) { return 1; } // 0x74 = 0111 1111
  else if (byte <= 0xbf) { assert(0); return 0; } // 1011 1111. Invalid for a starting byte.
  else if (byte <= 0xdf) { return 2; } // 1101 1111
  else if (byte <= 0xef) { return 3; } // 1110 1111
  else if (byte <= 0xf7) { return 4; } // 1111 0111
  else if (byte <= 0xfb) { return 5; } // 1111 1011
  else if (byte <= 0xfd) { return 6; } // 1111 1101
  else { assert(0); return 0; }
}

int forward_char(text_buffer_t *tb) {
  gapbuf_t *point = tb->point;

  // how many to skip?
  // if EOF, cannot do anything, so fail
  // otherwise,
  // - read the byte: it should be either a single byte sequence (then skip it)
  //   or a leader byte (then loop until you have skipped everything)

  if (gapbuf_at_right(point) && point->next == &tb->end) {
    return 0;
  } else {
    if (gapbuf_at_right(point)) {
      point = point->next;
    }

    unsigned char ch = 0;

    int ret = gapbuf_getc(point, &ch);
    assert(ret);

    if (ch <= 0x7F) { // plain ASCII
      ret = gapbuf_forward(point, 1);
      assert(ret);
      tb->point = point;
      if (ch == '\n') {
        tb->point_position.line_num++;
        tb->point_position.char_num = 0;
      } else {
        tb->point_position.char_num++;
      }
      return 1;
    } else { // leading byte
      assert(!((ch & 0xC0) == 0x80)); // should be a leading byte!

      size_t len = codepoint_size(ch);

      if (point->gap_end+len > point->limit) {
        size_t have = point->limit - point->gap_end;
        ret = gapbuf_forward(point, have);
        assert(ret);
        len -= have;
        point = point->next;
        ret = gapbuf_forward(point, len);
        assert(ret);
      } else {
        ret = gapbuf_forward(point, len);
        assert(ret);
      }

      tb->point = point;

      int ret = gapbuf_getc(point, &ch);

      tb->point_position.char_num++;
      tb->point = point;
      return 1;
    }
  }
}
int backward_char(text_buffer_t *tb) {
  gapbuf_t *point = tb->point;

  while (1) {
    unsigned char ch = 0;

    if (gapbuf_at_left(point)) {
      if (point->prev != &tb->start) {
        point = point->prev;
        continue;
      } else {
        tb->point = point;
        return 0;
      }
    } else {
      gapbuf_backward(point, 1);
    }

    int ret = gapbuf_getc(point, &ch);
    assert(ret);

    if (ch <= 0x7F) { // plain ASCII
      tb->point = point;
      if (ch == '\n') {
        size_t num = tb->point_position.line_num;
        num = num > 0 ? num - 1 : 0;
        tb->point_position.line_num = num;
        tb->point_position.char_num = 0; // FIXME: we can't really determine this! unless we go further and then get back.
      } else {
        size_t num = tb->point_position.char_num;
        num = num > 0 ? num - 1 : 0;
        tb->point_position.char_num = num;
      }
      return 1;
    } else if ((ch & 0xC0) == 0x80) {
      // skip it: non-leading char
    } else { // leading byte
      size_t num = tb->point_position.char_num;
      num = num > 0 ? num - 1 : 0;
      tb->point_position.char_num = num;
      tb->point = point;
      return 1;
    }
  }
}

// move the cursor forward, to the right
int forward_chars(text_buffer_t *tb, size_t length) {
  assert(is_tbuf(tb));
  int ret;
  int steps = 0;

  while (length > 0) {
    ret = forward_char(tb);
    if (!ret) {
      return steps;
    }
    length--;
    steps++;
  }
  assert(is_tbuf(tb));

  return steps;
}
// move the cursor backward, to the left
int backward_chars(text_buffer_t *tb, size_t length) {
  assert(is_tbuf(tb));
  int ret;
  int steps = 0;

  while (length > 0) {
    ret = backward_char(tb);

    if (!ret) {
      return steps;
    }
    length--;
    steps++;
  }

  assert(is_tbuf(tb));
  return steps;
}

// insert the string before the cursor
void insert_string(text_buffer_t *tb, const char *str, size_t length) {
  assert(is_tbuf(tb));

  while (length > 0) {
    if (gapbuf_full(tb->point)) {
      split_point(tb);
      assert(is_tbuf(tb));
    }
    
    size_t len = gapbuf_gap_size(tb->point);

    if (len > length) {
      len = length;
    }

    gapbuf_insert(tb->point, str, len);
    length -= len;
    str += len;
  }

  assert(is_tbuf(tb));
}

// delete the string after the cursor
void delete_string(text_buffer_t *tb, size_t length) {
  assert(is_tbuf(tb));

  while (length > 0) {
    gapbuf_t *point = tb->point;

    size_t have = point->limit - point->gap_end;
    have = length < have ? length : have;
    
    int ret = gapbuf_delete(point, have);
    assert(ret);
    length -= have;

    // delete the point if it becomes empty, unless it's the only gapbuffer we have
    if (gapbuf_empty(point) && !(point->prev == &tb->start && point->next == &tb->end)) {
      gapbuf_t *gb = point->next;

      gb->prev = point->prev;
      point->prev->next = gb;
      tb->point = gb;

      point->prev = NULL;
      point->next = NULL;
      gapbuf_free(point);
    }
  }

  assert(is_tbuf(tb));  
}

int text_buffer_set_point(text_buffer_t *tb, text_position_t *pos) {
  assert(is_tbuf(tb));
  assert(pos != NULL);

  textbuf_rewind(tb);

  while (text_position_cmp(&tb->point_position, pos) < 0) {
    if (!forward_char(tb)) {
      assert(is_tbuf(tb));
      return 0;
    }
  }
  assert(text_position_cmp(&tb->point_position, pos) == 0);

  assert(is_tbuf(tb));
  
  return 1;  
}

void text_buffer_get_point(text_buffer_t *tb, text_position_t *pos) {
  assert(is_tbuf(tb));
  assert(pos != NULL);

  *pos = tb->point_position;
}

void text_buffer_clear(text_buffer_t *tb) {
  gapbuf_t *point = tb->point;
  gapbuf_t *rover = tb->start.next;

  while (rover != &tb->end) {
    gapbuf_t *next = rover->next;

    if (rover != point) {
      rover->prev = NULL;
      rover->next = NULL;
      gapbuf_free(rover);
    } else {
      rover->prev = &tb->start;
      rover->next = &tb->end;      
    }
    
    rover = next;
  }
  gapbuf_clear(point);

  tb->start.prev = NULL;
  tb->start.next = point;
  tb->end.prev = point;
  tb->end.next = NULL;

  tb->point = point;

  assert(is_tbuf(tb));
}

void text_buffer_insert(text_buffer_t *tb, const char *text, size_t length) {
  insert_string(tb, text, length);
}

void text_buffer_delete(text_buffer_t *tb, text_position_t *pos) {
  assert(pos != NULL);
  assert(text_position_cmp(&tb->point_position, pos) < 0); // this should be a range!

  text_position_t current_pos = tb->point_position;

  // NOTE: range is exclusive
  while (text_position_cmp(&current_pos, pos) < 0) {
    // delete one char (codepoint)
    // if this codepoint is a '\n', advance
    unsigned char ch = 0;

    if (gapbuf_at_right(tb->point) && tb->point->next == &tb->end) {
      break; // no more text to delete
    } else {
      if (gapbuf_at_right(tb->point)) {
        tb->point = tb->point->next;
      }
      int ret = gapbuf_getc(tb->point, &ch);
      assert(ret);

      if (ch <= 0x7F) { // plain ASCII
        delete_string(tb, 1);
        if (ch == '\n') {
          current_pos.line_num++;
          current_pos.char_num = 0;
        } else {
          current_pos.char_num++;
        }
      } else { // leading byte
        assert(!((ch & 0xC0) == 0x80)); // should be a leading byte!
      
        current_pos.char_num++;
        size_t size = codepoint_size(ch);

        delete_string(tb, size);
      }
    }
  }
}

void text_buffer_read(text_buffer_t *tb, text_buffer_read_t read, void *state) {
  assert(is_tbuf(tb));

  // here we should go over all gapbuffers and expose all of their readable data to the [read] function
  gapbuf_t *rover = tb->start.next;
  while (rover != &tb->end) {
    gapbuf_t *next = rover->next;

    if (gapbuf_full(rover)) {
      if (!read(rover->buffer, rover->limit, state)) {
        break;
      }
    } else {
      if (rover->gap_start && !read(rover->buffer, rover->gap_start, state)) {
        break;
      }
      if (rover->gap_end < rover->limit && !read(rover->buffer + rover->gap_end, rover->limit - rover->gap_end, state)) {
        break;
      }
    }

    rover = next;
  }
}
