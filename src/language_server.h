#ifndef __LANGUAGE_SERVER_H__
#define __LANGUAGE_SERVER_H__

#include <stdio.h>
#include "json_rpc.h"
#include "file_system.h"

/*
set of "source files"
- each source file has
  - version:int
  - document URI (uniquely identifies a file)
  - open count (should be at most 1: how many times we get 'didOpen'/'didClose' notifications)
  - text (aka content)

what to do with this set?
- lookup by document URI
- insert (does not exist in the set yet)
- delete from the set

do we also need a mapping from document URIs to file names and back?
- yes, but something real cheap
- 
 */

typedef struct language_server_s {
  FILE *fin;
  FILE *fout;
  int   initialized;
  int   shutdown_requested;
  file_system_t fs;
} language_server_t;

typedef enum {
  LT_OFF,
  LT_MESSAGES,
  LT_VERBOSE
} lsp_trace_t;

typedef struct lsp_initialize_request_params_s {
  int parent_process_id; // negative if unset
  char *root_uri;
  lsp_trace_t trace;
} lsp_initialize_request_params_t;

typedef struct lsp_position_s {
  int line; // line position in a document, 0-based
  int character; // character offset on a line in a document, 0-based
  int byte_offset; // offset from the start of the file (not set during JSON parsing)
} lsp_position_t;

// this represents a selected porition of the document,
// i.e. a set such that for any position P in the set, P >= start && P < end
typedef struct lsp_range_s {
  lsp_position_t start;
  lsp_position_t end; // exclusive!
} lsp_range_t;

// this represents a location inside a resource
typedef struct lsp_location_s {
  char *uri;
  lsp_range_t range;
} lsp_location_t;

// this represents a textual edit applicable to a document
typedef struct lsp_text_edit_s {
  int range_present; // true if the below range is present (otherwise it is whole-document replacement!)
  lsp_range_t range; // the range to delete (to only insert, supply an empty range, i.e. start == end)
  char *new_text; // the text to insert after range
} lsp_text_edit_t;

typedef struct lsp_versioned_document_id_s {
  char *uri;
  int version;
} lsp_versioned_document_id_t;

// ONLY used in DidOpenTextDocumentParams
typedef struct lsp_text_document_item_s {
  lsp_versioned_document_id_t id;
  char *language_id;
  char *text;
} lsp_text_document_item_t;

typedef struct lsp_text_document_change_s {
  lsp_versioned_document_id_t   id;
  int                           numChanges;
  lsp_text_edit_t              *changes;
} lsp_text_document_change_t;

void server_exit(language_server_t *server);

void language_server_evaluate(language_server_t *ls, json_rpc_request_notification_t *request);
void language_server_loop();

#endif /* !__LANGUAGE_SERVER_H__ */
