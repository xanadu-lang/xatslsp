#ifndef __JSON_RPC_H__
#define __JSON_RPC_H__

#include "json.h"

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
