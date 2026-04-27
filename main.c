/*
 * main.c
 *
 * Demo and self-test entry point for the generic red-black tree library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "red_black.h"

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
        int cnt = 0;
        int *pos = rb_kmp_search_all_in_node(t, target, "am", &cnt);
        printf("  text: \"%s\"\n  pattern \"am\" found %d time(s) at index:", target, cnt);
        for (int k = 0; k < cnt; k++) printf(" %d", pos[k]);
        printf("\n");
        free(pos);
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
