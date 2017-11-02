#include <stdlib.h>
#include <stdio.h>
#define MEMORY_LATENCY_DEFAULT 20
#define KILO 1024
#define BYTES_IN_A_WORD 4
#define BYTE_OFFSET 2
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

    //printf("%d\n", C->blocks[i]->valid);
  }

  return C;
}
void hit_miss_update(int hit, unsigned int *accesses, unsigned int *misses){
  (*accesses)++;
  if(!hit) (*misses)++;
}
// The LRU field of the blocks in the set accessed should also be updated.
void lru_update(struct cache_t *cp, int set, struct cache_blk_t* block)
{
  int j;
  for(j = 0; j < cp->assoc; j++){
    if(cp->blocks[set][j].LRU > (*block).LRU)
      cp->blocks[set][j].LRU--;
  }
  (*block).LRU = cp->assoc;
}
void cache_update(struct cache_blk_t* block, struct cache_t *cp, unsigned long newTag, int set, char access_type)
{
  //printf("access_type inner: %d\n", access_type);
  (*block).tag = newTag;
  (*block).valid = 1;
  if (access_type == 1)
    (*block).dirty = 1; // access_type (0 for a read and 1 for a write) should be used to set/update the dirty bit.
  //printf("dirty inner: %d\n", (*block).dirty);
  
  lru_update(cp, set, block);
}
// The function should return the hit_latency, which is 0, in case of a hit.
int cache_check(struct cache_t *cp, int newTag, int set, char access_type, int cache_type, unsigned int *accesses, unsigned int *misses)
{
  int i;
  int wb = 0;
  struct cache_blk_t block, *block_pointer, *open_blk, *lru_blk;
  open_blk = NULL;
  lru_blk = NULL;
  //if (*db == 1 && newTag != block.tag)

  for(i = 0; i < cp->assoc; i++){

    block = cp->blocks[set][i];
    block_pointer = &cp->blocks[set][i];

  // if (*db == 1 && newTag != block.tag)
  //   printf("writeback");
    // printf("block.tag: %lu\n", block.tag);
    // printf("block.valid: %d\n", block.valid);
    // printf("block.LRU: %d\n", block.LRU);
    // printf("set: %d\n", set);
    // printf("tag: %d\n", newTag);
    // printf("block.valid: %d\n", block.valid);
    // printf("block.LRU: %d\n", block.LRU);

    if (block.tag == newTag && block.valid == 1 && block.LRU > 0){ //check if hit
      hit_miss_update(1, accesses, misses);
      lru_update(cp, set, &block);
      //printf("hit1\n");
      return 0;
    }
    else if(block.valid == 0 || block.LRU == 0){ //check if there's an open slot
      open_blk = block_pointer;
    }
    else if(block.LRU == 1){ //find lru block
      lru_blk = block_pointer;
    }
  }
  // If a miss, determine the victim in the set to replace (LRU). 
  if(open_blk != NULL){
      //printf("miss1\n");
    hit_miss_update(0, accesses, misses);
    cache_update(open_blk, cp, newTag, set, access_type);
    return 1; // In case of a miss, the function should return mem_latency if no write back is needed.
  }
  else if(lru_blk != NULL)
  { //queue is full: remove lru and determine wb
    if((int)(*lru_blk).dirty){ //wb
      wb = 1;
    }
    //printf("Other\n");
    hit_miss_update(0, accesses, misses);
    cache_update(lru_blk, cp, newTag, set, access_type);
    return ++wb;
    // If a write back is needed, the function should return 2*mem_latency.
    // In case of a miss, the function should return mem_latency if no write back is needed.
  }
  printf("hit2\n");
  return 0;
}
int cache_access(
  struct cache_t *cp, 
  unsigned long address, 
  char access_type, 
  int cache_type, 
  unsigned int *accesses, 
  unsigned int *misses, 
  unsigned int *db)
{
  printf("melp\n");

  int set_index = (address % (cp->nsets*cp->blocksize))/cp->blocksize;
  int tag = address / (cp->nsets*cp->blocksize);

    printf("address: %lu\n", address);
    printf("set_index: %d\n", set_index);
    printf("tag: %d\n", tag);

  if (access_type == 1)
    *db = 1;

  else if ((access_type == 0 && cp->blocks[set_index][tag].valid == 0) || (access_type == 0 && cp->blocks[set_index][tag].tag != tag))
    *db = 0;


    printf("zelp\n");


  //printf("cp->blocks[set_index][tag].valid: %d\n", cp->blocks[set_index][tag].valid);
  // if(cp->blocks[set_index][tag].valid)
  //   *db = cp->blocks[set_index][tag].dirty;
  // printf("%d\n", db);
  int wb;
  wb = cache_check(cp, tag, set_index, access_type, cache_type, accesses, misses);

  return (wb*(cp->mem_latency));
}