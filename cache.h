#include <stdlib.h>
#include <stdio.h>

struct cache_blk_t { /* note that no actual data will be stored in the cache */
  unsigned long tag;
  char valid;
  char dirty;
  unsigned LRU;	/*to be used to build the LRU stack for the blocks in a cache set*/
};

struct cache_t {
	// The cache is represented by a 2-D array of blocks. 
	// The first dimension of the 2D array is "nsets" which is the number of sets (entries)
	// The second dimension is "assoc", which is the number of blocks in each set.
  int nsets;					// number of sets
  int blocksize;				// block size
  int assoc;					// associativity
  int mem_latency;				// the miss penalty
  struct cache_blk_t **blocks;	// a pointer to the array of cache blocks
};

struct cache_t *
	cache_create(int size, int blocksize, int assoc, int mem_latency)
{
  int i;
  int nblocks = 1;			// number of blocks in the cache
  int nsets = 1;			// number of sets (entries) in the cache

  // YOUR JOB: calculate the number of sets and blocks in the cache
  //
  // nblocks = X;
  // nsets = Y;

  struct cache_t *C = (struct cache_t *)calloc(1, sizeof(struct cache_t));
		
  C->nsets = nsets; 
  C->assoc = assoc;
  C->mem_latency = mem_latency;

  C->blocks= (struct cache_blk_t **)calloc(nsets, sizeof(struct cache_blk_t *));

  for(i = 0; i < nsets; i++) {
		C->blocks[i] = (struct cache_blk_t *)calloc(assoc, sizeof(struct cache_blk_t));
	}

  return C;
}

int cache_access(struct cache_t *cp, unsigned long address, int access_type)
{
  //
  // Based on "address", determine the set to access in cp and examine the blocks
  // in the set to check hit/miss and update the golbal hit/miss statistics
  // If a miss, determine the victim in the set to replace (LRU). 
  //
  // The function should return the hit_latency, which is 0, in case of a hit.
  // In case of a miss, the function should return mem_latency if no write back is needed.
  // If a write back is needed, the function should return 2*mem_latency.
  // access_type (0 for a read and 1 for a write) should be used to set/update the dirty bit.
  // The LRU field of the blocks in the set accessed should also be updated.

	return(cp->mem_latency);
}