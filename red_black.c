/*
 * red_black.c
 *
 * A generic Red-Black Tree implementation in C that supports all basic C types
 * (char, short, int, long, long long, unsigned variants, float, double,
 *  long double, and string) with unbounded size.
 *
 * The tree stores copies of inserted data via void* and uses per-type
 * comparator / printer / destructor function pointers, making it fully
 * type-agnostic at the tree level.
 *
 * Compile:  gcc -Wall -Wextra -o rbtree main.c red_black.c -lm
 */

#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "red_black.h"

/* ================================================================== */
/*  Built-in comparators for every basic C type                       */
/* ================================================================== */

#define DEFINE_CMP(NAME, TYPE)                                                 \
  static int cmp_##NAME(const void *a, const void *b)                          \
  {                                                                            \
    TYPE va = *(const TYPE *)a;                                                \
    TYPE vb = *(const TYPE *)b;                                                \
    return (va > vb) - (va < vb);                                              \
  }

DEFINE_CMP(char, char)
DEFINE_CMP(uchar, unsigned char)
DEFINE_CMP(short, short)
DEFINE_CMP(ushort, unsigned short)
DEFINE_CMP(int, int)
DEFINE_CMP(uint, unsigned int)
DEFINE_CMP(long, long)
DEFINE_CMP(ulong, unsigned long)
DEFINE_CMP(llong, long long)
DEFINE_CMP(ullong, unsigned long long)
DEFINE_CMP(float, float)
DEFINE_CMP(double, double)
DEFINE_CMP(ldouble, long double)

static int cmp_string(const void *a, const void *b)
{
  return strcmp(*(const char *const *)a, *(const char *const *)b);
}

/* ================================================================== */
/*  Built-in printers                                                 */
/* ================================================================== */

static void print_char(const void *d)
{
  printf("%c", *(const char *)d);
}
static void print_uchar(const void *d)
{
  printf("%u", *(const unsigned char *)d);
}
static void print_short(const void *d)
{
  printf("%hd", *(const short *)d);
}
static void print_ushort(const void *d)
{
  printf("%hu", *(const unsigned short *)d);
}
static void print_int(const void *d)
{
  printf("%d", *(const int *)d);
}
static void print_uint(const void *d)
{
  printf("%u", *(const unsigned int *)d);
}
static void print_long(const void *d)
{
  printf("%ld", *(const long *)d);
}
static void print_ulong(const void *d)
{
  printf("%lu", *(const unsigned long *)d);
}
static void print_llong(const void *d)
{
  printf("%lld", *(const long long *)d);
}
static void print_ullong(const void *d)
{
  printf("%llu", *(const unsigned long long *)d);
}
static void print_float(const void *d)
{
  printf("%g", *(const float *)d);
}
static void print_double(const void *d)
{
  printf("%g", *(const double *)d);
}
static void print_ldouble(const void *d)
{
  printf("%Lg", *(const long double *)d);
}
static void print_string(const void *d)
{
  printf("%s", *(const char *const *)d);
}

/* ================================================================== */
/*  Built-in copy / free helpers for the string type                  */
/* ================================================================== */

static void *copy_string(const void *d)
{
  const char *src = *(const char *const *)d;
  char *dup = strdup(src);
  if (!dup)
  {
    perror("strdup");
    exit(EXIT_FAILURE);
  }
  char **box = malloc(sizeof(char *));
  if (!box)
  {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  *box = dup;
  return box;
}

static void free_string(void *d)
{
  char **box = (char **)d;
  free(*box);
  free(box);
}

static void *copy_fixed(const void *d, size_t sz)
{
  void *p = malloc(sz);
  if (!p)
  {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  memcpy(p, d, sz);
  return p;
}

/* ================================================================== */
/*  Type-descriptor table (maps RBType -> callbacks + elem_size)      */
/* ================================================================== */

static const TypeInfo TYPE_TABLE[] = {
    /* RB_CHAR    */ {sizeof(char), cmp_char, print_char, NULL, NULL},
    /* RB_UCHAR   */
    {sizeof(unsigned char), cmp_uchar, print_uchar, NULL, NULL},
    /* RB_SHORT   */ {sizeof(short), cmp_short, print_short, NULL, NULL},
    /* RB_USHORT  */
    {sizeof(unsigned short), cmp_ushort, print_ushort, NULL, NULL},
    /* RB_INT     */ {sizeof(int), cmp_int, print_int, NULL, NULL},
    /* RB_UINT    */ {sizeof(unsigned int), cmp_uint, print_uint, NULL, NULL},
    /* RB_LONG    */ {sizeof(long), cmp_long, print_long, NULL, NULL},
    /* RB_ULONG   */
    {sizeof(unsigned long), cmp_ulong, print_ulong, NULL, NULL},
    /* RB_LLONG   */ {sizeof(long long), cmp_llong, print_llong, NULL, NULL},
    /* RB_ULLONG  */
    {sizeof(unsigned long long), cmp_ullong, print_ullong, NULL, NULL},
    /* RB_FLOAT   */ {sizeof(float), cmp_float, print_float, NULL, NULL},
    /* RB_DOUBLE  */ {sizeof(double), cmp_double, print_double, NULL, NULL},
    /* RB_LDOUBLE */
    {sizeof(long double), cmp_ldouble, print_ldouble, NULL, NULL},
    /* RB_STRING  */
    {sizeof(char *), cmp_string, print_string, free_string, copy_string},
};

/* ================================================================== */
/*  Tree creation / destruction                                       */
/* ================================================================== */

static RBNode *rb_alloc_node(RBTree *t)
{
  RBNode *n = malloc(sizeof(RBNode));
  if (!n)
  {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  n->left = n->right = n->parent = t->nil;
  n->color = RED;
  n->data = NULL;
  return n;
}

RBTree *rb_create(RBType type)
{
  if (type == RB_CUSTOM)
  {
    fprintf(stderr, "rb_create: use rb_create_custom() for RB_CUSTOM\n");
    return NULL;
  }

  RBTree *t = malloc(sizeof(RBTree));
  if (!t)
  {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  t->nil = malloc(sizeof(RBNode));
  if (!t->nil)
  {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  t->nil->color = BLACK;
  t->nil->left = t->nil->right = t->nil->parent = t->nil;
  t->nil->data = NULL;

  t->root = t->nil;
  t->size = 0;
  t->type = type;

  const TypeInfo *ti = &TYPE_TABLE[type];
  t->elem_size = ti->elem_size;
  t->compare = ti->cmp;
  t->print = ti->print;
  t->free_elem = ti->free_elem;
  t->copy_elem = ti->copy_elem;
  return t;
}

RBTree *rb_create_custom(size_t elem_size, rb_cmp_fn cmp, rb_print_fn print,
                         rb_free_fn free_fn, rb_copy_fn copy_fn)
{
  RBTree *t = malloc(sizeof(RBTree));
  if (!t)
  {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  t->nil = malloc(sizeof(RBNode));
  if (!t->nil)
  {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  t->nil->color = BLACK;
  t->nil->left = t->nil->right = t->nil->parent = t->nil;
  t->nil->data = NULL;

  t->root = t->nil;
  t->size = 0;
  t->type = RB_CUSTOM;
  t->elem_size = elem_size;
  t->compare = cmp;
  t->print = print;
  t->free_elem = free_fn;
  t->copy_elem = copy_fn;
  return t;
}

static void rb_free_node(RBTree *t, RBNode *n)
{
  if (n == t->nil)
    return;
  if (n->data)
  {
    if (t->free_elem)
      t->free_elem(n->data);
    else
      free(n->data);
  }
  free(n);
}

static void rb_free_subtree(RBTree *t, RBNode *n)
{
  if (n == t->nil)
    return;
  rb_free_subtree(t, n->left);
  rb_free_subtree(t, n->right);
  rb_free_node(t, n);
}

void rb_destroy(RBTree *t)
{
  if (!t)
    return;
  rb_free_subtree(t, t->root);
  free(t->nil);
  free(t);
}

/* ================================================================== */
/*  Data copy helper                                                  */
/* ================================================================== */

static void *rb_copy_data(RBTree *t, const void *data)
{
  if (t->copy_elem)
    return t->copy_elem(data);
  return copy_fixed(data, t->elem_size);
}

/* ================================================================== */
/*  Rotations                                                         */
/* ================================================================== */

static void rb_left_rotate(RBTree *t, RBNode *x)
{
  RBNode *y = x->right;
  x->right = y->left;
  if (y->left != t->nil)
    y->left->parent = x;
  y->parent = x->parent;
  if (x->parent == t->nil)
    t->root = y;
  else if (x == x->parent->left)
    x->parent->left = y;
  else
    x->parent->right = y;
  y->left = x;
  x->parent = y;
}

static void rb_right_rotate(RBTree *t, RBNode *y)
{
  RBNode *x = y->left;
  y->left = x->right;
  if (x->right != t->nil)
    x->right->parent = y;
  x->parent = y->parent;
  if (y->parent == t->nil)
    t->root = x;
  else if (y == y->parent->left)
    y->parent->left = x;
  else
    y->parent->right = x;
  x->right = y;
  y->parent = x;
}

/* ================================================================== */
/*  Insert                                                            */
/* ================================================================== */

static void rb_insert_fixup(RBTree *t, RBNode *z)
{
  while (z->parent->color == RED)
  {
    if (z->parent == z->parent->parent->left)
    {
      RBNode *y = z->parent->parent->right;
      if (y->color == RED)
      { /* case 1 */
        z->parent->color = BLACK;
        y->color = BLACK;
        z->parent->parent->color = RED;
        z = z->parent->parent;
      }
      else
      {
        if (z == z->parent->right)
        { /* case 2 */
          z = z->parent;
          rb_left_rotate(t, z);
        }
        z->parent->color = BLACK; /* case 3 */
        z->parent->parent->color = RED;
        rb_right_rotate(t, z->parent->parent);
      }
    }
    else
    { /* mirror */
      RBNode *y = z->parent->parent->left;
      if (y->color == RED)
      {
        z->parent->color = BLACK;
        y->color = BLACK;
        z->parent->parent->color = RED;
        z = z->parent->parent;
      }
      else
      {
        if (z == z->parent->left)
        {
          z = z->parent;
          rb_right_rotate(t, z);
        }
        z->parent->color = BLACK;
        z->parent->parent->color = RED;
        rb_left_rotate(t, z->parent->parent);
      }
    }
  }
  t->root->color = BLACK;
}

int rb_insert(RBTree *t, const void *data)
{
  RBNode *y = t->nil;
  RBNode *x = t->root;

  while (x != t->nil)
  {
    y = x;
    int cmp = t->compare(data, x->data);
    if (cmp == 0)
      return 0; /* duplicate — not inserted */
    x = (cmp < 0) ? x->left : x->right;
  }

  RBNode *z = rb_alloc_node(t);
  z->data = rb_copy_data(t, data);
  z->parent = y;

  if (y == t->nil)
    t->root = z;
  else if (t->compare(z->data, y->data) < 0)
    y->left = z;
  else
    y->right = z;

  z->left = t->nil;
  z->right = t->nil;
  z->color = RED;

  rb_insert_fixup(t, z);
  t->size++;
  return 1;
}

/* ================================================================== */
/*  Transplant & Delete                                               */
/* ================================================================== */

static void rb_transplant(RBTree *t, RBNode *u, RBNode *v)
{
  if (u->parent == t->nil)
    t->root = v;
  else if (u == u->parent->left)
    u->parent->left = v;
  else
    u->parent->right = v;
  v->parent = u->parent;
}

static RBNode *rb_minimum_node(RBTree *t, RBNode *n)
{
  while (n->left != t->nil)
    n = n->left;
  return n;
}

static RBNode *rb_maximum_node(RBTree *t, RBNode *n)
{
  while (n->right != t->nil)
    n = n->right;
  return n;
}

static void rb_delete_fixup(RBTree *t, RBNode *x)
{
  while (x != t->root && x->color == BLACK)
  {
    if (x == x->parent->left)
    {
      RBNode *w = x->parent->right;
      if (w->color == RED)
      { /* case 1 */
        w->color = BLACK;
        x->parent->color = RED;
        rb_left_rotate(t, x->parent);
        w = x->parent->right;
      }
      if (w->left->color == BLACK && w->right->color == BLACK)
      { /* case 2 */
        w->color = RED;
        x = x->parent;
      }
      else
      {
        if (w->right->color == BLACK)
        { /* case 3 */
          w->left->color = BLACK;
          w->color = RED;
          rb_right_rotate(t, w);
          w = x->parent->right;
        }
        w->color = x->parent->color; /* case 4 */
        x->parent->color = BLACK;
        w->right->color = BLACK;
        rb_left_rotate(t, x->parent);
        x = t->root;
      }
    }
    else
    { /* mirror */
      RBNode *w = x->parent->left;
      if (w->color == RED)
      {
        w->color = BLACK;
        x->parent->color = RED;
        rb_right_rotate(t, x->parent);
        w = x->parent->left;
      }
      if (w->right->color == BLACK && w->left->color == BLACK)
      {
        w->color = RED;
        x = x->parent;
      }
      else
      {
        if (w->left->color == BLACK)
        {
          w->right->color = BLACK;
          w->color = RED;
          rb_left_rotate(t, w);
          w = x->parent->left;
        }
        w->color = x->parent->color;
        x->parent->color = BLACK;
        w->left->color = BLACK;
        rb_right_rotate(t, x->parent);
        x = t->root;
      }
    }
  }
  x->color = BLACK;
}

static RBNode *rb_find_node(RBTree *t, const void *data)
{
  RBNode *n = t->root;
  while (n != t->nil)
  {
    int cmp = t->compare(data, n->data);
    if (cmp == 0)
      return n;
    n = (cmp < 0) ? n->left : n->right;
  }
  return NULL;
}

int rb_delete(RBTree *t, const void *data)
{
  RBNode *z = rb_find_node(t, data);
  if (!z)
    return 0;

  RBNode *y = z;
  RBNode *x;
  Color y_orig = y->color;

  if (z->left == t->nil)
  {
    x = z->right;
    rb_transplant(t, z, z->right);
  }
  else if (z->right == t->nil)
  {
    x = z->left;
    rb_transplant(t, z, z->left);
  }
  else
  {
    y = rb_minimum_node(t, z->right);
    y_orig = y->color;
    x = y->right;
    if (y->parent == z)
    {
      x->parent = y;
    }
    else
    {
      rb_transplant(t, y, y->right);
      y->right = z->right;
      y->right->parent = y;
    }
    rb_transplant(t, z, y);
    y->left = z->left;
    y->left->parent = y;
    y->color = z->color;
  }

  rb_free_node(t, z);
  if (y_orig == BLACK)
    rb_delete_fixup(t, x);

  t->size--;
  return 1;
}

/* ================================================================== */
/*  Search / Contains                                                 */
/* ================================================================== */

const void *rb_search(RBTree *t, const void *data)
{
  RBNode *n = rb_find_node(t, data);
  return n ? n->data : NULL;
}

int rb_contains(RBTree *t, const void *data)
{
  return rb_find_node(t, data) != NULL;
}

/* ================================================================== */
/*  Min / Max                                                         */
/* ================================================================== */

const void *rb_minimum(RBTree *t)
{
  if (t->root == t->nil)
    return NULL;
  return rb_minimum_node(t, t->root)->data;
}

const void *rb_maximum(RBTree *t)
{
  if (t->root == t->nil)
    return NULL;
  return rb_maximum_node(t, t->root)->data;
}

/* ================================================================== */
/*  Successor / Predecessor                                           */
/* ================================================================== */

const void *rb_successor(RBTree *t, const void *data)
{
  RBNode *n = rb_find_node(t, data);
  if (!n)
    return NULL;

  if (n->right != t->nil)
    return rb_minimum_node(t, n->right)->data;

  RBNode *p = n->parent;
  while (p != t->nil && n == p->right)
  {
    n = p;
    p = p->parent;
  }
  return (p != t->nil) ? p->data : NULL;
}

const void *rb_predecessor(RBTree *t, const void *data)
{
  RBNode *n = rb_find_node(t, data);
  if (!n)
    return NULL;

  if (n->left != t->nil)
    return rb_maximum_node(t, n->left)->data;

  RBNode *p = n->parent;
  while (p != t->nil && n == p->left)
  {
    n = p;
    p = p->parent;
  }
  return (p != t->nil) ? p->data : NULL;
}

/* ================================================================== */
/*  Traversals                                                        */
/* ================================================================== */

static void inorder_walk(RBTree *t, RBNode *n, void (*visit)(const void *))
{
  if (n == t->nil)
    return;
  inorder_walk(t, n->left, visit);
  visit(n->data);
  inorder_walk(t, n->right, visit);
}

static void preorder_walk(RBTree *t, RBNode *n, void (*visit)(const void *))
{
  if (n == t->nil)
    return;
  visit(n->data);
  preorder_walk(t, n->left, visit);
  preorder_walk(t, n->right, visit);
}

static void postorder_walk(RBTree *t, RBNode *n, void (*visit)(const void *))
{
  if (n == t->nil)
    return;
  postorder_walk(t, n->left, visit);
  postorder_walk(t, n->right, visit);
  visit(n->data);
}

void rb_inorder(RBTree *t, void (*visit)(const void *))
{
  inorder_walk(t, t->root, visit);
}

void rb_preorder(RBTree *t, void (*visit)(const void *))
{
  preorder_walk(t, t->root, visit);
}

void rb_postorder(RBTree *t, void (*visit)(const void *))
{
  postorder_walk(t, t->root, visit);
}

/* ================================================================== */
/*  Print helpers                                                     */
/* ================================================================== */

static void rb_print_inorder(RBTree *t, RBNode *n)
{
  if (n == t->nil)
    return;
  rb_print_inorder(t, n->left);
  t->print(n->data);
  printf("(%s) ", n->color == RED ? "R" : "B");
  rb_print_inorder(t, n->right);
}

void rb_print(RBTree *t)
{
  printf("[ ");
  rb_print_inorder(t, t->root);
  printf("] (size=%zu)\n", t->size);
}

static void rb_print_tree_impl(RBTree *t, RBNode *n, const char *prefix,
                               int is_left)
{
  if (n == t->nil)
    return;

  printf("%s%s", prefix, is_left ? "+-- " : "\\-- ");
  t->print(n->data);
  printf(" (%s)\n", n->color == RED ? "R" : "B");

  char new_prefix[256];
  snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix,
           is_left ? "|   " : "    ");

  rb_print_tree_impl(t, n->left, new_prefix, 1);
  rb_print_tree_impl(t, n->right, new_prefix, 0);
}

void rb_print_tree(RBTree *t)
{
  if (t->root == t->nil)
  {
    printf("(empty tree)\n");
    return;
  }
  rb_print_tree_impl(t, t->root, "", 0);
}

/* ================================================================== */
/*  Size / Empty                                                      */
/* ================================================================== */

size_t rb_size(RBTree *t)
{
  return t->size;
}
int rb_empty(RBTree *t)
{
  return t->size == 0;
}

/* ================================================================== */
/*  RB-property verification (for testing / debugging)                */
/* ================================================================== */

static int rb_verify_impl(RBTree *t, RBNode *n, int black_count,
                          int *path_black)
{
  if (n == t->nil)
  {
    if (*path_black == -1)
      *path_black = black_count;
    return black_count == *path_black;
  }

  if (n->color == RED)
  {
    if (n->left->color == RED || n->right->color == RED)
    {
      fprintf(stderr, "VIOLATION: red node has red child\n");
      return 0;
    }
  }

  if (n->color == BLACK)
    black_count++;

  return rb_verify_impl(t, n->left, black_count, path_black) &&
         rb_verify_impl(t, n->right, black_count, path_black);
}

int rb_verify(RBTree *t)
{
  if (t->root->color != BLACK)
  {
    fprintf(stderr, "VIOLATION: root is not black\n");
    return 0;
  }
  int path_black = -1;
  return rb_verify_impl(t, t->root, 0, &path_black);
}

/* ================================================================== */
/*  KMP (Knuth-Morris-Pratt) string search                           */
/* ================================================================== */

/*
 * Build the KMP partial-match / failure table.
 * lps[i] = length of the longest proper prefix of pattern[0..i]
 *          that is also a suffix of pattern[0..i].
 * Caller must free() the returned array.
 */
static int *kmp_build_lps(const char *pattern, int m)
{
  int *lps = malloc((size_t)m * sizeof(int));
  if (!lps)
  {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  lps[0] = 0;
  int len = 0;
  int i = 1;

  while (i < m)
  {
    if (pattern[i] == pattern[len])
    {
      lps[i++] = ++len;
    }
    else if (len)
    {
      len = lps[len - 1];
    }
    else
    {
      lps[i++] = 0;
    }
  }
  return lps;
}

/*
 * KMP search: return the 0-based index of the first occurrence of
 * `pattern` in `text`, or -1 if not found.  O(n+m) time.
 */
static int kmp_search(const char *text, const char *pattern)
{
  int n = (int)strlen(text);
  int m = (int)strlen(pattern);
  if (m == 0)
    return 0;
  if (m > n)
    return -1;

  int *lps = kmp_build_lps(pattern, m);
  int i = 0, j = 0;

  while (i < n)
  {
    if (text[i] == pattern[j])
    {
      i++;
      j++;
      if (j == m)
      {
        free(lps);
        return i - j;
      }
    }
    else if (j)
    {
      j = lps[j - 1];
    }
    else
    {
      i++;
    }
  }
  free(lps);
  return -1;
}

/*
 * KMP: find ALL occurrence positions of `pattern` in `text`.
 * Returns a heap-allocated array of 0-based indices; *count receives the
 * number of matches.  Caller must free() the returned array (may be NULL
 * when *count == 0).
 */
static int *kmp_search_all(const char *text, const char *pattern, int *count)
{
  *count = 0;
  int n = (int)strlen(text);
  int m = (int)strlen(pattern);
  if (m == 0 || m > n)
    return NULL;

  int *lps = kmp_build_lps(pattern, m);
  int cap = 4;
  int *positions = malloc((size_t)cap * sizeof(int));
  if (!positions)
  {
    perror("malloc");
    free(lps);
    exit(EXIT_FAILURE);
  }

  int i = 0, j = 0;
  while (i < n)
  {
    if (text[i] == pattern[j])
    {
      i++;
      j++;
      if (j == m)
      {
        if (*count == cap)
        {
          cap *= 2;
          positions = realloc(positions, (size_t)cap * sizeof(int));
          if (!positions)
          {
            perror("realloc");
            free(lps);
            exit(EXIT_FAILURE);
          }
        }
        positions[(*count)++] = i - j;
        j = lps[j - 1]; /* allow overlapping matches */
      }
    }
    else if (j)
    {
      j = lps[j - 1];
    }
    else
    {
      i++;
    }
  }
  free(lps);
  if (*count == 0)
  {
    free(positions);
    return NULL;
  }
  return positions;
}

/* ================================================================== */
/*  String-tree search operations (uses KMP internally)               */
/* ================================================================== */

static StrResultSet *str_result_create(void)
{
  StrResultSet *rs = malloc(sizeof(StrResultSet));
  if (!rs)
  {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  rs->capacity = 8;
  rs->count = 0;
  rs->strings = malloc(rs->capacity * sizeof(char *));
  if (!rs->strings)
  {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  return rs;
}

static void str_result_push(StrResultSet *rs, const char *s)
{
  if (rs->count == rs->capacity)
  {
    rs->capacity *= 2;
    rs->strings = realloc(rs->strings, rs->capacity * sizeof(char *));
    if (!rs->strings)
    {
      perror("realloc");
      exit(EXIT_FAILURE);
    }
  }
  rs->strings[rs->count++] = s;
}

void str_result_free(StrResultSet *rs)
{
  if (!rs)
    return;
  free(rs->strings);
  free(rs);
}

static void rb_substr_walk(RBTree *t, RBNode *n, const char *pattern,
                           StrResultSet *rs)
{
  if (n == t->nil)
    return;
  rb_substr_walk(t, n->left, pattern, rs);

  const char *s = *(const char *const *)n->data;
  if (kmp_search(s, pattern) >= 0)
    str_result_push(rs, s);

  rb_substr_walk(t, n->right, pattern, rs);
}

static void rb_prefix_walk(RBTree *t, RBNode *n, const char *prefix,
                           size_t plen, StrResultSet *rs)
{
  if (n == t->nil)
    return;
  rb_prefix_walk(t, n->left, prefix, plen, rs);

  const char *s = *(const char *const *)n->data;
  if (strncmp(s, prefix, plen) == 0)
    str_result_push(rs, s);

  rb_prefix_walk(t, n->right, prefix, plen, rs);
}

static void rb_suffix_walk(RBTree *t, RBNode *n, const char *suffix,
                           size_t slen, StrResultSet *rs)
{
  if (n == t->nil)
    return;
  rb_suffix_walk(t, n->left, suffix, slen, rs);

  const char *s = *(const char *const *)n->data;
  size_t len = strlen(s);
  if (len >= slen && strcmp(s + len - slen, suffix) == 0)
    str_result_push(rs, s);

  rb_suffix_walk(t, n->right, suffix, slen, rs);
}

/*
 * rb_find_substring: find all strings in the tree that contain `pattern`
 *                    as a substring (KMP, O(N*m) over N stored strings).
 */
StrResultSet *rb_find_substring(RBTree *t, const char *pattern)
{
  StrResultSet *rs = str_result_create();
  rb_substr_walk(t, t->root, pattern, rs);
  return rs;
}

/*
 * rb_find_prefix: find all strings that start with `prefix`.
 */
StrResultSet *rb_find_prefix(RBTree *t, const char *prefix)
{
  StrResultSet *rs = str_result_create();
  rb_prefix_walk(t, t->root, prefix, strlen(prefix), rs);
  return rs;
}

/*
 * rb_find_suffix: find all strings that end with `suffix`.
 */
StrResultSet *rb_find_suffix(RBTree *t, const char *suffix)
{
  StrResultSet *rs = str_result_create();
  rb_suffix_walk(t, t->root, suffix, strlen(suffix), rs);
  return rs;
}

/*
 * rb_kmp_search_in_node: search for `pattern` inside a specific stored
 *                        string (exact key lookup first, then KMP).
 *                        Returns the 0-based index or -1.
 */
int rb_kmp_search_in_node(RBTree *t, const char *key, const char *pattern)
{
  const void *found = rb_search(t, &key);
  if (!found)
    return -1;
  const char *s = *(const char *const *)found;
  return kmp_search(s, pattern);
}

int *rb_kmp_search_all_in_node(RBTree *t, const char *key, const char *pattern,
                               int *count)
{
  if (!count)
    return NULL;
  *count = 0;

  const void *found = rb_search(t, &key);
  if (!found)
    return NULL;

  const char *s = *(const char *const *)found;
  return kmp_search_all(s, pattern, count);
}
