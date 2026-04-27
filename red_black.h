/*
 * red_black.h
 *
 * Public API for a generic Red-Black Tree implementation in C.
 */

#ifndef RED_BLACK_H
#define RED_BLACK_H

#include <stddef.h>

typedef enum { RED, BLACK } Color;

typedef enum {
    RB_CHAR,
    RB_UCHAR,
    RB_SHORT,
    RB_USHORT,
    RB_INT,
    RB_UINT,
    RB_LONG,
    RB_ULONG,
    RB_LLONG,
    RB_ULLONG,
    RB_FLOAT,
    RB_DOUBLE,
    RB_LDOUBLE,
    RB_STRING,
    RB_CUSTOM
} RBType;

typedef int  (*rb_cmp_fn)(const void *a, const void *b);
typedef void (*rb_print_fn)(const void *data);
typedef void (*rb_free_fn)(void *data);
typedef void *(*rb_copy_fn)(const void *data);

typedef struct RBNode {
    void          *data;
    Color          color;
    struct RBNode *left;
    struct RBNode *right;
    struct RBNode *parent;
} RBNode;

typedef struct RBTree {
    RBNode     *root;
    RBNode     *nil;        /* sentinel */
    size_t      size;
    size_t      elem_size;  /* sizeof(element) for fixed-size types */
    RBType      type;
    rb_cmp_fn   compare;
    rb_print_fn print;
    rb_free_fn  free_elem;
    rb_copy_fn  copy_elem;
} RBTree;

typedef struct StrResultSet {
    const char **strings;
    size_t       count;
    size_t       capacity;
} StrResultSet;

RBTree *rb_create(RBType type);
RBTree *rb_create_custom(size_t elem_size,
                         rb_cmp_fn cmp,
                         rb_print_fn print,
                         rb_free_fn free_fn,
                         rb_copy_fn copy_fn);
void rb_destroy(RBTree *t);

int rb_insert(RBTree *t, const void *data);
int rb_delete(RBTree *t, const void *data);

const void *rb_search(RBTree *t, const void *data);
int rb_contains(RBTree *t, const void *data);

const void *rb_minimum(RBTree *t);
const void *rb_maximum(RBTree *t);
const void *rb_successor(RBTree *t, const void *data);
const void *rb_predecessor(RBTree *t, const void *data);

void rb_inorder(RBTree *t, void (*visit)(const void *));
void rb_preorder(RBTree *t, void (*visit)(const void *));
void rb_postorder(RBTree *t, void (*visit)(const void *));

void rb_print(RBTree *t);
void rb_print_tree(RBTree *t);

size_t rb_size(RBTree *t);
int rb_empty(RBTree *t);
int rb_verify(RBTree *t);

StrResultSet *rb_find_substring(RBTree *t, const char *pattern);
StrResultSet *rb_find_prefix(RBTree *t, const char *prefix);
StrResultSet *rb_find_suffix(RBTree *t, const char *suffix);
void str_result_free(StrResultSet *rs);

int rb_kmp_search_in_node(RBTree *t, const char *key, const char *pattern);
int *rb_kmp_search_all_in_node(RBTree *t, const char *key, const char *pattern, int *count);

#define RB_INSERT_VAL(tree, type, val)     do { type _tmp = (val); rb_insert((tree), &_tmp); } while (0)

#define RB_DELETE_VAL(tree, type, val)     do { type _tmp = (val); rb_delete((tree), &_tmp); } while (0)

#endif /* RED_BLACK_H */
