/*
 * mm.c - A dynamic memory alloctor 
 * 姓名：陶涛  学号：1900012926
 * 
 * 主要使用显示链表与稍作修改的二叉树结构，已分配内存块无尾部信息。
 * 
 * 1.链表结构，主要有两个链表：
 *    链表一是存储大小 1~4 bytes的单向链表，链表头 free_list8。
 *  空闲链表内有一个链表头与指向下一个8-bytes块的指针。
 *  由于没有链表尾，需要利用第三位bit指示前一个的块是否8-bytes块。
 * 
 *    链表二是存储大小 5~12 bytes的单向链表，链表头 free_list16。
 *  空闲链表内包括链表头与链表尾，指向前驱的指针与指向后继的指针。
 *  
 * 2.二叉树结构
 *    大于 12 bytes的存储块放入搜索树中，该搜索树的结点的存储信息有
 *  链表头，链表尾，左儿子指针，右儿子指针，父节点指针，特殊指针。
 *  左侧放不超过该节点值的节点，右侧放大于该节点值的节点。
 *  搜索、插入与删除操作与正常二叉树结构相同。
 * 
 *    特殊指针是针对树的左侧可能出现的一条全部相等的节点链的跳跃指针。
 * 在出现连续相同的节点序列后，会在树的左侧形成一条值全相等的链，该指针
 * 可以从起始节点跳跃至尾部尾部节点，省去搜索时需要的更多次索引。缺点是
 * 对应的删除操作需要考虑更多情况。
 *
 * 3.其它优化
 *    realloc中对于可以直接在原oldptr上分配空间的情况进行了一些优化。
 * 
 * Ps:事实上仍然有许多地方没有处理好，导致效率并不是非常高，由于debug
 * 次数过多，mm_checkheap使用非常频繁，因此在mm_checkheap函数中写了
 * 非常多调试状态会输出的消息。
 * 
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, define DEBUG 0. 
 * DBG_printf use to output more information whenever you want to check 
 * more block. */
#define DEBUG 0
#if DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
# define DBG_printf(...) 
# define MAX_DEPTH 10000000
#else
# define dbg_printf(...)
# define DBG_printf(...)
# define MAX_DEPTH 0
#endif

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<11)

#define MAX(x, y) ((x) > (y)? (x) : (y))  
/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) 
/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))   

/* Read the size and get the message from head from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_FALLOC(p) (GET(p) & 0x2)
#define GET_FFREE4(p) (GET(p) & 0x4)
/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 
/* use for explicit list */
#define FREE_PREV(bp)  (bp)
#define FREE_NEXT(bp)  ((char *)(bp) + WSIZE)
/* binary search tree */
#define FREE_LEFT(bp)   (bp)
#define FREE_RIGHT(bp)  ((char *)(bp) + WSIZE)
#define FREE_PARENT(bp) ((char *)(bp) + DSIZE)
#define FREE_TNEXT(bp)  ((char *)(bp) + WSIZE*3)
/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) 
#define PREV_BLK(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) 
#define PREV_BLKP(bp)  (GET_FFREE4(HDRP(bp))?((char*)(bp)-DSIZE):PREV_BLK(bp))
/* change the next block's status about previous block */
#define NTPS_FREE(bp)  (GET(HDRP(NEXT_BLKP(bp))) &= (~0x2))
#define NTPS_ALLOC(bp)  ((GET(HDRP(NEXT_BLKP(bp))) |= 0x2))
#define NTPS_FREE4(bp)  (GET(HDRP(NEXT_BLKP(bp))) |= 0x4)
#define NTPS_NF4(bp)    (GET(HDRP(NEXT_BLKP(bp))) &= (~0x4))

/* global variabal*/
static void *head_ptr = 0;
static unsigned int free_list8 = 0; /* use for 8 bytes block */
static unsigned int free_list16 = 0;/* use for 16 bytes block */
static unsigned int free_root = 0;  /* use for BST */

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
/* for list struct and tree struct */
static void insert_list(void *bp);
static void del_list(void *bp);
static void tree_insert(void *bp);
static void tree_delete(void *bp);
/* use for tree's correctness checking */
static void *tree_check(void *cur, int depth);

/*
 * mm_init - Called when a new trace starts. 
 * CAUTION: You must reset all of your global pointers here. */
int mm_init(void)
{

    if((long)(head_ptr = mem_sbrk(4*WSIZE)) == -1)
        return -1;
    PUT(head_ptr, 0);                          /* Alignment padding */
    PUT(head_ptr + (1*WSIZE), PACK(DSIZE, 3)); /* Prologue header */ 
    PUT(head_ptr + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */ 
    PUT(head_ptr + (3*WSIZE), PACK(0, 3));     /* Epilogue header */
    head_ptr += (2*WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    void *t = 0;
    free_list8 = 0;
    free_list16 = 0;
    free_root = 0;
    if ((t = extend_heap(CHUNKSIZE/WSIZE)) == NULL) 
        return -1;
    free_root = t - head_ptr;
    PUT(FREE_PREV(t), 0);
    PUT(FREE_NEXT(t), 0);
    //dbg_printf("init block size: %d\n", GET_SIZE(HDRP(t)));
    return 0;
}
/*
 * malloc - Allocate a block by incrementing the brk pointer.
 *      Always allocate a block whose size is a multiple of the alignment.
 */
void *malloc(size_t size)
{
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    if (head_ptr == 0){
        mm_init();
    }
    /* Ignore spurious requests */
    if (size == 0){
        return NULL;
    }

    /* Adjust block size to include overhead and alignment reqs. */
    asize = DSIZE * ((size + (WSIZE) + (DSIZE-1)) / DSIZE); 
    /* dbg_printf("alloc size %ld\n", asize);//*/

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) { 
        place(bp, asize);
        return bp;
    }
    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)  
        return NULL;
    place(bp, asize);
    return bp;
}
/*
 * free - free block, implement in coalesce
 */
void free(void *ptr){
    if (ptr == 0) 
        return;

    if (head_ptr == 0){
        mm_init();
    }
    /* dbg_printf("free %p\n", ptr);//*/
    coalesce(ptr);
}
/*
 * realloc - Change the size of the block by mallocing a new block,
 *      copying its data, and freeing the old block.  
 *      when the old block is big enough with near block, 
 *      return oldptr correctly.
 * */
void *realloc(void *oldptr, size_t size)
{
    size_t oldsize;
    void *newptr;
    /*dbg_printf("enter ra, oldptr:%p size:%ld\n", oldptr, size);//*/

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
        free(oldptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(oldptr == NULL) {
        return malloc(size);
    }
    oldsize = GET_SIZE(HDRP(oldptr));
    /* when newsize is smaller than oldsize */
    if(size + WSIZE <= oldsize){
        if(size + WSIZE + DSIZE < oldsize) {
            /* be carefully to unsigned int sub */
            size = DSIZE*((size+WSIZE+(DSIZE-1))/DSIZE);
            size_t tems = (GET(HDRP(oldptr))&0x7);
            PUT(HDRP(oldptr), PACK(size,tems));
            void *temp = NEXT_BLKP(oldptr);
            PUT(HDRP(temp), PACK(oldsize-size, 2));
            if(oldsize-size>=2*DSIZE) 
                PUT(FTRP(temp),PACK(oldsize-size, 0));
            coalesce(temp);
        }
        return oldptr;
    } /* use the near space to realloc*/
    else if(size+WSIZE > oldsize && !GET_ALLOC(HDRP(NEXT_BLKP(oldptr)))){
        size_t nsize = GET_SIZE(HDRP(NEXT_BLKP(oldptr)));
        if(oldsize + nsize >= size + WSIZE) {
            size = DSIZE*((size+WSIZE+(DSIZE-1))/DSIZE);
            size_t tems = (GET(HDRP(oldptr))&0x7);
            del_list(NEXT_BLKP(oldptr));
            PUT(HDRP(oldptr), PACK(size,tems));
            if(oldsize + nsize == size) {
                NTPS_ALLOC(oldptr);
                NTPS_NF4(oldptr);
                return oldptr;
            }
            else {
                void *temp = NEXT_BLKP(oldptr);
                PUT(HDRP(temp), PACK(oldsize + nsize - size, 2));
                if(oldsize + nsize - size>=2*DSIZE) {
                    PUT(FTRP(temp),PACK(oldsize + nsize - size, 0));
                    NTPS_FREE(temp);
                    NTPS_NF4(temp);
                }
                else {
                    NTPS_FREE(temp);
                    NTPS_FREE4(temp);
                }
                insert_list(temp);
                return oldptr;
            }
        }
    }

    newptr = malloc(size);
    /* If realloc() fails the original block is left untouched */
    if(!newptr) {
        return 0;
    }
    /* Copy the old data. */
    if(size < oldsize) oldsize = size;
    memcpy(newptr, oldptr, oldsize);

    /* Free the old block. */
    free(oldptr);

    return newptr;
}
/*
 * calloc - Allocate the block and set it to zero.
 */
void *calloc (size_t nmemb, size_t size)
{
    //dbg_printf("enter calloc\n");
    size_t bytes = nmemb * size;
    void *newptr;

    newptr = malloc(bytes);
    memset(newptr, 0, bytes);

    return newptr;
}
/*
 * mm_checkheap - There are too many bugs in my code, so I always use this
 *      to check, so nah! First, check all of the block. Second, check the
 *      explicit list of 8 and 16 bytes. Finally, check the BST.
 */
void mm_checkheap(int lineno){
    DBG_printf("\ncheck. head:%p, free_list16:0x%x, ", head_ptr, free_list16);
    DBG_printf("free_list8:0x%x, free_root:0x%x\n", free_list8, free_root);
    void *bp;
    unsigned int prevA=0x2, prevF8=0x0;
    /* check all of the block */
    for (bp = head_ptr; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        unsigned int t1 = GET_FALLOC(HDRP(bp));
        unsigned int t2 = GET_FFREE4(HDRP(bp));
        if(t1^prevA) {
            dbg_printf("block %p has wrong bit about prev's state.\n", bp);
            exit(-1);
        }
        if(t2^prevF8) {
            dbg_printf("block %p has wrong bit about prev's size.\n", bp);
            exit(-1);
        }
        if(GET_ALLOC(HDRP(bp))) {
            DBG_printf("a block. addr:%p, size:%d, f:%d, org:0x%x\n",
                bp, GET_SIZE(HDRP(bp)), GET_FALLOC(HDRP(bp)), GET(HDRP(bp)));
            prevA = 2; prevF8 = 0;
        }
        else if(GET_SIZE(HDRP(bp)) == 2*DSIZE){
            DBG_printf("f block. addr:%p, size:%d, f:%d, prev:%d, next:%d\n",
                bp, GET_SIZE(HDRP(bp)), GET_FALLOC(HDRP(bp)),
                GET(FREE_PREV(bp)), GET(FREE_NEXT(bp)));
            DBG_printf("     foot. addr:%p, size:%d, org:0x%x\n", 
                NEXT_BLKP(bp), GET_SIZE(FTRP(bp)), GET(HDRP(bp)));
            
            if(GET_SIZE(FTRP(bp))!=GET_SIZE(HDRP(bp)) || 
                GET_ALLOC(FTRP(bp))!=GET_ALLOC(HDRP(bp))){
                dbg_printf("foot message is different form before.\n");
                exit(-1);
            }
            prevA = prevF8 = 0;
        }
        else if(GET_SIZE(HDRP(bp)) > 2*DSIZE) {
            DBG_printf("f block. addr:%p, size:%d, f:%d, org:0x%x ",
                bp, GET_SIZE(HDRP(bp)), GET_FALLOC(HDRP(bp)), GET(HDRP(bp)));
            DBG_printf("left:%p, right:%p, parent:%p, tnext:0x%x\n",
                head_ptr+GET(FREE_LEFT(bp)), head_ptr+GET(FREE_RIGHT(bp)),
                head_ptr+GET(FREE_PARENT(bp)), GET(FREE_TNEXT(bp)));
            DBG_printf("     foot. addr:%p, size:%d, org:0x%x\n", 
                NEXT_BLKP(bp), GET_SIZE(FTRP(bp)), GET(FTRP(bp)));

            if(GET_SIZE(FTRP(bp))!=GET_SIZE(HDRP(bp)) || 
                GET_ALLOC(FTRP(bp))!=GET_ALLOC(HDRP(bp))){
                dbg_printf("foot message is different form before.\n");
                exit(-1);
            }
            prevA = prevF8 = 0;
        }
        else {
            DBG_printf("f block. addr:%p, size:%d, f:%d, bias_n:%d\n",
                bp, GET_SIZE(HDRP(bp)), GET_FALLOC(HDRP(bp)), GET(bp));
            prevA = 0; prevF8 = 0x4;
        }
    }
    DBG_printf("end block. addr:%p, f:%d, a:%d, org:0x%x\n", 
        bp, GET_FALLOC(HDRP(bp)),GET_ALLOC(HDRP(bp)), GET(HDRP(bp)));
    if(GET_SIZE(HDRP(bp)) != 0x0) {
        dbg_printf("end block error.\n");
        exit(-1);
    }
    DBG_printf("\n");
    /* check free_list 8*/
    void *bp2 = head_ptr + free_list8;
    for(bp = head_ptr + free_list8; bp != head_ptr; ) {
        if(bp2 != head_ptr)
            bp2 = head_ptr + GET(bp2);
        if(bp == bp2) {
            dbg_printf("the list 8 loop.\n");
            exit(-1);
        }
        if(GET_SIZE(HDRP(bp)) != 8) {
            dbg_printf("the list 8 have a error block.\n");
            exit(-1);
        }
        if(bp2 != head_ptr)
            bp2 = head_ptr + GET(bp2);
        bp = head_ptr + GET(bp);
    }
    /* check free_list 16 */
    void *bp0 = head_ptr;
    bp2 = head_ptr + free_list16;
    for(bp = head_ptr+free_list16; bp != head_ptr; ) {
        if(bp2 != head_ptr)
            bp2 = head_ptr + GET(FREE_NEXT(bp2));
        if(bp == bp2) {
            dbg_printf("the list 16 loop.\n");
            exit(-1);
        }
        if(GET_SIZE(HDRP(bp)) != 16) {
            dbg_printf("the list 16 have a error block.\n");
            exit(-1);
        }
        if(bp0 != head_ptr+GET(FREE_PREV(bp))) {
            dbg_printf("the block have wrong prev.\n");
            exit(-1);
        }
        if(bp0 == head_ptr) 
            bp0 = head_ptr + free_list16;
        else 
            bp0 = head_ptr + GET(FREE_NEXT(bp0));
        
        if(bp2 != head_ptr)
            bp2 = head_ptr + GET(FREE_NEXT(bp2));
        bp = head_ptr + GET(FREE_NEXT(bp));
    }
    /* check the tree */
    if(tree_check(head_ptr+free_root, 0) != head_ptr) {
        dbg_printf("tree error.\n");
        exit(-1);
    }
    return;
}
/*
 * extend_heap - use for applying more space to storage
 * */
static void *extend_heap(size_t words) {
    char *bp;
    size_t size; 
    unsigned int prev;

    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; 
    if ((long)(bp = mem_sbrk(size)) == -1)  
        return NULL;  
    
    prev = GET(HDRP(bp)) & 0x6;
    PUT(HDRP(bp), PACK(size, 0|prev));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    return coalesce(bp);
}

/*
 * coalesce - coalesce the free block with its previous block 
 *       and next block correctly.
 * */
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_FALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            /* Case 1 */
        /*dbg_printf("enter coalesce case 1.\n");//*/
        PUT(HDRP(bp), PACK(size, 2));
        PUT(FTRP(bp), PACK(size, 0));
        insert_list(bp);
        NTPS_FREE(bp);
        if(GET_SIZE(HDRP(bp)) == DSIZE) {
            NTPS_FREE4(bp);
        }
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */
        /*dbg_printf("enter coalesce case 2.\n");//*/
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        NTPS_NF4(NEXT_BLKP(bp));
        del_list(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 2));
        PUT(FTRP(bp), PACK(size, 0));
        insert_list(bp);
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
        /*dbg_printf("enter coalesce case 3.\n");//*/
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        del_list(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 2));
        bp = PREV_BLKP(bp);/*sequence is important, ecpecially 8 bytes.*/
        insert_list(bp);
        NTPS_FREE(bp);
        NTPS_NF4(bp);
    }

    else {                                     /* Case 4 */
        /*dbg_printf("enter coalesce case 4.\n");//*/
        size += (GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(HDRP(NEXT_BLKP(bp))));
        del_list(NEXT_BLKP(bp));
        del_list(PREV_BLKP(bp));
        NTPS_NF4(NEXT_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 2));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        insert_list(bp);
    }

    return bp;
}
/*
 * place - change the block status to allocated and cut its tail
 *        to the free block (if it will cause fragment).
 *        asize must be divided by 8.
 * */
static void place(void *bp, size_t asize) {
    del_list(bp);
    size_t csize = GET_SIZE(HDRP(bp));
    /*the state must be success */
    size_t fstatus = GET(HDRP(bp)) & 0x6;

    /*dbg_printf("place %ld bits in %p\n", asize, bp);//*/
    if ((csize - asize) >= (DSIZE)) { 
        PUT(HDRP(bp), PACK(asize, (1|fstatus)));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 2));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        insert_list(bp);
        NTPS_FREE(bp);
        if((csize - asize) < (2*DSIZE)) {
            NTPS_FREE4(bp);
        }
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1|fstatus));
        NTPS_ALLOC(bp);
        NTPS_NF4(bp);
    }
}
/*
 * find_fit - find a fit block to the asize.
 *           The same size block is the best. 
 *           if there is not that blocks, chose the big enough block.
 * */
static void *find_fit(size_t asize) {
    void *bp = 0;
    if(asize == DSIZE) {
        if(free_list8 != 0) {
            bp = head_ptr + free_list8;
            return bp;
        }
        else if(free_list16 != 0) {
            bp = head_ptr + free_list16;
            return bp;
        }
        //strange staregy
        if(free_root != 0)
            bp = head_ptr + free_root;
        return bp; //*/
    }
    else if(asize == 2*DSIZE && free_list16 != 0) {
        if(free_list16 != 0) {
            bp = head_ptr + free_list16;
            return bp;
        }
        if(free_root != 0)
            bp = head_ptr + free_root;
        return bp;
    }

    void *cur = head_ptr + free_root;
    while(cur != head_ptr) {
        /*dbg_printf("search block %p\n",cur);//*/
        if(GET_SIZE(HDRP(cur)) < asize) {
            cur = head_ptr + GET(FREE_RIGHT(cur));
        }
        else if(GET_SIZE(HDRP(cur)) > asize) {
            bp = cur;
            void *t = head_ptr + GET(FREE_LEFT(cur));
            if(GET_SIZE(HDRP(t)) == GET_SIZE(HDRP(cur))) {
                t = head_ptr + GET(FREE_TNEXT(cur));
                if(t == head_ptr)
                    cur = t;
                else if(GET(FREE_LEFT(t)) == 0) 
                    return cur;
                else 
                    cur = head_ptr + GET(FREE_LEFT(t));
            }
            else 
                cur = t;
        }
        else
            return cur;
    }
    return bp;
}
/*
 * insert_list - insert the block to the head of the list for 8 and 16
 *               and call tree_insert when it's bigger than 16.
 * */
static void insert_list(void *bp) {
    /*dbg_printf("insert list enter, bp=%p\n", bp);//*/
    size_t size= GET_SIZE(HDRP(bp));
    if(size == DSIZE) {
        if(free_list8 == 0) {
            PUT(bp, 0);
            free_list8 = (unsigned int)(bp - head_ptr);
            return;
        }
        PUT(bp, free_list8);
        free_list8 = (unsigned int)(bp - head_ptr);
        return;
    }
    else if(size == 2*DSIZE) {
        if(free_list16 == 0) {
            free_list16 = (unsigned int)(bp - head_ptr);
            PUT(FREE_PREV(bp), 0);
            PUT(FREE_NEXT(bp), 0);
            return ;
        }
        PUT(FREE_NEXT(bp), free_list16);
        PUT(FREE_PREV(bp), 0);
        void *t = head_ptr + free_list16;
        PUT(FREE_PREV(t), (unsigned int)(bp - head_ptr));
        free_list16 = (unsigned int)(bp - head_ptr);
        return ;
    }
    tree_insert(bp);
}
/*
 * del_list - delete the block from list or tree.
 * */
static void del_list(void *bp) {
    /*dbg_printf("enter del_list. %p\n", bp);//*/
    size_t size = GET_SIZE(HDRP(bp));

    /*the least fragment's list */
    if(size == DSIZE) { 
        void *temp = head_ptr + free_list8;
        if(temp == bp) {
            free_list8 = GET(bp);
            return;
        }/* be careful to dead loop */
        while(head_ptr + GET(temp) != bp) { 
            temp = head_ptr + GET(temp);
        }
        PUT(temp, GET(bp));
        return ;
    }
    else if(size == 2*DSIZE) {
        void *prev = head_ptr + GET(FREE_PREV(bp));
        void *next = head_ptr + GET(FREE_NEXT(bp));
        if(prev == head_ptr && next == head_ptr) {
            free_list16 = 0;
        }
        else if(prev == head_ptr) {
            free_list16 = GET(FREE_NEXT(bp));
            PUT(FREE_PREV(next), 0);
        }
        else if(next == head_ptr) {
            PUT(FREE_NEXT(prev), 0);
        }
        else {
            PUT(FREE_NEXT(prev), GET(FREE_NEXT(bp)));
            PUT(FREE_PREV(next), GET(FREE_PREV(bp)));
        }
        return;
    }
    tree_delete(bp);
    /*dbg_printf("return from del_list\n");//*/
}
/*
 * tree_insert - insert the block to the BST.
 * */
static void tree_insert(void *bp) {
    /* initialize bp as a node */
    /*dbg_printf("enter tree ins,%p, size:%d\n", bp, GET_SIZE(HDRP(bp)));//*/
    PUT(FREE_LEFT(bp), 0);
    PUT(FREE_RIGHT(bp), 0);
    PUT(FREE_PARENT(bp), 0);
    PUT(FREE_TNEXT(bp), 0);

    if(free_root == 0) {
        free_root = (unsigned int)(bp - head_ptr);
        return ;
    }
    void *cur = head_ptr + free_root;
    size_t size = GET_SIZE(HDRP(bp));
    size_t tsize = GET_SIZE(HDRP(cur));
    while(1) {
        /*dbg_printf("search block i %p\n", cur);//*/
        if(size > tsize) {
            size_t t = GET(FREE_RIGHT(cur));
            if(t == 0) {
                PUT(FREE_RIGHT(cur), (unsigned int)(bp - head_ptr));
                PUT(FREE_PARENT(bp), (unsigned int)(cur - head_ptr));
                return;
            }
            cur = head_ptr + t;
        }
        else if(size <= tsize) {
            size_t t = GET(FREE_LEFT(cur));
            if(t == 0) {
                PUT(FREE_LEFT(cur), (unsigned int)(bp - head_ptr));
                if(size == tsize)
                    PUT(FREE_TNEXT(cur), (unsigned int)(bp - head_ptr));
                PUT(FREE_PARENT(bp), (unsigned int)(cur - head_ptr));
                return;
            }
            if(size == tsize) {
                PUT(FREE_LEFT(bp), GET(FREE_LEFT(cur)));
                PUT(FREE_PARENT(bp), (unsigned int)(cur - head_ptr));
                if(GET(FREE_TNEXT(cur)) != 0)
                    PUT(FREE_TNEXT(bp), GET(FREE_TNEXT(cur)));
                else 
                    PUT(FREE_TNEXT(cur), (unsigned int)(bp - head_ptr));
                void *temp = head_ptr + GET(FREE_LEFT(cur));
                PUT(FREE_PARENT(temp), (unsigned int)(bp - head_ptr));
                PUT(FREE_LEFT(cur), (unsigned int)(bp - head_ptr));
                return ;
            }
            void *p1 = head_ptr + t;
            if(GET_SIZE(HDRP(p1)) == tsize && GET(FREE_TNEXT(cur))!=0) {
                cur = head_ptr + GET(FREE_TNEXT(cur));
            }
            else {
                cur = head_ptr + t;
            }
        }
        tsize = GET_SIZE(HDRP(cur));
    }
}
/*
 * tree_delete - delete the block from tree.
 * */
static void tree_delete(void *bp) {
    void *temp = head_ptr + GET(FREE_PARENT(bp));
    /* dbg_printf("enter tree delete, %p\n", bp);//*/
    /* case 1 leaf node */
    if(GET(FREE_LEFT(bp))==0 && GET(FREE_RIGHT(bp))==0) {
        if(temp == head_ptr) {//root
            free_root = 0;
            return ;
        }
        /* jump node */
        if(GET_SIZE(HDRP(temp)) == GET_SIZE(HDRP(bp))) {
            void *newt = temp;
            PUT(FREE_LEFT(temp), 0);
            PUT(FREE_TNEXT(temp), 0);
            temp = head_ptr + GET(FREE_PARENT(temp));
            /* size of head_ptr is 8, so it could be over at root*/
            while(GET_SIZE(HDRP(temp)) == GET_SIZE(HDRP(newt))) {
                PUT(FREE_TNEXT(temp), (unsigned int)(newt - head_ptr));
                temp = head_ptr + GET(FREE_PARENT(temp));
            }
        } /* normal node */
        else {
            if(GET(FREE_LEFT(temp)) == (unsigned int)(bp - head_ptr))
                PUT(FREE_LEFT(temp), 0);
            else 
                PUT(FREE_RIGHT(temp), 0);
        }
    } /* case 2 node with only right node */
    else if(GET(FREE_LEFT(bp))==0 && GET(FREE_RIGHT(bp))!=0) {
        void *t = head_ptr+GET(FREE_RIGHT(bp));
        if(temp == head_ptr) { //root
            free_root = GET(FREE_RIGHT(bp));
            PUT(FREE_PARENT(t), 0);
            return ;
        }
        if(GET(FREE_LEFT(temp)) == (unsigned int)(bp - head_ptr))
            PUT(FREE_LEFT(temp), GET(FREE_RIGHT(bp)));
        else 
            PUT(FREE_RIGHT(temp), GET(FREE_RIGHT(bp)));
        PUT(FREE_PARENT(t),(unsigned)(temp-head_ptr));
    } /* case 3 node with only left node*/
    else if(GET(FREE_LEFT(bp))!=0 && GET(FREE_RIGHT(bp))==0) {
        void *t = head_ptr + GET(FREE_LEFT(bp));
        if(temp == head_ptr) { //root
            free_root = GET(FREE_LEFT(bp));
            PUT(FREE_PARENT(t), 0);
            return ;
        }
        int t1 = (GET_SIZE(HDRP(bp)) == GET_SIZE(HDRP(t)));
        int t2 = (GET_SIZE(HDRP(bp)) == GET_SIZE(HDRP(temp)));
        if((!t1)&& t2) {
            void *newt = temp;
            PUT(FREE_LEFT(temp), (unsigned int)(t - head_ptr));
            PUT(FREE_PARENT(t), (unsigned int)(newt-head_ptr));
            PUT(FREE_TNEXT(temp), 0);
            temp = head_ptr + GET(FREE_PARENT(temp));
            /* size of head_ptr is 8, so it could be over at root*/
            while(GET_SIZE(HDRP(temp)) == GET_SIZE(HDRP(newt))) {
                PUT(FREE_TNEXT(temp), (unsigned int)(newt - head_ptr));
                temp = head_ptr + GET(FREE_PARENT(temp));
            }
        }
        else {
            if(GET(FREE_LEFT(temp)) == (unsigned)(bp - head_ptr))
                PUT(FREE_LEFT(temp), GET(FREE_LEFT(bp)));
            else 
                PUT(FREE_RIGHT(temp), GET(FREE_LEFT(bp)));
            PUT(FREE_PARENT(t), (unsigned int)(temp - head_ptr));
        }
    } /* case 4 internal node*/
    else {
        void *t = head_ptr + GET(FREE_LEFT(bp));
        void *replace = 0;
        if(GET_SIZE(HDRP(t))==GET_SIZE(HDRP(bp))||(GET(FREE_RIGHT(t))==0)){
            /* replace node is it's left_child */
            if(temp == head_ptr) {
                void *s = head_ptr + GET(FREE_RIGHT(bp));
                free_root = (unsigned int)(t - head_ptr);
                PUT(FREE_PARENT(t), 0);
                PUT(FREE_RIGHT(t), GET(FREE_RIGHT(bp)));
                PUT(FREE_PARENT(s), (unsigned int)(t - head_ptr));
            }
            else {
                void *s = head_ptr + GET(FREE_RIGHT(bp));
                if(GET(FREE_LEFT(temp)) == (unsigned)(bp - head_ptr))
                    PUT(FREE_LEFT(temp), GET(FREE_LEFT(bp)));
                else 
                    PUT(FREE_RIGHT(temp), GET(FREE_LEFT(bp)));
                PUT(FREE_PARENT(t), (unsigned int)(temp - head_ptr));
                PUT(FREE_RIGHT(t), GET(FREE_RIGHT(bp)));
                PUT(FREE_PARENT(s), (unsigned int)(t - head_ptr));
            }
            return ;
        }
        else {
            while(GET(FREE_RIGHT(t))!=0) {
                t = head_ptr + GET(FREE_RIGHT(t));
            }
            replace = t;
            PUT(FREE_TNEXT(replace), 0);
            tree_delete(t);
        }

        if(temp == head_ptr) {
            free_root = (unsigned int)(replace - head_ptr);
            PUT(FREE_PARENT(replace), 0);
        }
        else {
            PUT(FREE_PARENT(replace), (unsigned int)(temp - head_ptr));
            if(GET(FREE_LEFT(temp)) == (unsigned int)(bp - head_ptr)) 
                PUT(FREE_LEFT(temp), (unsigned int)(replace - head_ptr));
            else 
                PUT(FREE_RIGHT(temp), (unsigned int)(replace - head_ptr));
        }
        temp = head_ptr + GET(FREE_LEFT(bp));
        PUT(FREE_LEFT(replace), (unsigned int)(temp - head_ptr));
        if(temp != head_ptr)
            PUT(FREE_PARENT(temp), (unsigned int)(replace - head_ptr));
        temp = head_ptr + GET(FREE_RIGHT(bp));
        PUT(FREE_RIGHT(replace), (unsigned int)(temp - head_ptr));
        if(temp != head_ptr)
            PUT(FREE_PARENT(temp), (unsigned int)(replace - head_ptr));
    }
}
/*
 * tree_check - only use for checking the correctness of the tree
 * */
static void *tree_check(void *cur, int depth) {
    if(MAX_DEPTH && depth >= MAX_DEPTH) {
        dbg_printf("The tree is too high. Ensure it's not dead loop\n");
        exit(-1);
    }
    if(cur == head_ptr) {
        return head_ptr;
    }
    void *temp = head_ptr+GET(FREE_PARENT(cur));
    size_t l = GET(FREE_LEFT(cur));
    size_t r = GET(FREE_RIGHT(cur));
    size_t tn= GET(FREE_TNEXT(cur));
    if(r != 0){
        if(cur!=tree_check(head_ptr+GET(FREE_RIGHT(cur)),depth+1)){
            dbg_printf("parent pointer error\n");
            exit(-1);
        }
    }
    if(tn != 0) {
        if(l == 0) {
            dbg_printf("tnext pointer error1\n");
            exit(-1);
        }
        void *deal = head_ptr+tn;
        while(deal != head_ptr+l && cur != head_ptr) {
            cur = head_ptr+l;
            l = GET(FREE_LEFT(cur));
            if(GET(FREE_RIGHT(cur))!=0) {
                dbg_printf("part error.\n");
                exit(-1);
            }
            if(GET(FREE_TNEXT(cur))!=tn) {
                dbg_printf("tnext pointer error2\n");
                exit(-1);
            }
        }
        if(cur == head_ptr) {
            dbg_printf("tnext pointer error3\n");
            exit(-1);
        }
        if(GET(FREE_TNEXT(cur))!=tn) {
            dbg_printf("tnext pointer error2\n");
            exit(-1);
        }
        if(cur!=tree_check(head_ptr+tn, depth+1)) {
            dbg_printf("parent pointer error.\n");
            exit(-1);
        }
    }
    else if(l!=0){
        if(cur!=tree_check(head_ptr+l,depth+1)) {
            dbg_printf("parent pointer error.\n");
            exit(-1);
        }
    }
    return temp;
}

