//--------------------------------------------------------------------------------------------------
// System Programming                       Memory Lab                                   Fall 2021
//
/// @file
/// @brief dynamic memory manager
/// @author Sangjun Son
/// @studid 2016-19516
//--------------------------------------------------------------------------------------------------


// Dynamic memory manager
// ======================
// This module implements a custom dynamic memory manager.
//
// Heap organization:
// ------------------
// The data segment for the heap is provided by the dataseg module. A 'word' in the heap is
// eight bytes.
//
// Implicit free list:
// -------------------
// - minimal block size: 32 bytes (header +footer + 2 data words)
// - h,f: header/footer of free block
// - H,F: header/footer of allocated block
//
// - state after initialization
//
//         initial sentinel half-block                  end sentinel half-block
//                   |                                             |
//   ds_heap_start   |   heap_start                         heap_end       ds_heap_brk
//               |   |   |                                         |       |
//               v   v   v                                         v       v
//               +---+---+-----------------------------------------+---+---+
//               |???| F | h :                                 : f | H |???|
//               +---+---+-----------------------------------------+---+---+
//                       ^                                         ^
//                       |                                         |
//               32-byte aligned                           32-byte aligned
//
// - allocation policies: first, next, best fit
// - block splitting: always at 32-byte boundaries
// - immediate coalescing upon free
//


#include <assert.h>
#include <error.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "dataseg.h"
#include "memmgr.h"

void mm_check(void);

/// @name global variables
/// @{
static void *ds_heap_start = NULL;                     ///< physical start of data segment
static void *ds_heap_brk   = NULL;                     ///< physical end of data segment
static void *heap_start    = NULL;                     ///< logical start of heap
static void *heap_end      = NULL;                     ///< logical end of heap
static void *next_block    = NULL;
static int  PAGESIZE       = 0;                        ///< memory system page size
static void *(*get_free_block)(size_t) = NULL;         ///< get free block for selected allocation policy
static int  mm_initialized = 0;                        ///< initialized flag (yes: 1, otherwise 0)
static int  mm_loglevel    = 0;                        ///< log level (0: off; 1: info; 2: verbose)
/// @}

/// @name Macro definitions
/// @{
#define MAX(a, b)          ((a) > (b) ? (a) : (b))     ///< MAX function

#define TYPE               unsigned long               ///< word type of heap
#define TYPE_SIZE          sizeof(TYPE)                ///< size of word type

#define ALLOC              1                           ///< block allocated flag
#define FREE               0                           ///< block free flag
#define STATUS_MASK        ((TYPE)(0x7))               ///< mask to retrieve flagsfrom header/footer
#define SIZE_MASK          (~STATUS_MASK)              ///< mask to retrieve size from header/footer

#define CHUNKSIZE          (1*(1 << 12))               ///< size by which heap is extended

#define BS                 32                          ///< minimal block size. Must be a power of 2
#define BS_MASK            (~(BS-1))                   ///< alignment mask

#define WORD(p)            ((TYPE)(p))                 ///< convert pointer to TYPE
#define PTR(w)             ((void*)(w))                ///< convert TYPE to void*

#define PREV_PTR(p)        ((p)-TYPE_SIZE)             ///< get pointer to word preceeding p

#define PACK(size,status)  ((size) | (status))         ///< pack size & status into boundary tag
#define SIZE(v)            (v & SIZE_MASK)             ///< extract size from boundary tag
#define STATUS(v)          (v & STATUS_MASK)           ///< extract status from boundary tag

#define GET(p)             (*(TYPE*)(p))               ///< read word at *p
#define GET_SIZE(p)        (SIZE(GET(p)))              ///< extract size from header/footer
#define GET_STATUS(p)      (STATUS(GET(p)))            ///< extract status from header/footer

// TODO add more macros as needed
#define PUT(p, v)          (*(TYPE*)(p) = (TYPE)(v))   ///< write data v at address p
#define NEXT_PTR(p)        ((p)+TYPE_SIZE)             ///< get pointer to word succeeding p
#define END(p)             (PREV_PTR(p+GET_SIZE(p)))   ///< get end pointer of the block whose start is p
#define START(p)           (NEXT_PTR(p-GET_SIZE(p)))   ///< get start pointer of the block whose end is p
#define INITCHUNK          (CHUNKSIZE << 4)            ///< initial chunk size

/// @brief print a log message if level <= mm_loglevel. The variadic argument is a printf format
///        string followed by its parametrs
#ifdef DEBUG
  #define LOG(level, ...) mm_log(level, __VA_ARGS__)

/// @brief print a log message. Do not call directly; use LOG() instead
/// @param level log level of message.
/// @param ... variadic parameters for vprintf function (format string with optional parameters)
static void mm_log(int level, ...)
{
  if (level > mm_loglevel) return;

  va_list va;
  va_start(va, level);
  const char *fmt = va_arg(va, const char*);

  if (fmt != NULL) vfprintf(stdout, fmt, va);

  va_end(va);

  fprintf(stdout, "\n");
}

#else
  #define LOG(level, ...)
#endif

/// @}


/// @name Program termination facilities
/// @{

/// @brief print error message and terminate process. The variadic argument is a printf format
///        string followed by its parameters
#define PANIC(...) mm_panic(__func__, __VA_ARGS__)

/// @brief print error message and terminate process. Do not call directly, Use PANIC() instead.
/// @param func function name
/// @param ... variadic parameters for vprintf function (format string with optional parameters)
static void mm_panic(const char *func, ...)
{
  va_list va;
  va_start(va, func);
  const char *fmt = va_arg(va, const char*);

  fprintf(stderr, "PANIC in %s%s", func, fmt ? ": " : ".");
  if (fmt != NULL) vfprintf(stderr, fmt, va);

  va_end(va);

  fprintf(stderr, "\n");

  exit(EXIT_FAILURE);
}
/// @}


static void* ff_get_free_block(size_t);
static void* nf_get_free_block(size_t);
static void* bf_get_free_block(size_t);

void mm_init(AllocationPolicy ap)
{
  LOG(1, "mm_init()");

  //
  // set allocation policy
  //
  char *apstr;
  switch (ap) {
    case ap_FirstFit: get_free_block = ff_get_free_block; apstr = "first fit"; break;
    case ap_NextFit:  get_free_block = nf_get_free_block; apstr = "next fit";  break;
    case ap_BestFit:  get_free_block = bf_get_free_block; apstr = "best fit";  break;
    default: PANIC("Invalid allocation policy.");
  }
  LOG(2, "  allocation policy       %s\n", apstr);

  //
  // retrieve heap status and perform a few initial sanity checks
  //
  ds_heap_stat(&ds_heap_start, &ds_heap_brk, NULL);
  PAGESIZE = ds_getpagesize();    

  LOG(2, "  ds_heap_start:          %p\n"
         "  ds_heap_brk:            %p\n"
         "  PAGESIZE:               %d\n",
         ds_heap_start, ds_heap_brk, PAGESIZE);

  if (ds_heap_start == NULL) PANIC("Data segment not initialized.");
  if (ds_heap_start != ds_heap_brk) PANIC("Heap not clean.");
  if (PAGESIZE == 0) PANIC("Reported pagesize == 0.");

  //
  // initialize heap
  //
  ds_sbrk(INITCHUNK);
  ds_heap_brk = ds_sbrk(0);
  LOG(2, "  allocated memory, new ds_heap_brk is at %p", ds_heap_brk);
  if (ds_heap_brk == (void*)-1) PANIC("Cannot extend heap.");

  heap_start = ds_heap_start;
  LOG(2, "  old heap_start is at %p", heap_start);
  next_block = heap_start = (TYPE*)(((TYPE)heap_start + BS) / BS * BS);
  LOG(2, "  new heap_start is at %p", heap_start);
  PUT(PREV_PTR(heap_start), PACK(0, ALLOC));
  
  heap_end = ds_heap_brk;
  LOG(2, "  old heap_end is   at %p", heap_end);
  heap_end = (TYPE*)(((TYPE)heap_end - 1) / BS * BS);
  LOG(2, "  new heap_start is at %p", heap_end);
  PUT(heap_end, PACK(0, ALLOC));
  
  size_t size = heap_end - heap_start;
  TYPE bdrtag = PACK(size, FREE);
  PUT(heap_start, bdrtag);
  PUT(PREV_PTR(heap_end), bdrtag);

  //
  // heap is initialized
  //
  mm_initialized = 1;
}


void* mm_malloc(size_t size)
{
  LOG(1, "mm_malloc(0x%lx (%ld))", size, size);

  assert(mm_initialized);
  size_t req_size = ceil((double)(size + 2*TYPE_SIZE) / BS) * BS;
  
  void *start_alloc = get_free_block(req_size);
  if (start_alloc == NULL) {
    LOG(1, "expand_heap(0x%lx (%ld))", req_size, req_size);
    
    void *p = heap_end;
    size_t size = (GET_STATUS(PREV_PTR(p)) == FREE) ? GET_SIZE(PREV_PTR(p)) : 0;
    LOG(2, "  last block");
    LOG(2, "    header               %p", START(PREV_PTR(p)));
    LOG(2, "    footer               %p", PREV_PTR(p));
    LOG(2, "    size                 %lx (%ld)", GET_SIZE(PREV_PTR(p)), GET_SIZE(PREV_PTR(p)));
    LOG(2, "    status               %d\n", GET_STATUS(PREV_PTR(p)));
    
    size_t chunk_size = ceil((double)(req_size - size + 1) / CHUNKSIZE) * CHUNKSIZE;
    if (chunk_size < INITCHUNK)
      chunk_size = INITCHUNK;
    
    LOG(2, "   increment heap by 0x%lx (%ld) bytes", chunk_size, chunk_size);
    LOG(1, "ds_sbrk(+0x%lx)", chunk_size);
    ds_sbrk(chunk_size);
    heap_end = ds_heap_brk = ds_sbrk(0);
    heap_end = (TYPE*)(((TYPE)heap_end - 1) / BS * BS);

    LOG(1, "ds_sbrk(+0x%lx)", 0);
    LOG(2, "  new heap_end at %p", heap_end);
    
    LOG(1, "coalesce()");
    LOG(2, "  coalescing with preceeding block");
    
    size += (size_t)(heap_end - p);
    start_alloc = heap_end - size;
    LOG(2, "  last block now at %p with size 0x%lx (%ld) bytes", start_alloc, size, size);
    
    TYPE bdrtag = PACK(size, FREE);
    PUT(start_alloc, bdrtag);
    PUT(PREV_PTR(heap_end), bdrtag);
    PUT(heap_end, PACK(0, ALLOC));
    
    next_block = start_alloc;
  }
  size_t prev_size = GET_SIZE(start_alloc);
  void *end_alloc = PREV_PTR(start_alloc + req_size);
  
  TYPE bdrtag_alloc = PACK(req_size, ALLOC);
  PUT(start_alloc, bdrtag_alloc);
  PUT(end_alloc,   bdrtag_alloc);
  
  size_t cur_size = prev_size - req_size;
  if (cur_size) {
    void *start_free = start_alloc + req_size;
    void *end_free = PREV_PTR(start_free + cur_size);

    TYPE bdrtag_free = PACK(cur_size, FREE);
    PUT(start_free, bdrtag_free);
    PUT(end_free,   bdrtag_free);
  }
  
  return start_alloc + TYPE_SIZE;
}

void* mm_calloc(size_t nmemb, size_t size)
{
  LOG(1, "mm_calloc(0x%lx, 0x%lx)", nmemb, size);

  assert(mm_initialized);

  //
  // calloc is simply malloc() followed by memset()
  //
  void *payload = mm_malloc(nmemb * size);

  if (payload != NULL) memset(payload, 0, nmemb * size);

  return payload;
}

void* mm_realloc(void *ptr, size_t size)
{
  LOG(1, "mm_realloc(%p, 0x%lx)", ptr, size);
  assert(mm_initialized);

  if (ptr == NULL) return mm_malloc(size);
  mm_free(ptr);
  
  void *(*prev_get_free_block)(size_t) = get_free_block;
  void *prev_next_block = next_block;
  
  get_free_block = nf_get_free_block;
  next_block = PREV_PTR(ptr);
  
  void *new_ptr = mm_malloc(size);
  if (new_ptr != ptr) {
    memcpy(new_ptr, ptr, GET_SIZE(next_block) - 2*TYPE_SIZE);
  }
  
  get_free_block = prev_get_free_block;
  next_block = prev_next_block;
  
  return new_ptr;
}

void mm_free(void *ptr)
{
  LOG(1, "mm_free(%p)", ptr);
  if (ptr == NULL) return;
  assert(mm_initialized);
  
  ptr = PREV_PTR(ptr);
  
  LOG(1, "coalesce()");
  TYPE bdrtag = PACK(GET_SIZE(ptr), FREE);
  PUT(ptr, bdrtag);
  PUT(END(ptr), bdrtag);
  
  // preceeding free block
  if (ptr != heap_start && GET_STATUS(PREV_PTR(ptr)) == FREE) {
    LOG(2, "  coalescing with preceeding block");
    void *prev_ptr = START(PREV_PTR(ptr));
    size_t size = GET_SIZE(prev_ptr) + GET_SIZE(ptr);
    TYPE bdrtag = PACK(size, FREE);
    PUT(prev_ptr, bdrtag);
    PUT(END(prev_ptr), bdrtag);
    ptr = prev_ptr;
    
    next_block = prev_ptr;
  }
  
  // succeeding free block
  if (NEXT_PTR(END(ptr)) != heap_end && GET_STATUS(NEXT_PTR(END(ptr))) == FREE) {
    LOG(2, "  coalescing with succeeding block");
    void *next_ptr = NEXT_PTR(END(ptr));
    TYPE bdrtag = PACK(GET_SIZE(next_ptr) + GET_SIZE(ptr), FREE);
    PUT(ptr, bdrtag);
    PUT(END(ptr), bdrtag);
    
    next_block = next_ptr;
    
    if (END(ptr) == PREV_PTR(heap_end)) {
      LOG(1, "shrink_heap(0x%lx (%ld))", INITCHUNK, INITCHUNK);

      void *p = PREV_PTR(heap_end);
      size_t size = (GET_STATUS(p) == FREE) ? GET_SIZE(p) : 0;
      LOG(2, "  last block");
      LOG(2, "    header               %p", START(p));
      LOG(2, "    footer               %p", p);
      LOG(2, "    size                 %lx (%ld)", GET_SIZE(p), GET_SIZE(p));
      LOG(2, "    status               %d\n", GET_STATUS(p));
      
      next_block = START(p);

      size_t chunk_size = size > INITCHUNK ? ((size - INITCHUNK) / CHUNKSIZE) * CHUNKSIZE : 0;
      if (chunk_size) {
        LOG(2, "   decrement heap by 0x%lx (%ld) bytes", chunk_size, chunk_size);
        LOG(1, "ds_sbrk(-0x%lx)", chunk_size);

        void *last_block = START(PREV_PTR(heap_end));

        ds_sbrk(-chunk_size);
        heap_end = ds_heap_brk = ds_sbrk(0);
        heap_end = (TYPE*)(((TYPE)heap_end - 1) / BS * BS);
        size = heap_end - last_block;

        TYPE bdrtag = PACK(size, FREE);
        PUT(last_block, bdrtag);
        PUT(PREV_PTR(heap_end), bdrtag);
        PUT(heap_end, PACK(0, ALLOC));

        LOG(1, "ds_sbrk(+0x%lx)", 0);
        LOG(2, "  new heap_end at %p", heap_end);
        LOG(2, "  last block now at %p with size 0x%lx (%ld) bytes", last_block, size, size);
        
        next_block = last_block;
      }
    }
  }
  
}

/// @name block allocation policites
/// @{

/// @brief find and return a free block of at least @a size bytes (first fit)
/// @param size size of block (including header & footer tags), in bytes
/// @retval void* pointer to header of large enough free block
/// @retval NULL if no free block of the requested size is avilable
static void* ff_get_free_block(size_t size)
{
  LOG(1, "ff_get_free_block(1x%lx (%lu))", size, size);
  assert(mm_initialized);
  void *p = heap_start;
  int N = 1;
  
  LOG(2, "  starting search at %p", p);
  while (p < heap_end) {
    LOG(2, "    %p %d %d", p, GET_SIZE(p), GET_STATUS(p));
    if ((GET_STATUS(p) == FREE) && (GET_SIZE(p) >= size)) break;
    p += GET_SIZE(p);
    N++;
  }
  if (p >= heap_end || GET_STATUS(p) == ALLOC) {
    LOG(2, "    %p %d %d", p, GET_SIZE(p), GET_STATUS(p));
    LOG(1, "  no suitable block found after %d tries.", N);
    return NULL;
  }
  LOG(1, "    --> match after %d tries.", N);
  return p;
}

/// @brief find and return a free block of at least @a size bytes (next fit)
/// @param size size of block (including header & footer tags), in bytes
/// @retval void* pointer to header of large enough free block
/// @retval NULL if no free block of the requested size is avilable
static void* nf_get_free_block(size_t size)
{
  LOG(1, "nf_get_free_block(0x%x (%lu))", size, size);
  assert(mm_initialized);
  void *p = next_block;
  int N = 0;
  LOG(2, "  starting search at %p", p);
  do {
    N++;
    LOG(2, "    %p %ld %d", p, GET_SIZE(p), GET_STATUS(p));
    if ((GET_STATUS(p) == FREE) && (GET_SIZE(p) >= size)) break;
    p = (GET_SIZE(p) > 0) ? p + GET_SIZE(p) : heap_start;
  } while (p != next_block);
  if (p == next_block && ((GET_STATUS(p) == ALLOC) || (GET_SIZE(p) < size))) {
    LOG(1, "  no suitable block found after %d tries.", N);
    return NULL;
  }
  LOG(1, "    --> match after %d tries.", N);
  return next_block = p;
}

/// @brief find and return a free block of at least @a size bytes (best fit)
/// @param size size of block (including header & footer tags), in bytes
/// @retval void* pointer to header of large enough free block
/// @retval NULL if no free block of the requested size is avilable
static void* bf_get_free_block(size_t size)
{
  LOG(1, "bf_get_free_block(0x%lx (%lu))", size, size);
  assert(mm_initialized);
  void *p = heap_start;
  void *best = NULL;
  int N = 1;
  
  LOG(2, "  starting search at %p", p);
  while (p < heap_end) {
    LOG(2, "    %p %ld %d", p, GET_SIZE(p), GET_STATUS(p));
    if ((GET_STATUS(p) == FREE) && (GET_SIZE(p) >= size)) {
      if (best == NULL || GET_SIZE(best) > GET_SIZE(p)) {
        best = p;
      }
    } 
    p += GET_SIZE(p);
    N++;
  }
  LOG(2, "    %p %ld %d", p, GET_SIZE(p), GET_STATUS(p));
  if (best == NULL) {
    LOG(1, "  no suitable block found after %d tries.", N);
    return NULL;
  }
  LOG(1, "  returning %p (size: %ld) after %d tries.", best, GET_SIZE(best), N);
  return best;
}

/// @}

void mm_setloglevel(int level)
{
  mm_loglevel = level;
}


void mm_check(void)
{
  assert(mm_initialized);

  void *p;
  char *apstr;
  if (get_free_block == ff_get_free_block) apstr = "first fit";
  else if (get_free_block == nf_get_free_block) apstr = "next fit";
  else if (get_free_block == bf_get_free_block) apstr = "best fit";
  else apstr = "invalid";

  LOG(2, "  allocation policy    %s\n", apstr);
  printf("\n----------------------------------------- mm_check ----------------------------------------------\n");
  printf("  ds_heap_start:          %p\n", ds_heap_start);
  printf("  ds_heap_brk:            %p\n", ds_heap_brk);
  printf("  heap_start:             %p\n", heap_start);
  printf("  heap_end:               %p\n", heap_end);
  printf("  allocation policy:      %s\n", apstr);
  printf("  next_block:             %p\n", next_block);   // this will be needed for the next fit policy

  printf("\n");
  p = PREV_PTR(heap_start);
  printf("  initial sentinel:       %p: size: %6lx (%7ld), status: %s\n",
         p, GET_SIZE(p), GET_SIZE(p), GET_STATUS(p) == ALLOC ? "allocated" : "free");
  p = heap_end;
  printf("  end sentinel:           %p: size: %6lx (%7ld), status: %s\n",
         p, GET_SIZE(p), GET_SIZE(p), GET_STATUS(p) == ALLOC ? "allocated" : "free");
  printf("\n");
  printf("  blocks:\n");

  long errors = 0;
  p = heap_start;
  while (p < heap_end) {
    TYPE hdr = GET(p);
    TYPE size = SIZE(hdr);
    TYPE status = STATUS(hdr);
    printf("    %p: size: %6lx (%7ld), status: %s\n", 
           p, size, size, status == ALLOC ? "allocated" : "free");

    void *fp = p + size - TYPE_SIZE;
    TYPE ftr = GET(fp);
    TYPE fsize = SIZE(ftr);
    TYPE fstatus = STATUS(ftr);

    if ((size != fsize) || (status != fstatus)) {
      errors++;
      printf("    --> ERROR: footer at %p with different properties: size: %lx, status: %lx\n", 
             fp, fsize, fstatus);
    }

    p = p + size;
    if (size == 0) {
      printf("    WARNING: size 0 detected, aborting traversal.\n");
      break;
    }
  }

  printf("\n");
  if ((p == heap_end) && (errors == 0)) printf("  Block structure coherent.\n");
  printf("-------------------------------------------------------------------------------------------------\n");
}
