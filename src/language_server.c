#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "picohttpparser.h"
#include "json.h"
#include "json_rpc.h"

typedef struct language_server_s {
  FILE *fin;
  FILE *fout;
  int   initialized;
} language_server_t;

int language_server_evaluate(FILE *fout, json_rpc_request_notification_t *request, void *state) {
  assert(request != NULL);
  const char *method = request->method->string;
  
  // before initialize:
  //  - all notifications are to be dropped (no response to those at all)
  //    - except for the exit notification
  //  - all requests should be responded to with code:-32002 (message: anything)
  // initialize:
  //  - respond with InitializeResult
  //  - during responding, may send back notifications:
  //    window/showMessage, window/logMessage and telemetry/event as well
  //    as the window/showMessageRequest request to the client. In case the
  //    client sets up a progress token in the initialize params
  //    (e.g. property workDoneToken) the server is also allowed to use that
  //    token (and only that token) using the $/progress notification sent
  //    from the server to the client.
  //  - this request may only be sent once
  /*
  InitializeParams:
  - "processId": number|null (ID of parent process; if that process is given and is not alive, we should exit)
  - "rootUri": string(URI) | null
  - "trace"?: 'off' | 'messages' | 'verbose' (if omitted, it is off by default)
  - "workspaceFolders": should we support this?
  response: InitializeResult:
  "capabilities": ServerCapabilities
  "serverInfo": {"name": string, "version": string}
  possible errors:
  - 1 // unknown protocol version
  error.data:
  - {retry:bool} // can client retry initialization?
  
  server capabilities:
  {"textDocumentSync": {"openClose": true, "change": 2, "save": {"includeText": false}}}
  */
  // after initialize
  // - client sends back the "initialized" notification
  
  if (0 && !strcmp(method, "sum")) {
  } else {
    json_rpc_method_not_found_error(fout, request);
    return 1;
  }
#if 0
  // TODO: Here, do a method lookup
  // - or continue to method execution:
  //   - validate all parameters
  //     - fail with -32602 	Invalid params 	Invalid method parameter(s).
  //       - a method may evaluate to this error
  //       - or it may evaluate to some other, internal error
  //       - or it may evaluate to an actual result
  //     - or execute the method
  /*
  methods we want? this depends on the actual application, so it's probably best to make this configurable.
  */
  printf("pretty value:\n");
  size_t size = 0;
  char *pretty = json_write_pretty(json_value, 0, 0, &size);
          if (pretty != NULL) {
            printf("%s\n", pretty);
            free(pretty);
          }
#endif
}

void language_server_loop() {
  FILE *fd = freopen(NULL, "rb", stdin); // ensure it is in binary mode (we will handle newlines)
  FILE *fout = stderr; // should actually be stdout
  language_server_t server = {.fin = fd, .fout = fout, .initialized = 0};

  while (1) { 
    int cont = json_rpc_server_step(fd, fout, &language_server_evaluate, &server);
    if (!cont) {
      break;
    }
  }  
}
