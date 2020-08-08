#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "json.h"
#include "json_rpc.h"

int validate_sum1_params(struct json_value_s *params, double *num) {
  assert(num != NULL);

  if (params == NULL)
    return 0;

  struct json_value_s* b_value = params;
  if (b_value->type != json_type_array)
    return 0;
  
  struct json_array_s* array = (struct json_array_s*)b_value->payload;
  if (array->length != 2)
    return 0;
  
  struct json_array_element_s* b_1st = array->start;

  struct json_value_s* b_1st_value = b_1st->value;
  if (b_1st_value->type != json_type_number)
    return 0;

  struct json_number_s *b_1st_number = (struct json_number_s *)b_1st_value->payload;
  if (strcmp(b_1st_number->number, "1"))
    return 0;

  struct json_array_element_s* b_2nd = b_1st->next;

  struct json_value_s* b_2nd_value = b_2nd->value;
  if (b_1st_value->type != json_type_number)
    return 0;

  struct json_number_s *b_2nd_number = (struct json_number_s *)b_2nd_value->payload;
  *num = atof(b_2nd_number->number);
  
  return 1;
}

int test_evaluate(FILE *fout, json_rpc_request_notification_t *request, void *state) {
  const char *mth = request->method->string;

  if (!strcmp(mth, "sum1")) {
    // the request must have two parameters in an array
    // and the first parameter must be 1
    double num = 0.0;
    
    if (!validate_sum1_params(request->params, &num)) {
      json_rpc_invalid_params_error(fout, request, "there must be 2 numbers in an array, the first number being 1");
    } else {
      num += 1.0;

      char output[50];
      output[0] = 0;
      int output_used = snprintf(output, sizeof(output), "%f", num);
      if (output_used >= 0 && output_used < sizeof(output)-1) {
        struct json_number_s num_json = {output, output_used};
        struct json_value_s num_value = {&num_json, json_type_number};

        json_rpc_success(fout, request, &num_value);
      } else {
        json_rpc_internal_error(fout, request, "INTERNAL ERROR! encoding error!");
      }
    }
  } else if (!strcmp(mth, "interror")) {
    json_rpc_internal_error(fout, request, "INTERNAL ERROR! Sorry, details unavailable.\nPlease try again later");
  } else if (!strcmp(mth, "interror2")) {
    // reply with a string that has some escape sequences in it
    json_rpc_internal_error(fout, request, "INTERNAL ERROR! \" ");
  } else {
    json_rpc_method_not_found_error(fout, request);
  }
  return 1;
}

size_t read_file(FILE *fp, char **buffer) {
  char *buf = NULL;
  size_t len;

  assert(fp != NULL);
  fseek(fp, 0, SEEK_END);
  len = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if (buffer != NULL) {
    buf = malloc(len+1);
    assert(buf != NULL);

    size_t read = fread(buf, 1, len, fp);
    assert(read > 0);
    buf[len] = 0;
    *buffer = buf;
  }
  return len;
}

void check_request(const char *name, const char *request, const char *response) {
  FILE *fin = tmpfile();
  FILE *fout = tmpfile();

  fwrite(request, strlen(request), 1, fin);
  fseek(fin, 0, SEEK_SET);

  int result = json_rpc_server_step(fin, fout, test_evaluate, NULL);
  assert(result == 1);

  char *buf;
  fseek(fout, 0, SEEK_SET);
  size_t buf_size = read_file(fout, &buf);
  assert(buf != NULL);

  if (strcmp(buf, response)) {
    fprintf(stderr, "%s failed! details:\nEXPECTED: %s\nACTUAL: %s\n", name, response, buf);
    assert(0);
  }
  free(buf);

  fclose(fin);
  fclose(fout);
}

int main(int argc, char **argv) {

  check_request("Empty request",
                "Content-Length: 2\r\n\r\n{}",
                "{\"jsonrpc\": \"2.0\", \"error\": \
{\"code\": -32600, \"message\": \"Invalid request\"}}");
  
  check_request("Malformed request",
                "Content-Length: 5\r\n\r\n{\"foo",
                "{\"jsonrpc\": \"2.0\", \"error\": \
{\"code\": -32700, \"message\": \"Parse error\", \"data\": \
\"Error parsing JSON: 6(line=1, offs=6): colon separating name/value pair was missing\"}}");

  check_request("Unknown method",
                "Content-Length: 61\r\n\r\n\
{\"jsonrpc\": \"2.0\", \"method\": \"sum\", \"params\": [1,2], \"id\": 1}",
                "{\"jsonrpc\": \"2.0\", \"error\": \
{\"code\": -32601, \"message\": \"Method not found\", \"data\": \"sum\"}, \"id\": 1}");

  check_request("Internal error",
                "Content-Length: 49\r\n\r\n\
{\"jsonrpc\": \"2.0\", \"method\": \"interror\", \"id\": 1}",
                "{\"jsonrpc\": \"2.0\", \"error\": \
{\"code\": -32603, \"message\": \"Internal error\", \"data\": \"INTERNAL ERROR! Sorry, details unavailable.\\nPlease try again later\"}, \"id\": 1}");

  check_request("Escaping of errors",
                "Content-Length: 50\r\n\r\n\
{\"jsonrpc\": \"2.0\", \"method\": \"interror2\", \"id\": 1}",
                "{\"jsonrpc\": \"2.0\", \"error\": \
{\"code\": -32603, \"message\": \"Internal error\", \"data\": \"INTERNAL ERROR! \\\" \"}, \"id\": 1}");
  

  check_request("Invalid parameters",
                "Content-Length: 62\r\n\r\n\
{\"jsonrpc\": \"2.0\", \"method\": \"sum1\", \"params\": [3,2], \"id\": 1}",
                "{\"jsonrpc\": \"2.0\", \"error\": \
{\"code\": -32602, \"message\": \"Invalid params\", \"data\": \"there must be 2 numbers in an array, the first number being 1\"}, \"id\": 1}");

  check_request("Succesful call",
                "Content-Length: 64\r\n\r\n\
{\"jsonrpc\": \"2.0\", \"method\": \"sum1\", \"params\": [1,4], \"id\": \"a\"}",
                "{\"jsonrpc\": \"2.0\", \"result\": 5.000000, \"id\": \"a\"}");

}
