#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "picohttpparser.h"
#include "json.h"

#include "json_rpc.h"

void json_rpc_parse_error(FILE *fout, struct json_value_s *id, struct json_parse_result_s *result) {
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
  
  fprintf(fout, "{\"jsonrpc\": \"2.0\", \"error\": ");
  
  fprintf(fout, "{\"code\": -32700, \"message\": \"Parse error\", \"data\": \"Error parsing JSON: %d(line=%d, offs=%d): %s\"}",
    result->error_offset, result->error_line_no, result->error_row_no, reason);
  
  if (id != NULL) {
    char *idstring = json_write_minified(id, NULL);
    if (idstring != NULL) {
      fprintf(fout, ", \"id\": %s", idstring);
      free(idstring);
    }
  }
  
  fprintf(fout, "}");
}

void json_rpc_invalid_request_error(FILE *fout, json_rpc_request_notification_t *request) {
  fprintf(fout, "{\"jsonrpc\": \"2.0\", \"error\": ");
  
  fprintf(fout, "{\"code\": -32600, \"message\": \"Invalid request\"}");
  
  if (request->id != NULL) {
    char *idstring = json_write_minified(request->id, NULL);
    if (idstring != NULL) {
      fprintf(fout, ", \"id\": %s", idstring);
      free(idstring);
    }
  }
  
  fprintf(fout, "}");
}

void json_rpc_method_not_found_error(FILE *fout, json_rpc_request_notification_t *request) {
  assert(request != NULL);

  fprintf(fout, "{\"jsonrpc\": \"2.0\", \"error\": ");
  
  char *method = request->method->string;
  struct json_value_s method_value = {request->method, json_type_string};
    
  char *method_json = json_write_minified(&method_value, NULL);
  
  fprintf(fout, "{\"code\": -32601, \"message\": \"Method not found\", \"data\": %s}", method_json);
  free(method_json);

  if (request->id != NULL) {
    char *idstring = json_write_minified(request->id, NULL);
    if (idstring != NULL) {
      fprintf(fout, ", \"id\": %s", idstring);
      free(idstring);
    }
  }
  
  fprintf(fout, "}");
}

void json_rpc_invalid_params_error(FILE *fout, json_rpc_request_notification_t *request, const char *reason) {
  fprintf(fout, "{\"jsonrpc\": \"2.0\", \"error\": ");
  
  fprintf(fout, "{\"code\": -32602, \"message\": \"Invalid params\"");

  if (reason != NULL && strlen(reason) > 0) {
    struct json_string_s reason_string = {reason, strlen(reason)};
    struct json_value_s reason_value = {&reason_string, json_type_string};
    
    char *reason_json = json_write_minified(&reason_value, NULL);
    if (reason_json != NULL) {
      fprintf(fout, ", \"data\": %s", reason_json);
      free(reason_json);
    }
  }

  fprintf(fout, "}");
  
  if (request->id != NULL) {
    char *idstring = json_write_minified(request->id, NULL);
    if (idstring != NULL) {
      fprintf(fout, ", \"id\": %s", idstring);
      free(idstring);
    }
  }
  
  fprintf(fout, "}");  
}

void json_rpc_internal_error(FILE *fout, json_rpc_request_notification_t *request, const char *reason) {
  fprintf(fout, "{\"jsonrpc\": \"2.0\", \"error\": ");
  
  fprintf(fout, "{\"code\": -32603, \"message\": \"Internal error\"");

  if (reason != NULL && strlen(reason) > 0) {
    struct json_string_s reason_string = {reason, strlen(reason)};
    struct json_value_s reason_value = {&reason_string, json_type_string};
    
    char *reason_json = json_write_minified(&reason_value, NULL);
    if (reason_json != NULL) {
      fprintf(fout, ", \"data\": %s", reason_json);
      free(reason_json);
    }
  }

  fprintf(fout, "}");  
  
  if (request->id != NULL) {
    char *idstring = json_write_minified(request->id, NULL);
    if (idstring != NULL) {
      fprintf(fout, ", \"id\": %s", idstring);
      free(idstring);
    }
  }
  
  fprintf(fout, "}");  
}

void json_rpc_success(FILE *fout, json_rpc_request_notification_t *request, const struct json_value_s *result) {
  fprintf(fout, "{\"jsonrpc\": \"2.0\", \"result\": ");

  char *result_json = json_write_minified(result, NULL);
  assert(result_json != NULL);
  fprintf(fout, "%s", result_json);
  free(result_json);

  if (request->id != NULL) {
    char *idstring = json_write_minified(request->id, NULL);
    if (idstring != NULL) {
      fprintf(fout, ", \"id\": %s", idstring);
      free(idstring);
    }
  }
  
  fprintf(fout, "}");    
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
    
    char *property_name = property->name->string;
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

struct phr_header *get_request_header(struct phr_header *headers, size_t num_headers, const char *header_name) {
  size_t header_name_length = strlen(header_name);

  for (size_t i = 0; i < num_headers; i++) {
    if (header_name_length == headers[i].name_len && !strncasecmp(header_name, headers[i].name, header_name_length)) {
      return &headers[i];
    }
  }

  return NULL;
}

char *parse_request(FILE *fd, struct phr_header *headers, size_t *num_headers, size_t *content_length) {
  char buf[4096];
  size_t buflen = 0, prevbuflen = 0;
  size_t rret;
  int pret = 0;
  
  if (headers == NULL || num_headers == NULL || *num_headers <= 0 || content_length == NULL)
    return NULL;

  while (1) {
    // read the request
    rret = fread(buf + buflen, 1, sizeof(buf) - buflen, fd);
    if (rret == 0)
      return NULL; // unexpected EOF

    prevbuflen = buflen;
    buflen += rret;

    // parse the headers
    pret = phr_parse_headers(buf, buflen, headers, num_headers, prevbuflen);
    if (pret > 0)
      break; /* successfully parsed the request */
    else if (pret == -1) {
      fprintf(stderr, "unable to parse the headers\n");
      return NULL;
    }
    /* request is incomplete, continue the loop */
    assert(pret == -2);
    if (buflen == sizeof(buf)) {
      fprintf(stderr, "headers too long, unable to parse\n");
      return NULL;
    }
  }

  struct phr_header *content_length_header = get_request_header(headers, *num_headers, "content-length");
  if (content_length_header == NULL) {
    fprintf(stderr, "Missing Content-Length header, but it is required");
    exit(1);
  }

  // read content-length bytes
  *content_length = atoi_len(content_length_header->value, (int)content_length_header->value_len);
  
  size_t buf_have = buflen - pret;
    
  char *content_buf = malloc( *content_length + 1 );
  if (content_buf == NULL) {
    fprintf(stderr, "unable to allocate %d bytes for content buffer\n", *content_length);
    return NULL;
  }

  if (buf_have >= *content_length) {
    memcpy(content_buf, buf + pret, *content_length);
  } else {
    // fprintf(stderr, "buf_have %d, content_length %d\n", buf_have, *content_length);
  
    size_t buf_to_read = *content_length - buf_have;

    memcpy(content_buf, buf + pret, buf_have);
    rret = fread(content_buf + buf_have, 1, buf_to_read, fd);
    if (rret == 0) {
      fprintf(stderr, "unexpected EOF reading request body\n");
      free(content_buf);
      return NULL;
    }  
  }
  
  content_buf[*content_length] = '\0';

  return content_buf;
}

int json_rpc_server_step(FILE *fd, FILE *fout, json_rpc_evaluate_t evaluate, void *state) {
  struct phr_header headers[32];

  size_t num_headers = sizeof(headers) / sizeof(headers[0]);
  size_t content_length;
  char *content = parse_request(fd, headers, &num_headers, &content_length);
    
  if (content == NULL) {
    // failed to parse anything
    return 1;
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
