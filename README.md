# About
This a memory allocator that uses a combination of segregated free-lists and a binary search tree. Therefore, this allows for
fast and efficient reclaimation!

> [!NOTE]
> This is ultimately a hobby project :)


## Features
The main functions for this allocator are:
- `Arena *arena_init()`
- `AllocatorResult arena_destroy(Arena *arena)`
- `void *arena_malloc(Arena *arena)`
- `void *arena_realloc(Arena *arena, void *src, const size_t new_size)`
- `void arena_free(Arena *arena, void *src)`

For reclaimation and freeblocks, this allocator implements:
- coalescing of freeblocks
- splitting of freeblocks
- segregated-freelists
- BST searching
for higher performance.


## Example
Here's a little example of how you can use it
```c
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "allocator.h"

int main() {
    Arena *arena = arena_init();
    if (arena == NULL) {
        return -1;
    }

    char *name = arena_malloc(arena, 6);
    if (name == NULL) {
        goto cleanup;
    }

    memcpy(name, "sacul", 6);
    printf("%s", name); // "sacul"

    arena_free(arena, name);
cleanup:
    arena_destroy(arena);
}
```

Want a more complex use case? Here's a little `Array` made using the allocator!

First, run this in the root directory which compiles the example:
```sh
make
```

Then execute it:
```sh
.\example
```
