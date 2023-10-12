/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*single word size, double word size, chunksize, and segregated list number*/
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)
#define SEG_LIST_NUM 20

/*find out maximum value*/
#define MAX(x, y) ((x)>(y)?(x):(y))

/*get address of p and put the value into address p*/
#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*)(p)=(val))

/*get the size from double word-size, and get whether it was allocated or not*/
#define PACK(size, alloc) ((size)|(alloc))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/*get the header and footer's address*/
#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp)+GET_SIZE(HDRP(bp))-DSIZE)

/*get the next, previous block's address*/
#define NEXT_BLKP(bp) ((char*)(bp)+GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char*)(bp)-GET_SIZE(((char*)(bp)-DSIZE)))

/*get the pred and succ pointer address*/
#define PRED(bp) ((char*)(bp))
#define SUCC(bp) ((char*)(bp)+WSIZE)

/*go to the previous list address and next list address*/
#define PREV_SEG(bp) (*(char**)(bp))
#define NEXT_SEG(bp) (*(char**)(SUCC(bp)))

/*track the address and update that pointer*/
#define UPDATE(p, q) (*(unsigned int*)(p)=(unsigned int)(q))

/*global variable of the first block's pointer*/
static char *heap_listp;
static void *seg_listp;

/*can get the segregated list starting pointers' address by index*/
#define SEG_LIST_ADDRESS(index) ((char**)seg_listp + index)
#define SEG_LIST(index) (*SEG_LIST_ADDRESS(index))

/*function headers*/
static void* extend_heap(size_t words);
static void* coalesce(void *bp);
static void delete_block(void* bp);
static void insert_seg_list(void* bp, size_t size);
static void* find_fit(size_t asize);
static void* place(void* bp, size_t asize);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{   
    int i;

    /*create initial empty heap using mem_sbrk()*/
    if((heap_listp = mem_sbrk((4+SEG_LIST_NUM)*WSIZE))==(void*)-1){
        return -1;
    }

    /*padding, prologue header*/
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(((SEG_LIST_NUM+2)*WSIZE), 1));
    heap_listp += (2*WSIZE);
    
    /*setting every seg list's linked list start address as NULL pointer*/
    seg_listp = heap_listp;
    for(i=0; i<SEG_LIST_NUM; i++){
        SEG_LIST(i) = NULL;
    }

    /*setting prologue footer and the epilogue header*/
    PUT(heap_listp + ((SEG_LIST_NUM)*WSIZE), PACK(((SEG_LIST_NUM+2)*WSIZE), 1));
    PUT(heap_listp + ((SEG_LIST_NUM+1)*WSIZE), PACK(0, 1));

    /*extend heap initialy and if cannot extend, then return -1*/
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL){
        return -1;
    }

    return 0;

}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{

    /*variables*/
    size_t asize;
    size_t extendsize;
    char *bp;

    /*error checking*/
    if (size==0){
        return NULL;
    }

    /*check alignment, set the size*/
    asize = ALIGN(size+DSIZE);

    /*find fit block and place it*/
    bp = find_fit(asize);

    if(bp!=NULL){
        place(bp, asize);
        return bp;
    }

    /*if cannot find fit block, extend the heap memory*/
    extendsize = MAX(asize, CHUNKSIZE);
    if((bp=extend_heap(extendsize/WSIZE))==NULL){
        return NULL;
    }
    place(bp, asize);

    return bp;

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{

    /*reset header and footer*/
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    /*insert new free block and coalesce*/
    insert_seg_list(ptr, size);
    coalesce(ptr);

    return;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
    void *mm_realloc(void *ptr, size_t size)
{
    size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
	    mm_free(ptr);
	    return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(ptr == NULL) {
	    return mm_malloc(size);
    }

	/*allocate new one and check for the error*/
    newptr = mm_malloc(size);
    if(!newptr) {
	    return 0;
    }

    /*get the old one's size and compare*/
    oldsize = GET_SIZE(HDRP(ptr));
    if(size<oldsize){
        oldsize = size;
    }

    /*copy the memory and free*/
    memcpy(newptr, ptr, oldsize);
    mm_free(ptr);

    return newptr;
}

/*function that extends heap by the size words*/
static void* extend_heap(size_t words){

    /*base pointer and the size variable*/
    char *bp;
    size_t size;

    /*check alignment and consider the pred and succ, add 2 more words*/
    size = ALIGN(words*WSIZE);

    /*extending heap area and error checking*/
    if ((long)(bp = mem_sbrk(size+DSIZE)) == -1){
        return NULL;
    }

    /*header, footer and next block's header setting*/
    PUT(HDRP(bp), PACK(size+DSIZE, 0));
    PUT(FTRP(bp), PACK(size+DSIZE, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    /*insert extended block to the seg-list*/
    insert_seg_list(bp, size+DSIZE);

    /*check the blocks nearby and return*/
    return coalesce(bp);
}

/*function that treats coalesce*/
static void* coalesce(void *bp){

    /*get the previous and next block's address and the size of blockself*/
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    /*each cases*/
    if (prev_alloc && next_alloc) {

        /*there's nothing to do with*/
        return bp;
    
    }
    else if (prev_alloc && !next_alloc) {

        /*delete the current one and the next one*/
        delete_block(bp);
        delete_block(NEXT_BLKP(bp));

        /*compute the size and set the new block's header and footer*/
	    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
	    PUT(HDRP(bp), PACK(size, 0));
	    PUT(FTRP(bp), PACK(size, 0));
    
    }
    else if (!prev_alloc && next_alloc) {

        /*delete the current one and the previous one*/
        delete_block(bp);
        delete_block(PREV_BLKP(bp));

        /*compute the size and set the new block's header and footer*/
	    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
	    PUT(FTRP(bp), PACK(size, 0));
	    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
	    bp = PREV_BLKP(bp);

    }
    else {

        /*delete the current one and the previous, next one*/
        delete_block(bp);
        delete_block(PREV_BLKP(bp));
        delete_block(NEXT_BLKP(bp));

        /*compute the size and set the new block's header and footer*/
	    size = ( size + GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))) );
        
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

    }

    /*insert new integrated block into the seg list*/
    insert_seg_list(bp, size);

    return bp;

}

/*function that deletes a block from the list*/
static void delete_block(void* bp){

    /*variables*/
    void* ptr;
    size_t temp = GET_SIZE(HDRP(bp));

    /*finding index*/
    int i;
    for(i=0; i<SEG_LIST_NUM; i++){
        if(temp<=(DSIZE<<1)){
            break;
        }
        temp>>=1;
    }

    ptr = SEG_LIST_ADDRESS(i);

    /*case of previous block is seg-list starting address block*/
    if(PREV_SEG(bp)==ptr){

        /*if the next is a NULL pointer*/
        if(NEXT_SEG(bp)==NULL){
            SEG_LIST(i) = NULL;
        }
        /*if the next is not a NULL pointer*/
        else{
            UPDATE(PRED(NEXT_SEG(bp)), ptr);
            UPDATE(ptr, NEXT_SEG(bp));
        }

    }
    /*case of previous block is not initial block*/
    else{

        /*if the next is a NULL pointer*/
        if(NEXT_SEG(bp)==NULL){
            UPDATE(SUCC(PREV_SEG(bp)), NULL);
        }
        /*if the next is not a NULL pointer*/
        else{
            UPDATE(SUCC(PREV_SEG(bp)), NEXT_SEG(bp));
            UPDATE(PRED(NEXT_SEG(bp)), PREV_SEG(bp));
        }

    }

    return;

}

/*function that inserts a block to the list*/
static void insert_seg_list(void* bp, size_t size){

    /*variables*/
    void* prev_bp = NULL;
    void* next_bp = NULL;
    size_t temp = size;

    /*finding index*/
    int i;
    for(i=0; i<SEG_LIST_NUM; i++){
        if(temp<=(DSIZE<<1)){
            break;
        }
        temp>>=1;
    }

    /*If over the index 19, then make it to fit in 19th index*/
    if(i==SEG_LIST_NUM){
        i = SEG_LIST_NUM-1;
    }

    /*using prev_bp and next_bp, search to find the position for the new block*/
    prev_bp=SEG_LIST_ADDRESS(i);
    next_bp=SEG_LIST(i);

    /*insert a block between prev_bp and next_bp*/
    while((prev_bp!=NULL)&&(next_bp!=NULL)&&(size>GET_SIZE(HDRP(next_bp)))){
        prev_bp = next_bp;
        next_bp = NEXT_SEG(next_bp);
    }

    /*four cases*/
    if(next_bp == NULL){

        /*case of nothing at the list*/
        if(prev_bp == SEG_LIST_ADDRESS(i)){
            SEG_LIST(i)=bp;
        }

        /*case of inserting at tail*/
        else{
            UPDATE(SUCC(prev_bp), bp);
        }

        UPDATE(PRED(bp), prev_bp);
        UPDATE(SUCC(bp), NULL);
    }
    else{

        /*case of inserting at head*/
        if(prev_bp == SEG_LIST_ADDRESS(i)){
            SEG_LIST(i)=bp;
        }

        /*case of inserting at the middle*/
        else{
            UPDATE(SUCC(prev_bp), bp);
        }

        UPDATE(PRED(bp), prev_bp);
        UPDATE(SUCC(bp), next_bp);
        UPDATE(PRED(next_bp), bp);
    }

    return;

}

static void* find_fit(size_t asize){
    
    /*varialbes*/
    size_t temp = asize;
    void* ptr = NULL;
    int i;

    /*check what index is fit into asize*/
    for(i=0; i<SEG_LIST_NUM; i++){

        if(((temp<=(DSIZE<<1)))&&(SEG_LIST(i)!=NULL)){
            ptr = SEG_LIST(i);

            /*search for the block in the seg-list*/
            while((ptr!=NULL)&&(asize>GET_SIZE(HDRP(ptr)))){
                ptr=NEXT_SEG(ptr);
            }
            if(ptr!=NULL){
                break;
            }
        }
        temp>>=1;
    }

    return ptr;
}

/*function that allocate an block and split, if needed*/
static void* place(void* bp, size_t asize){

    /*free block size and the sub*/
    size_t free_bsize = GET_SIZE(HDRP(bp));
    size_t sub=free_bsize-asize;

    /*delete the free block*/
    delete_block(bp);

    /*case 1: just matches to same block*/
    if(sub<(DSIZE<<1)){
        PUT(HDRP(bp), PACK(free_bsize, 1));
        PUT(FTRP(bp), PACK(free_bsize, 1));
    }
    /*case 2: needs to divide*/
    else{
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(sub, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(sub, 0));
        insert_seg_list(NEXT_BLKP(bp), sub);
    }

    return bp;
}
