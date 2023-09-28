#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include "record.h"
#include "id_query.h"

struct index_record {
  int64_t osm_id;
  const struct record* record;
};

struct indexed_data {
  struct index_record* irs;
  int n;
};

int compfunc(const void* a, const void* b) {
  return ((((struct index_record*)a)->osm_id) - (((struct index_record*)b)->osm_id));
}

struct indexed_data* mk_indexed(struct record* rs, int n) {

  struct index_record* arr = malloc(sizeof(struct index_record) * n);
  for (int i = 0; i < n; i++) {
    arr[i].osm_id = rs[i].osm_id;
    arr[i].record = &rs[i];
  };

  qsort(arr, n, sizeof(struct index_record), compfunc);
  
  // printf("%d elements:\n", n);
  // for (int i = 0; i < n ; i++) {
  //   printf("%d - %ld\n", i, arr[i].osm_id);
  // }
  
  struct indexed_data* data_out = malloc(sizeof(struct indexed_data));
  data_out->irs = arr;
  data_out->n = n;
  return data_out;
}

void free_indexed(struct indexed_data* data) {
  free(data->irs);
  free(data);
}

const struct record* lookup_indexed(struct indexed_data *data, int64_t needle) {
  int min = 0;
  int max = data->n - 1;

  while ((max - min) > 0) {
    int i = (max + min) / 2;
    //printf("min = %d, i = %d, max = %d\n", min, i, max);
    int64_t id = data->irs[i].osm_id;
    if (id > needle) {
      max = i - 1;
      //printf("Smaller than %ld\n", id);
    } else if (id < needle) {
      min = i + 1;
      //printf("Bigger than %ld\n", id);
    } else {
      return data->irs[i].record;
    } 
    
    if (max == min) {
      int i = max;
      int64_t id = data->irs[i].osm_id;
      if (id == needle) {
        return data->irs[i].record;
      }
    }
  } 
  return NULL;
}

int main(int argc, char** argv) {
  return id_query_loop(argc, argv,
                    (mk_index_fn)mk_indexed,
                    (free_index_fn)free_indexed,
                    (lookup_fn)lookup_indexed);
}