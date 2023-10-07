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
  double x = *((int*) axis) ? ((struct node*)a)->record->lat : ((struct node*)a)->record->lon;
  double y = *((int*) axis) ? ((struct node*)b)->record->lat : ((struct node*)b)->record->lon;
  if (x < y) return -1;
  else if (x > y) return 1;
  else return 0;
}

// Sorting nodes by axis.
struct node* sort_nodes_by_axis(struct nodes* nodes, int* axis_ptr) {
  int n = nodes->n;
  assert(n > 1);
  qsort_r(nodes->n_index, n, sizeof(struct node), axis_ptr, compare_double);
  return nodes->n_index;
}

// select median by axis from points, of a sorted array
// e.g if axis is 1, then look at latitude coordinates.
struct record* find_median(struct nodes* nodes, int* axis_ptr) {
  if (!nodes->n) return (void*) 0;
  if (nodes->n == 1) return nodes->n_index[0].record;

  int size = nodes->n;
  int median = (size - (size / 2) - 1);
  nodes->n_index = sort_nodes_by_axis(nodes, axis_ptr);

  return nodes->n_index[median].record;
}

// return array of nodes before median point
struct nodes* points_before_median(struct nodes* nodes, struct node* median) {
  if (!nodes->n) return (void*) 0;
  
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

// return array of nodes after median point
struct nodes* points_after_median(struct nodes* nodes, struct node* median) {
  if (!nodes->n) return (void*) 0;

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
  if (!nodes->n) return (void*) 0;
  
  int axis = depth % DIMENSIONS; // return 1 or 0

  struct node* new_node = malloc(sizeof(struct node));
  new_node->axis = axis;
  new_node->record = find_median(nodes, &axis); // select median by axis from points
  new_node->left = (void*) 0;
  new_node->right = (void*) 0;
  
  new_node->left = kdtree(points_before_median(nodes, new_node), depth + 1);
  new_node->right = kdtree(points_after_median(nodes, new_node), depth + 1);
  
  free(nodes->n_index);
  free(nodes);

  return new_node;
}

// converts rs input array to node type array
struct nodes* rs_to_nodes(struct record* rs, int n) {
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
  double node_axis = node->axis ? node->record->lat : node->record->lon;
  double query_axis = node->axis ? query->lat : query->lon;
  return node_axis - query_axis;
}

// radius
double eucl_dist(struct node* node, struct query* query) {
  double x0 = query->lon;
  double y0 = query->lat;
  double x1 = node->record->lon;
  double y1 = node->record->lat;
  double sum = pow((x0 - x1), 2) + pow((y0 - y1), 2);
  return sqrt(sum);
}

// recursive lookup
struct node* lookup(struct node* closest, struct query* query, struct node* node) {
  if ((void*) node == 0) {
    return closest;
  } else if (eucl_dist(node, query) < eucl_dist(closest, query)) {
    closest->record = node->record;
  }
  
  double diff = axis_diff(node, query);
  double radius = eucl_dist(closest, query);

  if (diff >= 0 || radius > fabs(diff)) {
    lookup(closest, query, node->left);
  }
  if (diff <= 0 || radius > fabs(diff)) {
    lookup(closest, query, node->right);
  }
  return closest;
}

// recursive freeing of tree structure
void deallocate_tree(struct node* node) {
  if ((void*) node == 0) {
    return;
  }
  deallocate_tree(node->left);
  deallocate_tree(node->right);
  free(node);
}


// Main functions

struct nodes* mk_kdtree(struct record* rs, int n) {
  struct nodes* data_out = malloc(sizeof(struct nodes));
  data_out->n_index = kdtree(rs_to_nodes(rs, n), 0);
  data_out->n = n;
  return data_out;
}

void free_kdtree(struct nodes* nodes) {
  deallocate_tree(nodes->n_index);
  free(nodes);
}

const struct record* lookup_kdtree(struct nodes* nodes, double lon, double lat) {
  struct query* query = malloc(sizeof(struct query));
  query->lon = lon;
  query->lat = lat;

  struct node* closest = malloc(sizeof(struct node));
  closest->record = nodes->n_index[0].record;
  closest = lookup(closest, query, &nodes->n_index[0]);

  free(query);
  return closest->record;
}

int main(int argc, char** argv) {
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_kdtree,
                          (free_index_fn)free_kdtree,
                          (lookup_fn)lookup_kdtree);
}
