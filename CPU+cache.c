/**************************************************************/
/* Group Omega  
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <string.h>
#include <math.h>
#include "cache.h"
#include "CPU.h" 


//declarations ___________________________________________________________
struct trace_item *tr_entry;
size_t size;
char *trace_file_name;
int trace_view_on = 0;

unsigned char PRED_METH = 0;  //prediction method state var
unsigned char SPECIAL_METH = 0;
unsigned char t_type = 0;
unsigned char t_sReg_a= 0;
unsigned char t_sReg_b= 0;
unsigned char t_dReg= 0;
unsigned int t_PC = 0;
unsigned int t_Addr = 0;

int hashmap[64];
unsigned int cycle_number = 0;
struct trace_item pipeline[5];
int flush = 5; // 5 stage pipeline takes and additional 5 cycles to flush 

// to keep cache statistics
unsigned int I_accesses = 0;
unsigned int I_misses = 0;
unsigned int D_read_accesses = 0;
unsigned int D_read_misses = 0;
unsigned int D_write_accesses = 0; 
unsigned int D_write_misses = 0;
int latency = 0;

int main(int argc, char **argv)
{

  //Initiailize ___________________________________________________________
  for(int i = 0; i < 64; i++)
  {
    hashmap[i] = -1;
  }

  fillPipe(pipeline);   //intialize pipe

  if (argc == 1) {
    fprintf(stdout, "\nUSAGE: tv <trace_file> <switch - any character>\n");
    fprintf(stdout, "\n(switch) to turn on or off individual item view.\n\n");
    exit(0);
  }


  trace_file_name = argv[1];
  if (argc >= 3) trace_view_on = atoi(argv[2]);

  fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

  trace_fd = fopen(trace_file_name, "rb");
  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  // here you should extract the cache parameters from the configuration file (cache size, associativity, latency)

  FILE *fp;
  int buff[100];

  fp = fopen("cache_config.txt", "r");

  int i;
  for(i = 0; i < 100; i++){
   fscanf(fp, "%d\n", buff + sizeof(int)*i);
  }

  unsigned int I_size = buff[0]; 
  unsigned int I_assoc = buff[4];
  unsigned int I_bsize = buff[8]; 
  unsigned int D_size = buff[12];
  unsigned int D_assoc = buff[16];
  unsigned int D_bsize = buff[20];
  unsigned int mem_latency = buff[24];


  fclose(fp);

  if(argc >= 4)
  {
    PRED_METH = atoi(argv[2]);
  }

  trace_init();

  struct cache_t *I_cache, *D_cache;

  I_cache = cache_create(I_size, I_bsize, I_assoc, mem_latency); 
  D_cache = cache_create(D_size, D_bsize, D_assoc, mem_latency);


//Main Loop ___________________________________________________________
  int count = 100;
  int revert = -1;
  unsigned int misses = 0;
  unsigned int accesses = 0;
  int dirtybit = 0;
  while(1) 
  {
    // count--;
    // if(count == 0)
    //   exit(0);

    step1(trace_view_on, &size, PRED_METH, pipeline, hashmap, I_cache, &tr_entry, &cycle_number, &D_read_accesses, &D_read_misses, &D_write_accesses
      &D_write_misses, &I_accesses, &I_misses);
    step2(trace_view_on, size, &flush, cycle_number, I_accesses, I_misses, D_read_accesses, 
      D_read_misses, D_write_accesses, D_write_misses, &tr_entry, pipeline);
    step3(&t_type, &t_sReg_a, &t_sReg_b, &t_dReg, &t_PC, &t_Addr, &cycle_number, &tr_entry);
    //4. Simulate the next instruction
    struct trace_item temp;
    if(data_hazard(pipeline, trace_view_on))
    {
      if(trace_view_on == 2)
        printf("\nC0: stall...\n");
      temp = stall_flow(pipeline);
    }
    else if (PRED_METH == 1 && (pipeline[3].type == ti_BRANCH) && pipeline[2].type == ti_NOP)
    {
      if(trace_view_on == 2)
        printf("\nC1: stall...\n");
      temp = stall_flow(pipeline);
    }
    else if (PRED_METH == 1 && pipeline[2].type == ti_BRANCH && cannot_predict(hashmap, pipeline[2].PC) && pipeline[2].Addr == pipeline[1].PC)
    {
      if(trace_view_on == 2)
        printf("\nC2: stall...\n");
      hash_update(hashmap, pipeline[2].PC, 1, trace_view_on);
      //SPECIAL_METH = 1;
      temp = stall_flow(pipeline);
    }
    else if (SPECIAL_METH == 1 && PRED_METH == 1)
    {
      if(trace_view_on == 2)
        printf("\nCSPECIAL: flow...\n");

      int result = 0;
      hash_update(hashmap, pipeline[3].PC, result, trace_view_on);
      temp = normal_flow(pipeline, &tr_entry);
      SPECIAL_METH = 0;
    }
    else if (PRED_METH == 1 && pipeline[2].type == ti_BRANCH && !cannot_predict(hashmap, pipeline[2].PC) && mispredict_branch(pipeline, hashmap, pipeline[2].PC)) 
    {
      if(trace_view_on == 2)
        printf("\nC3: stall...\n");
      int result = 0;
      if (pipeline[2].Addr == pipeline[1].PC) 
        result = 1;
      hash_update(hashmap, pipeline[2].PC, result, trace_view_on);
      temp = stall_flow(pipeline);
    }
    else if(PRED_METH == 0 && branch_taken(pipeline))
    {
      if(trace_view_on == 2)
        printf("\nC4: stall...\n");
      temp = stall_flow(pipeline); 
    }
    else //no hazards
    {
      if (PRED_METH == 1 && pipeline[2].type == ti_BRANCH && cannot_predict(hashmap, pipeline[2].PC) && pipeline[2].Addr != pipeline[1].PC)
        SPECIAL_METH = 1;
      if(trace_view_on == 2)
        printf("\nC5: flow...\n");
      temp = normal_flow(pipeline, &tr_entry); //move pipe
    }  
  }
  trace_uninit();
  exit(0);
}


