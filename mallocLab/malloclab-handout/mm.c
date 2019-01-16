/*
 This is an explicit free list which uses a doubly linked linked list to
 malloc and free memory locations for pointers in an efficient manner. This program
 will achieve 86/100 performance points. To increase performance poinst we could use
 a segregated freeing list, however for the lab specs this is sufficient. When there
 is room for the malloced amount, but not enough continuous space in memory the coalesce
 function arranges pointers and creates continous space. This program is built to work
 on 32 bit machines where pointers are 4 bytes. While this is much more efficient than
 an implicit free list it takes more memory as at each malloc call no matter the size
 we have to store room for a header, footer, and two pointers. Further details on the
 specifics can be read in the function descriptions.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "memlib.h"
#include "mm.h"


team_t team = {
	/* Team name */
	"Blue Baboons",
	/* First member's full name */
	"Ellie Van De Graaff",
	/* First member's email address */
	"elliekvdg@gmail.com",
	/* Second member's full name (leave blank if none) */
	"Jason Messer",
	/* Second member's email address (leave blank if none) */
	"achilles446@gmail.com",
};

//* Basic constants and macros: */
#define WSIZE      sizeof(void *) /* Word and header/footer size (bytes) */
#define DSIZE      (2 * WSIZE)    /* Doubleword size (bytes) */
#define CHUNKSIZE  (1 << 12)      /* Extend heap by this amount (bytes) */

/*Max value of 2 values*/
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p. */
#define GET(p)       (*(uintptr_t *)(p))
#define PUT(p, val)  (*(uintptr_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)   (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)  (GET(p) & 0x1)


/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)  ((void *)(bp) - WSIZE)
#define FTRP(bp)  ((void *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((void *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp)  ((void *)(bp) - GET_SIZE((void *)(bp) - DSIZE))

/* Given ptr in free list, get next and previous ptr in the list */
/* bp is address of the free block. Since minimum Block size is 16 bytes,
 we utilize to store the address of previous block pointer and next block pointer.
 */
#define GET_NEXT_PTR(bp)  (*(char **)(bp + WSIZE))
#define GET_PREV_PTR(bp)  (*(char **)(bp))

/* Puts pointers in the next and previous elements of free list */
#define SET_NEXT_PTR(bp, qp) (GET_NEXT_PTR(bp) = qp)
#define SET_PREV_PTR(bp, qp) (GET_PREV_PTR(bp) = qp)

/* Global declarations */
static char *heap_listp = 0;
static char *free_list_start = 0;

/* Function prototypes for internal helper routines */
static void *coalesce(void *ptr);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *ptr, size_t asize);

/* Function prototypes for maintaining free list*/
static void insert_in_free_list(void *ptr);
static void remove_from_free_list(void *ptr);

/* Function prototypes for heap consistency checker routines: */
static void checkblock(void *ptr);
static void checkheap(bool verbose);
static void printblock(void *ptr);

/**
 * Initialize the memory manager.
 * @param - void no parameter passed in
 * @return - int 0 for success or -1 for failure
 */

 /* Create the initial empty heap wherein we can begin memory allocation */
int mm_init(void){

  if ((heap_listp = mem_sbrk(8 * WSIZE)) == NULL){ //line:vm:mm:begininit
    return -1;
  }
  PUT(heap_listp, 0);                            /* Alignment padding */
  PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
  PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
  PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
  free_list_start = heap_listp + (2 * WSIZE);    //line:vm:mm:endinit
  /* $end mminit */
  /* $begin mminit */

  /* Extend the empty heap with a free block of minimum bytes */
  if (extend_heap(4) == NULL){
    return -1;
  }
  return 0;
}

/*
 The malloc function takes a size that the user inputs and then in essence
 goes to memory and allocates that memory for the user to store what they will.
 If the user inputs a 0 then the function will simply return, however if not the
 malloc function will allocate memory and return the address of the block so that
 the user can access it with a poinetr.
 */
void *mm_malloc(size_t size){
  size_t asize;      /* Adjusted block size */
  size_t extendsize; /* Amount to extend heap if no fit */
  void *bp;
  if (size == 0){
    return NULL;
  }

  /* Adjust block size to include overhead and alignment reqs. */
  if (size <= DSIZE){                                          //line:vm:mm:sizeadjust1
    asize = 2 * DSIZE;                                        //line:vm:mm:sizeadjust2
  }
  else{
    asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE); //line:vm:mm:sizeadjust3
  }
  /* Search the free list for a fit */
  if ((bp = find_fit(asize)) != NULL) {  //line:vm:mm:findfitcall
    place(bp, asize);                  //line:vm:mm:findfitplace
    return bp;
  }

  /* No fit found. Get more memory and place the block */
  extendsize = MAX(asize, CHUNKSIZE);                 //line:vm:mm:growheap1
  if ((bp = extend_heap(extendsize / WSIZE)) == NULL){
    return NULL;                                  //line:vm:mm:growheap2
  }
  place(bp, asize);                                 //line:vm:mm:growheap3
  return (bp);
}
/*
 The free function is the counterpart to the malloc function. The free function takes
 in a pointer passed in by the function call and goes to memory at that location
 and frees the memory which is this case really just changes the pointers in the
 doubly linked list, which "free's a block" of memory.
 */
void mm_free(void *bp){
  size_t size;
  if (bp == NULL){
    return;
  }
  size = GET_SIZE(HDRP(bp));
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  coalesce(bp);
}

void *mm_realloc(void *ptr, size_t size){
  if((int)size < 0){
        return NULL;
  }
  else if((int)size == 0){
        mm_free(ptr);
        return NULL;
    }
    else if(size > 0){
        size_t oldsize = GET_SIZE(HDRP(ptr));
        size_t newsize = size + 2 * WSIZE; // 2 words for header and footer
        if(newsize <= oldsize){
            return ptr;
        }
        else {
            size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
            size_t othersize;
            if(!next_alloc && ((othersize = oldsize + GET_SIZE(HDRP(NEXT_BLKP(ptr))))) >= newsize){
                remove_from_free_list(NEXT_BLKP(ptr));
                PUT(HDRP(ptr), PACK(othersize, 1));
                PUT(FTRP(ptr), PACK(othersize, 1));
                return ptr;
            }
            else {
                void *new_ptr = mm_malloc(newsize);
                place(new_ptr, newsize);
                memcpy(new_ptr, ptr, newsize);
                mm_free(ptr);
                return new_ptr;
            }
        }
    }
    else{
        return NULL;

    }
}
/*
 The coalesce function takes smaller unused memory blocks and combines them
 into one contiguous block of memory. It has 3 differnet cases wherein different
 procedues need to be done to coalesce. The cases are if there is no empty space, if
 there is empty space to the right, and then finally if there is empty space to the left.
 Each case does something different and returns the address of the new cotiguous block of
 memory.
 */
static void *coalesce(void *bp){
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;
  size_t size = GET_SIZE(HDRP(bp));

  if (!prev_alloc && !next_alloc) {            /* Case 1 */
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
    remove_from_free_list(PREV_BLKP(bp));
    remove_from_free_list(NEXT_BLKP(bp));
    bp = PREV_BLKP(bp);
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }

  else if (prev_alloc && !next_alloc) {      /* Case 2 */
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    remove_from_free_list(NEXT_BLKP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size,0));
  }

  else if (!prev_alloc && next_alloc) {      /* Case 3 */
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    bp = PREV_BLKP(bp);
    remove_from_free_list(bp);
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }

  /* $begin mmfree */
  insert_in_free_list(bp);
  return bp;
}
/*
 This function simply extends the heap, or grabs another "page" of memory
 wherin we can write to. This is done when the block we want to malloc is bigger
 than any contiguous block and coalescing wont be able to create a big enough block
 */
static void *extend_heap(size_t words){
  char *bp;
  size_t size;
  size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; 

  if(size < 16){
    size = 16;
  }

  /* Allocate an even number of words to maintain alignment */
  if ((int)(bp = mem_sbrk(size)) == -1){
    return NULL;
  }
  /* Initialize free block header/footer and the epilogue header */
  PUT(HDRP(bp), PACK(size, 0));         /* Free block header */  
  PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */ 
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ 

  /* Coalesce if the previous block was free */
  return coalesce(bp);                                        
}
/*
 In a way the find_fit function is a helper function that takes the size of the blocl
 that the person wants to malloc and goes and finds a location where there is enough
 space. If there is not enough space the function will call extend heap to make space.
 If it is absolutely not possible the function will return NULL, otherwise it will return
 the empty blocks address
 */
static void *find_fit(size_t asize){
  void* pointer;
  static int last_size = 0;
  static int counter = 0;

  if(last_size == (int)asize){
    if(counter > 30){
      int extend = MAX(asize, 4 * WSIZE);
      pointer = extend_heap(extend / 4);
      return pointer;
    }
    else{
      counter++;
    }
  }
  else{
    counter = 0;
  }
  for(pointer = free_list_start; GET_ALLOC(HDRP(pointer)) == 0; pointer = GET_NEXT_PTR(pointer)){
    if(asize <= (size_t)GET_SIZE(HDRP(pointer))){
      last_size = asize;
      return pointer;
    }
  }
  return NULL; /* No fit */
}
/*
 This is another helper function that splits up the large task of managing a heap
 into smaller parts. This is the part of the function that takes the pointer of the
 empty block and then places at that pointer the item that the user wanted to store there
 */
static void place(void *bp, size_t asize){
  size_t csize = GET_SIZE(HDRP(bp));

  if ((csize - asize) >= (4 * WSIZE)) {
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    remove_from_free_list(bp);
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize-asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
    coalesce(bp);
  }
  else {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
    remove_from_free_list(bp);
  }
}
// Simply inserst a block into a free list
static void insert_in_free_list(void *ptr){
    SET_NEXT_PTR(ptr, free_list_start);
    SET_PREV_PTR(free_list_start, ptr);
    SET_PREV_PTR(ptr, NULL);
    free_list_start = ptr;
}
//Removes a block from a free list
static void remove_from_free_list(void *ptr){
  if (GET_PREV_PTR(ptr)){
        SET_NEXT_PTR(GET_PREV_PTR(ptr), GET_NEXT_PTR(ptr));

  }
  else{
        free_list_start = GET_NEXT_PTR(ptr);
  }
  SET_PREV_PTR(GET_NEXT_PTR(ptr), GET_PREV_PTR(ptr));
}
//Simply checks the block the user wants to input for alignment and a valid header/footer
//Used in the heckHeap function to make sure that everything is working like it should
static void checkblock(void *ptr){
    
  if ((uintptr_t)ptr % DSIZE){
        printf("Error: %p is not doubleword aligned\n", ptr);
  }
  if (GET(HDRP(ptr)) != GET(FTRP(ptr))){
        printf("Error: header does not match footer\n");
    }
}
/*
 This function checks the heap to make sure that we haven't messed anything up
 and that everything is working propely. It is not currently implemented but was
 used for debugging. The checks are made by printing out the contents of memory
 at a location and making sure they match up, and checking to make sure our header and footer
 match the prologue and epilogue.
 */
void checkheap(bool verbose){
    void *ptr;
    
    if (verbose){
        printf("Heap (%p):\n", heap_listp); //Makes sure heap is consistent
    }
	
    if (GET_SIZE(HDRP(heap_listp)) != DSIZE || //Checking to see if header and prologue match
        !GET_ALLOC(HDRP(heap_listp))){
        printf("Bad prologue header\n");
    }
    checkblock(heap_listp); //Checking for header/footer and alignment
    
    for (ptr = heap_listp; GET_SIZE(HDRP(ptr)) > 0; ptr = (void *)NEXT_BLKP(ptr)) {
      if (verbose){
         printblock(ptr);
      }
        checkblock(ptr);
    }
    
    if (verbose){
        printblock(ptr); //Prints out what is at memory location to check for cosistency
    }
    if (GET_SIZE(HDRP(ptr)) != 0 || !GET_ALLOC(HDRP(ptr))){
        printf("Bad epilogue header\n"); //Error message for if heap is messed up 
    }
}
/*
 Simply goes to the block specified by the passed in pointer and outputs the
contents of memory at that location.
 */
static void printblock(void *ptr){
    bool halloc, falloc;
    size_t hsize, fsize;
    
    checkheap(false);
    hsize = GET_SIZE(HDRP(ptr));
    halloc = GET_ALLOC(HDRP(ptr));
    fsize = GET_SIZE(FTRP(ptr));
    falloc = GET_ALLOC(FTRP(ptr));
    
    if (hsize == 0) {
        printf("%p: end of heap\n", ptr);
        return;
    }
    
    printf("%p: header: [%zu:%c] footer: [%zu:%c]\n", ptr,
           hsize, (halloc ? 'a' : 'f'),
           fsize, (falloc ? 'a' : 'f'));
}



