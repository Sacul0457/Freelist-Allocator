#include "stdlib.h"
#include "stddef.h"
#include "stdio.h"

#include "allocator.h"

#define DEFAULT_ARRAY_CAPACITY 8

typedef struct {
    int *base;
    size_t size;
    size_t capacity;
} Array;


static inline Array *
array_init(Arena *arena)
{
    Array *array = arena_malloc(arena, sizeof(Array));
    if (array == NULL) {
        return NULL;
    }

    int *base = arena_malloc(arena, sizeof(int) * DEFAULT_ARRAY_CAPACITY);
    if (base == NULL) {
        arena_free(arena, array);
        return NULL;
    }

    array->base = base;
    array->capacity = DEFAULT_ARRAY_CAPACITY;
    array->size = 0;
    return array;
}


static inline int
array_resize(Arena *arena, Array *array, const size_t new_count)
{
    int *temp = arena_realloc(arena, array->base, new_count * sizeof(int));
    if (temp == NULL) {
        return -1;
    }
    array->base = temp;
    array->capacity = new_count;
    return 0;
}

static inline int
array_append(Arena *arena, Array *array, int value)
{
    if (arena == NULL || array == NULL) {
        return -1;
    }

    if (array->size == array->capacity) {
        if (array_resize(arena, array, array->capacity * 2) == -1) {
            return -1;
        }
    }
    array->base[array->size] = value;
    array->size++;
    return 0;
}

static inline void
array_destroy(Arena *arena, Array *array)
{
    if (arena == NULL || array == NULL) {
        return;
    }

    arena_free(arena, array->base);
    arena_free(arena, array);
}


static inline void
array_print(const Array *array)
{
    if (array == NULL) {
        return;
    }

    printf("[");
    for (size_t i = 0; i < array->size; i++) {
        printf("%d, ", array->base[i]);
    }
    printf("]\n");
}


int main()
{
    Arena *arena = arena_init();
    if (arena == NULL) {
        return -1;
    }

    Array *array = array_init(arena);
    if (array == NULL) {
        goto cleanup;
    }


    for (int i = 0; i < 10; i++) {
        array_append(arena, array, i);
    }

    arena_print(arena);
    array_print(array);

    array_destroy(arena, array);
    arena_print(arena);

cleanup:
    arena_destroy(arena);
    return 0;
}