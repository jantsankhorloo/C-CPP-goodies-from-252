/*
 * CS252: MyMalloc Project
 *
 * The current implementation gets memory from the OS
 * every time memory is requested and never frees memory.
 *
 * You will implement the allocator as indicated in the handout,
 * as well as the deallocator.
 *
 * You will also need to add the necessary locking mechanisms to
 * support multi-threaded programs.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include "MyMalloc.h"

static pthread_mutex_t mutex;

const int arenaSize = 2097152;

void increaseMallocCalls()  { _mallocCalls++; }

void increaseReallocCalls() { _reallocCalls++; }

void increaseCallocCalls()  { _callocCalls++; }

void increaseFreeCalls()    { _freeCalls++; }

extern void atExitHandlerInC()
{
    atExitHandler();
}

/* 
 * Initial setup of allocator. First chunk is retrieved from the OS,
 * and the fence posts and freeList are initialized.
 */
void initialize()
{
    // Environment var VERBOSE prints stats at end and turns on debugging
    // Default is on
    _verbose = 1;
    const char *envverbose = getenv("MALLOCVERBOSE");
    if (envverbose && !strcmp(envverbose, "NO")) {
        _verbose = 0;
    }

    pthread_mutex_init(&mutex, NULL);
    void *_mem = getMemoryFromOS(arenaSize);

    // In verbose mode register also printing statistics at exit
    atexit(atExitHandlerInC);

    // establish fence posts
    ObjectHeader * fencePostHead = (ObjectHeader *)_mem;
    fencePostHead->_allocated = 1;
    fencePostHead->_objectSize = 0;

    char *temp = (char *)_mem + arenaSize - sizeof(ObjectHeader);
    ObjectHeader * fencePostFoot = (ObjectHeader *)temp;
    fencePostFoot->_allocated = 1;
    fencePostFoot->_objectSize = 0;

    // Set up the sentinel as the start of the freeList
    _freeList = &_freeListSentinel;

    // Initialize the list to point to the _mem
    temp = (char *)_mem + sizeof(ObjectHeader);
    ObjectHeader *currentHeader = (ObjectHeader *)temp;
    currentHeader->_objectSize = arenaSize - (2*sizeof(ObjectHeader)); // ~2MB
    currentHeader->_leftObjectSize = 0;
    currentHeader->_allocated = 0;
    currentHeader->_listNext = _freeList;
    currentHeader->_listPrev = _freeList;
    _freeList->_listNext = currentHeader;
    _freeList->_listPrev = currentHeader;

    // Set the start of the allocated memory
    _memStart = (char *)currentHeader;

    _initialized = 1;
}

/* 
 * TODO: In allocateObject you will handle retrieving new memory for the malloc
 * request. The current implementation simply pulls from the OS for every
 * request.
 *
 * @param: amount of memory requested
 * @return: pointer to start of useable memory
 */
void * allocateObject(size_t size)
{
    // Make sure that allocator is initialized
    if (!_initialized)
        initialize();

    /* Add the ObjectHeader to the size and round the total size up to a 
     * multiple of 8 bytes for alignment.
     */
    size_t roundedSize = (size + sizeof(ObjectHeader) + 7) & ~7;

    // Naively get memory from the OS every time
    //void *_mem = getMemoryFromOS(arenaSize); 

    // Store the size in the header
    //ObjectHeader *o = (ObjectHeader *)_mem;
    //o->_objectSize = roundedSize;

    //pthread_mutex_unlock(&mutex);
    //printf("%zd\n", 8 + sizeof(struct ObjectHeader));       
    ObjectHeader * temp = _freeList->_listNext;
    while (temp != _freeList) {
    	size_t remainder = temp->_objectSize - roundedSize;
    	if ((temp->_objectSize >= roundedSize) && (remainder >= (8 + sizeof(ObjectHeader)))) {
	
		//printf("%s\n", "split");
		
		ObjectHeader * newHeader = (ObjectHeader *) ((char*)temp + temp->_objectSize - roundedSize);
		newHeader->_allocated = 1;
		newHeader->_objectSize = roundedSize;
		
		temp->_objectSize -= roundedSize;
		newHeader->_leftObjectSize = temp->_objectSize;
	 	//printf("%d\n", temp->_allocated);
		//update left obj size of what is right of allocated block	
		ObjectHeader * nextBlock = (ObjectHeader *) ((char*)newHeader + roundedSize);
		nextBlock->_leftObjectSize = roundedSize;
		
		pthread_mutex_unlock(&mutex);
		return (void *)((char*)newHeader + sizeof(ObjectHeader));
	}
	else if (temp->_objectSize >= roundedSize && remainder < (8 + sizeof(ObjectHeader))) {
		//printf("%s\n", "just enough");
		temp->_listPrev->_listNext = temp->_listNext;
		temp->_listNext->_listPrev = temp->_listPrev;
		temp->_allocated = 1;
		pthread_mutex_unlock(&mutex);
		return (void*)((char*)temp + sizeof(ObjectHeader));
	}
	temp = temp->_listNext;
	
	if (temp == _freeList) {
		//printf("%s\n", "newblock");
		//looped around without returning anything, more 2MB
		void * _mem = getMemoryFromOS(arenaSize); //for 2 dummy headers
		//add dummy headers
		ObjectHeader * fenceHead = (ObjectHeader *) _mem;
		fenceHead->_allocated = 1;
		fenceHead->_objectSize = 0;

		char * footer = (char *)_mem + arenaSize - sizeof(ObjectHeader);
		ObjectHeader * fenceFoot = (ObjectHeader *) footer;
		fenceFoot->_allocated = 1;
		fenceFoot->_objectSize = 0;

		//establish links
		footer = (char *)_mem + sizeof(ObjectHeader);
		ObjectHeader * newBlock = (ObjectHeader *) footer;
		newBlock->_objectSize = arenaSize - (2 * sizeof(ObjectHeader));
		newBlock->_leftObjectSize = 0;
		newBlock->_allocated = 0;

		newBlock->_listNext = temp->_listNext;
		temp->_listNext->_listPrev = newBlock;
		temp->_listNext = newBlock;
		newBlock->_listPrev = temp;
		
		//newBlock->_listNext = _freeList->_listNext;
		//newBlock->_listPrev = _freeList;
		//_freeList->_listNext->_listPrev = newBlock;
		//_freeList->_listNext = newBlock;
		
		
		temp = _freeList->_listNext;//start the iteration from new block
	}
	//temp = temp->_listNext;
    }
    
    // Return a pointer to useable memory
    //return (void *)((char *)o + sizeof(ObjectHeader));
}

/* 
 * TODO: In freeObject you will implement returning memory back to the free
 * list, and coalescing the block with surrounding free blocks if possible.
 *
 * @param: pointer to the beginning of the block to be returned
 * Note: ptr points to beginning of useable memory, not the block's header
 */
void freeObject(void *ptr)
{
    // Add your implementation here
    //gonna check left side block's header
    ObjectHeader * p = (ObjectHeader *) ((char*)ptr - sizeof(ObjectHeader));
    ObjectHeader * headerleft = (ObjectHeader *) ((char *) p - p->_leftObjectSize);
    ObjectHeader * headerright = (ObjectHeader *) ((char *) p + p->_objectSize);
    //printf("left = %d\n",headerleft->_allocated);
    //printf("right = %d\n, headerright->_allocated);
    if (headerleft->_allocated == 0 && headerright->_allocated == 0) {
	//printf("%s\n","both");
	//need to update remove right of ptr ffrom the freelist, combine it with ptr to the left of ptr.
	//and update what was right of right of ptr		
    	p->_allocated = 0;
	headerleft->_objectSize += (p->_objectSize + headerright->_objectSize);
	//updating leftobjsize of what was right right of ptr
	ObjectHeader * rightright = (ObjectHeader *) ((char*) headerright + headerright->_objectSize);
	//ObjectHeader * rightright = (ObjectHeader *) ((char*) headerleft + headerleft->_objectSize);
	rightright->_leftObjectSize = headerleft->_objectSize;
	//headerleft->_listNext = headerright->_listNext;
	//headerright->_listNext->_listPrev = headerleft;
	//delete headerright from the free list, left and right places might have been swappped,
	//so only work with the headerright pointer
	headerright->_listPrev->_listNext = headerright->_listNext;
	headerright->_listNext->_listNext = headerright->_listPrev;
    }
    else if (headerleft->_allocated == 0) {//coalesce left
	
	headerleft->_objectSize += p->_objectSize;
		//((ObjectHeader *) ((char*)ptr - sizeof(ObjectHeader)))->_allocated = 0;
	p->_allocated = 0;
	headerright->_leftObjectSize = headerleft->_objectSize;

    }
    else if (headerright->_allocated == 0) {//coalesce right
		//printf("%s\n","right");
		//need to update leftobjectsize of what was right of right of ptr (double right)
		//remove the right side from the freelist and add the deallocted ptr to the freelist
	p->_allocated = 0;
	p->_objectSize += headerright->_objectSize;
		//updating leftObjSize of what was right right of ptr
	ObjectHeader * rightright = (ObjectHeader *) ((char*) headerright + headerright->_objectSize);
	rightright->_leftObjectSize = p->_objectSize;
		//update freelist by putting p in place of headerright
	headerright->_listPrev->_listNext = p;
	p->_listPrev = headerright->_listPrev;
	p->_listNext = headerright->_listNext;
	headerright->_listNext->_listPrev = p;	
    }
    else {//both sides are allocated
	//no need to update left object size of anything
	//insert at head
	//printf("%s\n", "both not FREE");
	p->_allocated = 0;

	p->_listNext = _freeList->_listNext;
	p->_listPrev = _freeList;
	_freeList->_listNext->_listPrev = p;
	_freeList->_listNext = p;	
    }
    
    return;
}

/* 
 * Prints the current state of the heap.
 */
void print()
{
    printf("\n-------------------\n");

    printf("HeapSize:\t%zd bytes\n", _heapSize );
    printf("# mallocs:\t%d\n", _mallocCalls );
    printf("# reallocs:\t%d\n", _reallocCalls );
    printf("# callocs:\t%d\n", _callocCalls );
    printf("# frees:\t%d\n", _freeCalls );

    printf("\n-------------------\n");
}

/* 
 * Prints the current state of the freeList
 */
void print_list() {
    printf("FreeList: ");
    if (!_initialized) 
        initialize();

    ObjectHeader * ptr = _freeList->_listNext;

    while (ptr != _freeList) {
        long offset = (long)ptr - (long)_memStart;
        printf("[offset:%ld,size:%zd]", offset, ptr->_objectSize);
        ptr = ptr->_listNext;
        if (ptr != NULL)
            printf("->");
    }
    printf("\n");
}

/* 
 * This function employs the actual system call, sbrk, that retrieves memory
 * from the OS.
 *
 * @param: the chunk size that is requested from the OS
 * @return: pointer to the beginning of the chunk retrieved from the OS
 */
void * getMemoryFromOS(size_t size)
{
    _heapSize += size;

    // Use sbrk() to get memory from OS
    void *_mem = sbrk(size);

    // if the list hasn't been initialized, initialize memStart to mem
    if (!_initialized)
        _memStart = _mem;

    return _mem;
}

void atExitHandler()
{
    // Print statistics when exit
    if (_verbose)
        print();
}

/*
 * C interface
 */

extern void * malloc(size_t size)
{
    pthread_mutex_lock(&mutex);
    increaseMallocCalls();

    return allocateObject(size);
}

extern void free(void *ptr)
{
    pthread_mutex_lock(&mutex);
    increaseFreeCalls();

    if (ptr == 0) {
        // No object to free
        pthread_mutex_unlock(&mutex);
        return;
    }

    freeObject(ptr);
}

extern void * realloc(void *ptr, size_t size)
{
    pthread_mutex_lock(&mutex);
    increaseReallocCalls();

    // Allocate new object
    void *newptr = allocateObject(size);

    // Copy old object only if ptr != 0
    if (ptr != 0) {

        // copy only the minimum number of bytes
        ObjectHeader* hdr = (ObjectHeader *)((char *) ptr - sizeof(ObjectHeader));
        size_t sizeToCopy =  hdr->_objectSize;
        if (sizeToCopy > size)
            sizeToCopy = size;

        memcpy(newptr, ptr, sizeToCopy);

        //Free old object
        freeObject(ptr);
    }

    return newptr;
}

extern void * calloc(size_t nelem, size_t elsize)
{
    pthread_mutex_lock(&mutex);
    increaseCallocCalls();

    // calloc allocates and initializes
    size_t size = nelem *elsize;

    void *ptr = allocateObject(size);

    if (ptr) {
        // No error; initialize chunk with 0s
        memset(ptr, 0, size);
    }

    return ptr;
}

