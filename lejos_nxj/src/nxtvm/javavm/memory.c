
#include "types.h"
#include "trace.h"
#include "constants.h"
#include "specialsignatures.h"
#include "specialclasses.h"
#include "memory.h"
#include "threads.h"
#include "classes.h"
#include "language.h"
#include "configure.h"
#include "interpreter.h"
#include "exceptions.h"
#include "stdlib.h"

#include <string.h>

#ifdef VERIFY
static boolean memoryInitialized = false;
#endif

// Heap memory needs to be aligned to 4 bytes on ARM
// Value is in 2-byte units and must be a power of 2
#define MEMORY_ALIGNMENT 2

#define NULL_OFFSET 0xFFFF

// Size of stack frame in 2-byte words
#define NORM_SF_SIZE ((sizeof(StackFrame) + 1) / 2)

const byte typeSize[] = { 
  4, // 0 == T_REFERENCE
  SF_SIZE, // 1 == T_STACKFRAME
  0, // 2
  0, // 3
  1, // 4 == T_BOOLEAN
  2, // 5 == T_CHAR
  4, // 6 == T_FLOAT
  8, // 7 == T_DOUBLE
  1, // 8 == T_BYTE
  2, // 9 == T_SHORT
  4, // 10 == T_INT
  8  // 11 == T_LONG
};

typedef struct MemoryRegion_S {
#if SEGMENTED_HEAP
  struct MemoryRegion_S *next;  /* pointer to next region */
#endif
  TWOBYTES *end;                /* pointer to end of region */
  TWOBYTES contents;            /* start of contents, even length */
} MemoryRegion;

/**
 * Beginning of heap.
 */
#if SEGMENTED_HEAP
static MemoryRegion *memory_regions; /* list of regions */
#else
static MemoryRegion *region; /* list of regions */
#endif

static TWOBYTES memory_size;    /* total number of words in heap */
static TWOBYTES memory_free;    /* total number of free words in heap */

extern void deallocate (TWOBYTES *ptr, TWOBYTES size);
extern TWOBYTES *allocate (TWOBYTES size);

/**
 * @param numWords Number of 2-byte words used in allocating the object.
 */
#define initialize_state(OBJ_,NWORDS_) zero_mem(((TWOBYTES *) (OBJ_)) + NORM_OBJ_SIZE, (NWORDS_) - NORM_OBJ_SIZE)
#define get_object_size(OBJ_)          (get_class_record(get_na_class_index(OBJ_))->classSize)

/**
 * Zeroes out memory.
 * @param ptr The starting address.
 * @param numWords Number of two-byte words to clear.
 */
void zero_mem (register TWOBYTES *ptr, register TWOBYTES numWords)
{
  while (numWords--)
    *ptr++ = 0;
}

static inline void set_array (Object *obj, const byte elemType, const TWOBYTES length)
{
  #ifdef VERIFY
  assert (elemType <= (ELEM_TYPE_MASK >> ELEM_TYPE_SHIFT), MEMORY0); 
  assert (length <= (ARRAY_LENGTH_MASK >> ARRAY_LENGTH_SHIFT), MEMORY1);
  #endif
  obj->flags.all = IS_ALLOCATED_MASK | IS_ARRAY_MASK | ((TWOBYTES) elemType << ELEM_TYPE_SHIFT) | length;
  #ifdef VERIFY
  assert (is_array(obj), MEMORY3);
  #endif
}

Object *memcheck_allocate (const TWOBYTES size)
{
  Object *ref;
  ref = (Object *) allocate (size);
  if (ref == JNULL)
  {
    #ifdef VERIFY
    assert (outOfMemoryError != null, MEMORY5);
    #endif
    throw_exception (outOfMemoryError);
    return JNULL;
  }
  
  ref->monitorCount = 0;
  ref->threadId = 0;
#if SAFE
  ref->flags.all = 0;
#endif
  return ref;
}

/**
 * Checks if the class needs to be initialized.
 * If so, the static initializer is dispatched.
 * Otherwise, an instance of the class is allocated.
 *
 * @param btAddr Back-track PC address, in case
 *               a static initializer needs to be invoked.
 * @return Object reference or <code>null</code> iff
 *         NullPointerException had to be thrown or
 *         static initializer had to be invoked.
 */
Object *new_object_checked (const byte classIndex, byte *btAddr)
{
  #if 0
  trace (-1, classIndex, 0);
  #endif
  if (dispatch_static_initializer (get_class_record(classIndex), btAddr))
  {
#if DEBUG_MEMORY
  printf("New object checked returning null\n");
#endif
    return JNULL;
  }   
  return new_object_for_class (classIndex);
}

/**
 * Allocates and initializes the state of
 * an object. It does not dispatch static
 * initializers.
 */
Object *new_object_for_class (const byte classIndex)
{
  Object *ref;
  TWOBYTES instanceSize;

#if DEBUG_MEMORY
  printf("New object for class %d\n", classIndex);
#endif
  instanceSize = get_class_record(classIndex)->classSize;
  
  ref = memcheck_allocate (instanceSize);

  if (ref == null)
  {
#if DEBUG_MEMORY
  printf("New object for class returning null\n");
#endif
    return JNULL;
  }

  // Initialize default values

  ref->flags.all = IS_ALLOCATED_MASK | classIndex;

  initialize_state (ref, instanceSize);

  #if DEBUG_OBJECTS || DEBUG_MEMORY
  printf ("new_object_for_class: returning %d\n", (int) ref);
  #endif
  
  return ref;
}

/**
 * Return the size in words of an array of the given type
 */
TWOBYTES comp_array_size (const TWOBYTES length, const byte elemType)
{
  return NORM_OBJ_SIZE + (((TWOBYTES) length * typeSize[elemType]) + 1) / 2;
}

/**
 * Allocates an array. The size of the array is NORM_OBJ_SIZE
 * plus the size necessary to contain <code>length</code> elements
 * of the given type.
 */
Object *new_primitive_array (const byte primitiveType, STACKWORD length)
{
  Object *ref;
  TWOBYTES allocSize;

  // Check if length is too large to be representable
  if (length > (ARRAY_LENGTH_MASK >> ARRAY_LENGTH_SHIFT))
  {
    throw_exception (outOfMemoryError);
    return JNULL;
  }
  allocSize = comp_array_size (length, primitiveType);
#if DEBUG_MEMORY
  printf("New array of type %d, length %ld\n", primitiveType, length);
#endif
  ref = memcheck_allocate (allocSize);
#if DEBUG_MEMORY
  printf("Array ptr=%d\n", (int)ref);
#endif
  if (ref == null)
    return JNULL;
  set_array (ref, primitiveType, length);
  initialize_state (ref, allocSize);
  return ref;
}

TWOBYTES get_array_size (Object *obj)
{
  return comp_array_size (get_array_length (obj),
                          get_element_type (obj));	
}

void free_array (Object *objectRef)
{
  #ifdef VERIFY
  assert (is_array(objectRef), MEMORY7);
  #endif // VERIFY

  deallocate ((TWOBYTES *) objectRef, get_array_size (objectRef));
}

#if !FIXED_STACK_SIZE
Object *reallocate_array(Object *obj, STACKWORD newlen)
{
	byte elemType = get_element_type(obj);
 	Object *newArray = new_primitive_array(elemType, newlen);
  	
  	// If can't allocate new array, give in!
    if (newArray != JNULL)
    {
    	// Copy old array to new
    	memcpy(((byte *) newArray + HEADER_SIZE), ((byte *) obj + HEADER_SIZE), get_array_length(obj) * typeSize[elemType]);
    
  		// Free old array
  		free_array(obj);
    }
    
    return newArray;
}
#endif

/**
 * @param elemType Type of primitive element of multi-dimensional array.
 * @param totalDimensions Same as number of brackets in array class descriptor.
 * @param reqDimensions Number of requested dimensions for allocation.
 * @param numElemPtr Pointer to first dimension. Next dimension at numElemPtr+1.
 */
Object *new_multi_array (byte elemType, byte totalDimensions, 
                         byte reqDimensions, STACKWORD *numElemPtr)
{
  Object *ref;

  #ifdef WIMPY_MATH
  Object *aux;
  TWOBYTES ne;
  #endif

  #ifdef VERIFY
  assert (totalDimensions >= 1, MEMORY6);
  assert (reqDimensions <= totalDimensions, MEMORY8);
  #endif

  #if 0
  printf ("new_multi_array (%d, %d, %d)\n", (int) elemType, (int) totalDimensions, (int) reqDimensions);
  #endif

  if (reqDimensions == 0)
    return JNULL;

  #if 0
  printf ("num elements: %d\n", (int) *numElemPtr);
  #endif

  if (totalDimensions == 1)
    return new_primitive_array (elemType, *numElemPtr);


  ref = new_primitive_array (T_REFERENCE, *numElemPtr);
  if (ref == JNULL)
    return JNULL;

  while ((*numElemPtr)--)
  {
    #ifdef WIMPY_MATH

    aux = new_multi_array (elemType, totalDimensions - 1, reqDimensions - 1,
      numElemPtr + 1);
    ne = *numElemPtr;
    ref_array(ref)[ne] = ptr2word (aux);

    #else

    ref_array(ref)[*numElemPtr] = ptr2word (new_multi_array (elemType, totalDimensions - 1, reqDimensions - 1, numElemPtr + 1));

    #endif // WIMPY_MATH
  }


  return ref;
}

#ifdef WIMPY_MATH

void store_word (byte *ptr, byte aSize, STACKWORD aWord)
{
  byte *wptr;
  byte ctr;

  wptr = &aWord;
  ctr = aSize;
  while (ctr--)
  {
    #if LITTLE_ENDIAN
    ptr[ctr] = wptr[aSize-ctr-1];
    #else
    ptr[ctr] = wptr[ctr];
    #endif // LITTLE_ENDIAN
  }
}

#else
/**
 * Problem here is bigendian v. littleendian. Java has its
 * words stored bigendian, intel is littleendian.
 */
STACKWORD get_word(byte *ptr, byte aSize)
{
  STACKWORD aWord = 0;
  while (aSize--)
  {
    aWord = (aWord << 8) | (STACKWORD)(*ptr++);
  }
  
  return aWord;
}

void store_word (byte *ptr, byte aSize, STACKWORD aWord)
{
  ptr += aSize-1;
  while (aSize--)
  {
    *ptr-- = (byte) aWord;
    aWord = aWord >> 8;
  }
}

#endif // WIMPY_MATH

typedef union 
{
  struct
  {
    byte byte0;
    byte byte1;
    byte byte2;
    byte byte3;
  } st;
  STACKWORD word;
} AuxStackUnion;

void make_word (byte *ptr, byte aSize, STACKWORD *aWordPtr)
{
  // This switch statement is 
  // a workaround for a h8300-gcc bug.
  switch (aSize)
  {
    case 1:
      *aWordPtr = (JINT) (JBYTE) (ptr[0]);
      return;
    case 2:
      *aWordPtr = (JINT) (JSHORT) (((TWOBYTES) ptr[0] << 8) | (ptr[1]));
      return;
    #ifdef VERIFY
    default:
      assert (aSize == 4, MEMORY9);
    #endif // VERIFY
  }
  #if LITTLE_ENDIAN
  ((AuxStackUnion *) aWordPtr)->st.byte3 = *ptr++;  
  ((AuxStackUnion *) aWordPtr)->st.byte2 = *ptr++;  
  ((AuxStackUnion *) aWordPtr)->st.byte1 = *ptr++;  
  ((AuxStackUnion *) aWordPtr)->st.byte0 = *ptr;  
  #else
  ((AuxStackUnion *) aWordPtr)->st.byte0 = *ptr++;  
  ((AuxStackUnion *) aWordPtr)->st.byte1 = *ptr++;  
  ((AuxStackUnion *) aWordPtr)->st.byte2 = *ptr++;  
  ((AuxStackUnion *) aWordPtr)->st.byte3 = *ptr;  
  #endif
}

#if DEBUG_RCX_MEMORY

void scan_memory (TWOBYTES *numNodes, TWOBYTES *biggest, TWOBYTES *freeMem)
{
}

#endif // DEBUG_RCX_MEMORY


void memory_init ()
{
  #ifdef VERIFY
  memoryInitialized = true;
  #endif

#if SEGMENTED_HEAP
  memory_regions = null;
#endif
  memory_size = 0;
  memory_free = 0;
}

/**
 * @param region Beginning of region.
 * @param size Size of region in bytes.
 */
void memory_add_region (byte *start, byte *end)
{
#if SEGMENTED_HEAP
  MemoryRegion *region;
#endif
  TWOBYTES contents_size;

  /* word align upwards */
  region = (MemoryRegion *) (((unsigned int)start+1) & ~1);

#if SEGMENTED_HEAP
  /* initialize region header */
  region->next = memory_regions;

  /* add to list */
  memory_regions = region;
#endif
  region->end = (TWOBYTES *) ((unsigned int)end & ~1); /* 16-bit align
 downwards */

  /* create free block in region */
  contents_size = region->end - &(region->contents);
  ((Object*)&(region->contents))->flags.all = contents_size;

  /* memory accounting */
  memory_size += contents_size;
  memory_free += contents_size;

#if SEGMENTED_HEAP
  #if DEBUG_MEMORY
  printf ("Added memory region\n");
  printf ("  start:          %5d\n", (int)start);
  printf ("  end:            %5d\n", (int)end);
  printf ("  region:         %5d\n", (int)region);
  printf ("  region->next:   %5d\n", (int)region->next);
  printf ("  region->end:    %5d\n", (int)region->end);
  printf ("  memory_regions: %5d\n", (int)memory_regions);
  printf ("  contents_size:  %5d\n", (int)contents_size);
  #endif
#endif
}


/**
 * @param size Size of block including header in 2-byte words.
 */
TWOBYTES *allocate (TWOBYTES size)
{
  // Align memory to boundary appropriate for system	
  size = (size + (MEMORY_ALIGNMENT-1)) & ~(MEMORY_ALIGNMENT-1);

#if SEGMENTED_HEAP
  MemoryRegion *region;

#endif

#if DEBUG_MEMORY
  printf("Allocate %d - free %d\n", size, memory_free-size);
#endif

#if SEGMENTED_HEAP
  for (region = memory_regions; region != null; region = region->next)
#endif
  {
    TWOBYTES *ptr = &(region->contents);
    TWOBYTES *regionTop = region->end;

    while (ptr < regionTop) {
      TWOBYTES blockHeader = *ptr;


      if (blockHeader & IS_ALLOCATED_MASK) {
        /* jump over allocated block */
        TWOBYTES s = (blockHeader & IS_ARRAY_MASK) 
          ? get_array_size ((Object *) ptr)
          : get_object_size ((Object *) ptr);
          
        // Round up according to alignment
        s = (s + (MEMORY_ALIGNMENT-1)) & ~(MEMORY_ALIGNMENT-1);
        ptr += s;
      }
      else
      {
        if (size <= blockHeader) {
          /* allocate from this block */

#if COALESCE
          {
            TWOBYTES nextBlockHeader;

            /* NOTE: Theoretically there could be adjacent blocks
               that are too small, so we never arrive here and fail.
               However, in practice this should suffice as it keeps
               the big block at the beginning in one piece.
               Putting it here saves code space as we only have to
               search through the heap once, and deallocat remains
               simple.
            */
            
            while (true) {
              TWOBYTES *next_ptr = ptr + blockHeader;
              nextBlockHeader = *next_ptr;
              if (next_ptr >= regionTop ||
                  (nextBlockHeader & IS_ALLOCATED_MASK) != 0)
                break;
              blockHeader += nextBlockHeader;
              *ptr = blockHeader;
            }
          }
#endif
          if (size < blockHeader) {
            /* cut into two blocks */
            blockHeader -= size; /* first block gets remaining free space */
            *ptr = blockHeader;
            ptr += blockHeader; /* second block gets allocated */
#if SAFE
            *ptr = size;
#endif
            /* NOTE: allocating from the top downwards avoids most
                     searching through already allocated blocks */
          }
          memory_free -= size;

          return ptr;
        } else {
          /* continue searching */
          ptr += blockHeader;
        }
      }
    }
  }
  /* couldn't allocate block */
  return JNULL;
}

/**
 * @param size Must be exactly same size used in allocation.
 */
void deallocate (TWOBYTES *p, TWOBYTES size)
{
  // Align memory to boundary appropriate for system	
  size = (size + (MEMORY_ALIGNMENT-1)) & ~(MEMORY_ALIGNMENT-1);
  
  #ifdef VERIFY
  assert (size <= (FREE_BLOCK_SIZE_MASK >> FREE_BLOCK_SIZE_SHIFT), MEMORY3);
  #endif

  memory_free += size;

#if DEBUG_MEMORY
  printf("Deallocate %d at %d - free %d\n", size, (int)p, memory_free);
#endif

  ((Object*)p)->flags.all = size;
}


int getHeapSize() {
  return ((int)memory_size) << 1;
}


int getHeapFree() {
  return ((int)memory_free) << 1;
}

int getRegionAddress()
{
#if SEGMENTED_HEAP
	return 0xf002;
#else
	return (int)region;
#endif
}
