# Red-Black Tree: A Comprehensive Guide

## Table of Contents

1. [Introduction](#1-introduction)
2. [Red-Black Tree Properties](#2-red-black-tree-properties)
3. [Data Structures](#3-data-structures)
4. [Core Algorithms](#4-core-algorithms)
5. [Traversals](#5-traversals)
6. [Generic Type System](#6-generic-type-system)
7. [String-Specific Features](#7-string-specific-features)
8. [API Reference](#8-api-reference)
9. [Build and Run](#9-build-and-run)
10. [Complexity Summary](#10-complexity-summary)

---

## 1. Introduction

### 1.1 Binary Search Trees and Their Limitation

A **Binary Search Tree (BST)** is a tree data structure where every node satisfies
the ordering invariant:

- All keys in the **left** subtree are **less than** the node's key.
- All keys in the **right** subtree are **greater than** the node's key.

This allows search, insertion, and deletion in **O(h)** time, where *h* is the
height of the tree. In the best case (a perfectly balanced tree), *h = log2(n)*,
giving O(log n) operations. However, if elements are inserted in sorted order,
the tree degenerates into a linked list with *h = n*, making every operation
**O(n)** in the worst case.

```
Balanced BST (h=3)          Degenerate BST (h=5)

        8                   1
       / \                   \
      4   12                  2
     / \   \                   \
    2   6   14                  3
                                 \
                                  4
                                   \
                                    5
```

### 1.2 Why Self-Balancing Trees Matter

To guarantee O(log n) performance regardless of insertion order, we need
**self-balancing** BSTs. These trees automatically restructure themselves after
insertions and deletions to keep the height bounded at O(log n).

Common self-balancing BSTs include:

| Tree Type      | Balancing Strategy            | Typical Use            |
|----------------|-------------------------------|------------------------|
| AVL Tree       | Strict height-balance (\|h_L - h_R\| <= 1) | Read-heavy workloads |
| Red-Black Tree | Color-based relaxed balance   | General-purpose (STL, kernel) |
| B-Tree         | Multi-way branching           | Disk/database indexes  |

### 1.3 What Is a Red-Black Tree?

A **Red-Black Tree (RBT)** is a self-balancing BST where each node carries an
extra bit of information: its **color** (RED or BLACK). A set of coloring rules
ensures that no root-to-leaf path is more than twice as long as any other,
guaranteeing a height of at most **2 * log2(n + 1)**.

**History:**

- **1972** -- Rudolf Bayer invented *symmetric binary B-trees*, the conceptual
  ancestor.
- **1978** -- Leonidas Guibas and Robert Sedgewick introduced the red-black
  coloring scheme and coined the name "Red-Black Tree."
- Today, RBTs are used in the Linux kernel (`rbtree.h`), the C++ STL
  (`std::map`, `std::set`), Java's `TreeMap`, and many other systems.

---

## 2. Red-Black Tree Properties

A valid Red-Black Tree must satisfy **five properties** at all times:

### Property 1: Binary Coloring
> Every node is colored either **RED** or **BLACK**.

### Property 2: Black Root
> The **root** node is always **BLACK**.

### Property 3: Black Leaves (NIL Sentinels)
> Every **leaf** (NIL / null pointer) is considered **BLACK**.
> In this implementation, a dedicated sentinel node `t->nil` represents all
> external leaves.

### Property 4: Red Constraint (No Red-Red)
> If a node is **RED**, then **both** of its children must be **BLACK**.
> Equivalently: no two consecutive RED nodes may appear on any path from root
> to leaf.

### Property 5: Black-Height Invariant
> For every node, **all simple paths** from that node to any descendant leaf
> contain the **same number of BLACK nodes**. This count is called the
> **black-height** of the node.

### Why These Guarantee O(log n)

Consider a tree with black-height *bh*. The shortest root-to-leaf path consists
of *bh* black nodes (all black). The longest path alternates red and black,
giving at most *2 * bh* nodes. Since the shortest path has at least *bh* nodes
and a tree of height *h* has at most *2^h - 1* nodes:

```
    n >= 2^bh - 1
    bh <= log2(n + 1)
    h <= 2 * bh <= 2 * log2(n + 1)
```

Therefore the height is always **O(log n)**, guaranteeing logarithmic
performance for all basic operations.

### Visual Example

```
            8(B)
           /    \
        4(R)    12(R)
       /   \    /   \
     2(B) 6(B) 10(B) 14(B)
```

Verification:
- Root 8 is BLACK (Property 2).
- RED nodes 4 and 12 have only BLACK children (Property 4).
- Every root-to-NIL path passes through exactly 2 BLACK nodes (Property 5).

---

## 3. Data Structures

This implementation defines its core structures in `red_black.h`.

### 3.1 RBNode -- The Tree Node

```c
typedef struct RBNode {
    void          *data;     /* pointer to heap-allocated copy of the element */
    Color          color;    /* RED or BLACK */
    struct RBNode *left;     /* left child (or sentinel nil) */
    struct RBNode *right;    /* right child (or sentinel nil) */
    struct RBNode *parent;   /* parent node (or sentinel nil for root) */
} RBNode;
```

Each node stores a generic `void*` pointer to a heap-allocated copy of the
user's data. This allows the tree to hold any C type without templates or
code generation.

### 3.2 RBTree -- The Tree Container

```c
typedef struct RBTree {
    RBNode     *root;        /* pointer to the root node */
    RBNode     *nil;         /* sentinel node representing all NIL leaves */
    size_t      size;        /* number of elements currently stored */
    size_t      elem_size;   /* sizeof(element) for fixed-size types */
    RBType      type;        /* enum identifying the stored type */
    rb_cmp_fn   compare;     /* comparator function pointer */
    rb_print_fn print;       /* printer function pointer */
    rb_free_fn  free_elem;   /* custom destructor (NULL = plain free) */
    rb_copy_fn  copy_elem;   /* custom copy constructor (NULL = memcpy) */
} RBTree;
```

### 3.3 The Sentinel NIL Node

Instead of using `NULL` pointers for absent children, this implementation
allocates a single **sentinel node** (`t->nil`) per tree. The sentinel:

- Has color = BLACK (satisfying Property 3).
- Has `left`, `right`, and `parent` all pointing **back to itself**
  (self-referential).
- Eliminates `NULL` checks throughout the code -- every pointer dereference
  is valid.

```
    Sentinel nil:
    nil->color  = BLACK
    nil->left   = nil   (self-referential)
    nil->right  = nil   (self-referential)
    nil->parent = nil   (self-referential)
    nil->data   = NULL
```

This design simplifies rotation and fix-up code significantly. For example,
during delete fix-up, the code can safely read `w->left->color` even when
`w` is the sentinel, because `nil->left` is `nil` (which is BLACK).

---

## 4. Core Algorithms

### 4.1 Rotations

Rotations are the fundamental restructuring operation. They rearrange a
subtree's parent-child relationships while preserving the BST ordering
invariant. Each rotation runs in **O(1)** time.

#### Left Rotation

Pivots node `x` down-left, lifting its right child `y` into `x`'s position.

```
    Before left-rotate(x):       After left-rotate(x):

        x                            y
       / \                          / \
      a   y          ==>           x   c
         / \                      / \
        b   c                    a   b
```

**Step-by-step:**

1. Set `y = x->right`.
2. Move `y`'s left subtree `b` to become `x`'s right child.
3. If `b` is not NIL, update `b->parent = x`.
4. Set `y->parent = x->parent`.
5. If `x` was the root, set `t->root = y`.
   Otherwise, replace `x` with `y` in `x->parent`'s child pointer.
6. Set `y->left = x` and `x->parent = y`.

Implementation in `red_black.c`:

```c
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
```

#### Right Rotation

The mirror of left rotation. Pivots node `y` down-right, lifting its left
child `x`.

```
    Before right-rotate(y):      After right-rotate(y):

          y                          x
         / \                        / \
        x   c        ==>          a    y
       / \                            / \
      a   b                          b   c
```

### 4.2 Insertion

Insertion proceeds in two phases: a standard BST insertion followed by a
color fix-up pass.

#### Phase 1: BST Insertion

Walk down the tree comparing the new key against each node. When a NIL
sentinel is reached, attach the new node there. The new node is always
colored **RED** (this cannot violate the black-height invariant, but may
violate the red-red constraint).

```c
int rb_insert(RBTree *t, const void *data) {
    RBNode *y = t->nil;
    RBNode *x = t->root;

    while (x != t->nil) {
        y = x;
        int cmp = t->compare(data, x->data);
        if (cmp == 0)
            return 0;           /* duplicate -- not inserted */
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
```

#### Phase 2: Insert Fix-Up

After inserting a RED node `z`, the only property that can be violated is
**Property 4** (no red-red). The fix-up walks up the tree, handling three
cases (and their mirror images when the parent is a right child):

**Case 1: Uncle is RED (recoloring)**

```
        G(B)                    G(R)  <-- may violate with G's parent
       /    \                  /    \
     P(R)   U(R)    ==>     P(B)   U(B)
     /                       /
   z(R)                    z(R)

   Action: Recolor P and U to BLACK, G to RED.
           Move z up to G and repeat.
```

**Case 2: Uncle is BLACK, z is inner child (triangle -- rotate to line)**

```
        G(B)                   G(B)
       /    \                 /    \
     P(R)   U(B)   ==>     z(R)  U(B)    (now Case 3)
       \                   /
       z(R)              P(R)

   Action: Left-rotate on P, converting to Case 3.
```

**Case 3: Uncle is BLACK, z is outer child (line -- rotate and recolor)**

```
        G(B)                   P(B)
       /    \                 /    \
     P(R)   U(B)   ==>     z(R)  G(R)
     /                              \
   z(R)                            U(B)

   Action: Right-rotate on G, swap colors of P and G.
```

After the fix-up loop, the root is forced to BLACK (Property 2).

### 4.3 Deletion

Deletion is more complex. It proceeds in two phases: a standard BST removal
followed by a color fix-up if the removed node was BLACK.

#### Phase 1: BST Removal with Transplant

The `rb_transplant` helper replaces subtree rooted at `u` with subtree
rooted at `v`:

```c
static void rb_transplant(RBTree *t, RBNode *u, RBNode *v) {
    if (u->parent == t->nil)
        t->root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;
    v->parent = u->parent;
}
```

Three sub-cases for deleting node `z`:

1. **z has no left child:** Replace z with its right child.
2. **z has no right child:** Replace z with its left child.
3. **z has two children:** Find z's in-order successor `y` (minimum of right
   subtree), splice y out, and put y in z's position with z's color.

```c
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
```

#### Phase 2: Delete Fix-Up

When a BLACK node is removed, the black-height is reduced on one side. Node
`x` (the replacement) carries an "extra black" that must be resolved. The
fix-up handles **four cases** (and their mirrors):

**Case 1: Sibling `w` is RED**

```
       P(B)                     W(B)
      /    \                   /    \
    x(B)   w(R)    ==>      P(R)    D
           / \              /   \
          C   D           x(B)   C

   Action: Recolor w BLACK, P RED. Left-rotate on P.
           Now x's new sibling is BLACK -> fall into Cases 2-4.
```

**Case 2: Sibling `w` is BLACK, both w's children are BLACK**

```
       P(?)                    P(?)   <-- push problem up
      /    \                  /    \
    x(B)   w(B)    ==>     x(B)   w(R)
           / \                     / \
         C(B) D(B)               C(B) D(B)

   Action: Recolor w RED. Move x up to P and repeat.
```

**Case 3: Sibling `w` is BLACK, w's left child RED, right child BLACK**

```
       P(?)                    P(?)
      /    \                  /    \
    x(B)   w(B)    ==>     x(B)   C(B)   (now Case 4)
           / \                       \
         C(R) D(B)                   w(R)
                                       \
                                       D(B)

   Action: Recolor C BLACK, w RED. Right-rotate on w.
```

**Case 4: Sibling `w` is BLACK, w's right child is RED**

```
       P(?)                    w(P's color)
      /    \                  /       \
    x(B)   w(B)    ==>     P(B)      D(B)
           / \             /   \
          C   D(R)       x(B)   C

   Action: w takes P's color, P becomes BLACK, D becomes BLACK.
           Left-rotate on P. Set x = root (terminates loop).
```

After the loop, `x` is recolored BLACK.

### 4.4 Search

Search is a standard BST binary search. Starting at the root, compare the
target key against each node and descend left or right accordingly.

```c
const void *rb_search(RBTree *t, const void *data) {
    RBNode *n = rb_find_node(t, data);
    return n ? n->data : NULL;
}
```

Time complexity: **O(log n)** (bounded by tree height).

---

## 5. Traversals

The implementation provides three classic tree traversals plus visual
printing utilities.

### 5.1 In-Order Traversal (Left, Root, Right)

Visits nodes in **sorted ascending order**. This is the most common
traversal for BSTs.

```
Tree:       8
           / \
          4   12

In-order: 4, 8, 12
```

### 5.2 Pre-Order Traversal (Root, Left, Right)

Visits the root first, then left subtree, then right subtree. Useful for
serializing a tree (the output can reconstruct the same tree structure).

```
Pre-order: 8, 4, 12
```

### 5.3 Post-Order Traversal (Left, Right, Root)

Visits children before the parent. Useful for deletion (free children
before freeing the parent).

```
Post-order: 4, 12, 8
```

### 5.4 Pretty-Print (ASCII Tree)

`rb_print_tree()` outputs an indented ASCII representation showing each
node's value, color, and parent-child relationship:

```
\-- 19 (B)
    +-- 8 (B)
    |   +-- 0 (B)
    |   |   +-- -5 (R)
    |   \-- 12 (B)
    \-- 38 (B)
        +-- 31 (B)
        \-- 77 (R)
            +-- 41 (B)
            |   \-- 55 (R)
            \-- 100 (B)
```

`rb_print()` outputs a flat in-order listing with colors:

```
[ -5(R) 0(B) 8(B) 12(B) 19(B) 31(B) 38(B) 41(B) 55(R) 77(R) 100(B) ] (size=11)
```

---

## 6. Generic Type System

### 6.1 The RBType Enum

The tree supports **15 element types** out of the box:

| Enum Value    | C Type               | Category      |
|---------------|----------------------|---------------|
| `RB_CHAR`     | `char`               | Signed integer |
| `RB_UCHAR`    | `unsigned char`      | Unsigned integer |
| `RB_SHORT`    | `short`              | Signed integer |
| `RB_USHORT`   | `unsigned short`     | Unsigned integer |
| `RB_INT`      | `int`                | Signed integer |
| `RB_UINT`     | `unsigned int`       | Unsigned integer |
| `RB_LONG`     | `long`               | Signed integer |
| `RB_ULONG`    | `unsigned long`      | Unsigned integer |
| `RB_LLONG`    | `long long`          | Signed integer |
| `RB_ULLONG`   | `unsigned long long` | Unsigned integer |
| `RB_FLOAT`    | `float`              | Floating point |
| `RB_DOUBLE`   | `double`             | Floating point |
| `RB_LDOUBLE`  | `long double`        | Floating point |
| `RB_STRING`   | `char *`             | String        |
| `RB_CUSTOM`   | user-defined         | Custom        |

### 6.2 The TypeInfo Dispatch Table

For each built-in type, a static table maps `RBType` to four callbacks:

```c
typedef struct {
    size_t      elem_size;   /* sizeof(element) */
    rb_cmp_fn   cmp;         /* comparator */
    rb_print_fn print;       /* printer */
    rb_free_fn  free_elem;   /* destructor (NULL = plain free) */
    rb_copy_fn  copy_elem;   /* copy constructor (NULL = memcpy) */
} TypeInfo;
```

When `rb_create(RB_INT)` is called, the tree looks up `TYPE_TABLE[RB_INT]`
and installs the `cmp_int`, `print_int` callbacks, with `free_elem = NULL`
(plain `free()`) and `copy_elem = NULL` (memcpy-based copy).

### 6.3 The DEFINE_CMP Macro

All numeric comparators are generated from a single macro:

```c
#define DEFINE_CMP(NAME, TYPE)                                     \
static int cmp_##NAME(const void *a, const void *b) {              \
    TYPE va = *(const TYPE *)a;                                    \
    TYPE vb = *(const TYPE *)b;                                    \
    return (va > vb) - (va < vb);                                  \
}
```

The expression `(va > vb) - (va < vb)` is a branchless three-way comparison
that returns:
- `-1` if `a < b`
- `0` if `a == b`
- `+1` if `a > b`

### 6.4 void* Data Storage

The tree stores **copies** of inserted data, not pointers to the caller's
variables. For fixed-size types, `copy_fixed()` does a `malloc + memcpy`.
For strings, `copy_string()` duplicates both the `char*` pointer container
and the string content itself (a "boxed" string).

### 6.5 Custom Types via rb_create_custom()

Users can store arbitrary types by providing their own callbacks:

```c
RBTree *rb_create_custom(
    size_t      elem_size,   /* sizeof(YourStruct) */
    rb_cmp_fn   cmp,         /* comparison function */
    rb_print_fn print,       /* print function (or NULL) */
    rb_free_fn  free_fn,     /* destructor (or NULL for plain free) */
    rb_copy_fn  copy_fn      /* copy constructor (or NULL for memcpy) */
);
```

Example for a `Point` struct:

```c
typedef struct { double x, y; } Point;

int cmp_point(const void *a, const void *b) {
    const Point *pa = (const Point *)a;
    const Point *pb = (const Point *)b;
    if (pa->x != pb->x) return (pa->x > pb->x) - (pa->x < pb->x);
    return (pa->y > pb->y) - (pa->y < pb->y);
}

void print_point(const void *d) {
    const Point *p = (const Point *)d;
    printf("(%g,%g)", p->x, p->y);
}

RBTree *t = rb_create_custom(sizeof(Point), cmp_point, print_point, NULL, NULL);
```

---

## 7. String-Specific Features

When the tree stores strings (`RB_STRING`), the library provides additional
search capabilities powered by the **Knuth-Morris-Pratt (KMP)** algorithm.

### 7.1 KMP Algorithm Overview

KMP performs pattern matching in **O(n + m)** time (where *n* is text length
and *m* is pattern length) by precomputing a **Longest Proper Prefix which
is also Suffix (LPS)** table.

#### LPS Table Construction

For pattern `P[0..m-1]`, `lps[i]` = length of the longest proper prefix of
`P[0..i]` that is also a suffix of `P[0..i]`.

Example for pattern `"ABABC"`:

```
Index:    0  1  2  3  4
Pattern:  A  B  A  B  C
LPS:      0  0  1  2  0
```

The LPS table allows the search to skip unnecessary comparisons when a
mismatch occurs, achieving linear time.

#### Single-Match Search

`kmp_search(text, pattern)` returns the 0-based index of the first
occurrence, or -1 if not found.

#### All-Match Search

`kmp_search_all(text, pattern, &count)` returns a heap-allocated array of
all occurrence positions (supports overlapping matches).

### 7.2 Tree-Level String Search Functions

These functions walk the entire tree in-order and apply KMP or string
comparison to each stored string:

- **`rb_find_substring(t, pattern)`** -- Returns all strings containing
  `pattern` as a substring.
- **`rb_find_prefix(t, prefix)`** -- Returns all strings starting with
  `prefix` (uses `strncmp`).
- **`rb_find_suffix(t, suffix)`** -- Returns all strings ending with
  `suffix` (compares tail bytes).

All three return a `StrResultSet*` that must be freed with
`str_result_free()`.

### 7.3 Per-Node KMP Search

- **`rb_kmp_search_in_node(t, key, pattern)`** -- Exact-match lookup of
  `key` in the tree, then KMP search within that string for `pattern`.
  Returns the 0-based index or -1.
- **`rb_kmp_search_all_in_node(t, key, pattern, &count)`** -- Same, but
  returns all positions. Caller must `free()` the returned array.

### 7.4 StrResultSet

```c
typedef struct StrResultSet {
    const char **strings;   /* array of pointers to matched strings */
    size_t       count;     /* number of matches found */
    size_t       capacity;  /* allocated capacity */
} StrResultSet;
```

Usage pattern:

```c
StrResultSet *rs = rb_find_substring(t, "red");
for (size_t i = 0; i < rs->count; i++)
    printf("  %s\n", rs->strings[i]);
str_result_free(rs);
```

---

## 8. API Reference

### 8.1 Tree Lifecycle

| Function | Signature | Description | Time |
|----------|-----------|-------------|------|
| `rb_create` | `RBTree *rb_create(RBType type)` | Create a tree for a built-in type | O(1) |
| `rb_create_custom` | `RBTree *rb_create_custom(size_t, rb_cmp_fn, rb_print_fn, rb_free_fn, rb_copy_fn)` | Create a tree for a custom type | O(1) |
| `rb_destroy` | `void rb_destroy(RBTree *t)` | Free all nodes and the tree | O(n) |

### 8.2 Insertion and Deletion

| Function | Signature | Description | Time |
|----------|-----------|-------------|------|
| `rb_insert` | `int rb_insert(RBTree *t, const void *data)` | Insert a copy of data; returns 1 on success, 0 if duplicate | O(log n) |
| `rb_delete` | `int rb_delete(RBTree *t, const void *data)` | Delete the node matching data; returns 1 on success, 0 if not found | O(log n) |

### 8.3 Search and Query

| Function | Signature | Description | Time |
|----------|-----------|-------------|------|
| `rb_search` | `const void *rb_search(RBTree *t, const void *data)` | Returns pointer to stored data, or NULL | O(log n) |
| `rb_contains` | `int rb_contains(RBTree *t, const void *data)` | Returns 1 if found, 0 otherwise | O(log n) |
| `rb_minimum` | `const void *rb_minimum(RBTree *t)` | Returns pointer to minimum element | O(log n) |
| `rb_maximum` | `const void *rb_maximum(RBTree *t)` | Returns pointer to maximum element | O(log n) |
| `rb_successor` | `const void *rb_successor(RBTree *t, const void *data)` | Returns the next larger element | O(log n) |
| `rb_predecessor` | `const void *rb_predecessor(RBTree *t, const void *data)` | Returns the next smaller element | O(log n) |

### 8.4 Traversals and Printing

| Function | Signature | Description | Time |
|----------|-----------|-------------|------|
| `rb_inorder` | `void rb_inorder(RBTree *t, void (*visit)(const void *))` | In-order traversal with visitor callback | O(n) |
| `rb_preorder` | `void rb_preorder(RBTree *t, void (*visit)(const void *))` | Pre-order traversal | O(n) |
| `rb_postorder` | `void rb_postorder(RBTree *t, void (*visit)(const void *))` | Post-order traversal | O(n) |
| `rb_print` | `void rb_print(RBTree *t)` | Print flat in-order listing with colors | O(n) |
| `rb_print_tree` | `void rb_print_tree(RBTree *t)` | Print indented ASCII tree | O(n) |

### 8.5 Utility

| Function | Signature | Description | Time |
|----------|-----------|-------------|------|
| `rb_size` | `size_t rb_size(RBTree *t)` | Return element count | O(1) |
| `rb_empty` | `int rb_empty(RBTree *t)` | Return 1 if empty | O(1) |
| `rb_verify` | `int rb_verify(RBTree *t)` | Validate all RB properties; returns 1 if valid | O(n) |

### 8.6 String Search (RB_STRING trees only)

| Function | Signature | Description | Time |
|----------|-----------|-------------|------|
| `rb_find_substring` | `StrResultSet *rb_find_substring(RBTree *t, const char *pattern)` | Find all strings containing pattern | O(N * m) |
| `rb_find_prefix` | `StrResultSet *rb_find_prefix(RBTree *t, const char *prefix)` | Find all strings starting with prefix | O(N * p) |
| `rb_find_suffix` | `StrResultSet *rb_find_suffix(RBTree *t, const char *suffix)` | Find all strings ending with suffix | O(N * s) |
| `str_result_free` | `void str_result_free(StrResultSet *rs)` | Free a result set | O(1) |
| `rb_kmp_search_in_node` | `int rb_kmp_search_in_node(RBTree *t, const char *key, const char *pattern)` | KMP search within a specific stored string | O(log n + m) |
| `rb_kmp_search_all_in_node` | `int *rb_kmp_search_all_in_node(...)` | All KMP matches within a stored string | O(log n + k + m) |

### 8.7 Convenience Macros

```c
RB_INSERT_VAL(tree, type, val)   /* insert a literal value by address */
RB_DELETE_VAL(tree, type, val)   /* delete a literal value by address */
```

Example:

```c
RBTree *t = rb_create(RB_INT);
RB_INSERT_VAL(t, int, 42);      /* equivalent to: int _tmp=42; rb_insert(t, &_tmp); */
RB_DELETE_VAL(t, int, 42);
```

---

## 9. Build and Run

### 9.1 Compilation

```bash
gcc -Wall -Wextra -o rbtree main.c red_black.c -lm
```

| Flag | Purpose |
|------|---------|
| `-Wall -Wextra` | Enable comprehensive warnings |
| `-lm` | Link math library (if needed by extensions) |

### 9.2 Execution

```bash
./rbtree        # Linux / macOS
.\rbtree.exe    # Windows
```

### 9.3 What the Demo Exercises

`main.c` contains six demonstration functions:

| Demo | What It Tests |
|------|---------------|
| `demo_int` | Insert 11 integers, search, min/max, successor/predecessor, delete, verify |
| `demo_double` | Insert 7 doubles, print tree, verify |
| `demo_string` | Insert 18 strings, exact search, KMP substring/prefix/suffix search, KMP all-positions, delete, re-verify |
| `demo_char` | Insert characters from "REDBLACKTREE" (deduplicates), print, verify |
| `demo_all_numeric_types` | Exercise all 10 remaining numeric types (uchar through long double) |
| `demo_stress` | Insert 10,000 integers, verify, delete all evens, verify again |

### 9.4 Project File Layout

```
red_black_tree/
+-- red_black.h              Public API header
+-- red_black.c              Library implementation
+-- main.c                   Demo / self-test entry point
+-- .gitignore               Excludes *.exe from version control
+-- red_black_tree_guide.md  This document
```

---

## 10. Complexity Summary

| Operation | Average Case | Worst Case | Notes |
|-----------|-------------|------------|-------|
| Insert | O(log n) | O(log n) | At most 2 rotations per insert |
| Delete | O(log n) | O(log n) | At most 3 rotations per delete |
| Search | O(log n) | O(log n) | Standard BST binary search |
| Minimum / Maximum | O(log n) | O(log n) | Walk to leftmost / rightmost |
| Successor / Predecessor | O(log n) | O(log n) | In-order next / previous |
| In/Pre/Post-order traversal | O(n) | O(n) | Visit every node once |
| rb_verify | O(n) | O(n) | Recursive check of all properties |
| rb_find_substring | O(N * m) | O(N * m) | N = stored strings, m = pattern length |
| rb_find_prefix | O(N * p) | O(N * p) | p = prefix length |
| rb_find_suffix | O(N * s) | O(N * s) | s = suffix length |
| rb_size / rb_empty | O(1) | O(1) | Stored counter |
| rb_create / rb_destroy | O(1) / O(n) | O(1) / O(n) | Allocate / free all nodes |

**Space complexity:** O(n) -- each node stores a fixed-size struct plus a
heap-allocated copy of the element data.

---

*This document describes the Red-Black Tree implementation in `red_black.h`
and `red_black.c` of this repository.*
