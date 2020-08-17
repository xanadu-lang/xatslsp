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

#include "picohttpparser.h"
#include "json.h"
#include "json_rpc.h"
#include "language_server.h"

/* ****** ****** */

int is_process_running(int pid) {
  int ret = kill((pid_t)pid, 0);
  return (ret == 0);
}

/* ****** ****** */

int validate_json_value_type(FILE *fout, json_rpc_request_notification_t *request, const char *jsonpath, struct json_value_s *value, int nullable, enum json_type_e type, const char *msg_okay) {
  char message[256];
  message[0] = 0;
  char *format = NULL;

  if (json_value_is_null(value)) {
    if (!nullable) {
      format = "the parameter '%s' is null but it should be %s";
    } else {
      return 1;
    }
  } else {
    if (value->type != type) {
      format = "the parameter '%s' is of mismatched type; should be %s";
    } else {
      return 1;
    }
  }
  
  int message_used = snprintf(message, sizeof(message), format, jsonpath, msg_okay);
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

  return 1;
}

#if 0
static
int server_parse_textDocument_didOpen_notification(FILE *fout, json_rpc_request_notification_t *request, lsp_didOpenTextDocument_notification_params_t *params) {
  // {"textDocument": TextDocumentItem}
  return 1;
}

static
int server_parse_textDocument_didChange_notification(FILE *fout, json_rpc_request_notification_t *request, lsp_didChangeTextDocument_notification_params_t *params) {
  /*
{
    textDocument: VersionedTextDocumentIdentifier;
    contentChanges: TextDocumentContentChangeEvent[];
}
   */
    /*
      TextDocumentContentChangeEvent =
 { // replacement
"range": Range,
"text": string
}
|
{ // append
"text": string
}
     */
  return 1;
}

static
int server_parse_textDocument_didClose_notification(FILE *fout, json_rpc_request_notification_t *request, lsp_didCloseTextDocument_notification_params_t *params) {
  // {"textDocument": TextDocumentIdentifier}
  return 1;
}
#endif

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

  // NOTE:
  // - "openClose": true means that both document open and document sent notifications are sent by the client
  // - "change": 2 means that docs are synced by sending the full content on open; after that only incremental updates are sent by the client
  json_rpc_custom_success(server->fout, request, "{\"capabilities\": {\"textDocumentSync\": {\"openClose\": true, \"change\": 2, \"save\": {\"includeText\": false}}},\
\"serverInfo\": {\"name\": \"xatslsp\", \"version\": \"0.1\"}}");
}

void server_shutdown(language_server_t *server, json_rpc_request_notification_t *request) {
  if (!server->initialized) {
    json_rpc_invalid_params_error(server->fout, request, "Server already uninitialized");
  }
  // TODO: free everything, etc.
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
    // json_rpc_method_not_found_error(fout, request);
    // "textDocument/didOpen"
    // "textDocument/didChange"
    // "textDocument/didClose"
    if (!strcmp(method, "exit")) {
      server_exit(server);
    } else {
      fprintf(stderr, "skipping it\n");
    }
  } else {
    if (!strcmp(method, "initialize")) {
      fprintf(stderr, "initialize request\n");
      server_initialize(server, request);
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
  language_server_t server = {.fin = fd, .fout = fout, .initialized = 0, .shutdown_requested = 0};

  while (1) {
    int cont = json_rpc_server_step(fd, fout, &language_server_json_rpc_evaluate, &server);
    if (!cont) {
      break;
    }
  }  
}
