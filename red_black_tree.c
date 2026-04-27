/*
 * red_black_tree.c
 *
 * A generic Red-Black Tree implementation in C that supports all basic C types
 * (char, short, int, long, long long, unsigned variants, float, double,
 *  long double, and string) with unbounded size.
 *
 * The tree stores copies of inserted data via void* and uses per-type
 * comparator / printer / destructor function pointers, making it fully
 * type-agnostic at the tree level.
 *
 * Compile:  gcc -Wall -Wextra -o rbtree red_black_tree.c -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <float.h>

/* ------------------------------------------------------------------ */
/*  Colour & element-type enums                                       */
/* ------------------------------------------------------------------ */

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

/* ------------------------------------------------------------------ */
/*  Callback typedefs                                                 */
/* ------------------------------------------------------------------ */

typedef int  (*rb_cmp_fn)(const void *a, const void *b);
typedef void (*rb_print_fn)(const void *data);
typedef void (*rb_free_fn)(void *data);
typedef void *(*rb_copy_fn)(const void *data);

/* ------------------------------------------------------------------ */
/*  Node                                                              */
/* ------------------------------------------------------------------ */

typedef struct RBNode {
    void          *data;
    Color          color;
    struct RBNode *left;
    struct RBNode *right;
    struct RBNode *parent;
} RBNode;

/* ------------------------------------------------------------------ */
/*  Tree                                                              */
/* ------------------------------------------------------------------ */

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

/* ================================================================== */
/*  Built-in comparators for every basic C type                       */
/* ================================================================== */

#define DEFINE_CMP(NAME, TYPE)                                     \
static int cmp_##NAME(const void *a, const void *b) {              \
    TYPE va = *(const TYPE *)a;                                    \
    TYPE vb = *(const TYPE *)b;                                    \
    return (va > vb) - (va < vb);                                  \
}

DEFINE_CMP(char,    char)
DEFINE_CMP(uchar,   unsigned char)
DEFINE_CMP(short,   short)
DEFINE_CMP(ushort,  unsigned short)
DEFINE_CMP(int,     int)
DEFINE_CMP(uint,    unsigned int)
DEFINE_CMP(long,    long)
DEFINE_CMP(ulong,   unsigned long)
DEFINE_CMP(llong,   long long)
DEFINE_CMP(ullong,  unsigned long long)
DEFINE_CMP(float,   float)
DEFINE_CMP(double,  double)
DEFINE_CMP(ldouble, long double)

static int cmp_string(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

/* ================================================================== */
/*  Built-in printers                                                 */
/* ================================================================== */

static void print_char(const void *d)    { printf("%c",   *(const char *)d); }
static void print_uchar(const void *d)   { printf("%u",   *(const unsigned char *)d); }
static void print_short(const void *d)   { printf("%hd",  *(const short *)d); }
static void print_ushort(const void *d)  { printf("%hu",  *(const unsigned short *)d); }
static void print_int(const void *d)     { printf("%d",   *(const int *)d); }
static void print_uint(const void *d)    { printf("%u",   *(const unsigned int *)d); }
static void print_long(const void *d)    { printf("%ld",  *(const long *)d); }
static void print_ulong(const void *d)   { printf("%lu",  *(const unsigned long *)d); }
static void print_llong(const void *d)   { printf("%lld", *(const long long *)d); }
static void print_ullong(const void *d)  { printf("%llu", *(const unsigned long long *)d); }
static void print_float(const void *d)   { printf("%g",   *(const float *)d); }
static void print_double(const void *d)  { printf("%g",   *(const double *)d); }
static void print_ldouble(const void *d) { printf("%Lg",  *(const long double *)d); }
static void print_string(const void *d)  { printf("%s",   *(const char *const *)d); }

/* ================================================================== */
/*  Built-in copy / free helpers for the string type                  */
/* ================================================================== */

static void *copy_string(const void *d) {
    const char *src = *(const char *const *)d;
    char *dup = strdup(src);
    if (!dup) { perror("strdup"); exit(EXIT_FAILURE); }
    char **box = malloc(sizeof(char *));
    if (!box) { perror("malloc"); exit(EXIT_FAILURE); }
    *box = dup;
    return box;
}

static void free_string(void *d) {
    char **box = (char **)d;
    free(*box);
    free(box);
}

static void *copy_fixed(const void *d, size_t sz) {
    void *p = malloc(sz);
    if (!p) { perror("malloc"); exit(EXIT_FAILURE); }
    memcpy(p, d, sz);
    return p;
}

/* ================================================================== */
/*  Type-descriptor table (maps RBType -> callbacks + elem_size)      */
/* ================================================================== */

typedef struct {
    size_t      elem_size;
    rb_cmp_fn   cmp;
    rb_print_fn print;
    rb_free_fn  free_elem;   /* NULL means plain free() */
    rb_copy_fn  copy_elem;   /* NULL means memcpy-based copy */
} TypeInfo;

static const TypeInfo TYPE_TABLE[] = {
    /* RB_CHAR    */ { sizeof(char),               cmp_char,    print_char,    NULL, NULL },
    /* RB_UCHAR   */ { sizeof(unsigned char),      cmp_uchar,   print_uchar,   NULL, NULL },
    /* RB_SHORT   */ { sizeof(short),              cmp_short,   print_short,   NULL, NULL },
    /* RB_USHORT  */ { sizeof(unsigned short),     cmp_ushort,  print_ushort,  NULL, NULL },
    /* RB_INT     */ { sizeof(int),                cmp_int,     print_int,     NULL, NULL },
    /* RB_UINT    */ { sizeof(unsigned int),       cmp_uint,    print_uint,    NULL, NULL },
    /* RB_LONG    */ { sizeof(long),               cmp_long,    print_long,    NULL, NULL },
    /* RB_ULONG   */ { sizeof(unsigned long),      cmp_ulong,   print_ulong,   NULL, NULL },
    /* RB_LLONG   */ { sizeof(long long),          cmp_llong,   print_llong,   NULL, NULL },
    /* RB_ULLONG  */ { sizeof(unsigned long long), cmp_ullong,  print_ullong,  NULL, NULL },
    /* RB_FLOAT   */ { sizeof(float),              cmp_float,   print_float,   NULL, NULL },
    /* RB_DOUBLE  */ { sizeof(double),             cmp_double,  print_double,  NULL, NULL },
    /* RB_LDOUBLE */ { sizeof(long double),        cmp_ldouble, print_ldouble, NULL, NULL },
    /* RB_STRING  */ { sizeof(char *),             cmp_string,  print_string,  free_string, copy_string },
};

/* ================================================================== */
/*  Tree creation / destruction                                       */
/* ================================================================== */

static RBNode *rb_alloc_node(RBTree *t) {
    RBNode *n = malloc(sizeof(RBNode));
    if (!n) { perror("malloc"); exit(EXIT_FAILURE); }
    n->left = n->right = n->parent = t->nil;
    n->color = RED;
    n->data  = NULL;
    return n;
}

RBTree *rb_create(RBType type) {
    if (type == RB_CUSTOM) {
        fprintf(stderr, "rb_create: use rb_create_custom() for RB_CUSTOM\n");
        return NULL;
    }

    RBTree *t = malloc(sizeof(RBTree));
    if (!t) { perror("malloc"); exit(EXIT_FAILURE); }

    t->nil = malloc(sizeof(RBNode));
    if (!t->nil) { perror("malloc"); exit(EXIT_FAILURE); }
    t->nil->color  = BLACK;
    t->nil->left   = t->nil->right = t->nil->parent = t->nil;
    t->nil->data   = NULL;

    t->root = t->nil;
    t->size = 0;
    t->type = type;

    const TypeInfo *ti = &TYPE_TABLE[type];
    t->elem_size = ti->elem_size;
    t->compare   = ti->cmp;
    t->print     = ti->print;
    t->free_elem = ti->free_elem;
    t->copy_elem = ti->copy_elem;
    return t;
}

RBTree *rb_create_custom(size_t elem_size,
                         rb_cmp_fn cmp,
                         rb_print_fn print,
                         rb_free_fn free_fn,
                         rb_copy_fn copy_fn) {
    RBTree *t = malloc(sizeof(RBTree));
    if (!t) { perror("malloc"); exit(EXIT_FAILURE); }

    t->nil = malloc(sizeof(RBNode));
    if (!t->nil) { perror("malloc"); exit(EXIT_FAILURE); }
    t->nil->color  = BLACK;
    t->nil->left   = t->nil->right = t->nil->parent = t->nil;
    t->nil->data   = NULL;

    t->root      = t->nil;
    t->size      = 0;
    t->type      = RB_CUSTOM;
    t->elem_size = elem_size;
    t->compare   = cmp;
    t->print     = print;
    t->free_elem = free_fn;
    t->copy_elem = copy_fn;
    return t;
}

static void rb_free_node(RBTree *t, RBNode *n) {
    if (n == t->nil) return;
    if (n->data) {
        if (t->free_elem)
            t->free_elem(n->data);
        else
            free(n->data);
    }
    free(n);
}

static void rb_free_subtree(RBTree *t, RBNode *n) {
    if (n == t->nil) return;
    rb_free_subtree(t, n->left);
    rb_free_subtree(t, n->right);
    rb_free_node(t, n);
}

void rb_destroy(RBTree *t) {
    if (!t) return;
    rb_free_subtree(t, t->root);
    free(t->nil);
    free(t);
}

/* ================================================================== */
/*  Data copy helper                                                  */
/* ================================================================== */

static void *rb_copy_data(RBTree *t, const void *data) {
    if (t->copy_elem)
        return t->copy_elem(data);
    return copy_fixed(data, t->elem_size);
}

/* ================================================================== */
/*  Rotations                                                         */
/* ================================================================== */

static void rb_left_rotate(RBTree *t, RBNode *x) {
    RBNode *y = x->right;
    x->right  = y->left;
    if (y->left != t->nil)
        y->left->parent = x;
    y->parent = x->parent;
    if (x->parent == t->nil)
        t->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->left   = x;
    x->parent = y;
}

static void rb_right_rotate(RBTree *t, RBNode *y) {
    RBNode *x = y->left;
    y->left   = x->right;
    if (x->right != t->nil)
        x->right->parent = y;
    x->parent = y->parent;
    if (y->parent == t->nil)
        t->root = x;
    else if (y == y->parent->left)
        y->parent->left = x;
    else
        y->parent->right = x;
    x->right  = y;
    y->parent = x;
}

/* ================================================================== */
/*  Insert                                                            */
/* ================================================================== */

static void rb_insert_fixup(RBTree *t, RBNode *z) {
    while (z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            RBNode *y = z->parent->parent->right;
            if (y->color == RED) {                        /* case 1 */
                z->parent->color         = BLACK;
                y->color                 = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {              /* case 2 */
                    z = z->parent;
                    rb_left_rotate(t, z);
                }
                z->parent->color         = BLACK;         /* case 3 */
                z->parent->parent->color = RED;
                rb_right_rotate(t, z->parent->parent);
            }
        } else {                                          /* mirror */
            RBNode *y = z->parent->parent->left;
            if (y->color == RED) {
                z->parent->color         = BLACK;
                y->color                 = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rb_right_rotate(t, z);
                }
                z->parent->color         = BLACK;
                z->parent->parent->color = RED;
                rb_left_rotate(t, z->parent->parent);
            }
        }
    }
    t->root->color = BLACK;
}

int rb_insert(RBTree *t, const void *data) {
    RBNode *y = t->nil;
    RBNode *x = t->root;

    while (x != t->nil) {
        y = x;
        int cmp = t->compare(data, x->data);
        if (cmp == 0)
            return 0;           /* duplicate — not inserted */
        x = (cmp < 0) ? x->left : x->right;
    }

    RBNode *z = rb_alloc_node(t);
    z->data   = rb_copy_data(t, data);
    z->parent = y;

    if (y == t->nil)
        t->root = z;
    else if (t->compare(z->data, y->data) < 0)
        y->left = z;
    else
        y->right = z;

    z->left  = t->nil;
    z->right = t->nil;
    z->color = RED;

    rb_insert_fixup(t, z);
    t->size++;
    return 1;
}

/* ================================================================== */
/*  Transplant & Delete                                               */
/* ================================================================== */

static void rb_transplant(RBTree *t, RBNode *u, RBNode *v) {
    if (u->parent == t->nil)
        t->root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;
    v->parent = u->parent;
}

static RBNode *rb_minimum_node(RBTree *t, RBNode *n) {
    while (n->left != t->nil)
        n = n->left;
    return n;
}

static RBNode *rb_maximum_node(RBTree *t, RBNode *n) {
    while (n->right != t->nil)
        n = n->right;
    return n;
}

static void rb_delete_fixup(RBTree *t, RBNode *x) {
    while (x != t->root && x->color == BLACK) {
        if (x == x->parent->left) {
            RBNode *w = x->parent->right;
            if (w->color == RED) {                             /* case 1 */
                w->color         = BLACK;
                x->parent->color = RED;
                rb_left_rotate(t, x->parent);
                w = x->parent->right;
            }
            if (w->left->color == BLACK &&
                w->right->color == BLACK) {                    /* case 2 */
                w->color = RED;
                x = x->parent;
            } else {
                if (w->right->color == BLACK) {                /* case 3 */
                    w->left->color = BLACK;
                    w->color       = RED;
                    rb_right_rotate(t, w);
                    w = x->parent->right;
                }
                w->color         = x->parent->color;          /* case 4 */
                x->parent->color = BLACK;
                w->right->color  = BLACK;
                rb_left_rotate(t, x->parent);
                x = t->root;
            }
        } else {                                               /* mirror */
            RBNode *w = x->parent->left;
            if (w->color == RED) {
                w->color         = BLACK;
                x->parent->color = RED;
                rb_right_rotate(t, x->parent);
                w = x->parent->left;
            }
            if (w->right->color == BLACK &&
                w->left->color  == BLACK) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->left->color == BLACK) {
                    w->right->color = BLACK;
                    w->color        = RED;
                    rb_left_rotate(t, w);
                    w = x->parent->left;
                }
                w->color         = x->parent->color;
                x->parent->color = BLACK;
                w->left->color   = BLACK;
                rb_right_rotate(t, x->parent);
                x = t->root;
            }
        }
    }
    x->color = BLACK;
}

static RBNode *rb_find_node(RBTree *t, const void *data) {
    RBNode *n = t->root;
    while (n != t->nil) {
        int cmp = t->compare(data, n->data);
        if (cmp == 0) return n;
        n = (cmp < 0) ? n->left : n->right;
    }
    return NULL;
}

int rb_delete(RBTree *t, const void *data) {
    RBNode *z = rb_find_node(t, data);
    if (!z) return 0;

    RBNode *y = z;
    RBNode *x;
    Color y_orig = y->color;

    if (z->left == t->nil) {
        x = z->right;
        rb_transplant(t, z, z->right);
    } else if (z->right == t->nil) {
        x = z->left;
        rb_transplant(t, z, z->left);
    } else {
        y = rb_minimum_node(t, z->right);
        y_orig = y->color;
        x = y->right;
        if (y->parent == z) {
            x->parent = y;
        } else {
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

const void *rb_search(RBTree *t, const void *data) {
    RBNode *n = rb_find_node(t, data);
    return n ? n->data : NULL;
}

int rb_contains(RBTree *t, const void *data) {
    return rb_find_node(t, data) != NULL;
}

/* ================================================================== */
/*  Min / Max                                                         */
/* ================================================================== */

const void *rb_minimum(RBTree *t) {
    if (t->root == t->nil) return NULL;
    return rb_minimum_node(t, t->root)->data;
}

const void *rb_maximum(RBTree *t) {
    if (t->root == t->nil) return NULL;
    return rb_maximum_node(t, t->root)->data;
}

/* ================================================================== */
/*  Successor / Predecessor                                           */
/* ================================================================== */

const void *rb_successor(RBTree *t, const void *data) {
    RBNode *n = rb_find_node(t, data);
    if (!n) return NULL;

    if (n->right != t->nil)
        return rb_minimum_node(t, n->right)->data;

    RBNode *p = n->parent;
    while (p != t->nil && n == p->right) {
        n = p;
        p = p->parent;
    }
    return (p != t->nil) ? p->data : NULL;
}

const void *rb_predecessor(RBTree *t, const void *data) {
    RBNode *n = rb_find_node(t, data);
    if (!n) return NULL;

    if (n->left != t->nil)
        return rb_maximum_node(t, n->left)->data;

    RBNode *p = n->parent;
    while (p != t->nil && n == p->left) {
        n = p;
        p = p->parent;
    }
    return (p != t->nil) ? p->data : NULL;
}

/* ================================================================== */
/*  Traversals                                                        */
/* ================================================================== */

static void inorder_walk(RBTree *t, RBNode *n, void (*visit)(const void *)) {
    if (n == t->nil) return;
    inorder_walk(t, n->left, visit);
    visit(n->data);
    inorder_walk(t, n->right, visit);
}

static void preorder_walk(RBTree *t, RBNode *n, void (*visit)(const void *)) {
    if (n == t->nil) return;
    visit(n->data);
    preorder_walk(t, n->left, visit);
    preorder_walk(t, n->right, visit);
}

static void postorder_walk(RBTree *t, RBNode *n, void (*visit)(const void *)) {
    if (n == t->nil) return;
    postorder_walk(t, n->left, visit);
    postorder_walk(t, n->right, visit);
    visit(n->data);
}

void rb_inorder(RBTree *t, void (*visit)(const void *)) {
    inorder_walk(t, t->root, visit);
}

void rb_preorder(RBTree *t, void (*visit)(const void *)) {
    preorder_walk(t, t->root, visit);
}

void rb_postorder(RBTree *t, void (*visit)(const void *)) {
    postorder_walk(t, t->root, visit);
}

/* ================================================================== */
/*  Print helpers                                                     */
/* ================================================================== */

static void rb_print_inorder(RBTree *t, RBNode *n) {
    if (n == t->nil) return;
    rb_print_inorder(t, n->left);
    t->print(n->data);
    printf("(%s) ", n->color == RED ? "R" : "B");
    rb_print_inorder(t, n->right);
}

void rb_print(RBTree *t) {
    printf("[ ");
    rb_print_inorder(t, t->root);
    printf("] (size=%zu)\n", t->size);
}

static void rb_print_tree_impl(RBTree *t, RBNode *n,
                                const char *prefix, int is_left) {
    if (n == t->nil) return;

    printf("%s%s", prefix, is_left ? "+-- " : "\\-- ");
    t->print(n->data);
    printf(" (%s)\n", n->color == RED ? "R" : "B");

    char new_prefix[256];
    snprintf(new_prefix, sizeof(new_prefix), "%s%s",
             prefix, is_left ? "|   " : "    ");

    rb_print_tree_impl(t, n->left,  new_prefix, 1);
    rb_print_tree_impl(t, n->right, new_prefix, 0);
}

void rb_print_tree(RBTree *t) {
    if (t->root == t->nil) {
        printf("(empty tree)\n");
        return;
    }
    rb_print_tree_impl(t, t->root, "", 0);
}

/* ================================================================== */
/*  Size / Empty                                                      */
/* ================================================================== */

size_t rb_size(RBTree *t)  { return t->size; }
int    rb_empty(RBTree *t) { return t->size == 0; }

/* ================================================================== */
/*  RB-property verification (for testing / debugging)                */
/* ================================================================== */

static int rb_verify_impl(RBTree *t, RBNode *n, int black_count,
                           int *path_black) {
    if (n == t->nil) {
        if (*path_black == -1)
            *path_black = black_count;
        return black_count == *path_black;
    }

    if (n->color == RED) {
        if (n->left->color == RED || n->right->color == RED) {
            fprintf(stderr, "VIOLATION: red node has red child\n");
            return 0;
        }
    }

    if (n->color == BLACK)
        black_count++;

    return rb_verify_impl(t, n->left,  black_count, path_black) &&
           rb_verify_impl(t, n->right, black_count, path_black);
}

int rb_verify(RBTree *t) {
    if (t->root->color != BLACK) {
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
static int *kmp_build_lps(const char *pattern, int m) {
    int *lps = malloc((size_t)m * sizeof(int));
    if (!lps) { perror("malloc"); exit(EXIT_FAILURE); }

    lps[0] = 0;
    int len = 0;
    int i   = 1;

    while (i < m) {
        if (pattern[i] == pattern[len]) {
            lps[i++] = ++len;
        } else if (len) {
            len = lps[len - 1];
        } else {
            lps[i++] = 0;
        }
    }
    return lps;
}

/*
 * KMP search: return the 0-based index of the first occurrence of
 * `pattern` in `text`, or -1 if not found.  O(n+m) time.
 */
static int kmp_search(const char *text, const char *pattern) {
    int n = (int)strlen(text);
    int m = (int)strlen(pattern);
    if (m == 0) return 0;
    if (m > n)  return -1;

    int *lps = kmp_build_lps(pattern, m);
    int i = 0, j = 0;

    while (i < n) {
        if (text[i] == pattern[j]) {
            i++; j++;
            if (j == m) { free(lps); return i - j; }
        } else if (j) {
            j = lps[j - 1];
        } else {
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
static int *kmp_search_all(const char *text, const char *pattern, int *count) {
    *count = 0;
    int n = (int)strlen(text);
    int m = (int)strlen(pattern);
    if (m == 0 || m > n) return NULL;

    int *lps = kmp_build_lps(pattern, m);
    int  cap = 4;
    int *positions = malloc((size_t)cap * sizeof(int));
    if (!positions) { perror("malloc"); free(lps); exit(EXIT_FAILURE); }

    int i = 0, j = 0;
    while (i < n) {
        if (text[i] == pattern[j]) {
            i++; j++;
            if (j == m) {
                if (*count == cap) {
                    cap *= 2;
                    positions = realloc(positions, (size_t)cap * sizeof(int));
                    if (!positions) { perror("realloc"); free(lps); exit(EXIT_FAILURE); }
                }
                positions[(*count)++] = i - j;
                j = lps[j - 1];     /* allow overlapping matches */
            }
        } else if (j) {
            j = lps[j - 1];
        } else {
            i++;
        }
    }
    free(lps);
    if (*count == 0) { free(positions); return NULL; }
    return positions;
}

/* ================================================================== */
/*  String-tree search operations (uses KMP internally)               */
/* ================================================================== */

typedef struct {
    const char **strings;
    size_t       count;
    size_t       capacity;
} StrResultSet;

static StrResultSet *str_result_create(void) {
    StrResultSet *rs = malloc(sizeof(StrResultSet));
    if (!rs) { perror("malloc"); exit(EXIT_FAILURE); }
    rs->capacity = 8;
    rs->count    = 0;
    rs->strings  = malloc(rs->capacity * sizeof(char *));
    if (!rs->strings) { perror("malloc"); exit(EXIT_FAILURE); }
    return rs;
}

static void str_result_push(StrResultSet *rs, const char *s) {
    if (rs->count == rs->capacity) {
        rs->capacity *= 2;
        rs->strings = realloc(rs->strings, rs->capacity * sizeof(char *));
        if (!rs->strings) { perror("realloc"); exit(EXIT_FAILURE); }
    }
    rs->strings[rs->count++] = s;
}

void str_result_free(StrResultSet *rs) {
    if (!rs) return;
    free(rs->strings);
    free(rs);
}

static void rb_substr_walk(RBTree *t, RBNode *n,
                           const char *pattern, StrResultSet *rs) {
    if (n == t->nil) return;
    rb_substr_walk(t, n->left, pattern, rs);

    const char *s = *(const char *const *)n->data;
    if (kmp_search(s, pattern) >= 0)
        str_result_push(rs, s);

    rb_substr_walk(t, n->right, pattern, rs);
}

static void rb_prefix_walk(RBTree *t, RBNode *n,
                           const char *prefix, size_t plen, StrResultSet *rs) {
    if (n == t->nil) return;
    rb_prefix_walk(t, n->left, prefix, plen, rs);

    const char *s = *(const char *const *)n->data;
    if (strncmp(s, prefix, plen) == 0)
        str_result_push(rs, s);

    rb_prefix_walk(t, n->right, prefix, plen, rs);
}

static void rb_suffix_walk(RBTree *t, RBNode *n,
                           const char *suffix, size_t slen, StrResultSet *rs) {
    if (n == t->nil) return;
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
StrResultSet *rb_find_substring(RBTree *t, const char *pattern) {
    StrResultSet *rs = str_result_create();
    rb_substr_walk(t, t->root, pattern, rs);
    return rs;
}

/*
 * rb_find_prefix: find all strings that start with `prefix`.
 */
StrResultSet *rb_find_prefix(RBTree *t, const char *prefix) {
    StrResultSet *rs = str_result_create();
    rb_prefix_walk(t, t->root, prefix, strlen(prefix), rs);
    return rs;
}

/*
 * rb_find_suffix: find all strings that end with `suffix`.
 */
StrResultSet *rb_find_suffix(RBTree *t, const char *suffix) {
    StrResultSet *rs = str_result_create();
    rb_suffix_walk(t, t->root, suffix, strlen(suffix), rs);
    return rs;
}

/*
 * rb_kmp_search_in_node: search for `pattern` inside a specific stored
 *                        string (exact key lookup first, then KMP).
 *                        Returns the 0-based index or -1.
 */
int rb_kmp_search_in_node(RBTree *t, const char *key, const char *pattern) {
    const void *found = rb_search(t, &key);
    if (!found) return -1;
    const char *s = *(const char *const *)found;
    return kmp_search(s, pattern);
}

/* ================================================================== */
/*  Convenience macros for type-safe insertion                        */
/* ================================================================== */

#define RB_INSERT_VAL(tree, type, val) \
    do { type _tmp = (val); rb_insert((tree), &_tmp); } while (0)

#define RB_DELETE_VAL(tree, type, val) \
    do { type _tmp = (val); rb_delete((tree), &_tmp); } while (0)

#define RB_SEARCH_VAL(tree, type, val) \
    ({ type _tmp = (val); rb_search((tree), &_tmp); })

#define RB_CONTAINS_VAL(tree, type, val) \
    ({ type _tmp = (val); rb_contains((tree), &_tmp); })

/* ================================================================== */
/*  Demo / self-test                                                  */
/* ================================================================== */

static void separator(const char *title) {
    printf("\n========== %s ==========\n", title);
}

static void demo_int(void) {
    separator("int tree");
    RBTree *t = rb_create(RB_INT);

    int vals[] = {41, 38, 31, 12, 19, 8, 0, -5, 100, 77, 55};
    size_t n = sizeof(vals) / sizeof(vals[0]);

    for (size_t i = 0; i < n; i++)
        rb_insert(t, &vals[i]);

    printf("In-order: ");
    rb_print(t);
    printf("\nTree structure:\n");
    rb_print_tree(t);

    int key = 19;
    printf("\nSearch 19: %s\n", rb_contains(t, &key) ? "found" : "not found");

    const int *min = (const int *)rb_minimum(t);
    const int *max = (const int *)rb_maximum(t);
    printf("Min: %d, Max: %d\n", *min, *max);

    key = 31;
    const int *succ = (const int *)rb_successor(t, &key);
    const int *pred = (const int *)rb_predecessor(t, &key);
    printf("Successor of 31: %d\n", succ ? *succ : 0);
    printf("Predecessor of 31: %d\n", pred ? *pred : 0);

    printf("\nDelete 38:\n");
    key = 38;
    rb_delete(t, &key);
    rb_print(t);
    rb_print_tree(t);
    printf("RB valid: %s\n", rb_verify(t) ? "yes" : "NO");

    rb_destroy(t);
}

static void demo_double(void) {
    separator("double tree");
    RBTree *t = rb_create(RB_DOUBLE);

    double vals[] = {3.14, 2.71, 1.41, 1.73, 0.577, 2.236, 9.81};
    size_t n = sizeof(vals) / sizeof(vals[0]);

    for (size_t i = 0; i < n; i++)
        rb_insert(t, &vals[i]);

    printf("In-order: ");
    rb_print(t);
    printf("\nTree structure:\n");
    rb_print_tree(t);
    printf("RB valid: %s\n", rb_verify(t) ? "yes" : "NO");

    rb_destroy(t);
}

static void demo_string(void) {
    separator("string (char*) tree — full demo");
    RBTree *t = rb_create(RB_STRING);

    const char *words[] = {
        "red_black_tree",  "binary_search",   "hash_table",
        "linked_list",     "priority_queue",   "breadth_first",
        "depth_first",     "dynamic_programming", "greedy_algorithm",
        "backtracking",    "divide_and_conquer",  "graph_theory",
        "tree_traversal",  "red_herring",      "blackberry",
        "red_planet",      "greenhouse",       "treehouse",
    };
    size_t n = sizeof(words) / sizeof(words[0]);

    printf("\n--- Inserting %zu strings ---\n", n);
    for (size_t i = 0; i < n; i++)
        rb_insert(t, &words[i]);

    printf("In-order: ");
    rb_print(t);
    printf("\nTree structure:\n");
    rb_print_tree(t);

    /* --- Exact search (BST, O(log n)) --- */
    printf("\n--- Exact search (BST O(log n)) ---\n");
    const char *exact[] = {"hash_table", "quick_sort", "red_black_tree"};
    for (size_t i = 0; i < 3; i++) {
        printf("  search \"%s\": %s\n", exact[i],
               rb_contains(t, &exact[i]) ? "FOUND" : "not found");
    }

    /* --- KMP substring search --- */
    printf("\n--- KMP substring search ---\n");
    const char *patterns[] = {"red", "tree", "first", "ing", "black"};
    for (size_t i = 0; i < 5; i++) {
        StrResultSet *rs = rb_find_substring(t, patterns[i]);
        printf("  substring \"%s\" (%zu matches):", patterns[i], rs->count);
        for (size_t j = 0; j < rs->count; j++)
            printf(" \"%s\"", rs->strings[j]);
        printf("\n");
        str_result_free(rs);
    }

    /* --- KMP all-positions within a single string --- */
    printf("\n--- KMP all-positions in \"dynamic_programming\" for \"am\" ---\n");
    {
        const char *target = "dynamic_programming";
        const void *found = rb_search(t, &target);
        if (found) {
            const char *s = *(const char *const *)found;
            int cnt;
            int *pos = kmp_search_all(s, "am", &cnt);
            printf("  text: \"%s\"\n  pattern \"am\" found %d time(s) at index:", s, cnt);
            for (int k = 0; k < cnt; k++) printf(" %d", pos[k]);
            printf("\n");
            free(pos);
        }
    }

    /* --- Prefix search --- */
    printf("\n--- Prefix search ---\n");
    const char *prefixes[] = {"red", "tree", "gr"};
    for (size_t i = 0; i < 3; i++) {
        StrResultSet *rs = rb_find_prefix(t, prefixes[i]);
        printf("  prefix \"%s\" (%zu matches):", prefixes[i], rs->count);
        for (size_t j = 0; j < rs->count; j++)
            printf(" \"%s\"", rs->strings[j]);
        printf("\n");
        str_result_free(rs);
    }

    /* --- Suffix search --- */
    printf("\n--- Suffix search ---\n");
    const char *suffixes[] = {"tree", "first", "ing"};
    for (size_t i = 0; i < 3; i++) {
        StrResultSet *rs = rb_find_suffix(t, suffixes[i]);
        printf("  suffix \"%s\" (%zu matches):", suffixes[i], rs->count);
        for (size_t j = 0; j < rs->count; j++)
            printf(" \"%s\"", rs->strings[j]);
        printf("\n");
        str_result_free(rs);
    }

    /* --- Min / Max / Successor / Predecessor (lexicographic) --- */
    printf("\n--- Min / Max / Successor / Predecessor ---\n");
    const char **smin = (const char **)rb_minimum(t);
    const char **smax = (const char **)rb_maximum(t);
    printf("  min: \"%s\"\n  max: \"%s\"\n", *smin, *smax);

    const char *ref = "hash_table";
    const char **succ = (const char **)rb_successor(t, &ref);
    const char **pred = (const char **)rb_predecessor(t, &ref);
    printf("  successor  of \"%s\": \"%s\"\n", ref, succ ? *succ : "(none)");
    printf("  predecessor of \"%s\": \"%s\"\n", ref, pred ? *pred : "(none)");

    /* --- Delete a few strings and re-verify --- */
    printf("\n--- Delete + re-verify ---\n");
    const char *to_del[] = {"red_herring", "greedy_algorithm", "treehouse"};
    for (size_t i = 0; i < 3; i++) {
        printf("  delete \"%s\": %s\n", to_del[i],
               rb_delete(t, &to_del[i]) ? "OK" : "not found");
    }
    printf("\nAfter deletions: ");
    rb_print(t);
    rb_print_tree(t);
    printf("RB valid: %s\n", rb_verify(t) ? "yes" : "NO");

    /* --- KMP search after deletions to confirm consistency --- */
    printf("\n--- KMP substring \"red\" after deletions ---\n");
    {
        StrResultSet *rs = rb_find_substring(t, "red");
        printf("  matches (%zu):", rs->count);
        for (size_t j = 0; j < rs->count; j++)
            printf(" \"%s\"", rs->strings[j]);
        printf("\n");
        str_result_free(rs);
    }

    rb_destroy(t);
}

static void demo_char(void) {
    separator("char tree");
    RBTree *t = rb_create(RB_CHAR);

    char chars[] = "REDBLACKTREE";
    for (size_t i = 0; i < strlen(chars); i++)
        rb_insert(t, &chars[i]);

    printf("In-order: ");
    rb_print(t);
    printf("\nTree structure:\n");
    rb_print_tree(t);
    printf("RB valid: %s\n", rb_verify(t) ? "yes" : "NO");

    rb_destroy(t);
}

static void demo_all_numeric_types(void) {
    separator("all numeric types (insert + verify)");

    {
        RBTree *t = rb_create(RB_UCHAR);
        unsigned char v;
        for (v = 10; v < 20; v++) rb_insert(t, &v);
        printf("unsigned char : "); rb_print(t);
        printf("  valid: %s\n", rb_verify(t) ? "yes" : "NO");
        rb_destroy(t);
    }
    {
        RBTree *t = rb_create(RB_SHORT);
        short vals[] = {-100, 200, -300, 400, 0};
        for (size_t i = 0; i < 5; i++) rb_insert(t, &vals[i]);
        printf("short         : "); rb_print(t);
        printf("  valid: %s\n", rb_verify(t) ? "yes" : "NO");
        rb_destroy(t);
    }
    {
        RBTree *t = rb_create(RB_USHORT);
        unsigned short vals[] = {1000, 2000, 500, 3000, 1500};
        for (size_t i = 0; i < 5; i++) rb_insert(t, &vals[i]);
        printf("unsigned short: "); rb_print(t);
        printf("  valid: %s\n", rb_verify(t) ? "yes" : "NO");
        rb_destroy(t);
    }
    {
        RBTree *t = rb_create(RB_UINT);
        unsigned int vals[] = {42, 7, 99, 1, 55};
        for (size_t i = 0; i < 5; i++) rb_insert(t, &vals[i]);
        printf("unsigned int  : "); rb_print(t);
        printf("  valid: %s\n", rb_verify(t) ? "yes" : "NO");
        rb_destroy(t);
    }
    {
        RBTree *t = rb_create(RB_LONG);
        long vals[] = {-100000L, 200000L, -300000L, 400000L, 0L};
        for (size_t i = 0; i < 5; i++) rb_insert(t, &vals[i]);
        printf("long          : "); rb_print(t);
        printf("  valid: %s\n", rb_verify(t) ? "yes" : "NO");
        rb_destroy(t);
    }
    {
        RBTree *t = rb_create(RB_ULONG);
        unsigned long vals[] = {100000UL, 200000UL, 50000UL, 300000UL, 150000UL};
        for (size_t i = 0; i < 5; i++) rb_insert(t, &vals[i]);
        printf("unsigned long : "); rb_print(t);
        printf("  valid: %s\n", rb_verify(t) ? "yes" : "NO");
        rb_destroy(t);
    }
    {
        RBTree *t = rb_create(RB_LLONG);
        long long vals[] = {-1000000000LL, 2000000000LL, -3000000000LL,
                            4000000000LL, 0LL};
        for (size_t i = 0; i < 5; i++) rb_insert(t, &vals[i]);
        printf("long long     : "); rb_print(t);
        printf("  valid: %s\n", rb_verify(t) ? "yes" : "NO");
        rb_destroy(t);
    }
    {
        RBTree *t = rb_create(RB_ULLONG);
        unsigned long long vals[] = {10000000000ULL, 20000000000ULL,
                                     5000000000ULL,  30000000000ULL,
                                     15000000000ULL};
        for (size_t i = 0; i < 5; i++) rb_insert(t, &vals[i]);
        printf("unsigned llong: "); rb_print(t);
        printf("  valid: %s\n", rb_verify(t) ? "yes" : "NO");
        rb_destroy(t);
    }
    {
        RBTree *t = rb_create(RB_FLOAT);
        float vals[] = {1.1f, 2.2f, 0.5f, 3.3f, -1.5f};
        for (size_t i = 0; i < 5; i++) rb_insert(t, &vals[i]);
        printf("float         : "); rb_print(t);
        printf("  valid: %s\n", rb_verify(t) ? "yes" : "NO");
        rb_destroy(t);
    }
    {
        RBTree *t = rb_create(RB_LDOUBLE);
        long double vals[] = {1.1L, 2.2L, 0.5L, 3.3L, -1.5L};
        for (size_t i = 0; i < 5; i++) rb_insert(t, &vals[i]);
        printf("long double   : "); rb_print(t);
        printf("  valid: %s\n", rb_verify(t) ? "yes" : "NO");
        rb_destroy(t);
    }
}

static void demo_stress(void) {
    separator("stress test (10 000 inserts + deletes)");
    RBTree *t = rb_create(RB_INT);

    const int N = 10000;
    for (int i = 0; i < N; i++)
        rb_insert(t, &i);

    printf("After %d inserts: size=%zu, valid=%s\n",
           N, rb_size(t), rb_verify(t) ? "yes" : "NO");

    for (int i = 0; i < N; i += 2)
        rb_delete(t, &i);

    printf("After deleting evens: size=%zu, valid=%s\n",
           rb_size(t), rb_verify(t) ? "yes" : "NO");

    rb_destroy(t);
}

/* ================================================================== */
/*  main                                                              */
/* ================================================================== */

int main(void) {
    demo_int();
    demo_double();
    demo_string();
    demo_char();
    demo_all_numeric_types();
    demo_stress();

    printf("\nAll demos completed successfully.\n");
    return 0;
}
