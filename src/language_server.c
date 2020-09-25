#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

// for "kill"
#include <sys/types.h>
#include <signal.h>
// end of "kill"

#include "json.h"
#include "json_rpc.h"
#include "language_server.h"

/* ****** ****** */

int is_process_running(int pid) {
  int ret = kill((pid_t)pid, 0);
  return (ret == 0);
}

/* ****** ****** */

static const char* json_type_names[] = {
  "json_type_string",
  "json_type_number",
  "json_type_object",
  "json_type_array",
  "json_type_true",
  "json_type_false",
  "json_type_null"
};
static const char *json_type_name(enum json_type_e jt) {
  if (jt < json_type_string || jt > json_type_null) {
    return "UNKNOWN";
  } else {
    return json_type_names[jt];
  }
}

int validate_json_value_type_silent(const char *jsonpath, struct json_value_s *value, int nullable, enum json_type_e type, const char *msg_okay) {
  char message[256];
  message[0] = 0;
  char *format = NULL;

  if (json_value_is_null(value)) {
    if (!nullable) {
      format = "the parameter '%s' is null but it should be %s (%s)";
    } else {
      return 1;
    }
  } else {
    if (value->type != type) {
      format = "the parameter '%s' is of mismatched type; should be %s (%s)";
    } else {
      return 1;
    }
  }
  
  int message_used = snprintf(message, sizeof(message), format, jsonpath, msg_okay, json_type_name(type));
  if (message_used >= 0 && message_used < sizeof(message)-1) {
    fprintf(stderr, "error parsing parameter: %s\n", message);
    return 0;
  } else {
    fprintf(stderr, "INTERNAL ERROR! encoding error while trying to format validation message");
    return 0;
  }
}

int validate_json_value_type(FILE *fout, json_rpc_request_notification_t *request, const char *jsonpath, struct json_value_s *value, int nullable, enum json_type_e type, const char *msg_okay) {
  char message[256];
  message[0] = 0;
  char *format = NULL;

  if (json_value_is_null(value)) {
    if (!nullable) {
      format = "the parameter '%s' is null but it should be %s (%s)";
    } else {
      return 1;
    }
  } else {
    if (value->type != type) {
      format = "the parameter '%s' is of mismatched type; should be %s (%s)";
    } else {
      return 1;
    }
  }
  
  int message_used = snprintf(message, sizeof(message), format, jsonpath, msg_okay, json_type_name(type));
  if (message_used >= 0 && message_used < sizeof(message)-1) {
    json_rpc_invalid_params_error(fout, request, message);
    return 0;
  } else {
    json_rpc_internal_error(fout, request, "INTERNAL ERROR! encoding error while trying to format validation message");
    return 0;
  }
}

static
int server_parse_initialize_request(FILE *fout, json_rpc_request_notification_t *request, lsp_initialize_request_params_t *params) {
  params->parent_process_id = -1;
  params->root_uri = NULL;
  params->trace = LT_OFF;

  if (!validate_json_value_type(fout, request, "/params", request->params, 0, json_type_object, "InitializeParams")) {
    return 0;
  }

  struct json_object_s *object = json_value_as_object(request->params);
  assert(object != NULL);

  struct json_object_element_s* property = object->start;
  while (property != NULL) {
    const char *property_name = property->name->string;
    struct json_value_s* property_value = property->value;

    if (!strcmp(property_name, "processId")) {
      if (!validate_json_value_type(fout, request, "/params/processId", property_value, 1, json_type_number, "number (parent process id)")) {
        return 0;
      }
      if (!json_value_is_null(property_value)) {
        struct json_number_s *num = json_value_as_number(property_value);
        assert(num != NULL);
        params->parent_process_id = atoi(num->number);
      }
    } else if (!strcmp(property_name, "rootUri")) {
      if (!validate_json_value_type(fout, request, "/params/rootUri", property_value, 1, json_type_string, "Document URI")) {
        return 0;
      }
      if (!json_value_is_null(property_value)) {
        struct json_string_s *string_value = json_value_as_string(property_value);
        assert(string_value != NULL);
        params->root_uri = string_value->string;
      }
    } else if (!strcmp(property_name, "trace")) {
      if (!validate_json_value_type(fout, request, "/params/trace", property_value, 0, json_type_string, "trace level (off, messages, verbose, or unset)")) {
        return 0;
      }
      struct json_string_s *string_value = json_value_as_string(property_value);
      assert(string_value != NULL);
      const char *str = string_value->string;
      
      if (!strcmp(str, "off")) {
        params->trace = LT_OFF;
      } else if (!strcmp(str, "messages")) {
        params->trace = LT_MESSAGES;
      } else if (!strcmp(str, "verbose")) {
        params->trace = LT_VERBOSE;
      } else {
        json_rpc_invalid_params_error(fout, request, "trace should be one of \"off\", \"messages\", \"verbose\"");
        return 0;
      }
    }

    property = property->next;
  }

  // validate
  return (params->root_uri != NULL);
}

static int parse_textDocument_didOpen(json_rpc_request_notification_t *request, char **uri, char **language_id, int *version, struct json_string_s **contents) {
  assert(request != NULL);
  assert(uri != NULL);
  assert(language_id != NULL);
  assert(version != NULL);
  assert(contents != NULL);

  *uri = NULL;
  *language_id = NULL;
  *version = 0;
  *contents = NULL;

  if (!validate_json_value_type_silent("/params", request->params, 0, json_type_object, "DidOpenTextDocumentParams")) {
    return 0;
  }
  
  struct json_object_s *params_object = json_value_as_object(request->params);
  assert(params_object != NULL);

  struct json_object_s *item_object = NULL;
  struct json_object_element_s* property = NULL;

  property = params_object->start;
  while (property != NULL) {
    const char *property_name = property->name->string;
    struct json_value_s* property_value = property->value;

    if (!strcmp(property_name, "textDocument")) {
      if (!validate_json_value_type_silent( "/params/textDocument", property_value, 0, json_type_object, "TextDocumentItem")) {
        return 0;
      }
      item_object = json_value_as_object(property_value);
      break;
    }
    property = property->next;
  }

  assert(item_object != NULL);
  property = item_object->start;
  while (property != NULL) {
    const char *property_name = property->name->string;
    struct json_value_s* property_value = property->value;

    if (!strcmp(property_name, "uri")) {
      if (!validate_json_value_type_silent("/params/textDocument/uri", property_value, 0, json_type_string, "DocumentUri") ||
          json_value_is_null(property_value)) {
        return 0;
      }
      struct json_string_s *string_value = json_value_as_string(property_value);
      assert(string_value != NULL);
      *uri = string_value->string;
    } else if (!strcmp(property_name, "languageId")) {
      if (!validate_json_value_type_silent("/params/textDocument/languageId", property_value, 1, json_type_string, "language identifier")) {
        // NOTE: happens with lsp-mode (probabaly have to set up the language id there)
        fprintf(stderr, "language id is of weird type, ignored! type: %d\n", property_value->type);
      } else {
        if (!json_value_is_null(property_value)) {
          struct json_string_s *string_value = json_value_as_string(property_value);
          assert(string_value != NULL);
          *language_id = string_value->string;
        }
      }
    } else if (!strcmp(property_name, "version")) {
      if (!validate_json_value_type_silent("/params/textDocument/version", property_value, 0, json_type_number, "document version")
          || json_value_is_null(property_value)) {
        return 0;
      }
      struct json_number_s *num = json_value_as_number(property_value);
      assert(num != NULL);
      *version = atoi(num->number);
    } else if (!strcmp(property_name, "text")) {
      if (!validate_json_value_type_silent("/params/textDocument/text", property_value, 0, json_type_string, "document content") ||
          json_value_is_null(property_value)) {
        return 0;
      }
      struct json_string_s *string_value = json_value_as_string(property_value);
      assert(string_value != NULL);
      *contents = string_value;
    }

    property = property->next;
  }

  // validate
  return (*uri != NULL || *contents != NULL);
}

static int parse_Range(const char *path, struct json_value_s *value, int *start_line, int *start_char, int *end_line, int *end_char) {
  assert(path != NULL);
  assert(value != NULL);
  assert(start_line != NULL);
  assert(start_char != NULL);
  assert(end_line != NULL);
  assert(end_char != NULL);

  int got_start_line = 0;
  int got_start_char = 0;
  int got_end_line = 0;
  int got_end_char = 0;

  *start_line = -1;
  *start_char = -1;
  *end_line = -1;
  *end_char = -1;

  if (!validate_json_value_type_silent(path, value, 0, json_type_object, "Range")) {
    fprintf(stderr, "parse_Range: object expected\n");
    return 0;
  }

  struct json_object_s *range_object = json_value_as_object(value);
  assert(range_object != NULL);

  struct json_object_element_s* property = NULL;
  struct json_object_s *start_object = NULL;
  struct json_object_s *end_object = NULL;

  property = range_object->start;
  while (property != NULL) {
    const char *property_name = property->name->string;
    struct json_value_s* property_value = property->value;

    if (!strcmp(property_name, "start")) {
      if (!validate_json_value_type_silent(path, property_value, 0, json_type_object, "Position")) {
        fprintf(stderr, "parse_Range: object expected for \"start\"\n");
        return 0;
      }
      start_object = json_value_as_object(property_value);
    } else if (!strcmp(property_name, "end")) {
      if (!validate_json_value_type_silent(path, property_value, 0, json_type_object, "Position")) {
        fprintf(stderr, "parse_Range: object expected for \"end\"\n");
        return 0;
      }
      end_object = json_value_as_object(property_value);
    }
    
    property = property->next;
  }

  if (start_object == NULL || end_object == NULL) {
    fprintf(stderr, "parse_Range: start/end are mandatory\n");
    return 0;
  }

  // parse start
  property = start_object->start;
  while (property != NULL) {
    const char *property_name = property->name->string;
    struct json_value_s* property_value = property->value;

    if (!strcmp(property_name, "line")) {
      if (!validate_json_value_type_silent("line", property_value, 0, json_type_number, "line")
          || json_value_is_null(property_value)) {
        fprintf(stderr, "parse_Range: start line should be a number\n");
        return 0;
      }
      got_start_line = 1;
      struct json_number_s *num = json_value_as_number(property_value);
      assert(num != NULL);
      *start_line = atoi(num->number);
    } else if (!strcmp(property_name, "character")) {
      if (!validate_json_value_type_silent("character", property_value, 0, json_type_number, "character")
          || json_value_is_null(property_value)) {
        fprintf(stderr, "parse_Range: start character should be a number\n");
        return 0;
      }
      got_start_char = 1;
      struct json_number_s *num = json_value_as_number(property_value);
      assert(num != NULL);
      *start_char = atoi(num->number);
    }
    
    property = property->next;
  }

  // parse end
  property = end_object->start;
  while (property != NULL) {
    const char *property_name = property->name->string;
    struct json_value_s* property_value = property->value;

    if (!strcmp(property_name, "line")) {
      if (!validate_json_value_type_silent("line", property_value, 0, json_type_number, "line")
          || json_value_is_null(property_value)) {
        fprintf(stderr, "parse_Range: end line should be a number\n");
        return 0;
      }
      got_end_line = 1;
      struct json_number_s *num = json_value_as_number(property_value);
      assert(num != NULL);
      *end_line = atoi(num->number);
    } else if (!strcmp(property_name, "character")) {
      if (!validate_json_value_type_silent("character", property_value, 0, json_type_number, "character")
          || json_value_is_null(property_value)) {
        fprintf(stderr, "parse_Range: end character should be a number\n");
        return 0;
      }
      got_end_char = 1;
      struct json_number_s *num = json_value_as_number(property_value);
      assert(num != NULL);
      *end_char = atoi(num->number);
    }
    
    property = property->next;
  }

  if (!(got_start_line && got_start_char && got_end_line && got_end_char)) {
    fprintf(stderr, "parse_Range: missing required fields\n");
    return 0;
  }

  return 1;
}

static int parse_textDocument_didChange(json_rpc_request_notification_t *request, char **uri, int *version, file_edit_t **file_edits, size_t *file_edits_length) {
  assert(request != NULL);
  assert(uri != NULL);
  assert(version != NULL);
  assert(file_edits != NULL);
  assert(file_edits_length != NULL);

  *uri = NULL;
  *version = 0;
  *file_edits = NULL;
  *file_edits_length = 0;
  
  if (!validate_json_value_type_silent("/params", request->params, 0, json_type_object, "DidChangeTextDocumentParams")) {
    fprintf(stderr, "parse_textDocument_didChange: parameter should be JSON object\n");
    return 0;
  }

  struct json_object_s *params_object = json_value_as_object(request->params);
  assert(params_object != NULL);

  struct json_object_s *textDocument_object = NULL;
  struct json_array_s *contentChanges_array = NULL;
  struct json_object_element_s* property = NULL;

  property = params_object->start;
  while (property != NULL) {
    const char *property_name = property->name->string;
    struct json_value_s* property_value = property->value;

    if (!strcmp(property_name, "textDocument")) {
      if (textDocument_object != NULL || !validate_json_value_type_silent("/params/textDocument", property_value, 0, json_type_object, "VersionedTextDocumentItem")) {
        fprintf(stderr, "parse_textDocument_didChange: textDocument is not an object\n");
        return 0;
      }
      textDocument_object = json_value_as_object(property_value);
    } else if (!strcmp(property_name, "contentChanges")) {
      if (contentChanges_array != NULL || !validate_json_value_type_silent("/params/contentChanges", property_value, 0, json_type_array, "TextDocumentContentChangeEvent[]")) {
        fprintf(stderr, "parse_textDocument_didChange: contentChanges is not an object\n");
        return 0;
      }
      contentChanges_array = json_value_as_array(property_value);
    }
    property = property->next;
  }

  //
  // textDocument
  //
  assert(textDocument_object != NULL);
  property = textDocument_object->start;
  while (property != NULL) {
    const char *property_name = property->name->string;
    struct json_value_s* property_value = property->value;

    if (!strcmp(property_name, "uri")) {
      if (!validate_json_value_type_silent("/params/textDocument/uri", property_value, 0, json_type_string, "DocumentUri") ||
          json_value_is_null(property_value)) {
        fprintf(stderr, "parse_textDocument_didChange: textDocument/uri is null or wrong type\n");
        return 0;
      }
      struct json_string_s *string_value = json_value_as_string(property_value);
      assert(string_value != NULL);
      *uri = string_value->string;
    } else if (!strcmp(property_name, "version")) {
      if (!validate_json_value_type_silent("/params/textDocument/version", property_value, 0, json_type_number, "document version")
          || json_value_is_null(property_value)) {
        fprintf(stderr, "parse_textDocument_didChange: textDocument/version is null or wrong type\n");
        return 0;
      }
      struct json_number_s *num = json_value_as_number(property_value);
      assert(num != NULL);
      *version = atoi(num->number);
    }

    property = property->next;
  }

  //
  // contentChanges
  //
  assert(contentChanges_array != NULL);
  size_t length = contentChanges_array->length;
  if (length == 0) {
    return 0;
  }
  *file_edits_length = length;
  *file_edits = malloc(length * sizeof(file_edit_t));
  if (*file_edits == NULL) {
    fprintf(stderr, "failed to allocate file edits array\n");
    return 0;
  }
  struct json_array_element_s *element = contentChanges_array->start;
  file_edit_t *edit = *file_edits;
  size_t i = 0;
  while (element != NULL && i < length) {
    struct json_value_s *elem_value = element->value;

    edit->start_line = -1;
    edit->start_char = -1;
    edit->end_line = -1;
    edit->end_char = -1;
    edit->text = NULL;
    edit->text_length = 0;

    if (!validate_json_value_type_silent("/params/contentChanges", elem_value, 0, json_type_object, "TextDocumentContentChangeEvent")) {
      fprintf(stderr, "parse_textDocument_didChange: contentChanges[] is null or wrong type\n");
      return 0;
    }

    struct json_object_s *edit_object = json_value_as_object(elem_value);
    assert(edit_object != NULL);

    property = edit_object->start;
    while (property != NULL) {
      const char *property_name = property->name->string;
      struct json_value_s* property_value = property->value;

      if (!strcmp(property_name, "text")) {
        if (!validate_json_value_type_silent("/params/contentChanges[]/text", property_value, 0, json_type_string, "text") ||
            json_value_is_null(property_value)) {
          fprintf(stderr, "parse_textDocument_didChange: contentChanges[]/text is null or wrong type\n");
          return 0;
        }
        struct json_string_s *string_value = json_value_as_string(property_value);
        assert(string_value != NULL);
        edit->text = string_value->string;
        edit->text_length = string_value->string_size;
      } else if (!strcmp(property_name, "range")) {
        if (!parse_Range("/params/contentChanges[]/range", property_value, &edit->start_line, &edit->start_char, &edit->end_line, &edit->end_char)) {
          fprintf(stderr, "parse_textDocument_didChange: contentChanges[]/range is null or wrong type\n");
          return 0;
        }
      }

      property = property->next;
    }

    // validate
    if (!((edit->start_line == -1
           && edit->end_line == -1
           && edit->start_char == -1
           && edit->end_char == -1)
          || (edit->start_line < edit->end_line
              || edit->start_line == edit->end_line
              && edit->start_char <= edit->end_char))
        ) {
      fprintf(stderr, "parse_textDocument_didChange: validation failed: edit(start=%ld,%ld;end=%ld,%ld)%s\n",
              edit->start_line, edit->start_char,
              edit->end_line, edit->end_char,
              edit->text);
      return 0;
    }

    element = element->next;
    edit++;
    i++;
  }

  // validate
  return (*uri != NULL && *file_edits != NULL);
}

static
int parse_textDocument_didSave(json_rpc_request_notification_t *request, char **uri, int *version, int *have_version) {
  assert(uri != NULL && version != NULL && have_version != NULL);

  *uri = NULL;
  *version = 0;
  *have_version = 0;

  if (!validate_json_value_type_silent("/params", request->params, 0, json_type_object, "DidSaveTextDocumentParams")) {
    return 0;
  }

  struct json_object_s *params_object = json_value_as_object(request->params);
  assert(params_object != NULL);

  struct json_object_element_s* property = NULL;
  struct json_object_s *textDocument_object = NULL;
  
  property = params_object->start;
  while (property != NULL) {
    const char *property_name = property->name->string;
    struct json_value_s* property_value = property->value;

    if (!strcmp(property_name, "textDocument")) {
      if (!validate_json_value_type_silent("/params/textDocument", property_value, 0, json_type_object, "TextDocumentIdentifer")
          || json_value_is_null(property_value)) {
        return 0;
      }
      textDocument_object = json_value_as_object(property_value);
      break;
    }
    property = property->next;
  }
  assert(textDocument_object != NULL);

  property = textDocument_object->start;
  while (property != NULL) {
    const char *property_name = property->name->string;
    struct json_value_s* property_value = property->value;

    if (!strcmp(property_name, "uri")) {
      if (!validate_json_value_type_silent("/params/textDocument/uri", property_value, 0, json_type_string, "DocumentURI")
          || json_value_is_null(property_value)) {
        return 0;
      }
      struct json_string_s *string_value = json_value_as_string(property_value);
      assert(string_value != NULL);
      *uri = string_value->string;
    } else if (!strcmp(property_name, "version")) {
      if (!validate_json_value_type_silent("/params/textDocument/version", property_value, 0, json_type_number, "document version")
          || json_value_is_null(property_value)) {
        fprintf(stderr, "parse_textDocument_didSave: textDocument/version is null or wrong type\n");
        return 0;
      }
      *have_version = 1;
      struct json_number_s *num = json_value_as_number(property_value);
      assert(num != NULL);
      *version = atoi(num->number);
    }
    property = property->next;
  }

  return 1;
}

static
int parse_textDocument_didClose(json_rpc_request_notification_t *request, char **uri) {
  assert(uri != NULL);

  *uri = NULL;

  if (!validate_json_value_type_silent("/params", request->params, 0, json_type_object, "DidCloseTextDocumentParams")) {
    return 0;
  }

  struct json_object_s *params_object = json_value_as_object(request->params);
  assert(params_object != NULL);

  struct json_object_element_s* property = NULL;
  struct json_object_s *textDocument_object = NULL;
  
  property = params_object->start;
  while (property != NULL) {
    const char *property_name = property->name->string;
    struct json_value_s* property_value = property->value;

    if (!strcmp(property_name, "textDocument")) {
      if (!validate_json_value_type_silent("/params/textDocument", property_value, 0, json_type_object, "TextDocumentIdentifer")
          || json_value_is_null(property_value)) {
        return 0;
      }
      textDocument_object = json_value_as_object(property_value);
      break;
    }
    property = property->next;
  }
  assert(textDocument_object != NULL);

  property = textDocument_object->start;
  while (property != NULL) {
    const char *property_name = property->name->string;
    struct json_value_s* property_value = property->value;

    if (!strcmp(property_name, "uri")) {
      if (!validate_json_value_type_silent("/params/textDocument/uri", property_value, 0, json_type_string, "DocumentURI")
          || json_value_is_null(property_value)) {
        return 0;
      }
      struct json_string_s *string_value = json_value_as_string(property_value);
      assert(string_value != NULL);
      *uri = string_value->string;
    }
    property = property->next;
  }

  return 1;
}


/* ****** ****** */

void server_textDocument_didOpen(language_server_t *server, json_rpc_request_notification_t *request) {
  if (!server->initialized) {
    fprintf(stderr, "server not initialized yet!\n");
    return;
  }

  char *uri;
  char *language_id;
  int version;
  struct json_string_s *contents;

  if (!parse_textDocument_didOpen(request, &uri, &language_id, &version, &contents)) {
    fprintf(stderr, "textDocument/didOpen: unable to parse parameters!\n");
    return;
  }

  file_system_open(&server->fs, uri, version, contents->string, contents->string_size);
}

void server_textDocument_didChange(language_server_t *server, json_rpc_request_notification_t *request) {
  if (!server->initialized) {
    fprintf(stderr, "server not initialized yet!\n");
    return;
  }

  char *uri = NULL;
  int version = 0;
  file_edit_t *file_edits = NULL;
  size_t file_edits_length = 0;

  if (!parse_textDocument_didChange(request, &uri, &version, &file_edits, &file_edits_length)) {
    if (file_edits != NULL) {
      free(file_edits);
    }
    fprintf(stderr, "textDocument/didChange: unable to parse parameters!\n");
    return;
  }

  if (!file_system_change(&server->fs, uri, version, file_edits, file_edits_length)) {
    fprintf(stderr, "textDocument/didChange: error while applying changes!\n");
  }

  if (file_edits != NULL) {
    free(file_edits);
  }
}

int textbuf_fprint_read(char *buffer, size_t length, void *state) {
  FILE *fp = (FILE *)state;

  for (int i = 0; i < length; i++) {
    fprintf(fp, "%c", buffer[i]);
  }
  return 1;
}

void textbuf_fprint(text_buffer_t *tb, FILE *fp) {
  assert(tb != NULL && fp != NULL);

  text_buffer_read(tb, textbuf_fprint_read, fp);
}

void server_textDocument_didSave(language_server_t *server, json_rpc_request_notification_t *request) {
  if (!server->initialized) {
    fprintf(stderr, "server not initialized yet!\n");
    return;
  }

  char *uri;
  int version;
  int have_version = 0;
  if (!parse_textDocument_didSave(request, &uri, &version, &have_version)) {
    fprintf(stderr, "textDocument/didSave: unable to parse parameters!\n");
    return;
  }

  file_t *file = file_system_lookup(&server->fs, uri);
  if (file == NULL) {
    fprintf(stderr, "textDocument/didSave: unable to find the file identified by document URI %s!\n", uri);
  } else {
    // Sublime Text doesn't send its version in this message, but Emacs LSP-Mode does
    if (have_version && file->version != version) {
      fprintf(stderr, "textDocument/didSave: file identified by URI %s should have version %d, but actually has %d!\n", uri, version, file->version);
    }
    
    // DEBUG: write the contents of the file to stderr so we can compare the sync
    // textbuf_fprint(&file->text, stderr);
  }
}

void server_textDocument_didClose(language_server_t *server, json_rpc_request_notification_t *request) {
  if (!server->initialized) {
    fprintf(stderr, "server not initialized yet!\n");
    return;
  }

  char *uri;
  if (!parse_textDocument_didClose(request, &uri)) {
    fprintf(stderr, "textDocument/didClose: unable to parse parameter!\n");
    return;
  }

  file_system_close(&server->fs, uri);
}

/* ****** ****** */

void server_exit(language_server_t *server) {
  int retcode = server->shutdown_requested? 0 : 1;
  exit(retcode);  
}

void server_initialize(language_server_t *server, json_rpc_request_notification_t *request) {
  if (server->initialized) {
    json_rpc_invalid_params_error(server->fout, request, "Server already initialized");
    return;
  }

  lsp_initialize_request_params_t params;
  memset(&params, 0, sizeof(params));

  if (!server_parse_initialize_request(server->fout, request, &params)) {
    return;
  }

  if (params.parent_process_id > 0) {
    int parent_running = is_process_running(params.parent_process_id);
    if (!parent_running) {
      server_exit(server);
    }
  }
  server->initialized = 1;

  fprintf(stderr, "initialized, sending response\n");

  // NOTE about "textDocumentSync" capabilities:
  // - "openClose": true means that both document open and document sent notifications are sent by the client
  // - "change": 2 means that docs are synced by sending the full content on open; after that only incremental updates are sent by the client
  json_rpc_custom_success(server->fout, request, "{\"capabilities\": {\"textDocumentSync\": {\"openClose\": true, \"change\": 2, \"save\": {\"includeText\": false}}}, \
\"serverInfo\": {\"name\": \"xatsls\", \"version\": \"0.1.5\"}}");
}

void server_shutdown(language_server_t *server, json_rpc_request_notification_t *request) {
  if (!server->initialized) {
    json_rpc_invalid_params_error(server->fout, request, "Server already uninitialized");
  }
  // TODO: free everything, etc.
  file_system_free(&server->fs);
  server->shutdown_requested = 1;

  json_rpc_success(server->fout, request, json_null);
}

/* ****** ****** */

void language_server_evaluate(language_server_t *server, json_rpc_request_notification_t *request) {
  assert(request != NULL);
  assert(server != NULL);

  const char *method = request->method->string;

  fprintf(stderr, "got method: %s\n", method);

  if (json_rpc_request_is_notification(request)) {
    if (!strcmp(method, "exit")) {
      server_exit(server);
    } else if (!strcmp(method, "textDocument/didOpen")) {
      server_textDocument_didOpen(server, request);
    } else if (!strcmp(method, "textDocument/didChange")) {
      server_textDocument_didChange(server, request);
    } else if (!strcmp(method, "textDocument/didClose")) {
      server_textDocument_didClose(server, request);
    } else if (!strcmp(method, "textDocument/didSave")) {
      server_textDocument_didSave(server, request);
    } else {
      fprintf(stderr, "skipping notification: %s\n", method);
    }
  } else {
    if (!strcmp(method, "initialize")) {
      fprintf(stderr, "initialize request\n");
      server_initialize(server, request);
      fprintf(stderr, "initialize request done\n");
    } else if (!strcmp(method, "shutdown")) {
      fprintf(stderr, "shutdown request\n");
      server_shutdown(server, request);
    } else {
      json_rpc_method_not_found_error(server->fout, request);
    }
  }
}

int language_server_json_rpc_evaluate(FILE *fout, json_rpc_request_notification_t *request, void *state) {
  assert(state != NULL);
  language_server_t *server = (language_server_t *)state;

  language_server_evaluate(server, request);
  return 1;
}

void language_server_loop() {
  FILE *fd = stdin;
  FILE *fout = stdout;
  language_server_t server = {
    .fin = fd,
    .fout = fout,
    .initialized = 0,
    .shutdown_requested = 0
  };
  file_system_init(&server.fs);

  while (1) {
    int cont = json_rpc_server_step(fd, fout, &language_server_json_rpc_evaluate, &server);
    if (!cont) {
      break;
    }
  }
}
