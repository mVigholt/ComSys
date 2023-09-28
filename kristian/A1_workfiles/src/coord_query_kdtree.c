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

struct query {
  double lon;
  double lat;
  int axis // 0 = longitude, 1 = latitude
};

struct point {
  double lon;
  double lat;
};

struct node {
  int isNull; // 0 = FALSE; 1 = TRUE
  struct point point;
  int axis; // 0 = longitude, 1 = latitude
  struct node* left;
  struct node* right;
  struct record* r;
};

struct kdtree_data {
  struct record* kdtree;
  int n;
};

double eucl_dist(double lon, double lat, double x, double y) {
  double sum = pow((x - lon), 2) + pow((y - lat), 2);
  return sqrt(sum);
}

double axis_diff(struct node node, struct query query) {
  double node_axis = node.axis ? node.point.lat : node.point.lon;
  double query_axis = query.axis ? query.lat : query.lon;
  return node_axis - query_axis;
};

double radius(struct node node, struct node closest) {
  // calculate radius
  return 0.02;
}

struct node* kdtree(struct record* rs, int depth) {
  struct node* new_node = malloc(sizeof(struct node));
  
  int axis = depth % DIMENSIONS;
  // select median by axis from points
  new_node->point = median;
  new_node->axis = axis;
  new_node->left = kdtree(points_before_median, depth + 1);
  new_node->right = kdtree(points_after_median, depth + 1);
  
  return new_node;
}

struct record* lookup(struct node closest_node, struct query query, struct node node) {
  if (node.isNull) {
    return;
  } else if (eucl_dist(node.point, query_point) < eucl_dist(closest_node.point, query_point)) {
    // closest_node = node;
  }
  double diff = axis_diff(node, query);
  double radius = radius(node, closest_node);
  if (diff >= 0 || radius > abs(diff)) {
    lookup(closest_node, query, node.left);
  }
  if (diff <= 0 || radius > abs(diff)) {
    lookup(closest_node, query, node.right);
  }
}

struct kdtree_data* mk_kdtree(struct record* rs, int n) {
  struct kdtree_data* data_out = malloc(sizeof(struct kdtree_data));
  data_out->kdtree = kdtree(rs, 0);
  data_out->n = n;
  return data_out;
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
