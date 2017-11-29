//
// CS252: MyMalloc Project
//
// The current implementation gets memory from the OS
// every time memory is requested and never frees memory.
//
// You will implement the allocator as indicated in the handout.
//
// Also you will need to add the necessary locking mechanisms to
// support multi-threaded programs.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include "MyMalloc.h"

static pthread_mutex_t mutex;

const int ArenaSize = 2097152;
const int NumberOfFreeLists = 1;

// Header of an object. Used both when the object is allocated and freed
struct ObjectHeader
{
    size_t _objectSize;         // Real size of the object.
    int _allocated;             // 1 = yes, 0 = no 2 = sentinel
    struct ObjectHeader * _next;       // Points to the next object in the freelist (if free).
    struct ObjectHeader * _prev;       // Points to the previous object.
};

struct ObjectFooter
{
    size_t _objectSize;
    int _allocated;
};

//STATE of the allocator

// Size of the heap
static size_t _heapSize;

// initial memory pool
static void * _memStart;

// number of chunks request from OS
static int _numChunks;

// True if heap has been initialized
static int _initialized;

// Verbose mode
static int _verbose;

// # malloc calls
static int _mallocCalls;

// # free calls
static int _freeCalls;

// # realloc calls
static int _reallocCalls;

// # realloc calls
static int _callocCalls;

// Free list is a sentinel
static struct ObjectHeader _freeListSentinel; // Sentinel is used to simplify list operations
static struct ObjectHeader *_freeList;


//FUNCTIONS

//Initializes the heap
void initialize();

// Allocates an object
void * allocateObject( size_t size );

// Frees an object
void freeObject( void * ptr );

// Returns the size of an object
size_t objectSize( void * ptr );

// At exit handler
void atExitHandler();

//Prints the heap size and other information about the allocator
void print();
void print_list();

// Gets memory from the OS
void * getMemoryFromOS( size_t size );

void increaseMallocCalls()
{
    _mallocCalls++;
}

void increaseReallocCalls()
{
    _reallocCalls++;
}

void increaseCallocCalls()
{
    _callocCalls++;
}

void increaseFreeCalls()
{
    _freeCalls++;
}

extern void
atExitHandlerInC()
{
    atExitHandler();
}

void initialize()
{
    // Environment var VERBOSE prints stats at end and turns on debugging
    // Default is on
    _verbose = 1;
    const char * envverbose = getenv( "MALLOCVERBOSE" );
    if ( envverbose && !strcmp( envverbose, "NO") )
    {
        _verbose = 0;
    }

    pthread_mutex_init(&mutex, NULL);
    void * _mem = getMemoryFromOS( ArenaSize + (2*sizeof(struct ObjectHeader)) + (2*sizeof(struct ObjectFooter)) );

    // In verbose mode register also printing statistics at exit
    atexit( atExitHandlerInC );

    //establish fence posts
    struct ObjectFooter * fencepost1 = (struct ObjectFooter *)_mem;
    fencepost1->_allocated = 1;
    fencepost1->_objectSize = 123456789;
    char * temp =
        (char *)_mem + (2*sizeof(struct ObjectFooter)) + sizeof(struct ObjectHeader) + ArenaSize;
    struct ObjectHeader * fencepost2 = (struct ObjectHeader *)temp;
    fencepost2->_allocated = 1;
    fencepost2->_objectSize = 123456789;
    fencepost2->_next = NULL;
    fencepost2->_prev = NULL;

    //initialize the list to point to the _mem
    temp = (char *) _mem + sizeof(struct ObjectFooter);
    struct ObjectHeader * currentHeader = (struct ObjectHeader *) temp;
    temp = (char *)_mem + sizeof(struct ObjectFooter) + sizeof(struct ObjectHeader) + ArenaSize;
    struct ObjectFooter * currentFooter = (struct ObjectFooter *) temp;
    _freeList = &_freeListSentinel;
    currentHeader->_objectSize = ArenaSize + sizeof(struct ObjectHeader) + sizeof(struct ObjectFooter); //2MB
    currentHeader->_allocated = 0;
    currentHeader->_next = _freeList;
    currentHeader->_prev = _freeList;
    currentFooter->_allocated = 0;
    currentFooter->_objectSize = currentHeader->_objectSize;
    _freeList->_prev = currentHeader;
    _freeList->_next = currentHeader;
    _freeList->_allocated = 2; // sentinel. no coalescing.
    _freeList->_objectSize = 0;
    _memStart = (char*) currentHeader;
}

void * allocateObject( size_t size )
{
    //Make sure that allocator is initialized
    if ( !_initialized )
    {
        _initialized = 1;
        initialize();
    }

    // Add the ObjectHeader/Footer to the size and round the total size up to a multiple of
    // 8 bytes for alignment.
    size_t roundedSize = (size + sizeof(struct ObjectHeader) + sizeof(struct ObjectFooter) + 7) & ~7;

    //add code by myself
    struct ObjectHeader *ptr = _freeList;
    //traverse the free list
    while (ptr->_next != _freeList)
    {
        if (ptr->_next->_objectSize >= roundedSize)
        {
            //get memory from freeList
            //the given memeory does not include the header, so change the position of the pointer
            void * _mem = (void *) ((char *)ptr->_next + sizeof(struct ObjectHeader));
            //save freeList->_next node;
            struct ObjectHeader* temp = ptr->_next;
            ptr->_next = (struct ObjectHeader *)((char *)ptr->_next + roundedSize);
            //update freeList->_next

            //reomove the list from freeList
            if ((temp->_objectSize - roundedSize) > (8 + sizeof(struct ObjectHeader) + sizeof(struct ObjectFooter)))
            {
                ptr->_next->_objectSize = temp->_objectSize - roundedSize;
                ptr->_next->_allocated = 0;
                ptr->_next->_prev = temp->_prev;
                ptr->_next->_next = temp->_next;
                //update the pointer of footer
                struct ObjectFooter *foot = (struct ObjectFooter *)((char *)ptr->_next + (ptr->_next->_objectSize - sizeof(struct ObjectFooter)));
                //update the information of footer
                foot->_objectSize = ptr->_next->_objectSize;
                foot->_allocated = 0;
            }
            else
            {
                //simply remove the whole block
                temp->_prev->_next = temp->_next;
                temp->_next->_prev = temp->_prev;
            }

            //update header which will be used in memory
            struct ObjectHeader *header =  temp;     
            header->_objectSize = roundedSize;
            header->_allocated = 1;
            header->_prev = NULL;
            header->_next = NULL;

            //update footer which will be used in memory
            struct ObjectFooter *footer = (struct ObjectFooter *)((char*)header + roundedSize - sizeof(struct ObjectFooter));
            footer->_objectSize = roundedSize;
            footer->_allocated = 1;

            pthread_mutex_unlock(&mutex);

            return (void *)_mem;
        }
        ptr = ptr -> _next;
    }
    //new memory should be got from OS
    void * _mem = getMemoryFromOS(ArenaSize + 2 * sizeof(struct ObjectHeader) + 2 * sizeof(struct ObjectFooter));
    //header h points to the header of the chunk of memory
    struct ObjectHeader *h = (struct ObjectHeader *)((char *)_mem + sizeof(struct ObjectFooter));

    //add fensepost
    struct ObjectFooter *fencepost1 = (struct ObjectFooter *)_mem;
    fencepost1->_objectSize = 123456789;
    fencepost1->_allocated = 1;
    struct ObjectHeader *fencepost2 = (struct ObjectHeader *)((char *)_mem + ArenaSize + sizeof(struct ObjectHeader) + 2 * sizeof(struct ObjectFooter));
    fencepost2->_objectSize = 123456789;
    fencepost2->_allocated = 1;
    fencepost2->_prev = NULL;
    fencepost2->_next = NULL;

    //initialize header and create footer for the chunk that will be remain in the freeList
    struct ObjectHeader *header = (struct ObjectHeader *)((char *)h + roundedSize);
    header->_objectSize = ArenaSize + sizeof(struct ObjectHeader) + sizeof(struct ObjectFooter) - roundedSize;
    header->_allocated = 0;
    header->_prev = _freeList;
    header->_next= (_freeList->_next)->_next;
    struct ObjectFooter *footer = (struct ObjectFooter *)((char*)fencepost2 - sizeof(struct ObjectFooter));
    footer->_objectSize = ArenaSize + sizeof(struct ObjectHeader) + sizeof(struct ObjectFooter) - roundedSize;
    footer->_allocated = 0;

    //update the memory block which will be returned
    ptr = _freeList;
    h->_objectSize = roundedSize;
    h->_allocated = 1;
    h->_prev = ptr;
    h->_next = header;
    //create footer
    struct ObjectFooter *f  = (struct ObjectFooter *)((char*)h + roundedSize - sizeof(struct ObjectFooter));
    f->_objectSize = roundedSize;
    f->_allocated = 1;

    //link the old freeList to the new free chunk if the size of remaining block is not so small 
    if ((header->_objectSize - roundedSize) > (8 + sizeof(struct ObjectHeader) + sizeof(struct ObjectFooter)))
        _freeList->_next = header;

    void *new_mem = (void *)((char *)h + sizeof(struct ObjectHeader));

    pthread_mutex_unlock(&mutex);

    return (void *)new_mem;

    //add code by myself end

    // Naively get memory from the OS every time
    //void * _mem = getMemoryFromOS( roundedSize );

    // Store the size in the header
    //struct ObjectHeader * o = (struct ObjectHeader *) _mem;

    //o->_objectSize = roundedSize;

    //pthread_mutex_unlock(&mutex);

    // Return a pointer to usable memory
    //return (void *) (o + 1);

}

void freeObject( void * ptr )
{
    // Add your code here
    struct ObjectHeader *header = (struct ObjectHeader *)((char *)ptr - sizeof(struct ObjectHeader));
    header->_allocated = 0;
    struct ObjectFooter *footer = (struct ObjectFooter *)((char *)header + header->_objectSize - sizeof(struct ObjectFooter));
    footer->_allocated = 0;

    //ptr2 is used to iterate the list
    struct ObjectHeader * ptr2 = _freeList;
    //check whether the freeList is free
    if (_freeList->_next == _freeList)
    {
        header->_prev = _freeList;
        header->_next = _freeList;
        _freeList->_prev = header;
        _freeList->_next = header;
    }

    else
    {
        while (ptr2->_next != _freeList)
        {
            if ((void *)ptr2->_next > ptr)
            {
                //found where should the block be added back
                //preparation for coalescing
                struct ObjectFooter *leftFooter = (struct ObjectFooter *)((char *)header - sizeof(struct ObjectFooter));
                struct ObjectHeader *rightHeader = (struct ObjectHeader *)((char *)header+ header->_objectSize);
                //add the block back to the list
                struct ObjectHeader *leftToPtr = (ptr2->_next)->_prev;
                struct ObjectHeader *rightToPtr = ptr2->_next;
                header->_prev = leftToPtr;
                leftToPtr->_next = header;
                header->_next = rightToPtr;
                rightToPtr->_prev = header;

                if (leftFooter->_allocated == 0)
                {
                    //coalescing left
                    leftToPtr->_objectSize = leftToPtr->_objectSize + header->_objectSize;
                    leftToPtr->_allocated = 0;
                    leftToPtr->_next = rightToPtr;
                    rightToPtr->_prev = leftToPtr;
                    //update information in footer
                    //first found the location of the footer
                    struct ObjectFooter *tempFooter = (struct ObjectFooter *)((char *)leftToPtr + leftToPtr->_objectSize - sizeof(struct ObjectFooter));
                    tempFooter->_objectSize = leftToPtr->_objectSize;
                    tempFooter->_allocated = 0;

                    header = leftToPtr;     
                }
                if (rightHeader->_allocated == 0)
                {
                    //coalescing right
                    header->_objectSize = header->_objectSize + rightToPtr->_objectSize;
                    header->_allocated = 0;
                    header->_next = rightToPtr->_next;
                    (rightToPtr->_next)->_prev = header;
                    //update information in footer
                    //first found the location of the footer
                    struct ObjectFooter *tempFooter2 = (struct ObjectFooter *)((char *)header + header->_objectSize - sizeof(struct ObjectFooter));
                    tempFooter2->_objectSize = header->_objectSize;
                    tempFooter2->_allocated = 0;
                }
                break;
            }
            ptr2 = ptr2->_next;
        }
    }
    //add my code end
    return;

}

size_t objectSize( void * ptr )
{
    // Return the size of the object pointed by ptr. We assume that ptr is a valid obejct.
    struct ObjectHeader * o =
        (struct ObjectHeader *) ( (char *) ptr - sizeof(struct ObjectHeader) );

    // Substract the size of the header
    return o->_objectSize;
}

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

void print_list()
{
    printf("FreeList: ");
    if ( !_initialized )
    {
        _initialized = 1;
        initialize();
    }
    struct ObjectHeader * ptr = _freeList->_next;
    while(ptr != _freeList)
    {
        long offset = (long)ptr - (long)_memStart;
        printf("[offset:%ld,size:%zd]",offset,ptr->_objectSize);
        ptr = ptr->_next;
        if(ptr != NULL)
        {
            printf("->");
        }
    }
    printf("\n");
}

void * getMemoryFromOS( size_t size )
{
    // Use sbrk() to get memory from OS
    _heapSize += size;

    void * _mem = sbrk( size );

    if(!_initialized)
    {
        _memStart = _mem;
    }

    _numChunks++;

    return _mem;
}

void atExitHandler()
{
    // Print statistics when exit
    if ( _verbose )
    {
        print();
    }
}

//
// C interface
//

extern void *
malloc(size_t size)
{
    pthread_mutex_lock(&mutex);
    increaseMallocCalls();

    return allocateObject( size );
}

extern void
free(void *ptr)
{
    pthread_mutex_lock(&mutex);
    increaseFreeCalls();

    if ( ptr == 0 )
    {
        // No object to free
        pthread_mutex_unlock(&mutex);
        return;
    }

    freeObject( ptr );
}

extern void *
realloc(void *ptr, size_t size)
{
    pthread_mutex_lock(&mutex);
    increaseReallocCalls();

    // Allocate new object
    void * newptr = allocateObject( size );

    // Copy old object only if ptr != 0
    if ( ptr != 0 )
    {

        // copy only the minimum number of bytes
        size_t sizeToCopy =  objectSize( ptr );
        if ( sizeToCopy > size )
        {
            sizeToCopy = size;
        }

        memcpy( newptr, ptr, sizeToCopy );

        //Free old object
        freeObject( ptr );
    }

    return newptr;
}

extern void *
calloc(size_t nelem, size_t elsize)
{
    pthread_mutex_lock(&mutex);
    increaseCallocCalls();

    // calloc allocates and initializes
    size_t size = nelem * elsize;

    void * ptr = allocateObject( size );

    if ( ptr )
    {
        // No error
        // Initialize chunk with 0s
        memset( ptr, 0, size );
    }

    return ptr;
}

