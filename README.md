# Red-Black Tree

A generic Red-Black Tree library in C that supports all basic C types
(`char`, `short`, `int`, `long`, `long long`, unsigned variants, `float`,
`double`, `long double`, `char *`) and user-defined custom types.

The tree stores heap-allocated copies of inserted data via `void*` and uses
per-type comparator / printer / destructor function pointers, making it
fully type-agnostic at the library level.

## Features

- **Insert / Delete / Search** in O(log n) guaranteed worst-case time
- **Min / Max / Successor / Predecessor** queries
- **In-order, Pre-order, Post-order** traversals with visitor callbacks
- **Pretty-print** -- ASCII tree structure and flat in-order listing with colors
- **RB-property verification** (`rb_verify`) for testing and debugging
- **15 built-in types** -- select with a single enum value (`RB_INT`, `RB_DOUBLE`, `RB_STRING`, ...)
- **Custom types** -- provide your own compare / print / copy / free callbacks
- **KMP string search** -- substring, prefix, and suffix search across all stored strings
- **Convenience macros** -- `RB_INSERT_VAL`, `RB_DELETE_VAL` for ergonomic value insertion

## Quick Start

### Build

```bash
gcc -Wall -Wextra -o rbtree main.c red_black.c -lm
```

### Run the demo

```bash
./rbtree          # Linux / macOS
.\rbtree.exe      # Windows
```

### Basic usage

```c
#include "red_black.h"

int main(void) {
    RBTree *t = rb_create(RB_INT);

    int vals[] = {41, 38, 31, 12, 19, 8};
    for (int i = 0; i < 6; i++)
        rb_insert(t, &vals[i]);

    int key = 31;
    if (rb_contains(t, &key))
        printf("Found %d\n", key);

    rb_print_tree(t);
    rb_destroy(t);
    return 0;
}
```

### String tree with KMP search

```c
RBTree *t = rb_create(RB_STRING);

const char *words[] = {"red_black_tree", "hash_table", "linked_list"};
for (int i = 0; i < 3; i++)
    rb_insert(t, &words[i]);

StrResultSet *rs = rb_find_substring(t, "list");
for (size_t i = 0; i < rs->count; i++)
    printf("  %s\n", rs->strings[i]);
str_result_free(rs);

rb_destroy(t);
```

### Custom type

```c
typedef struct { double x, y; } Point;

int cmp_point(const void *a, const void *b) {
    const Point *pa = a, *pb = b;
    if (pa->x != pb->x) return (pa->x > pb->x) - (pa->x < pb->x);
    return (pa->y > pb->y) - (pa->y < pb->y);
}

RBTree *t = rb_create_custom(sizeof(Point), cmp_point, NULL, NULL, NULL);
```

## API Overview

| Category | Functions |
|----------|-----------|
| Lifecycle | `rb_create`, `rb_create_custom`, `rb_destroy` |
| Mutation | `rb_insert`, `rb_delete` |
| Query | `rb_search`, `rb_contains`, `rb_minimum`, `rb_maximum`, `rb_successor`, `rb_predecessor` |
| Traversal | `rb_inorder`, `rb_preorder`, `rb_postorder` |
| Display | `rb_print`, `rb_print_tree` |
| Utility | `rb_size`, `rb_empty`, `rb_verify` |
| String search | `rb_find_substring`, `rb_find_prefix`, `rb_find_suffix`, `rb_kmp_search_in_node`, `rb_kmp_search_all_in_node` |

See [`red_black.h`](red_black.h) for full signatures.

## Project Layout

```
red_black.h                    Public API header
red_black.c                    Library implementation (~870 lines)
main.c                         Demo and self-test entry point (~330 lines)
.gitignore                     Excludes *.exe from version control
Doc/red_black_tree_guide.md    Detailed beginner-friendly guide
Doc/red_black_tree_guide.docx  Same guide in Word format with syntax-highlighted code
```

## Documentation

A comprehensive guide is included in two formats:

- [**red_black_tree_guide.md**](Doc/red_black_tree_guide.md) -- Markdown (readable on GitHub)
- [**red_black_tree_guide.docx**](Doc/red_black_tree_guide.docx) -- Word document with syntax-highlighted C code

The guide covers:
1. BST motivation and RBT history
2. The five RB properties with O(log n) height proof
3. Data structures (RBNode, RBTree, sentinel NIL)
4. Rotation, insertion, and deletion algorithms with ASCII diagrams
5. Traversals and pretty-printing
6. Generic type system and custom type support
7. KMP string search integration
8. Complete API reference with time complexities
9. Build instructions and demo walkthrough
10. Complexity summary table

## License

This project is provided as-is for educational purposes.
