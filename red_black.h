/*
 * red_black.h
 *
 * Public API for a generic Red-Black Tree implementation in C.
 */

#ifndef RED_BLACK_H
#define RED_BLACK_H

#include <stddef.h>

/**
 * @brief Node color used by Red-Black Tree balancing rules.
 */
typedef enum
{
  RED,
  BLACK
} Color;

/**
 * @brief Built-in element kinds supported by rb_create().
 */
typedef enum
{
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

/**
 * @brief Compare two elements.
 * @param a Pointer to first element.
 * @param b Pointer to second element.
 * @return Negative if a<b, zero if equal, positive if a>b.
 */
typedef int (*rb_cmp_fn)(const void *a, const void *b);

/**
 * @brief Print one element.
 * @param data Pointer to the element to print.
 */
typedef void (*rb_print_fn)(const void *data);

/**
 * @brief Free one element payload.
 * @param data Pointer to payload to free.
 */
typedef void (*rb_free_fn)(void *data);

/**
 * @brief Copy one element payload.
 * @param data Pointer to source payload.
 * @return Heap-allocated copy of payload.
 */
typedef void *(*rb_copy_fn)(const void *data);

/**
 * @brief Tree node that stores one payload and relational links.
 */
typedef struct RBNode
{
  void *data;           /**< Heap payload for this key. */
  Color color;          /**< RED or BLACK. */
  struct RBNode *left;  /**< Left child (or tree nil sentinel). */
  struct RBNode *right; /**< Right child (or tree nil sentinel). */
  struct RBNode *parent;/**< Parent node (or tree nil sentinel). */
} RBNode;

/**
 * @brief Main Red-Black Tree object and type behavior callbacks.
 */
typedef struct RBTree
{
  RBNode *root;       /**< Root node, or nil when empty. */
  RBNode *nil;        /**< Per-tree BLACK sentinel node. */
  size_t size;        /**< Number of stored elements. */
  size_t elem_size;   /**< Element byte size for fixed-size types. */
  RBType type;        /**< Selected built-in type (or RB_CUSTOM). */
  rb_cmp_fn compare;  /**< Compare callback used by ordering logic. */
  rb_print_fn print;  /**< Print callback used by debug/display APIs. */
  rb_free_fn free_elem; /**< Payload free callback (NULL means free()). */
  rb_copy_fn copy_elem; /**< Payload copy callback (NULL means memcpy copy). */
} RBTree;

/**
 * @brief Type descriptor used internally to map RBType to behavior.
 */
typedef struct
{
  size_t elem_size;      /**< Element size in bytes. */
  rb_cmp_fn cmp;         /**< Comparator for this type. */
  rb_print_fn print;     /**< Printer for this type. */
  rb_free_fn free_elem;  /**< Payload free callback; NULL means free(). */
  rb_copy_fn copy_elem;  /**< Payload copy callback; NULL means memcpy copy. */
} TypeInfo;

/**
 * @brief Result set container returned by string search helpers.
 */
typedef struct StrResultSet
{
  const char **strings; /**< Matched string pointers (owned by tree). */
  size_t count;         /**< Number of valid matches in strings[]. */
  size_t capacity;      /**< Allocated slot count in strings[]. */
} StrResultSet;

/**
 * @brief Create a tree for one built-in RBType.
 * @param type Built-in element kind (must not be RB_CUSTOM).
 * @return New tree pointer on success, NULL when type is RB_CUSTOM.
 */
RBTree *rb_create(RBType type);

/**
 * @brief Create a tree for custom payload types.
 * @param elem_size Element size in bytes for memcpy fallback copy.
 * @param cmp Comparator callback (required).
 * @param print Printer callback (optional, required for rb_print APIs).
 * @param free_fn Payload free callback (optional, NULL means free()).
 * @param copy_fn Payload copy callback (optional, NULL means memcpy copy).
 * @return New tree pointer on success.
 */
RBTree *rb_create_custom(size_t elem_size, rb_cmp_fn cmp, rb_print_fn print,
                         rb_free_fn free_fn, rb_copy_fn copy_fn);

/**
 * @brief Destroy a tree and free all nodes/payloads.
 * @param t Tree pointer to destroy (NULL is allowed).
 */
void rb_destroy(RBTree *t);

/**
 * @brief Insert one element copy into tree.
 * @param t Target tree.
 * @param data Pointer to source element.
 * @return 1 when inserted, 0 when duplicate key already exists.
 */
int rb_insert(RBTree *t, const void *data);

/**
 * @brief Delete one element from tree.
 * @param t Target tree.
 * @param data Pointer to key to remove.
 * @return 1 when deleted, 0 when key is not found.
 */
int rb_delete(RBTree *t, const void *data);

/**
 * @brief Search for an exact key.
 * @param t Target tree.
 * @param data Pointer to key to look up.
 * @return Pointer to stored payload, or NULL when not found.
 */
const void *rb_search(RBTree *t, const void *data);

/**
 * @brief Check whether a key exists.
 * @param t Target tree.
 * @param data Pointer to key to test.
 * @return 1 when found, 0 otherwise.
 */
int rb_contains(RBTree *t, const void *data);

/**
 * @brief Get smallest element in tree.
 * @param t Target tree.
 * @return Pointer to smallest payload, or NULL when tree is empty.
 */
const void *rb_minimum(RBTree *t);

/**
 * @brief Get largest element in tree.
 * @param t Target tree.
 * @return Pointer to largest payload, or NULL when tree is empty.
 */
const void *rb_maximum(RBTree *t);

/**
 * @brief Get in-order successor of given key.
 * @param t Target tree.
 * @param data Pointer to existing key.
 * @return Pointer to successor payload, or NULL when absent/no successor.
 */
const void *rb_successor(RBTree *t, const void *data);

/**
 * @brief Get in-order predecessor of given key.
 * @param t Target tree.
 * @param data Pointer to existing key.
 * @return Pointer to predecessor payload, or NULL when absent/no predecessor.
 */
const void *rb_predecessor(RBTree *t, const void *data);

/**
 * @brief Traverse tree in sorted order and call visitor per node.
 * @param t Target tree.
 * @param visit Visitor callback receiving payload pointer.
 */
void rb_inorder(RBTree *t, void (*visit)(const void *));

/**
 * @brief Traverse tree in root-left-right order.
 * @param t Target tree.
 * @param visit Visitor callback receiving payload pointer.
 */
void rb_preorder(RBTree *t, void (*visit)(const void *));

/**
 * @brief Traverse tree in left-right-root order.
 * @param t Target tree.
 * @param visit Visitor callback receiving payload pointer.
 */
void rb_postorder(RBTree *t, void (*visit)(const void *));

/**
 * @brief Print in-order payloads with node colors.
 * @param t Target tree.
 */
void rb_print(RBTree *t);

/**
 * @brief Print tree structure as ASCII hierarchy.
 * @param t Target tree.
 */
void rb_print_tree(RBTree *t);

/**
 * @brief Get number of stored elements.
 * @param t Target tree.
 * @return Current size.
 */
size_t rb_size(RBTree *t);

/**
 * @brief Check whether tree has no elements.
 * @param t Target tree.
 * @return 1 when empty, 0 otherwise.
 */
int rb_empty(RBTree *t);

/**
 * @brief Verify Red-Black invariants for debugging/testing.
 * @param t Target tree.
 * @return 1 when valid, 0 when any invariant is violated.
 */
int rb_verify(RBTree *t);

/**
 * @brief Find all strings containing a substring pattern.
 * @param t String tree (RB_STRING).
 * @param pattern Substring pattern to search.
 * @return New StrResultSet pointer (free with str_result_free()).
 */
StrResultSet *rb_find_substring(RBTree *t, const char *pattern);

/**
 * @brief Find all strings with a prefix.
 * @param t String tree (RB_STRING).
 * @param prefix Prefix to match.
 * @return New StrResultSet pointer (free with str_result_free()).
 */
StrResultSet *rb_find_prefix(RBTree *t, const char *prefix);

/**
 * @brief Find all strings with a suffix.
 * @param t String tree (RB_STRING).
 * @param suffix Suffix to match.
 * @return New StrResultSet pointer (free with str_result_free()).
 */
StrResultSet *rb_find_suffix(RBTree *t, const char *suffix);

/**
 * @brief Free a StrResultSet allocated by string search APIs.
 * @param rs Result set pointer (NULL is allowed).
 */
void str_result_free(StrResultSet *rs);

/**
 * @brief Run KMP search in one stored string node.
 * @param t String tree (RB_STRING).
 * @param key Exact key to locate in tree first.
 * @param pattern Pattern to search inside found string.
 * @return First match index, or -1 when key/pattern is not found.
 */
int rb_kmp_search_in_node(RBTree *t, const char *key, const char *pattern);

/**
 * @brief Run KMP all-match search in one stored string node.
 * @param t String tree (RB_STRING).
 * @param key Exact key to locate in tree first.
 * @param pattern Pattern to search.
 * @param count Output pointer receiving number of matches.
 * @return Heap array of 0-based match indexes, or NULL when none.
 */
int *rb_kmp_search_all_in_node(RBTree *t, const char *key, const char *pattern,
                               int *count);

/**
 * @brief Insert one literal value by temporary stack copy.
 */
#define RB_INSERT_VAL(tree, type, val)                                         \
  do                                                                           \
  {                                                                            \
    type _tmp = (val);                                                         \
    rb_insert((tree), &_tmp);                                                  \
  } while (0)

/**
 * @brief Delete one literal value by temporary stack copy.
 */
#define RB_DELETE_VAL(tree, type, val)                                         \
  do                                                                           \
  {                                                                            \
    type _tmp = (val);                                                         \
    rb_delete((tree), &_tmp);                                                  \
  } while (0)

#endif /* RED_BLACK_H */
