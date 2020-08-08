#ifndef __LANGUAGE_SERVER_H__
#define __LANGUAGE_SERVER_H__

#include <stdio.h>
#include "json_rpc.h"

typedef struct language_server_s {
  FILE *fin;
  FILE *fout;
  int   initialized;
} language_server_t;

int language_server_evaluate(language_server_t *ls, json_rpc_request_notification_t *request);
void language_server_loop();

#endif /* !__LANGUAGE_SERVER_H__ */
