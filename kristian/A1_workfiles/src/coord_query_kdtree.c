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
};

struct node {
  int axis; // 0 (FALSE) = longitude (x), 1 (TRUE) = latitude (y). The x-axis or the y-axis aka. the lon value or lat value.
  struct node* left;
  struct node* right;
  struct record* record;
};

struct nodes {
  struct node* n_index;
  int n;
};

// Compares point values by axis. Used in qsort() in find_median()
// ref: https://en.wikipedia.org/wiki/Qsort
int compare_double(void* axis, const void* a, const void* b) {
  //printf("compare_double\n");
  double x = *((int*) axis) ? ((struct node*)a)->record->lat : ((struct node*)a)->record->lon;
  double y = *((int*) axis) ? ((struct node*)b)->record->lat : ((struct node*)b)->record->lon;

  if (x < y) return -1;
  else if (x > y) return 1;
  else return 0;
}

// Sorting nodes by axis.
struct node* sort_nodes_by_axis(struct nodes* nodes, int* axis_ptr) {
  //printf("sort_nodes_by_axis\n");
  int n = nodes->n;
  assert(n > 1);
  qsort_r(nodes->n_index, n, sizeof(struct node), axis_ptr, compare_double);
  return nodes->n_index;
}

// take median of sorted array (by points by axis) and the median instead of a random array 
// select median by axis from points
// e.g if axis is 1, then look at latitude coordinates. Select the median latitude among a randomly selected set of coordinates. 
// points = array of original records
struct record* find_median(struct nodes* nodes, int* axis_ptr) {
  //printf("find_median\n");
  if (!nodes->n) return (void*) 0;
  if (nodes->n == 1) return nodes->n_index[0].record;

  int size = nodes->n;
  int median = (size - (size / 2) - 1);
  nodes->n_index = sort_nodes_by_axis(nodes, axis_ptr);

  return nodes->n_index[median].record;
}

// return array* of nodes before median point
struct nodes* points_before_median(struct nodes* nodes, struct node* median) {
  //printf("points_before_median\n");
  if (!nodes->n) return (void*) 0;
  //if (nodes->n == 1) return (void*) 0;
  
  //assert(nodes->n != 1);
  int axis = median->axis;
  double median_point = axis ? median->record->lat : median->record->lon;
  int64_t count = 0;
  int64_t capacity = 100;
  int n = nodes->n;
  struct node* arr = malloc(sizeof(struct node) * capacity);
  
  for (int i = 0; i < n; i++) {
    double rs_point = axis ? nodes->n_index[i].record->lat : nodes->n_index[i].record->lon;
    if (count == capacity) {
      capacity *= 2;
      arr = realloc(arr, sizeof(struct node) * capacity);
    }
    if (rs_point < median_point) {
      arr[count].record = nodes->n_index[i].record;
      count++;
    }
  }

  struct nodes* nodes_out = malloc(sizeof(struct nodes));
  nodes_out->n_index = arr ? arr : (void*) 0;
  nodes_out->n = count;

  return nodes_out;
}

// return array* of records after median point
struct nodes* points_after_median(struct nodes* nodes, struct node* median) {
  //printf("points_after_median\n");
  if (!nodes->n) return (void*) 0;
  //if (nodes->n == 1) return (void*) 0;

  //assert(nodes->n != 1);
  int axis = median->axis;
  double median_point = axis ? median->record->lat : median->record->lon;
  int count = 0;
  int capacity = 100;
  int n = nodes->n;
  struct node* arr_a = malloc(sizeof(struct node) * capacity);
  
  for (int i = 0; i < n; i++) {
    double rs_point = axis ? nodes->n_index[i].record->lat : nodes->n_index[i].record->lon;
    if (count == capacity) {
      capacity *= 2;
      arr_a = realloc(arr_a, sizeof(struct node) * capacity);
    }
    if (rs_point > median_point) {
      arr_a[count].record = nodes->n_index[i].record;
      count++;
    }
  }

  struct nodes* nodes_out = malloc(sizeof(struct nodes));
  nodes_out->n_index = arr_a ? arr_a : (void*) 0;
  nodes_out->n = count;

  return nodes_out;
}

// take the array of records and the starting depth (0) as input
struct node* kdtree(struct nodes* nodes, int depth) {
  //printf("kdtree\n");
  if (!nodes->n) return (void*) 0;
  
  int axis = depth % DIMENSIONS; // return 1 or 0

  struct node* new_node = malloc(sizeof(struct node));
  new_node->axis = axis;
  new_node->record = find_median(nodes, &axis); // select median by axis from points
  new_node->left = (void*) 0;
  new_node->right = (void*) 0;
  
  new_node->left = kdtree(points_before_median(nodes, new_node), depth + 1);
  new_node->right = kdtree(points_after_median(nodes, new_node), depth + 1);
  
  return new_node;
}

struct nodes* rs_to_nodes(struct record* rs, int n) {
  //printf("rs_to_nodes\n");
  struct node* node_arr = malloc(sizeof(struct node) * n);
  for (int i = 0; i < n; i++) {
    node_arr[i].record = &rs[i];
  }
  struct nodes* nodes_out = malloc(sizeof(struct nodes));
  nodes_out->n_index = node_arr;
  nodes_out->n = n;
  return nodes_out;
}

// difference in axis
double axis_diff(struct node* node, struct query* query) {
  printf("axis_diff\n");
  double node_axis = node->axis ? node->record->lat : node->record->lon;
  double query_axis = node->axis ? query->lat : query->lon;
  return node_axis - query_axis;
}

// radius
double eucl_dist(struct node* node, struct query* query) {
  printf("eucl_dist\n");
  double x0 = query->lon;
  double y0 = query->lat;
  double x1 = node->record->lon;
  double y1 = node->record->lat;
  double sum = pow((x0 - x1), 2) + pow((y0 - y1), 2);
  return sqrt(sum);
}

// recursive lookup
struct node* lookup(struct node* closest, struct query* query, struct node* node) {
  printf("lookup\n");

  printf("node_ptr: %p\n", node);
  //assert(node);

  printf("loop\n");
  if (/* (void*) node->left == 0 || (void*) node->right == 0 */ (void*) node == 0) {
    printf("return\n");
    printf("closest_ptr: %p\n", closest);
    return closest;
  } else if (eucl_dist(node, query) < eucl_dist(closest, query)) {
    printf("update closest\n");
    closest->record = node->record;
  }

  printf("node: %s\n", node->record->name);
  printf("closest: %s\n", closest->record->name);
  //assert(node->right);
  //assert(node->right);
  printf("left: %p\n", node->left);
  printf("right: %p\n", node->right);
  

  double diff = axis_diff(node, query);
  double radius = eucl_dist(closest, query);
  printf("diff: %f\n", diff);
  printf("radius: %f\n", radius);
  
  assert(diff);
  assert(radius);

  printf("recursion\n");
  if (diff >= 0 || radius > fabs(diff)) {
    printf("lookup left\n");
    lookup(closest, query, node->left);
  }
  if (diff <= 0 || radius > fabs(diff)) {
    printf("lookup right\n");
    lookup(closest, query, node->right);
  }
  return closest;
}

struct nodes* mk_kdtree(struct record* rs, int n) {
  //printf("mk_kdtree\n");
  struct nodes* data_out = malloc(sizeof(struct nodes));
  data_out->n_index = kdtree(rs_to_nodes(rs, n), 0);
  data_out->n = n;
  
  return data_out;
}

void free_node_rec(struct node* node) {
  if ((void*) node == 0) {
    return;
  }
  free_node_rec(node->left);
  free_node_rec(node->right);
  free(node);
}

void free_kdtree(struct nodes* nodes) {
  printf("free_kdtree\n");
  free_node_rec(&nodes->n_index[0]);
  free(nodes);
}

const struct record* lookup_kdtree(struct nodes* nodes, double lon, double lat) {
  printf("lookup_kdtree\n");

  struct query* query = malloc(sizeof(struct query));
  query->lon = lon;
  query->lat = lat;
  assert(query != NULL);

  struct node* closest = malloc(sizeof(struct node));
  closest->record = nodes->n_index[0].record;
  closest = lookup(closest, query, &nodes->n_index[0]);

  printf("lookup_kdtree node_out: %p\n", closest);

  free(query);
  return closest->record;
}

int main(int argc, char** argv) {
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_kdtree,
                          (free_index_fn)free_kdtree,
                          (lookup_fn)lookup_kdtree);
}
