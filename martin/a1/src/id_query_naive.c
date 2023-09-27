#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include "record.h"
#include "id_query.h"

struct naive_data {
  struct record *rs;
  int n;
};

struct naive_data* mk_naive(struct record* rs, int n) {
  // TODO
  struct naive_data *data_out = malloc(sizeof(data_out));
  data_out->rs = rs;
  data_out->n = n;
  return data_out;
  //assert(0);

}

void free_naive(struct naive_data* data) {
  // TODO
  free(data->rs);
  //assert(0);
}

const struct record* lookup_naive(struct naive_data *data, int64_t needle) {
  // TODO
  int size = data->n;
  for (int i = 0; i < size; i++) {
    int64_t id = data->rs[i].osm_id;
    if (id == needle) {
      return &data->rs[i];
    }
  }
  //assert(0);
  return NULL;
}

int main(int argc, char** argv) {
  return id_query_loop(argc, argv,
                    (mk_index_fn)mk_naive,
                    (free_index_fn)free_naive,
                    (lookup_fn)lookup_naive);
}