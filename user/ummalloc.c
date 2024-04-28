/* Reference: CSAPP, https://blog.csdn.net/weixin_47089544/article/details/124184251 */

#include "kernel/types.h"

//
#include "user/user.h"

//
#include "ummalloc.h"

#define WORDSIZE	4
#define ALIGNMENT	8
#define CHUNKSIZE	(1<<12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc)	((size) | (alloc))

#define GET(p)		(*(unsigned int *)(p))
#define PUT(p, val)	(*(unsigned int *)(p) = val)

#define GET_SIZE(p)		(GET(p) & ~0x7)
#define GET_ALLOC(p)	(GET(p) & 0x1)

#define HDRP(bp)	((char* )(bp) - WORDSIZE)
#define FTRP(bp)	((char* )(bp) + GET_SIZE(HDRP(bp)) - ALIGNMENT)

#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE(((char *)(bp) - WORDSIZE)))
#define PREV_BLKP(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - ALIGNMENT)))

static void *heap_listp;

static void *extend(uint words);
static void *merge(void *ptr);
static void *find_fit(uint asize);
static void place(void *bp, uint asize);

int mm_init(void)
{
    if ((heap_listp = sbrk(4*WORDSIZE)) == (void *)-1)
    	return -1;
    
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WORDSIZE), PACK(ALIGNMENT, 1));
    PUT(heap_listp + (2*WORDSIZE), PACK(ALIGNMENT, 1));
    PUT(heap_listp + (3*WORDSIZE), PACK(0, 1));
    heap_listp += (2*WORDSIZE);

    if (extend(CHUNKSIZE/WORDSIZE) == 0) {
    	return -1;
   	}

    return 0;
}

static void *extend(uint words)
{
	char *bp;
	uint size;

	size = (words % 2) ? (words+1) * WORDSIZE : words * WORDSIZE;
	if ((long)(bp = sbrk(size)) == -1)
		return 0;

	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

	return merge(bp);
}

void mm_free(void *ptr)
{
	uint size = GET_SIZE(HDRP(ptr));
	
	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));
	merge(ptr);
}

static void *merge(void *ptr)
{
	uint prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
	uint next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
	uint size = GET_SIZE(HDRP(ptr));
	
	if (prev_alloc && next_alloc) {
		return ptr;
	} else if (prev_alloc && !next_alloc) {
		size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
		PUT(HDRP(ptr), PACK(size, 0));
		PUT(FTRP(ptr), PACK(size, 0));
	} else if (!prev_alloc && next_alloc) {
		size += GET_SIZE(FTRP(PREV_BLKP(ptr)));
		PUT(FTRP(ptr), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
		ptr = PREV_BLKP(ptr);
	} else if (!prev_alloc && !next_alloc) {
		size += GET_SIZE(HDRP(PREV_BLKP(ptr))) 
			+ GET_SIZE(FTRP(NEXT_BLKP(ptr)));
		PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(ptr)), PACK(size, 0));
		ptr = PREV_BLKP(ptr);
	}
	
	return ptr;
}

void *mm_malloc(uint size)
{
    uint asize;
    uint extenALIGNMENT;
   	char *bp;

   	if (size == 0)
   		return 0;

   	if (size <= ALIGNMENT)
   		asize = 2*ALIGNMENT;
   	else 
   		asize = ALIGNMENT * ((size + (ALIGNMENT) + (ALIGNMENT-1)) / ALIGNMENT);

   	if ((bp = find_fit(asize)) != 0) {
   		place(bp, asize);
   		return bp;
   	}
   	
   	extenALIGNMENT = MAX(asize, CHUNKSIZE);
   	if ((bp = extend(extenALIGNMENT/WORDSIZE)) == 0)
   		return 0;
   	place(bp, asize);

   	return bp;
}

static void *find_fit(uint asize)
{
	void* p;

	for (p = heap_listp; GET_SIZE(HDRP(p)) > 0; p = NEXT_BLKP(p)) {
		if (!GET_ALLOC(HDRP(p)) && (asize <= GET_SIZE(HDRP(p))))
			return p;
	}
	
	return 0;
}

static void place(void *bp, uint asize)
{
	uint size = GET_SIZE(HDRP(bp));
	if (size - asize >= 2*WORDSIZE) {
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(size-asize, 0));
		PUT(FTRP(bp), PACK(size-asize, 0));
	} else {
		PUT(HDRP(bp), PACK(size, 1));
		PUT(FTRP(bp), PACK(size, 1));
	}
}

void *mm_realloc(void *ptr, uint size)
{
    void *oldptr = ptr;
    void *newptr;
	void *nextptr;

	uint blockSize;
    uint extenALIGNMENT;
	uint asize;
	uint sizesum;
	
	if (ptr == 0) {
		return mm_malloc(size);
	} else if (size == 0) {
		mm_free(ptr);
		return 0;
	}

	if (size <= ALIGNMENT)
   		asize = 2*ALIGNMENT;
   	else 
   		asize = ALIGNMENT * ((size + (ALIGNMENT) + (ALIGNMENT-1)) / ALIGNMENT);
	
	blockSize = GET_SIZE(HDRP(ptr));

	
	if (asize == blockSize) {
		return ptr;
	} else if (asize < blockSize) {
		place(ptr, asize);
		return ptr;
	} else {
		nextptr = NEXT_BLKP(ptr);
		sizesum = GET_SIZE(HDRP(nextptr))+blockSize;
		if (!GET_ALLOC(HDRP(nextptr)) && sizesum >= asize) {
			PUT(HDRP(ptr), PACK(sizesum, 0));
			place(ptr, asize);
			return ptr;
		} else {
			newptr = find_fit(asize);
			if (newptr == 0) {
				extenALIGNMENT = MAX(asize, CHUNKSIZE);
				if ((newptr = extend(extenALIGNMENT/WORDSIZE)) == 0) {
					return 0;
				}
			}
			place(newptr, asize);
			memcpy(newptr, oldptr, blockSize-2*WORDSIZE);
			mm_free(oldptr);
			return newptr;
		}
	}

}
