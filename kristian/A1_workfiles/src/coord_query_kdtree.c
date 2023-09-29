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
#define DIMENSIONS 2 //dimensions for kdtree
#define RANDOM_POINTS_SIZE 10

struct query {
  double lon;
  double lat;
  int axis; // 0 = longitude, 1 = latitude
};

struct point {
  double lon;
  double lat;
};

struct node {
  //int isNull; // 0 = FALSE; 1 = TRUE
  struct point point;
  int axis; // 0 (FALSE) = longitude, 1 (TRUE) = latitude. The x-axis or the y-axis aka. the lon value or lat value.
  struct node* left;
  struct node* right;
  const struct record* record;
};

struct kdtree_data {
  struct node* kdtree;
  int n;
};

// radius
double eucl_dist(double lon, double lat, double x, double y) {
  double sum = pow((x - lon), 2) + pow((y - lat), 2);
  return sqrt(sum);
}

// difference in axis
double axis_diff(struct node node, struct query query) {
  double node_axis = node.axis ? node.point.lat : node.point.lon;
  double query_axis = query.axis ? query.lat : query.lon;
  return node_axis - query_axis;
}

// Compares longitude values. Used in qsort() in find_median()
// ref: https://en.wikipedia.org/wiki/Qsort
int compare_double(const void* a, const void* b) {
  double x = ((struct record*)a)->lon;
  double y = ((struct record*)b)->lon;
  if (x < y) {
    return -1;
  } else if (x > y) {
    return 1;
  } else {
    return 0;
  }
}

// select median by axis from points
// e.g if axis is 1, then look at latitude coordinates. Select the median latitude among a randomly selected set of coordinates. 
// points = array of original records
struct record* find_median(struct record* rs, int axis, int n) {
  srand(time(0));
  int median = (RANDOM_POINTS_SIZE / 2) - 1;
  struct record* rs_arr = malloc(sizeof(struct record) * RANDOM_POINTS_SIZE);
  for (size_t i = 0; i < RANDOM_POINTS_SIZE; i++) {
    rs_arr[i] = rs[rand() % n];
  }
  qsort(rs_arr, RANDOM_POINTS_SIZE, sizeof(struct record), compare_double);
  //free(rs_arr);
  return &rs_arr[median];
}

// return array* of records before median point
struct record* points_before_median(struct record* rs, struct node* median) {
  if (!rs) {
    return (void*) 0;
  }
  return rs;
}

// return array* of records after median point
struct record* points_after_median(struct record* rs, struct node* median) {
  return rs;
}

// take the array of records and the starting depth (0) as input
struct node* kdtree(struct record* rs, int depth, int n) {
  if (!rs) {
    return (void*) 0;
  }

  //struct node* new_node = malloc(sizeof(struct node));
  struct node* new_node;
  int axis = depth % DIMENSIONS; // return 1 or 0

  new_node->record = find_median(rs, axis, n); // select median by axis from points
  new_node->point.lon = new_node->record->lon;
  new_node->point.lat = new_node->record->lat;
  new_node->axis = axis;

  //new_node->left = kdtree(points_before_median(rs, new_node), depth + 1, n);
  //new_node->right = kdtree(points_after_median(rs, new_node), depth + 1, n);
  
  printf("kdtree\n");
  return new_node;
  
}

struct record* lookup(struct node closest_node, struct query query, struct node node) {
  // if ((void*) 0) {
  //   return;
  // } else if (eucl_dist(node.point, query_point) < eucl_dist(closest_node.point, query_point)) {
  //   // closest_node = node;
  // }
  // double diff = axis_diff(node, query);
  // double radius = eucl_dist(node, closest_node);
  // if (diff >= 0 || radius > abs(diff)) {
  //   lookup(closest_node, query, node.left);
  // }
  // if (diff <= 0 || radius > abs(diff)) {
  //   lookup(closest_node, query, node.right);
  // }
  assert(0);
}

struct kdtree_data* mk_kdtree(struct record* rs, int n) {
  struct kdtree_data* data_out = malloc(sizeof(struct kdtree_data));
  data_out->kdtree = kdtree(rs, 0, n);
  data_out->n = n;
  printf("mk_kdtree\n");
  return data_out;
  //TO DO
}

void free_kdtree(struct kdtree_data* data) {
  //free(data->kdtree);
  //free(data);
  printf("free_kdtree\n");
  assert(0);
}

const struct record* lookup_kdtree(struct kdtree_data *data, double lon, double lat) {
  printf("lookup_kdtree\n");
  assert(0);
  //TO DO
}

int main(int argc, char** argv) {
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_kdtree,
                          (free_index_fn)free_kdtree,
                          (lookup_fn)lookup_kdtree);
}
