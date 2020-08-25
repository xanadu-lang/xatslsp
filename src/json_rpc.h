#ifndef __JSON_RPC_H__
#define __JSON_RPC_H__

#include <assert.h>
#include <string.h>

#include "json.h"

/* ****** ****** */

json_weak
struct json_string_s json_make_string(const char *value) {
  assert(value != NULL);
  struct json_string_s string = {value, strlen(value)};
  return string;
}

json_weak
struct json_number_s json_make_number(const char *num) {
  assert(num != NULL);
  struct json_number_s number = {num, strlen(num)};
  return number;
}

json_weak
struct json_object_s json_object_empty() {
  struct json_object_s obj = {NULL, 0};
  return obj;
}

// NOTE: expects NULL-terminated [struct json_object_element_s *prop] arguments!
// - will cons all of them onto the property list of [obj]
void json_object_extend(struct json_object_s *obj, ...);

json_weak
struct json_value_s json_value_string(const struct json_string_s *str) {
  assert(str != NULL);
  struct json_value_s value = {str, json_type_string};
  return value;
}

json_weak
struct json_value_s json_value_number(const struct json_number_s *num) {
  assert(num != NULL);
  struct json_value_s value = {num, json_type_number};
  return value;
}

json_weak
struct json_value_s json_value_object(const struct json_object_s *obj) {
  if (obj != NULL) {
    struct json_value_s value = {obj, json_type_object};
    return value;
  } else {
    struct json_value_s value = {NULL, json_type_null};
    return value;
  }
}

#define JSON_PROP_NUMBER(name, prop, value)                             \
  struct json_string_s name##_prop_string = json_make_string((prop));   \
  struct json_number_s name##_value_number = json_make_number((value)); \
  struct json_value_s  name##_value_number_v = json_value_number(&(name##_value_number)); \
  struct json_object_element_s name = {&(name##_prop_string), &(name##_value_number_v), NULL};
#define JSON_PROP_STRING(name, prop, value)                             \
  struct json_string_s name##_prop_string = json_make_string((prop));   \
  struct json_string_s name##_value_string = json_make_string((value)); \
  struct json_value_s  name##_value_string_v = json_value_string(&(name##_value_string)); \
  struct json_object_element_s name = {&(name##_prop_string), &(name##_value_string_v), NULL};
#define JSON_PROP_OBJECT(name, prop, object) \
  struct json_string_s name##_prop_string = json_make_string((prop));   \
  struct json_value_s name##_value_object = json_value_object(&object); \
  struct json_object_element_s name = {&(name##_prop_string), &(name##_value_object), NULL};
#define JSON_PROP_VALUE(name, prop, value)                              \
  struct json_string_s name##_prop_string = json_make_string((prop));   \
  struct json_object_element_s name = {&(name##_prop_string), (value), NULL};

/* ****** ****** */

typedef struct json_rpc_request_notification_s {
  struct json_value_s   *json_root; // the object that owns the memory
  struct json_string_s  *method;
  struct json_value_s   *params;
  struct json_value_s   *id; // if non-NULL, then it is request; else, notification
} json_rpc_request_notification_t;

void json_rpc_parse_error(FILE *fout, struct json_value_s *id, struct json_parse_result_s *result);
void json_rpc_invalid_request_error(FILE *fout, json_rpc_request_notification_t *request);
void json_rpc_method_not_found_error(FILE *fout, json_rpc_request_notification_t *request);
void json_rpc_invalid_params_error(FILE *fout, json_rpc_request_notification_t *request, const char *reason);
void json_rpc_internal_error(FILE *fout, json_rpc_request_notification_t *request, const char *reason);
void json_rpc_custom_error(FILE *fout, json_rpc_request_notification_t *request, int error_code, const char *message);

void json_rpc_custom_success(FILE *fout, json_rpc_request_notification_t *request, const char *json);
void json_rpc_success(FILE *fout, json_rpc_request_notification_t *request, const struct json_value_s *json);

int json_rpc_parse_request_notification(struct json_value_s *root, json_rpc_request_notification_t *res);

int json_rpc_request_is_notification(json_rpc_request_notification_t *request);

typedef
int (*json_rpc_evaluate_t)(FILE *fout, json_rpc_request_notification_t *request, void *state);

// NOTE: this is more of a function template than a function
int json_rpc_server_step(FILE *fd, FILE *fout, json_rpc_evaluate_t evaluate, void *state);

#endif /* !__JSON_RPC_H__ */
