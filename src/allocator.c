#include <stdio.h>
#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <string.h>

#include "allocator.h"
#include "treenode.h"

#define POOL_CAPACITY 256000
#define DEFAULT_ARENA_SIZE 5


static inline Pool *
pool_init(Arena *arena);

static inline AllocatorResult 
pool_destroy(Arena *arena, Pool *pool);

static inline AllocatorResult 
pool_reset(Pool *pool);

Arena *
arena_init()
{
    Arena *arena = malloc(sizeof(Arena));
    if (arena == NULL) {
        return NULL;
    }
    arena->size = 0;
    arena->head = NULL;

    for (int i = 0; i < DEFAULT_ARENA_SIZE; i++) {
        if (pool_init(arena) == NULL) {
            arena_destroy(arena);
            return NULL;
        }
    }
    return arena;
}

static inline AllocatorResult _pool_destroy_no_connect(Pool *pool);

AllocatorResult
arena_destroy(Arena *arena)
{
    if (arena == NULL) {
        return ALLOC_RET_ENCOUNTERED_NULLPTR;
    }

    if (arena->head) {
        Pool *current = arena->head;
        while (current) {
            Pool *next = current->next;
            _pool_destroy_no_connect(current);
            current = next;
        }
    }
    
    free(arena);
    return ALLOC_RET_OK;
}

// reset all the pools in the arenas
// and reset the number of pools to default size
AllocatorResult
arena_reset(Arena *arena)
{
    if (arena == NULL || arena->head == NULL) {
        return ALLOC_RET_ENCOUNTERED_NULLPTR;
    }

    Pool *pool = arena->head;

    while (pool) {
        Pool *next = pool->next;
        if (arena->size > DEFAULT_ARENA_SIZE) {
            pool_destroy(arena, pool);
        }
        else {
            pool_reset(pool);
        }
        pool = next;
    }

    assert(arena->size == DEFAULT_ARENA_SIZE);
    return ALLOC_RET_OK;
}

#define POOL_IS_FULL(pool) ((pool)->live_bytes >= POOL_CAPACITY)


static inline void
init_freelist(Pool *pool)
{
    for (int i = 0; i < FREELIST_SIZE; i++) {
        pool->free_list[i] = NULL;
    }
}

static inline Pool *
pool_init(Arena *arena)
{
    if (!arena) {
        return NULL;
    }

    Pool *new_pool = malloc(sizeof(Pool));
    if (new_pool == NULL) {
        return NULL;
    }

    void *base = malloc(POOL_CAPACITY);
    if (base == NULL) {
        free(new_pool);
        return NULL;
    }

    new_pool->base = base;
    new_pool->offset = 0;
    new_pool->live_bytes = 0;
    new_pool->next = NULL;
    init_freelist(new_pool);

    // link the arena
    if (arena->size == 0) {
        arena->head = new_pool;
    }
    else {
        Pool *curr_pool = arena->head;
        assert(curr_pool != NULL);
        for (size_t i = 0; i < arena->size - 1; i++) {
            curr_pool = curr_pool->next;
        }
        assert(curr_pool != NULL);
        curr_pool->next = new_pool;
    }

    arena->size++;
    return new_pool;
}


// auto connects the linked lists
static inline AllocatorResult
pool_destroy(Arena *arena, Pool *pool)
{
    if (arena == NULL || pool == NULL || arena->head == NULL) {
        return ALLOC_RET_ENCOUNTERED_NULLPTR;
    }

    Pool *current = arena->head;
    while (current->next) {
        if (current->next == pool) {
            break;
        }
        current = current->next;
    }
    assert(current != NULL);
    if (current->next != pool) {
        return ALLOC_RET_FAIL;
    }

    current->next = current->next->next;
    free(pool->base);
    free(pool);
    arena->size--;
    return ALLOC_RET_OK;
}

static inline AllocatorResult
pool_reset(Pool *pool)
{
    if (pool == NULL) {
        return ALLOC_RET_ENCOUNTERED_NULLPTR;
    }

    pool->offset = 0;
    pool->live_bytes = 0;
    init_freelist(pool);
    return ALLOC_RET_OK;
}


// does not connect the pools together
static inline AllocatorResult
_pool_destroy_no_connect(Pool *pool)
{
    if (pool == NULL) {
        return ALLOC_RET_ENCOUNTERED_NULLPTR;
    }

    free(pool->base);
    free(pool);
    return ALLOC_RET_OK;
}

static inline size_t
align_up(const size_t value)
{
    const size_t alignment = alignof(max_align_t);
    return value + (alignment - value % alignment) % alignment;
}

static inline int
get_freenode_index(const size_t size)
{
    if (size <= 32)   return 0;
    if (size <= 64)   return 1;
    if (size <= 128)  return 2;
    if (size <= 256)  return 3;
    if (size <= 512)  return 4;
    if (size <= 1024) return 5;
    if (size <= 2048) return 6;

    return 7;
}

static inline void
connect_to_freelist(Pool *pool, FreeNode *node, const size_t size)
{
    assert(pool && node);

    if (size > 2048) {
        FreeNode **root_ptr = &pool->free_list[FREELIST_SIZE - 1];
        assert(root_ptr != NULL);
        bst_insert_node(root_ptr, node, size);
    }
    else {
        const int free_node_index = get_freenode_index(size);
        node->list.prev = NULL;
        node->list.next = NULL;

        FreeNode **head_ptr = &pool->free_list[free_node_index];
        assert(head_ptr != NULL);
        FreeNode *head = *head_ptr;
        if (head != NULL) {
            head->list.prev = node;
            node->list.next = head;
        }

        *head_ptr = node;
    }
}

static inline void
disconnect_from_freelist(Pool *pool, FreeNode *node, const size_t size)
{
    assert(pool != NULL && node != NULL);

    if (size > 2048) {
        FreeNode **root_ptr = &pool->free_list[FREELIST_SIZE - 1];
        assert(root_ptr != NULL);
        bst_pop_node(root_ptr, node, size);
    }
    else {
        const int free_node_index = get_freenode_index(size);

        FreeNode **head = &pool->free_list[free_node_index];
        assert(head != NULL);

        if (node->list.next != NULL) {
            node->list.next->list.prev = node->list.prev;
        }

        if (node == (*head)) {
            *head = node->list.next;
        }
        else {
            // we know that node->prev is definitely not NULL
            assert(node->list.prev != NULL);
            node->list.prev->list.next = node->list.next;
        }
        node->list.prev = NULL;
        node->list.next = NULL;
    }
}


#define GET_HEADER(src) (BlockHeader *)((char *)(src) - sizeof(BlockHeader))
#define GET_NODE(header) (FreeNode *)((char *)(header) + sizeof(BlockHeader))
#define MIN_FREEBLOCK_SIZE (sizeof(BlockHeader) + sizeof(FreeNode))


static inline void
split_free_node(Pool *pool, BlockHeader *header, const size_t size)
{
    if ((header->size - size) < MIN_FREEBLOCK_SIZE) {
        return;
    }
    const size_t new_header_size = header->size - size;
    BlockHeader *new_header = (BlockHeader *)((char *)header + size);
    new_header->size = new_header_size;
    new_header->allocated = false;

    FreeNode *new_node = GET_NODE(new_header);
    connect_to_freelist(pool, new_node, new_header_size);

    header->size = size; // set the original header to the new requested size
}

// finds a free_node that can store size
// and increments live_bytes
static inline void *
reclaim_free_list(Pool *pool, const size_t size)
{
    const int minimum_group_index = get_freenode_index(size);
    for (int i = minimum_group_index; i < FREELIST_SIZE; i++) {
        FreeNode *node = NULL;
        BlockHeader *header = NULL;
        if (i == FREELIST_SIZE - 1) {
            node = bst_get_most_suitable_node(pool->free_list[i], size);
            if (node == NULL) {
                return NULL;
            }
            header = GET_HEADER(node);
        }
        else {
            node = pool->free_list[i];
            while (node) {
                header = GET_HEADER(node);
                if (header->size >= size) {
                    goto success;
                }
                node = node->list.next;
            }
            continue;
        }

        success:
        disconnect_from_freelist(pool, node, header->size);
        split_free_node(pool, header, size);

        header->allocated = true;
        pool->live_bytes += header->size; // this has to come after split_free_node as it modifies 'header's' size
        return (void *)node;
    }

    return NULL;
}


static inline void *
_arena_malloc(Arena *arena, Pool *pool, const size_t size)
{
    if (pool == NULL) {
        return NULL;
    }

    const size_t block_size = align_up(size + sizeof(BlockHeader));

    void *ret = NULL;
    if (block_size + pool->offset > POOL_CAPACITY) {
        // try to reclaim the memory for freed lists
        ret = reclaim_free_list(pool, block_size);
    }
    else {
        BlockHeader *header = (BlockHeader *)((char *)pool->base + pool->offset);
        header->size = block_size;
        header->allocated = true;

        ret = ((char *)pool->base) + pool->offset + sizeof(BlockHeader);
        pool->offset += block_size;
        pool->live_bytes += block_size;
    }
    if (POOL_IS_FULL(pool) && pool->next == NULL) {
        pool_init(arena);
    }

    return ret;
}

void *
arena_malloc(Arena *arena, const size_t size)
{
    if (arena == NULL || size == 0) {
        return NULL;
    }

    if (size > POOL_CAPACITY) {
        fprintf(stderr, "ERROR: %zu > POOL_CAPACITY (%d)\n", size, POOL_CAPACITY);
        return NULL;
    }


    Pool *curr_pool = arena->head;
    while (curr_pool) {
        void *ret = _arena_malloc(arena, curr_pool, size);
        if (ret) {
            return ret;
        }
        curr_pool = curr_pool->next;
    }
    
    // all other pools have not enough space
    // allocate new pool
    Pool *new_pool = pool_init(arena);
    if (!new_pool) {
        return NULL;
    }

    return _arena_malloc(arena, new_pool, size);
}

// frees the pool if the number of pools
// in the arena is more than the default size
// If not, resets the pool
static inline bool
cleanup_pool_if_needed(Arena *arena, Pool *pool)
{
    assert(arena && pool);
    if (pool->live_bytes != 0) {
        return false;
    }

    if (arena->size > DEFAULT_ARENA_SIZE) {
        pool_destroy(arena, pool);
    }
    else {
        pool_reset(pool);
    }
    return true;
}


static inline Pool *
find_pool(const Arena *arena, const void *src)
{
    Pool *pool = arena->head;
    while (pool) {
        assert(pool->base != NULL);
        uintptr_t src_addr = (uintptr_t)src;
        uintptr_t pool_start = (uintptr_t)pool->base;
        uintptr_t pool_end = pool_start + pool->offset;

        if (src_addr >= pool_start && src_addr < pool_end) {
            return pool;
        }

        pool = pool->next;
    }
    return NULL;
}

static inline void
coalesce_freeblocks(Pool *pool, BlockHeader *header)
{
    assert(pool != NULL && header != NULL);

    uintptr_t next_addr =
        (uintptr_t)((char *)header + header->size);

    uintptr_t pool_end =
        (uintptr_t)((char *)pool->base + pool->offset);

    if (next_addr >= pool_end) {
        return;
    }

    BlockHeader *adjacent_header = (BlockHeader *)next_addr;
    if (adjacent_header->allocated == true) {
        return;
    }

    FreeNode *adjacent_freenode = GET_NODE(adjacent_header);
    disconnect_from_freelist(pool, adjacent_freenode, adjacent_header->size);

    header->size += adjacent_header->size;
}

static inline void
_free_from_pool(Arena *arena, Pool *pool, void *src)
{
    assert(arena && pool && src);

    BlockHeader *header = GET_HEADER(src);
    header->allocated = false;
    pool->live_bytes -= header->size; // decrement the header size before coalsecing which may increase header->size

    coalesce_freeblocks(pool, header);

    FreeNode *node = (FreeNode *)src;

    if (!cleanup_pool_if_needed(arena, pool)) {
        connect_to_freelist(pool, node, header->size);
    }
}

void
arena_free(Arena *arena, void *src)
{
    if (src == NULL) {
        return;
    }

    Pool *pool = find_pool(arena, src);
    if (pool == NULL) {
        return;
    }

    _free_from_pool(arena, pool, src);
}

void *
arena_realloc(Arena *arena, void *src, const size_t new_size)
{
    if (arena == NULL || src == NULL) {
        return NULL;
    }

    if (new_size == 0) {
        arena_free(arena, src);
        return NULL;
    }

    BlockHeader *header = GET_HEADER(src);
    assert(header->allocated == true);

    const size_t actual_size = header->size - sizeof(BlockHeader);
    if (new_size <= actual_size) {
        return src;
    }

    Pool *pool = find_pool(arena, src);
    if (pool == NULL) {
        return NULL;
    }

    const uintptr_t pool_used_ptr = (uintptr_t)((char *)pool->base + pool->offset);
    const uintptr_t src_end_ptr = (uintptr_t)((char *)header + header->size);

    if (pool_used_ptr == src_end_ptr) {
        const size_t new_block_size = align_up(new_size + sizeof(BlockHeader));
        const size_t additional = new_block_size - header->size;
        if ((additional) + pool->offset <= POOL_CAPACITY) {
            pool->offset += additional;
            pool->live_bytes += additional;
            header->size = new_block_size;

            return src;
        }
    }
    else if (!POOL_IS_FULL(pool)) {
        BlockHeader *adjacent_header = (BlockHeader *)(src_end_ptr);
        if (adjacent_header->allocated == false && 
            (header->size + adjacent_header->size - sizeof(BlockHeader)) >= new_size
        ) {
            FreeNode *free_node = GET_NODE(adjacent_header);
            disconnect_from_freelist(pool, free_node, adjacent_header->size);
            
            header->size += adjacent_header->size;
            pool->live_bytes += adjacent_header->size;
            return src;
        }
    }

    void *ret = arena_malloc(arena, new_size);
    if (ret != NULL) {
        memcpy(ret, src, header->size - sizeof(BlockHeader));
        _free_from_pool(arena, pool, src); // allow for reclaimation in the future
    }
    return ret;
}

void
arena_print(const Arena *arena)
{
    if (arena == NULL) {
        return;
    }

    size_t total_offset = 0;
    size_t total_live_bytes = 0;
    Pool *curr = arena->head;
    while (curr) {
        total_offset += curr->offset;
        total_live_bytes += curr->live_bytes;
        curr = curr->next;
    }

    printf("<Arena, size=%zu, head=%p, total_offset=%zu, total_live_bytes=%zu>\n", arena->size, arena->head, total_offset, total_live_bytes);
}


void
pool_print(const Pool *pool)
{
    if (pool == NULL) {
        return;
    }
    printf("<Pool, offset=%zu, live_bytes=%zu capacity=%d, base=%p, next=%p>\n", pool->offset, pool->live_bytes, POOL_CAPACITY, pool->base, pool->next);
}