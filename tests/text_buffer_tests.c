#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "text_buffer.h"

typedef struct test_state_s {
  char *string;
  size_t length;
} test_state_t;

int test_read(char *buffer, size_t length, void *state) {
  assert(state != NULL);
  test_state_t *test_state = (test_state_t *)state;

  assert(length <= test_state->length);
  assert(!memcmp(buffer, test_state->string, length));
  test_state->string += length;
  test_state->length -= length;
  
  return 1;
}

void test_replace_all() {
  text_buffer_t tb;

  text_buffer_init(&tb, TEXT_BUFFER_CHUNK_SIZE);

  // empty -> some content
  text_buffer_insert(&tb, "hello, world!", strlen("hello, world!"));
  test_state_t test_state1 = {"hello, world!", strlen("hello, world!")};
  text_buffer_read(&tb, test_read, (void *)&test_state1);

  // some content -> fully replaced
  text_buffer_clear(&tb);
  text_buffer_insert(&tb, "hello, there!", strlen("hello, there!"));
  test_state_t test_state2 = {"hello, there!", strlen("hello, there!")};
  text_buffer_read(&tb, test_read, (void *)&test_state2);
  
  text_buffer_free(&tb);
}

void gapbuf_tests() {
  {
    gapbuf_t gb;
    unsigned char ch = 0;
    assert(gapbuf_init(32, &gb));
    assert(!gapbuf_getc(&gb, &ch));
    assert(gapbuf_strncmp(&gb, "", 0) == 0);

    assert(gapbuf_insert(&gb, "1234567890", 10));
    assert(!gapbuf_getc(&gb, &ch));
    assert(gapbuf_insert(&gb, "1234567890", 10));
    assert(!gapbuf_getc(&gb, &ch));
    assert(gapbuf_insert(&gb, "1234567890", 10));
    assert(!gapbuf_getc(&gb, &ch));
    assert(gapbuf_insert(&gb, "AZ", 2));
    assert(!gapbuf_getc(&gb, &ch));

    assert(gapbuf_length(&gb) == 32);
    assert(gapbuf_strncmp(&gb, "123456789012345678901234567890AZ", 32) == 0);
    assert(gapbuf_full(&gb));

    assert(gapbuf_backward(&gb, 32));
    assert(gapbuf_getc(&gb, &ch) && ch == '1');
    assert(gapbuf_strncmp(&gb, "123456789012345678901234567890AZ", 32) == 0);
    assert(gapbuf_forward(&gb, 32));
    assert(!gapbuf_getc(&gb, &ch));
    assert(gapbuf_strncmp(&gb, "123456789012345678901234567890AZ", 32) == 0);
    
    assert(gapbuf_backward(&gb, 32));
    assert(gapbuf_delete(&gb, 32));

    assert(gapbuf_empty(&gb));
    gapbuf_free(&gb);
  }
  {
    gapbuf_t gb;
    unsigned char ch = 0;
    assert(gapbuf_init(16, &gb));
    assert(gapbuf_insert(&gb, "ABCDEF", strlen("ABCDEF")));
    assert(!gapbuf_getc(&gb, &ch));
    assert(gapbuf_backward(&gb, 3));
    assert(gapbuf_getc(&gb, &ch) && ch == 'D');
    assert(gapbuf_insert(&gb, "12345", strlen("12345")));
    assert(gapbuf_getc(&gb, &ch) && ch == 'D');
    assert(gapbuf_length(&gb) == strlen("ABCDEF") + strlen("12345"));
    
    assert(gapbuf_strncmp(&gb, "ABC12345DEF", strlen("ABC12345DEF")) == 0);
    gapbuf_free(&gb);
  }
  {
    gapbuf_t gb;
    unsigned char ch = 0;
    assert(gapbuf_init(16, &gb));
    assert(!gapbuf_getc(&gb, &ch));
    assert(gapbuf_delete(&gb, 16) == 0);
    assert(gapbuf_full(&gb) == 0);
    assert(gapbuf_forward(&gb, 8) == 0);
    assert(gapbuf_backward(&gb, 8) == 0);
    assert(gapbuf_length(&gb) == 0);
    assert(!gapbuf_strncmp(&gb, "", 0));
    gapbuf_free(&gb);
  }
  {
    gapbuf_t gb;
    unsigned char ch = 0;
    assert(gapbuf_init(32, &gb));
    assert(gapbuf_insert(&gb, "1234567890ABCDEF", 16));
    assert(!gapbuf_getc(&gb, &ch));
    assert(gapbuf_length(&gb) == 16);
    assert(!gapbuf_strncmp(&gb, "1234567890ABCDEF", 16));
    assert(gapbuf_full(&gb) == 0);
    assert(gapbuf_empty(&gb) == 0);
    assert(gapbuf_point(&gb) == 16);
    gapbuf_free(&gb);
  }
  {
    gapbuf_t gb;
    unsigned char ch = 0;
    assert(gapbuf_init(32, &gb));
    assert(gapbuf_insert(&gb, "1234567890", 10));
    assert(!gapbuf_getc(&gb, &ch));
    assert(gapbuf_backward(&gb, 1));
    assert(gapbuf_getc(&gb, &ch) && ch == '0');
    assert(gapbuf_strncmp(&gb, "1234567890", 10) == 0);
    assert(gapbuf_insert(&gb, "A", 1));
    assert(gapbuf_getc(&gb, &ch) && ch == '0');
    assert(gapbuf_strncmp(&gb, "123456789A0", 11) == 0);
    assert(gapbuf_length(&gb) == 11);
  }
}

typedef struct textbuf_compare_s {
  const char *text;
  size_t length;
  int result;
} textbuf_compare_t;

int textbuf_eq_string_read(char *buffer, size_t length, void *state) {
  textbuf_compare_t *comp = (textbuf_compare_t *)state;

  if (!comp->result) {
    return 0;
  }

  if (length > comp->length) {
    comp->result = 0;
    return 0;
  }
  if (!strncmp(comp->text, buffer, length)) {
    comp->text += length;
    comp->length -= length;
    return 1;
  }
  comp->result = 0;
  return 0;
}

int textbuf_eq_string(text_buffer_t *tb, const char *text) {
  assert(tb != NULL && text != NULL);
  
  size_t length = strlen(text);
  textbuf_compare_t comp = {text, length, 1};
  text_buffer_read(tb, textbuf_eq_string_read, &comp);

  return comp.result;
}

int textbuf_fprint_read(char *buffer, size_t length, void *state) {
  FILE *fp = (FILE *)state;

  fprintf(fp, "length: %ld ", length);

  for (int i = 0; i < length; i++)
    {
      //fprintf(fp, "%02X", buffer[i]);
      fprintf(fp, "\\%o", (unsigned char)buffer[i]);
    }
  fprintf(fp, "\n");

  //fprintf(fp, "%.*s", (int)length, buffer);
  return 1;
}

void textbuf_fprint(text_buffer_t *tb, FILE *fp) {
  assert(tb != NULL && fp != NULL);

  fprintf(fp, "point(gap_start= %ld, gap_end= %ld, limit= %ld)\n", tb->point->gap_start, tb->point->gap_end, tb->point->limit);
  text_buffer_read(tb, textbuf_fprint_read, fp);
}

void textbuf_prim_tests() {

  fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
  {
    text_buffer_t tb;

    text_buffer_init(&tb, 16);

    // noop
    forward_chars(&tb, 10);
    backward_chars(&tb, 10);

    insert_string(&tb, "1234567890", 10);
    assert(textbuf_eq_string(&tb, "1234567890"));

    // go back and delete the string we just inserted
    backward_chars(&tb, 10);
    textbuf_fprint(&tb, stderr);
    //assert(tb.point_offset == 0);
    delete_string(&tb, 10);
    textbuf_fprint(&tb, stderr);
    assert(textbuf_eq_string(&tb, ""));
    //assert(tb.point_offset == 0);
    
    text_buffer_free(&tb);
  }

  // split test: split with cursor after the end
  fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
  {
    text_buffer_t tb;
    const char *literal = "12345678";
    size_t literal_length = strlen(literal);

    text_buffer_init(&tb, literal_length * 2);

    // insert until full
    insert_string(&tb, literal, literal_length);
    insert_string(&tb, literal, literal_length);
    assert(gapbuf_full(tb.point));

    // insert one more time to induce a split
    insert_string(&tb, "ABCDEF", strlen("ABCDEF"));
    assert(gapbuf_at_right(tb.point));

    // check that the point has a prev that isn't sentinel
    assert(&tb.start != tb.point->prev);
    // check that both buffers are now non-full
    assert(!gapbuf_full(tb.point->prev) && !gapbuf_empty(tb.point->prev));
    assert(!gapbuf_full(tb.point) && !gapbuf_empty(tb.point));
    // check contents of both buffers
    assert(textbuf_eq_string(&tb, "1234567812345678ABCDEF"));

    text_buffer_free(&tb);
  }

  // split test: split with cursor before the beginning
  fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
  {
    text_buffer_t tb;
    const char *literal = "12345678";
    size_t literal_length = strlen(literal);

    text_buffer_init(&tb, literal_length * 2);

    // insert until full
    insert_string(&tb, literal, literal_length);
    insert_string(&tb, literal, literal_length);
    assert(gapbuf_full(tb.point));

    // rewind
    backward_chars(&tb, 16);

    // insert one more time to induce a split
    insert_string(&tb, "ABCDEF", strlen("ABCDEF"));
    assert(tb.point->gap_start == strlen("ABCDEF"));

    // check that the point has a next that isn't sentinel
    assert(tb.point->next != &tb.end);
    assert(tb.point->next->gap_start == 0);
    // check that both buffers are now non-full
    assert(!gapbuf_full(tb.point->next) && !gapbuf_empty(tb.point->next));
    assert(!gapbuf_full(tb.point) && !gapbuf_empty(tb.point));
    // check contents of both buffers

    //fprintf(stderr, "tb = \n");
    //textbuf_fprint(&tb, stderr);
    //fprintf(stderr, "\n");

    assert(textbuf_eq_string(&tb, "ABCDEF1234567812345678"));

    text_buffer_free(&tb);
  }
}

static inline int is_codepoint_start(int byte) {
  if (byte == 0) { return 0; } // NULL byte.
  else if (byte <= 0x7f) { return 1; } // 0x74 = 0111 1111
  else if (byte <= 0xbf) { return 1; } // 1011 1111. Invalid for a starting byte.
  else if (byte <= 0xdf) { return 1; } // 1101 1111
  else if (byte <= 0xef) { return 1; } // 1110 1111
  else if (byte <= 0xf7) { return 1; } // 1111 0111
  else if (byte <= 0xfb) { return 1; } // 1111 1011
  else if (byte <= 0xfd) { return 1; } // 1111 1101
  else { return 0; }
}

void textbuf_clear_tests() {
  // clear test
  fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
  {
    text_buffer_t tb;
    const char *literal = "12345678";
    size_t literal_length = strlen(literal);

    text_buffer_init(&tb, literal_length);

    // insert multiple copies, so we have multiple gapbufs
    insert_string(&tb, literal, literal_length);
    insert_string(&tb, literal, literal_length);
    insert_string(&tb, literal, literal_length);
    assert(gapbuf_full(tb.point));

    // rewind so we are somewhere in the middle
    backward_chars(&tb, 16);

    // clear it!
    text_buffer_clear(&tb);

    assert(tb.point->prev == &tb.start && tb.point->next == &tb.end);
    assert(gapbuf_at_left(tb.point) && gapbuf_empty(tb.point));
    assert(textbuf_eq_string(&tb, ""));

    text_buffer_free(&tb);
  }
}

void textbuf_utf8_nav_tests() {
  // navigation test
  fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
  {
    text_buffer_t tb;
    const char *literal = "1\320\277\321\200\320\270\320\262\320\265\321\202"
      " \320\274\320\270\321\200"; // "1привет, мир" encoded as UTF-8
    size_t literal_length = strlen(literal);

    fprintf(stderr, "literal: %s\n", literal);

    assert(literal_length > 16);
    text_buffer_init(&tb, 16);

    fprintf(stderr, "literal_length %ld bytes\n", literal_length);

    // a codepoint encoded in UTF-8 is split between the two underlying gapbuffers
    insert_string(&tb, literal, literal_length);
    assert(textbuf_eq_string(&tb, literal));
    textbuf_fprint(&tb, stderr);
    fprintf(stderr, "string inserted\n");
    assert(tb.point->next == &tb.end);

    unsigned char ch = 0;
    
    // navigate and ensure that we only stop at codepoint boundaries
    //fprintf(stderr, "0: tb: point %ld, pos line %ld, end %ld\n", tb.point_offset, tb.point_position.line_num, tb.end_offset);
    assert(!gapbuf_getc(tb.point, &ch));
    
    backward_chars(&tb, 1);
    //fprintf(stderr, "1: tb: point %ld, pos line %ld, end %ld\n", tb.point_offset, tb.point_position.line_num, tb.end_offset);
    assert(gapbuf_getc(tb.point, &ch));
    fprintf(stderr, "1: getc = %o\n", ch);
    assert(ch == 0321);
    assert(is_codepoint_start(ch));

    backward_chars(&tb, 1);
    //fprintf(stderr, "2: tb: point %ld, pos line %ld, end %ld\n", tb.point_offset, tb.point_position.line_num, tb.end_offset);
    fprintf(stderr, "2: getc = %o\n", ch);
    assert(gapbuf_getc(tb.point, &ch));
    assert(ch == 0320);
    assert(is_codepoint_start(ch));

    backward_chars(&tb, 1);
    //fprintf(stderr, "3: tb: point %ld, pos line %ld, end %ld\n", tb.point_offset, tb.point_position.line_num, tb.end_offset);
    fprintf(stderr, "3: getc = %o\n", ch);
    assert(gapbuf_getc(tb.point, &ch));
    assert(ch == 0320);
    assert(is_codepoint_start(ch));

    forward_chars(&tb, 1);
    assert(gapbuf_getc(tb.point, &ch));
    fprintf(stderr, "4: getc = %o\n", ch);
    assert(ch == 0320);
    assert(is_codepoint_start(ch));

    forward_chars(&tb, 1);
    assert(gapbuf_getc(tb.point, &ch));
    fprintf(stderr, "5: getc = %o\n", ch);
    assert(ch == 0321);
    assert(is_codepoint_start(ch));

    text_buffer_free(&tb);
  }
}

void textbuf_of_file(text_buffer_t *tb, const char *path) {
  assert(path != NULL);
  assert(tb != NULL);

  char buf[1024];
  size_t num_read;

  FILE *file = fopen(path, "rb");
  assert(file != NULL);

  while (1) {
    num_read = fread(buf, 1, sizeof buf, file);
    if (num_read <= 0) {
      break;
    }
    insert_string(tb, buf, num_read);
  }
  fclose(file);
}

int file_of_textbuf_read(char *buffer, size_t length, void *state) {
  FILE *fp = (FILE *)state;

  size_t written = fwrite(buffer, length, 1, fp);
  if (written != 1) {
    fprintf(stderr, "error writing to the file!\n");
    return 0;
  }

  return 1;
}

void file_of_textbuf(text_buffer_t *tb, const char *path) {
  assert(path != NULL);
  assert(tb != NULL);

  FILE *file = fopen(path, "wb");
  assert(file != NULL);

  text_buffer_read(tb, file_of_textbuf_read, file);

  fclose(file);
}

void textbuf_pos_nav_delete_tests() {
  // set position & delete
  fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
  {
    text_buffer_t tb;
    const char *literal = "1\320\277\321\200\320\270\320\262\320\265\321\202"
      " \320\274\320\270\321\200"; // "1привет, мир" encoded as UTF-8
    size_t literal_length = strlen(literal);

    fprintf(stderr, "literal: %s\n", literal);

    assert(literal_length > 16);
    text_buffer_init(&tb, 16);

    fprintf(stderr, "literal_length %ld bytes\n", literal_length);

    text_position_t tp;

    // a codepoint encoded in UTF-8 is split between the two underlying gapbuffers
    insert_string(&tb, literal, literal_length);
    textbuf_fprint(&tb, stderr);
    assert(textbuf_eq_string(&tb, literal));

    tp.line_num = 1;
    tp.char_num = 0;
    assert(!text_buffer_set_point(&tb, &tp));

    tp.line_num = 0;
    tp.char_num = 11; // as far as we can go!
    assert(text_buffer_set_point(&tb, &tp));

    tp.line_num = 0;
    tp.char_num = 12;
    assert(!text_buffer_set_point(&tb, &tp));

    // alright, now delete some part of the string
    tp.line_num = 0;
    tp.char_num = 8;
    assert(text_buffer_set_point(&tb, &tp));
    fprintf(stderr, "BEFORE:");
    textbuf_fprint(&tb, stderr);  
    tp.char_num += 3;
    text_buffer_delete(&tb, &tp);
    fprintf(stderr, "AFTER:");    
    textbuf_fprint(&tb, stderr);    
    const char *literal_after = "1\320\277\321\200\320\270\320\262\320\265\321\202 "; // "1привет, " encoded as UTF-8
    assert(textbuf_eq_string(&tb, literal_after));

    text_buffer_free(&tb);
  }
  fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
  {
    // UTF-8 multiline text
    text_buffer_t tb;

    text_buffer_init(&tb, 32);

    // insert some string with multiple lines
    textbuf_of_file(&tb, "text_buffer_tests.txt");
    fprintf(stderr, "READ:");
    textbuf_fprint(&tb, stderr);

    // delete " еще"
    {
      text_position_t tp;
      tp.line_num = 1;
      tp.char_num = 3;
      assert(text_buffer_set_point(&tb, &tp));
      tp.line_num = 1;
      tp.char_num = 7;
      text_buffer_delete(&tb, &tp);
    }
    
    // delete but leave the \n: 3,0 delete 3,4: "один" (but leaves us a newline!)
    {
      text_position_t tp;
      tp.line_num = 3;
      tp.char_num = 0;
      assert(text_buffer_set_point(&tb, &tp));
      tp.line_num = 3;
      tp.char_num = 4;
      text_buffer_delete(&tb, &tp);
    }
    // delete straddling a newline: 4,0 delete 5,0: deletes the whole line, including a newline ("два")
    {
      text_position_t tp;
      tp.line_num = 4;
      tp.char_num = 0;
      assert(text_buffer_set_point(&tb, &tp));
      tp.line_num = 5;
      tp.char_num = 0;
      text_buffer_delete(&tb, &tp);
    }

    // insert & replace a part
    {
      text_position_t tp;
      tp.line_num = 4;
      tp.char_num = 0;
      assert(text_buffer_set_point(&tb, &tp));
      const char *literal = "\n{\n  \"foo\": \"bar\"\n}\n";
      text_buffer_insert(&tb, literal, strlen(literal));

      tp.line_num = 6;
      tp.char_num = 9;
      assert(text_buffer_set_point(&tb, &tp));
      tp.line_num = 6;
      tp.char_num = 14;
      text_buffer_delete(&tb, &tp);
      text_buffer_insert(&tb, "512", strlen("512"));
    }
    
    // dump it back to a file so we can check if it's OK
    file_of_textbuf(&tb, "text_buffer_tests.txt.out");

    text_buffer_free(&tb);
  }
}

int main(int argc, char **argv) {

  gapbuf_tests();
  textbuf_prim_tests();
  textbuf_clear_tests();
  textbuf_utf8_nav_tests();
  textbuf_pos_nav_delete_tests();

  // TODO: probably, add a separate "cursor" facility: it's an index into the string
  // - kinda like the frozen iterator that is baked into the text_buffer...
  // - the idea is that the cursor can be moved without moving the gaps
  
  //test_replace_all();
  // TODO: another test
  // - starting with a buffer...
  // - delete, but do not insert anything new
  // - insert new text (when the deletion range is 0 length)
  // - replace some text with some other text
  
  return 0;
}
