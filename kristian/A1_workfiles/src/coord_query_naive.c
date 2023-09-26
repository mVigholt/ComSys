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

struct naive_data {
  struct record *rs;
  int n;
};

struct naive_data* mk_naive(struct record* rs, int n) {
  //assert(0);
  // TODO
  int record_size = sizeof(struct record);
  int arr_size = n;
  struct naive_data *data_out = malloc(arr_size * record_size);
  data_out->rs = rs;
  data_out->n = n;
  return data_out;
}

void free_naive(struct naive_data* data) {
  //assert(0);
  // TODO
  free(data->rs);
}

double eucl_dist(double lon, double lat, double x, double y) {
  double sum = pow((x - lon), 2) + pow((y - lat), 2);
  return sqrt(sum);
}

const struct record* lookup_naive(struct naive_data *data, double lon, double lat) {
  //assert(0);
  // TODO
  int size = data->n;
  double eucl_dist_to_point;
  struct record candidate; //pointer to record

  for (int i = 0; i < size; i++) {
    struct record record = data->rs[i];
    double r_lon = record.lon;
    double r_lat = record.lat;

    // calc eucl dist to point
    double dist = eucl_dist(r_lon, r_lat, lon, lat);
    
    if (!i) {
      // first record [0], init variables
      eucl_dist_to_point = dist;
      candidate = record;
    } else if (dist < eucl_dist_to_point) { //saving smallest distance
      // save to candidate and eucl_dist_to_point
      eucl_dist_to_point = dist;
      candidate = record;
    }
    // else go to next
  }
  return &candidate;
}

int main(int argc, char** argv) {
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_naive,
                          (free_index_fn)free_naive,
                          (lookup_fn)lookup_naive);
}
