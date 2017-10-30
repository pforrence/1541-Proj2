#include <stdlib.h>
#include <stdio.h>
#define MEMORY_LATENCY_DEFAULT 20
#define KILO 1024
#include <math.h>


struct cache_blk_t { /* note that no actual data will be stored in the cache */
  unsigned long tag;
  char valid;
  char dirty;
  unsigned LRU; /*to be used to build the LRU stack for the blocks in a cache set*/
};

struct cache_t {
  // The cache is represented by a 2-D array of blocks. 
  // The first dimension of the 2D array is "nsets" which is the number of sets (entries)
  // The second dimension is "assoc", which is the number of blocks in each set.
  int nsets;          // number of sets
  int blocksize;        // block size
  int assoc;          // associativity
  int mem_latency;        // the miss penalty
  struct cache_blk_t **blocks;  // a pointer to the array of cache blocks
};

struct cache_t *
  cache_create(int size, int blocksize, int assoc, int mem_latency)
{
  int i;
  int nblocks = 1;      // number of blocks in the cache
  int nsets = 1;      // number of sets (entries) in the cache

  // YOUR JOB: calculate the number of sets and blocks in the cache
  //
  nblocks = (KILO*size)/blocksize; //so long as block and cache size are both measured in the same metric
  nsets = nblocks/assoc;//

  struct cache_t *C = (struct cache_t *)calloc(1, sizeof(struct cache_t));
    
  C->nsets = nsets; 
  C->blocksize = blocksize;
  C->assoc = assoc;
  C->mem_latency = mem_latency;

  C->blocks= (struct cache_blk_t **)calloc(nsets, sizeof(struct cache_blk_t *));

  for(i = 0; i < nsets; i++) {
    C->blocks[i] = (struct cache_blk_t *)calloc(assoc, sizeof(struct cache_blk_t));
  }

  return C;
}
void lru_update()
{
  return;
}
void cache_update()
{
  return;
}
int cache_check(struct cache_t *cp, int newTag, int set, int access_type)
{

  for(int i = 0; i < blcksPerSet; i++)
  {
    if (cp->blocks[set].tag == newTag) 
      return 1;
  }

  if (cp->blocks[set].tag == NULL)
    return 0;

  lru_update();

  //if dirtybit of cache access is 1, we write back
  cache_update();
  hit_miss_update();
  if (access_type == 1)
    dirtybit = 1;
  return -1;
}
int cache_access(struct cache_t *cp, unsigned long address, int access_type)
{
  int set = address%(cachesizeinWords/associativity);
  int tag = address/(cachesizeinWords/associativity);

  set = set/wordsPerBlock;

  int hit;
  hit = cache_check(cp, tag, set, access_type);

  if (hit == 1)//if (valid bit == 1 && tag == tag)
    cp->mem_latency = 0;
  else if (hit == 0)//if (valid bit == 0 || (valid bit == 1 && dirtybit == 0))
    cp->mem_latency = MEMORY_LATENCY_DEFAULT;
  else if (hit == -1) //if (valid bit == 1 && dirtybit == 1)
    cp->mem_latency = 2*MEMORY_LATENCY_DEFAULT;

  //
  // Based on "address", determine the set to access in cp and examine the blocks
  int byte_offset = 2;
  unsigned long word_address = address >> byte_offset;
  int block_index = (word_address / cp->blocksize) mod (nsets * assoc); /*where nsets*assoc=cache size in blocks*/
  int block_offset = word_address & (cp->blocksize - 1); /*blocksize, in words, determines # offset bits*/
  
  int index_bc = log(cp->nsets) / log(2);
  int block_offset_bc = log(cp->blocksize) / log(2);
  unsigned long tag_field = address >> (index_bc + block_offset_bc);


  // in the set to check hit/miss and update the global hit/miss statistics
  if(cp->blocks[block_index]->tag == tag_field /*&& valid*/){
    return 0;  // The function should return the hit_latency, which is 0, in case of a hit.
  }
  else{
    // If a miss, determine the victim in the set to replace (LRU). 
    // The LRU field of the blocks in the set accessed should also be updated.
  
  cp->dirty = (char)access_type;  // access_type (0 for a read and 1 for a write) should be used to set/update the dirty bit.
  if(access_type)
    return(2*(cp->mem_latency));  // If a write back is needed, the function should return 2*mem_latency.
    else
      return(cp->mem_latency);  // In case of a miss, the function should return mem_latency if no write back is needed.
  }
}