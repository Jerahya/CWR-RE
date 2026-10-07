#pragma once

// Switch: minimal mimalloc API on newlib malloc.
//
// mimalloc has no libnx port. The engine's JimboAllocator only needs the calls
// below, so on Switch this header shadows the real one (include path set in
// engine/Poseidon/CMakeLists.txt). newlib's dlmalloc is thread-safe under libnx.

#include <malloc.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef struct mi_heap_s mi_heap_t;

static inline mi_heap_t* mi_heap_get_default(void)
{
    // Opaque non-null token; JimboAllocator only passes it back to us.
    static char token;
    return (mi_heap_t*)&token;
}

static inline void* mi_zalloc_aligned(size_t size, size_t alignment)
{
    void* p = memalign(alignment, size ? size : 1);
    if (p)
        memset(p, 0, size);
    return p;
}

static inline size_t mi_usable_size(const void* p)
{
    return p ? malloc_usable_size((void*)p) : 0;
}

static inline bool mi_is_in_heap_region(const void* p)
{
    return p != NULL;
}

static inline void mi_free(void* p)
{
    free(p);
}

static inline bool mi_heap_check_owned(mi_heap_t* heap, const void* p)
{
    (void)heap;
    return p != NULL;
}

static inline void mi_heap_collect(mi_heap_t* heap, bool force)
{
    (void)heap;
    (void)force;
    malloc_trim(0);
}
