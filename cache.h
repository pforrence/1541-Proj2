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
void hit_miss_update(struct cache_t *cp, int hit, int dirty_bit){
  if(dirty_bit){ // for d cache only
    write_accesses++;
    if(!hit) write_misses++;
  }
  else{ // for both i and d caches
    read_accesses++;
    if(!hit) read_misses++;
  }
}
// The LRU field of the blocks in the set accessed should also be updated.
void lru_update(struct cache_t *cp, int set, struct cache_blk_t block)
{
  int j;
  struct cache_blk_t old_block;
  for(j = 0; j < cp->assoc; j++){
    old_block = cp->blocks[set][j];
    if(old_block.LRU > block.LRU)
      old_block.LRU --;
  }
  block.LRU = cp->assoc;
}
void cache_update(struct cache_blk_t block, struct cache_t *cp, int newTag, int set, int access_type)
{
  block.tag = newTag;
  block.valid = '1';
  block.dirty = (char)access_type; // access_type (0 for a read and 1 for a write) should be used to set/update the dirty bit.
  lru_update(cp, set, block, 0);
}
// The function should return the hit_latency, which is 0, in case of a hit.
int cache_check(struct cache_t *cp, int newTag, int set, int access_type)
{
  int i;
  int wb = 0;
  struct cache_blk_t block, open_block, lru_block;
  
  for(i = 0; i < cp->assoc; i++){
    block = cp->blocks[set][i];
    if (block.tag == newTag && (int)block.valid && block.LRU > 0){ //check if hit
      hit_miss_update(cp, 1, block.dirty);
      lru_update(cp, set, block);
      return 0;
    }
    else if(!(int)block.valid || block.LRU == 0){ //check if there's an open slot
      open_block = block;
    }
    else if(block.LRU == 1){ //find lru block
      lru_block = block;
    }
  }
  // If a miss, determine the victim in the set to replace (LRU). 
  if(open_block){
    hit_miss_update(cp, 0, 0);
    cache_update(open_block, cp, newTag, set, access_type);
    return 1; // In case of a miss, the function should return mem_latency if no write back is needed.
  }
  else if(lru_block){ //queue is full: remove lru and determine wb
    if((int)lru_block.dirty){ //wb
      wb = 1;
    }
    hit_miss_update(cp, 0, wb);
    cache_update(lru_block, cp, newTag, set, access_type);
    return ++wb;
    // If a write back is needed, the function should return 2*mem_latency.
    // In case of a miss, the function should return mem_latency if no write back is needed.
  }
  return 0;
}
int cache_access(struct cache_t *cp, unsigned long address, int access_type)
{
  //
  int bsize_w = (cp->blocksize)/BIT32; /*blocksize, in words*/
  // Based on "address", determine the set to access in cp and examine the blocks
  int byte_offset = 2;
  unsigned long word_address = address >> byte_offset;
  int set_index = (word_address / bsize_w) mod (nsets * assoc); /*where nsets*assoc=cache size in blocks*/
  int block_offset = word_address & (bsize_w - 1); /*blocksize determines # offset bits*/
  
  int index_bc = log(cp->nsets) / log(2); /*num bits in index*/
  int boffset_bc = log(bsize_w) / log(2); /*num bits in block offset*/
  unsigned long tag_field = address >> (index_bc + boffset_bc);

  int dirty_bit = cp->blocks[set_index][tag_field]->dirty;

  int wb;
  wb = cache_check(cp, tag_field, set_index);

  return (wb*(cp->mem_latency));
}