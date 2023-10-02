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
  int axis; // 0 = longitude, 1 = latitude
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
double axis_diff(struct node* node, struct query query) {
  double node_axis = node->axis ? node->record->lat : node->record->lon;
  double query_axis = query.axis ? query.lat : query.lon;
  return node_axis - query_axis;
}

// Compares longitude values. Used in qsort() in find_median()
// ref: https://en.wikipedia.org/wiki/Qsort
int compare_double(void* axis, const void* a, const void* b) {
  printf("compare_double\n");
  double x = *((int*) axis) ? ((struct node*)a)->record->lat : ((struct node*)a)->record->lon;
  printf("compare_double\n");
  double y = *((int*) axis) ? ((struct node*)b)->record->lat : ((struct node*)b)->record->lon;
  if (x < y) {
    return -1;
  } else if (x > y) {
    return 1;
  } else {
    return 0;
  }
}

// take median of sorted array (by points by axis) and the median instead of a random array 
// select median by axis from points
// e.g if axis is 1, then look at latitude coordinates. Select the median latitude among a randomly selected set of coordinates. 
// points = array of original records
struct record* find_median(struct nodes* nodes, int* axis_ptr) {
  printf("find_median\n");

  if (nodes->n == 1) {
    return (void*) 0;
  }

  srand(time(0));
  int size = nodes->n;
  int median = (size / 2);
  struct node* arr = malloc(sizeof(struct node) * size);
  for (int i = 0; i < size; i++) {
    arr[i].record = nodes->n_index[rand()%size].record;
  }
  
  qsort_r(arr, size, sizeof(struct node), axis_ptr, compare_double);

  struct record* out = arr[median].record;
  free(arr);
  return out;
}

// return array* of nodes before median point
struct nodes* points_before_median(struct nodes* nodes, struct node* median) {
  printf("points_before_median\n");
  if (!nodes->n_index) {
    // CHECK RETURN TYPE HERE!? CHeck all void return type values throught the tree.
    printf("void");
    return (void*) 0;
  }

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
      count++;
      arr[i].record = nodes->n_index[i].record;
    }
  }

  struct nodes* nodes_out = malloc(sizeof(struct nodes));
  nodes_out->n_index = arr;
  nodes_out->n = count;
  // for (int i = 0; i < nodes_out->n; i++) {
  //   printf("ID: %lli\n", nodes_out->n_index[i].record->osm_id);
  // };
  printf("ID: %p\n", nodes_out->n_index);
  printf("pbm: count = %i\n", nodes_out->n);
  return nodes_out;
}

// return array* of records after median point
struct record* points_after_median(struct record* rs, struct node* median) {
  assert(0);
}

// take the array of records and the starting depth (0) as input
struct node* kdtree(struct nodes* nodes, int depth) {
  printf("kdtree\n");
  if (!nodes) {
    return (void*) 0;
  }
  
  int axis = depth % DIMENSIONS; // return 1 or 0
  struct node* new_node = malloc(sizeof(struct node));
  new_node->axis = axis;
  new_node->record = find_median(nodes, &axis); // select median by axis from points
  
  new_node->left = kdtree(points_before_median(nodes, new_node), depth + 1);
  //new_node->right = kdtree(points_after_median(rs, new_node), depth + 1, n);
  
  return new_node;
}

struct nodes* rs_to_nodes(struct record* rs, int n) {
  printf("rs_to_nodes\n");
  struct node* node_arr = malloc(sizeof(struct node) * n);
  for (int i = 0; i < n; i++) {
    node_arr[i].record = &rs[i];
  }
  struct nodes* nodes_out = malloc(sizeof(struct nodes));
  nodes_out->n_index = node_arr;
  nodes_out->n = n;
  return nodes_out;
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
  printf("mk_kdtree\n");
  struct kdtree_data* data_out = malloc(sizeof(struct kdtree_data));
  data_out->kdtree = kdtree(rs_to_nodes(rs, n), 0);
  data_out->n = n;
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
