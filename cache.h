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
  (*block).tag = newTag;
  (*block).valid = 1;
  (*block).dirty = access_type; // access_type (0 for a read and 1 for a write) should be used to set/update the dirty bit.
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

  for(i = 0; i < cp->assoc; i++){
    
    block = cp->blocks[set][i];
    block_pointer = &cp->blocks[set][i];
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
  //printf("hit2\n");
  return 0;
}
int cache_access(struct cache_t *cp, 
  unsigned long address, 
  char access_type, 
  int cache_type, 
  unsigned int *accesses, 
  unsigned int *misses, 
  unsigned int *db)
{
  int set_index = (address % (cp->nsets*cp->blocksize))/cp->blocksize;
  int tag = address / (cp->nsets*cp->blocksize);

  //printf("address(hex) : %lx\n", address);

  //printf("cp->blocksize: %d\n", cp->blocksize);
  //printf("cp->nsets: %d\n", cp->nsets);

  // int wordsPerBlock = (cp->blocksize)/BYTES_IN_A_WORD;
  // // printf("%d\n", bsize_w);
  // // Based on "address", determine the set to access in cp and examine the blocks
  // unsigned long word_address = address >> BYTE_OFFSET;
  // // printf("%lu\n", word_address);
  
  // int set_index = ((word_address)%(cp->nsets* cp->assoc))/wordsPerBlock;
  // // int set_index = (word_address / wordsPerBlock) % (cp->nsets * cp->assoc); /*where nsets*assoc=cache size in blocks*/
  
  // //printf("%d\n", set_index);
  // // int tag = ((word_address)/(wordsPerCache/cp->assoc));

  // int block_offset = word_address & (wordsPerBlock - 1); /*blocksize determines # offset bits*/
  // // printf("%d\n", block_offset);

  // int index_bc = log(cp->nsets) / log(2); /*num bits in index*/
  // // printf("index_bc: %d\n", index_bc);
  // int boffset_bc = log(wordsPerBlock) / log(2); /*num bits in block offset*/
  // // printf("boffset bc: %d\n", boffset_bc);
  

  // unsigned long tag_field = word_address >> (index_bc + boffset_bc);
  // //unsigned long tag = (word_address / wordsPerBlock) / (cp->nsets * cp->assoc);
  // unsigned long tag = ((word_address)/(cp->nsets* cp->assoc));


  // printf("(index_bc + boffset_bc): %d\n", (index_bc + boffset_bc));
  // printf("word_address: %lu\n", word_address);
  // printf("word_address >> 1: %lu\n", word_address >> 1);
  // printf("word_address >> 2: %lu\n", word_address >> 2);
  // printf("word_address >> 3: %lu\n", word_address >> 3);
  // printf("word_address >> 4: %lu\n", word_address >> 4);
  // printf("word_address >> 5: %lu\n", word_address >> 5);
  // printf("word_address >> 6: %lu\n", word_address >> 6);
  // printf("word_address >> 7: %lu\n", word_address >> 7);
  // printf("word_address >> 8: %lu\n", word_address >> 8);
  // printf("word_address >> 9: %lu\n", word_address >> 9);
  // printf("word_address >> 10: %lu\n", word_address >> 10);
  // printf("word_address >> 11: %lu\n", word_address >> 11);
  // printf("word_address >> 12: %lu\n", word_address >> 12);
  // printf("word_address >> 13: %lu\n", word_address >> 13);
  // printf("word_address >> 14: %lu\n", word_address >> 14);
  // printf("word_address >> 15: %lu\n", word_address >> 15);
  // printf("word_address >> 16: %lu\n", word_address >> 16);
  // printf("set: %d\n\n", set_index);
  // printf("tag: %d\n\n", tag);

  if(cp->blocks[set_index][tag].valid)
    *db = cp->blocks[set_index][tag].dirty;
  // printf("%d\n", db);

  int wb;
  wb = cache_check(cp, tag, set_index, access_type, cache_type, accesses, misses);

  return (wb*(cp->mem_latency));
}