#ifndef FREELIST_ALLOCATOR_HEADER
#define FREELIST_ALLOCATOR_HEADER

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    ALLOC_RET_FAIL = -1,
    ALLOC_RET_ENCOUNTERED_NULLPTR = -2,
    ALLOC_RET_OK = 1,
} AllocatorResult;


// =========================
//     Pools & Arenas
// =========================

typedef struct {
    size_t size;
    bool allocated;
} BlockHeader;


typedef struct FreeNode {
    union
    {
        struct {
            struct FreeNode *next;
            struct FreeNode *prev;
        } list;

        struct {
            struct FreeNode *left;
            struct FreeNode *right;
        } tree;
    };
} FreeNode;


#define FREELIST_SIZE 8

typedef struct Pool {
    size_t live_bytes;
    size_t offset; 

    void *base;
    struct Pool *next;
    FreeNode *free_list[FREELIST_SIZE];
} Pool;

typedef struct Arena {
    size_t size;
    Pool *head;
} Arena;


Arena *arena_init();
AllocatorResult arena_reset(Arena *arena);
AllocatorResult arena_destroy(Arena *arena);

void *arena_malloc(Arena *arena, const size_t size);
void *arena_realloc(Arena *arena, void *src, const size_t new_size);
void arena_free(Arena *arena, void *src);

void arena_print(const Arena *arena);
void pool_print(const Pool *pool);

#endif
