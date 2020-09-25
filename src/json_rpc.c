#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>

#include "json.h"

#include "json_rpc.h"

/* ****** ****** */

void json_object_extend(struct json_object_s *obj, ...) {
  assert(obj != NULL);

  va_list args;

  va_start(args, obj);
  while (1) {
    struct json_object_element_s *prop = va_arg(args, struct json_object_element_s *);
    if (prop == NULL) {
      break;
    }
    if (prop->value == NULL) {
      continue; // skip it (if you want the null literal as property of object, use the corresponding value!)
    }

    struct json_object_element_t *tmp = obj->start;
    obj->start = prop;
    prop->next = tmp;
    obj->length++;    
  }
  va_end(args);
}

/* ****** ****** */

static void json_rpc_write_response(FILE *fout, struct json_value_s *value) {
  if (value == NULL) {
    fprintf(fout, "Content-Length: 0\r\n\r\n\r\n");
    fflush(fout);
  } else {
    size_t size = 0;
    char *string = json_write_minified(value, &size);

    assert(string != NULL);
    fprintf(fout, "Content-Length: %ld\r\n\r\n%s\r\n", size, string);
    fflush(fout);

    free(string);
  }
}

static void json_rpc_error(FILE *fout, const struct json_value_s *id, const char *code, const char *message, const struct json_value_s *data) {
  JSON_PROP_NUMBER(error_code, "code", code);
  JSON_PROP_STRING(error_message, "message", message);
  JSON_PROP_VALUE(error_data, "data", data);

  struct json_object_s error_obj = json_object_empty();
  json_object_extend(&error_obj, &error_data, &error_message, &error_code, NULL);

  struct json_object_s response_obj = json_object_empty();
  JSON_PROP_STRING(jsonrpc, "jsonrpc", "2.0");
  JSON_PROP_OBJECT(error, "error", error_obj);

  if (id != NULL) {
    JSON_PROP_VALUE(id_prop, "id", id);
    json_object_extend(&response_obj, &id_prop, &error, &jsonrpc, NULL);

    struct json_value_s response = json_value_object(&response_obj);
    json_rpc_write_response(fout, &response);
  } else {
    json_object_extend(&response_obj, &error, &jsonrpc, NULL);

    struct json_value_s response = json_value_object(&response_obj);
    json_rpc_write_response(fout, &response);
  }
}

/* ****** ****** */

void json_parse_error_reason(char *data_buf, size_t data_buf_size, struct json_parse_result_s *result) {
  assert(data_buf != NULL && data_buf_size >= 32);
  assert(result != NULL);
  assert(result->error != json_parse_error_none);

  const char *reason;
  switch (result->error) {
  case json_parse_error_expected_comma_or_closing_bracket:
    reason = "expected either a comma or a closing brace or bracket to close an object or array";
    break;
  case json_parse_error_expected_colon:
    reason = "colon separating name/value pair was missing";
    break;
  case json_parse_error_expected_opening_quote:
    reason = "expected string to begin with a double quote";
    break;
  case json_parse_error_invalid_string_escape_sequence:
    reason = "invalid escaped sequence in string";
    break;
  case json_parse_error_invalid_number_format:
    reason = "invalid number format";
    break;
  case json_parse_error_invalid_value:
    reason = "invalid value";
    break;
  case json_parse_error_premature_end_of_buffer:
    reason = "reached end of buffer before object/array was complete";
    break;
  case json_parse_error_invalid_string:
    reason = "string was malformed";
    break;
  case json_parse_error_allocator_failed:
    reason = "a call to malloc, or a user provider allocator, failed.";
    break;
  case json_parse_error_unexpected_trailing_characters:
    reason = "the JSON input had unexpected trailing characters that weren't part of the JSON value";
    break;
  case json_parse_error_unknown:
  default:
    reason = "unknown error";
    break;
  }

  data_buf[0] = 0;
  int data_used = snprintf(data_buf, data_buf_size, "Error parsing JSON: %lu(line=%lu, offs=%lu): %s",
                           result->error_offset, result->error_line_no, result->error_row_no, reason);
  if (!(data_used >= 0 && data_used < data_buf_size-1)) {
    snprintf(data_buf, data_buf_size, "Unable to print error location: buffer overflow!");
  }

}

void json_rpc_parse_error(FILE *fout, struct json_value_s *id, struct json_parse_result_s *result) {
  assert(result->error != json_parse_error_none);

  char data_buf[1024];
  json_parse_error_reason(data_buf, sizeof(data_buf), result);

  struct json_string_s data_string = json_make_string(data_buf);
  struct json_value_s data = json_value_string(&data_string);
  json_rpc_error(fout, id, "-32700", "Parse error", &data);
}

void json_rpc_invalid_request_error(FILE *fout, json_rpc_request_notification_t *request) {
  assert(request != NULL);

  json_rpc_error(fout, request->id, "-32600", "Invalid request", NULL);
}

void json_rpc_method_not_found_error(FILE *fout, json_rpc_request_notification_t *request) {
  assert(request != NULL);

  struct json_value_s method_value = json_value_string(request->method);
  json_rpc_error(fout, request->id, "-32601", "Method not found", &method_value);
}

void json_rpc_invalid_params_error(FILE *fout, json_rpc_request_notification_t *request, const char *reason) {
  assert(request != NULL);

  if (reason != NULL && strlen(reason) > 0) {
    struct json_string_s reason_string = json_make_string (reason);
    struct json_value_s reason_value = json_value_string (&reason_string);
    
    json_rpc_error(fout, request->id, "-32602", "Invalid params", &reason_value);
  } else {
    json_rpc_error(fout, request->id, "-32602", "Invalid params", NULL);
  }
}

void json_rpc_internal_error(FILE *fout, json_rpc_request_notification_t *request, const char *reason) {
  assert(request != NULL);

  if (reason != NULL && strlen(reason) > 0) {
    struct json_string_s reason_string = json_make_string (reason);
    struct json_value_s reason_value = json_value_string (&reason_string);
    
    json_rpc_error(fout, request->id, "-32603", "Internal error", &reason_value);
  } else {
    json_rpc_error(fout, request->id, "-32603", "Internal error", NULL);
  }
}

void json_rpc_custom_error(FILE *fout, json_rpc_request_notification_t *request, int error_code, const char *message) {
  assert(request != NULL);

  char error_buf[16];
  error_buf[0] = 0;
  int error_used = snprintf(error_buf, sizeof(error_buf), "%d", error_code);
  assert(error_used >= 0 && error_used < sizeof(error_buf)-1);

  if (message == NULL) {
    message = "(no message)";
  }

  struct json_string_s message_string = json_make_string (message);
  struct json_value_s message_value = json_value_string (&message_string);
    
  json_rpc_error(fout, request->id, error_buf, message, NULL);
}

void json_rpc_custom_success(FILE *fout, json_rpc_request_notification_t *request, const char *json) {
  assert(request != NULL);
  assert(json != NULL);
  
  struct json_parse_result_s parse_result;
  size_t json_length = strlen(json);
  struct json_value_s *json_value = json_parse_ex(json, json_length, json_parse_flags_default, NULL, NULL, &parse_result);

  if (parse_result.error != json_parse_error_none) {
    char data_buf[1024];
    json_parse_error_reason(data_buf, sizeof(data_buf), &parse_result);

    struct json_string_s data_string = json_make_string(data_buf);
    struct json_value_s data = json_value_string(&data_string);

    json_rpc_error(fout, request->id, "-32603", "Internal error", &data);
  } else {
    json_rpc_success(fout, request, json_value);
  }

  if (json_value != json_null) {
    free(json_value);
  }
}

void json_rpc_success(FILE *fout, json_rpc_request_notification_t *request, const struct json_value_s *result) {
  assert(request != NULL);

  struct json_value_s null_result = {NULL, json_type_null};
  if (result == NULL) {
    result = &null_result;
  }

  struct json_object_s response_obj = json_object_empty();
  JSON_PROP_STRING(jsonrpc, "jsonrpc", "2.0");
  JSON_PROP_VALUE(result_prop, "result", result);

  if (request->id != NULL) {
    JSON_PROP_VALUE(id_prop, "id", request->id);
    json_object_extend(&response_obj, &id_prop, &result_prop, &jsonrpc, NULL);

    struct json_value_s response = json_value_object(&response_obj);
    json_rpc_write_response(fout, &response);
  } else {
    json_object_extend(&response_obj, &result_prop, &jsonrpc, NULL);

    struct json_value_s response = json_value_object(&response_obj);
    json_rpc_write_response(fout, &response);
  }
}

int json_rpc_parse_request_notification(struct json_value_s *root, json_rpc_request_notification_t *res) {
  struct json_object_s* object = json_value_as_object(root);
  
  if (object == NULL || res == NULL)
    return 0;
    
  int has_jsonrpc = 0;
  struct json_string_s *method = NULL;
  struct json_value_s *params = NULL;
  struct json_value_s *id = NULL;

  struct json_object_element_s* property = object->start;
  while (property != NULL) {
    
    const char *property_name = property->name->string;
    struct json_value_s* property_value = property->value;
    struct json_string_s *string_value = json_value_as_string(property_value);

    if (!strcmp(property_name, "jsonrpc")) {
      has_jsonrpc = (string_value != NULL && !strcmp(string_value->string, "2.0"));
    } else if (!strcmp(property_name, "method")) {
      method = string_value;
    } else if (!strcmp(property_name, "params")) {
      params = property_value;
    } else if (!strcmp(property_name, "id")) {
      id = property_value;
    }

    property = property->next;
  }

  int method_valid = method != NULL;  
  int params_valid = params == NULL || params != NULL && (json_value_as_array(params) != NULL || json_value_as_object(params) != NULL);
  int id_valid = id == NULL || id != NULL && (json_value_as_string(id) != NULL || json_value_as_number(id) != NULL);

  if (method_valid) res->method = method;
  if (params_valid) res->params = params;
  if (id_valid) res->id = id;

  int valid = has_jsonrpc && method_valid && params_valid && id_valid;
  return valid;
}

int json_rpc_request_is_notification(json_rpc_request_notification_t *request) {
  assert(request != NULL);
  return (request->id == NULL);
}

int atoi_len(const char *str, int len) {
  int i;
  int ret = 0;
  for (i = 0; i < len; ++i) {
    if (str[i] < '0' || str[i] > '9')
      break; // not a digit!
    ret = ret * 10 + (str[i] - '0');
  }
  return ret;
}

char *parse_request(FILE *fd, size_t *content_length) {
  char buf[1024];
  size_t buflen = 0;
  const char *content_length_string = "content-length: ";
  size_t content_length_string_length = strlen(content_length_string);

  assert(content_length != NULL);

  while (1) {
    int c = fgetc(fd);
    if (c == EOF) {
      fprintf(stderr, "parse_request: unexpected EOF while reading request\n");
      return NULL;
    }
    if (c == '\n') {
      if (buflen == 0) {
        break;
      }
      if (!memcmp(buf, content_length_string, content_length_string_length)) {
        if (buflen <= content_length_string_length) {
          fprintf(stderr, "parse_request: unable to parse content-length\n");
          return NULL;
        }
        *content_length = atoi_len(buf + content_length_string_length, buflen - content_length_string_length);
      }
      buf[0] = '\0';
      buflen = 0;
    } else if (c != '\r') {
      buf[buflen++] = tolower(c);
    }
  }

  char *content_buf = malloc(*content_length + 1);
  if (content_buf == NULL) {
    fprintf(stderr, "unable to allocate %lu bytes for content buffer\n", *content_length);
    return NULL;
  }
  for (size_t i = 0; i < *content_length; i++) {
    int c = fgetc(fd);
    if (c == EOF) {
      free(content_buf);
      fprintf(stderr, "unable to read request body\n");
      return NULL;
    }
    content_buf[i] = c;
  }
  content_buf[*content_length] = '\0';

  return content_buf;
}

int json_rpc_server_step(FILE *fd, FILE *fout, json_rpc_evaluate_t evaluate, void *state) {
  size_t content_length;
  char *content = parse_request(fd, &content_length);
    
  if (content == NULL) {
    fprintf(stderr, "json_rpc_server_step: failed to parse anything\n");
    return 0;
  }
  // printf("RAW content: %s with length %d\n", content, content_length);
  
  int cont;
  struct json_parse_result_s parse_result;
  struct json_value_s *json_value = json_parse_ex(content, content_length, json_parse_flags_default, NULL, NULL, &parse_result);

  if (parse_result.error != json_parse_error_none || json_value == json_null) {
   json_rpc_parse_error(fout, NULL, &parse_result);
   cont = 1;
  } else {
    json_rpc_request_notification_t request;

    memset(&request, 0, sizeof(request));
    if (!json_rpc_parse_request_notification(json_value, &request)) {
      json_rpc_invalid_request_error(fout, &request);
      cont = 1;
    } else {
      cont = evaluate(fout, &request, state);
    }
  }

  if (json_value != json_null) {
    free(json_value);
  }
  free(content);

  return cont;
}
