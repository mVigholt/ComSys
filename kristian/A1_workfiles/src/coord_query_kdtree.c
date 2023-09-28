#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include "record.h"
#include "coord_query.h"

#include "math.h"


struct kdtree_data {
  struct record* kdtree;
  int n;
};

double eucl_dist(double lon, double lat, double x, double y) {
  double sum = pow((x - lon), 2) + pow((y - lat), 2);
  return sqrt(sum);
}

struct kdtree_data* mk_kdtree(struct record* rs, int n) {
  assert(0);
  //TO DO
}

void free_kdtree(struct kdtree_data* data) {
  free(data->kdtree);
  free(data);
}

const struct record* lookup_kdtree(struct naive_data *data, double lon, double lat) {
  assert(0);
  //TO DO
}

int main(int argc, char** argv) {
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_kdtree,
                          (free_index_fn)free_kdtree,
                          (lookup_fn)lookup_kdtree);
}
