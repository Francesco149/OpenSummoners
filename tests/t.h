/*
 * tests/t.h — assertion macros for OpenSummoners host-side unit tests.
 *
 * Each test function has signature `int name(void)` and returns:
 *   0 on pass
 *   1 on fail (T_FAIL has already emitted a message)
 *   2 on skip (e.g. vendor data missing)
 *
 * The driver in test_main.c iterates the registered tests, runs them,
 * and exits 0 iff there are zero failures.  Skips don't count as failures.
 *
 * Convention matches the sibling openrecet project's tests/ harness so
 * porting tests across projects is a copy-paste.
 */
#ifndef OPENSUMMONERS_TESTS_T_H
#define OPENSUMMONERS_TESTS_T_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define T_FAIL(...) do {                                         \
    fprintf(stderr, "  fail %s:%d: ", __FILE__, __LINE__);       \
    fprintf(stderr, __VA_ARGS__);                                \
    fputc('\n', stderr);                                         \
    return 1;                                                    \
} while (0)

#define T_SKIP(...) do {                                         \
    fprintf(stderr, "  skip: ");                                 \
    fprintf(stderr, __VA_ARGS__);                                \
    fputc('\n', stderr);                                         \
    return 2;                                                    \
} while (0)

#define T_ASSERT(cond) do {                                      \
    if (!(cond)) T_FAIL("assertion failed: %s", #cond);          \
} while (0)

#define T_ASSERT_EQ_U(a, b) do {                                 \
    unsigned long long _a = (unsigned long long)(a);             \
    unsigned long long _b = (unsigned long long)(b);             \
    if (_a != _b) T_FAIL("expected %s == %s (got %llu, want %llu)", \
                         #a, #b, _a, _b);                        \
} while (0)

#define T_ASSERT_EQ_I(a, b) do {                                 \
    long long _a = (long long)(a);                               \
    long long _b = (long long)(b);                               \
    if (_a != _b) T_FAIL("expected %s == %s (got %lld, want %lld)", \
                         #a, #b, _a, _b);                        \
} while (0)

#define T_ASSERT_EQ_P(a, b) do {                                 \
    const void *_a = (const void *)(a);                          \
    const void *_b = (const void *)(b);                          \
    if (_a != _b) T_FAIL("expected %s == %s (got %p, want %p)",  \
                         #a, #b, _a, _b);                        \
} while (0)

#define T_ASSERT_MEM_EQ(a, b, n) do {                            \
    if (memcmp((a), (b), (n)) != 0)                              \
        T_FAIL("memory differs over %zu bytes", (size_t)(n));    \
} while (0)

#endif /* OPENSUMMONERS_TESTS_T_H */
