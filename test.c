#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>

#define ALIGNMENT 8

#define ALIGN_UP(n, align) (((n) + ((align) - 1)) & ~((align) - 1))

typedef struct{
    char *buffer;
    size_t capacity;
    size_t offset;
} Arena;

bool arena_init(Arena *arena, size_t capacity ){
    if(!arena|| capacity==0) return false;

    arena->buffer=(char*)mmap(
        NULL,
        capacity,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    if(arena->buffer ==MAP_FAILED){
        arena->buffer=NULL;
        arena->capacity=0;
        arena->offset=0;

        return false;
    }


    arena->capacity=capacity;
    arena->offset=0;

    return true;
}


void *arena_alloc(Arena *arena, size_t size){
    if(!arena || size==0 || !arena->buffer) return NULL;

    size_t aligned_offset=ALIGN_UP(arena->offset, ALIGNMENT);

    if(aligned_offset +size > arena->capacity || aligned_offset+size< aligned_offset){
        return NULL;
    }

    void *ptr = arena->buffer+aligned_offset;

    arena->offset = aligned_offset + size;

    return ptr;
}

void arena_reset(Arena *arena){
    if(arena){
        arena->offset = 0;
    }


}



void arena_destroy(Arena *arena){
    if(arena && arena->buffer){
        munmap(arena->buffer, arena->capacity);
        arena->buffer = NULL;
        arena->offset = 0;
    }

}

static double get_time_sec(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

#define NUM_ALLOCS 1000000

#define ALLOC_SIZE 32

int main(void){
printf(" benchmarking 1000000 allocations (size = %d bytes) \n", ALLOC_SIZE);


void **ptrs = (void**)malloc(NUM_ALLOCS * sizeof(void *));

if(!ptrs){
    perror("malloc failed for pointers array");
    return 1;

}


double t0 = get_time_sec();


for(size_t i=0; i< NUM_ALLOCS; i++){
    ptrs[i] = malloc(ALLOC_SIZE);
    *(volatile char *)ptrs[i] = (char)i;
}


for(size_t i = 0; i<NUM_ALLOCS; i++){
    free(ptrs[i]);


}


double t1 = get_time_sec();

double malloc_time = t1- t0;

printf("standard malloc / free:  %8.4f s  (%6.1f ns/op)\n",
           malloc_time, (malloc_time / NUM_ALLOCS) * 1e9);
    free(ptrs);


    Arena arena;
    // Align up each allocation to 8 bytes + total capacity needed
    size_t aligned_per_alloc = ALIGN_UP(ALLOC_SIZE, ALIGNMENT);
    size_t total_capacity = NUM_ALLOCS * aligned_per_alloc;
    if (!arena_init(&arena, total_capacity)) {
        perror("arena_init failed");
        return 1;
    }
    double t2 = get_time_sec();
    for (size_t i = 0; i < NUM_ALLOCS; i++) {
        void *p = arena_alloc(&arena, ALLOC_SIZE);
        // Write to touch the page
        *(volatile char *)p = (char)i;
    }

    arena_reset(&arena);

    double t3 = get_time_sec();

    double arena_time=t3-t2;

    printf("mmap Arena (alloc+reset): %8.4f s  (%6.1f ns/op)\n",
           arena_time, (arena_time / NUM_ALLOCS) * 1e9);
    arena_destroy(&arena);

   printf("\nSpeedup: %.2fx faster\n", malloc_time / arena_time);
    return 0;

}



