/**************************************************************/
/* Group Omega  
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <string.h>
#include <math.h>
#include "CPU.h" 
#include "cache.h"

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
  if (argc == 3)
  {
    trace_view_on = atoi(argv[2]);
  } 

  fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

  trace_fd = fopen(trace_file_name, "rb");

  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  // FILE *fp;
  // fp = fopen(trace_file_name, );
  // I_size = atoi(fgets(fp));
  // printf("\n%d: ", I_size);
  // fclose(fp);

  // here you should extract the cache parameters from the configuration file (cache size, associativity, latency)
  unsigned int I_size = 16; 
  unsigned int I_assoc = 4;
  unsigned int I_bsize = 8; 
  unsigned int D_size = 16;
  unsigned int D_assoc = 4;
  unsigned int D_bsize = 8;
  unsigned int mem_latency = 20;




  if(argc >= 4)
  {
    PRED_METH = atoi(argv[2]);
  }

  char fname[25];
  strcpy(fname, argv[4]);
  FILE *f;
  f = fopen(fname, "a");

  trace_init();
  struct cache_t *I_cache, *D_cache;
  I_cache = cache_create(I_size, I_bsize, I_assoc, mem_latency); 
  D_cache = cache_create(D_size, D_bsize, D_assoc, mem_latency);


//Main Loop ___________________________________________________________
  
  int count = 100;
  int revert = -1;
  while(1) 
  {
    // count--;
    // if(count == 0)
    //   exit(0);

    revert--;
    size = trace_get_item(&tr_entry);
   
    if (!size) {  /* no more instructions (trace_items) to simulate */
      printf("+ Simulation terminates at cycle : %u\n", cycle_number);
      printf("I-cache accesses %u and misses %u\n", I_accesses, I_misses);
      printf("D-cache Read accesses %u and misses %u\n", D_read_accesses, D_read_misses);
      printf("D-cache Write accesses %u and misses %u\n", D_write_accesses, D_write_misses);
      //break ;
    }
    // else{              /* parse the next instruction to simulate */
    //   cycle_number++;
    //   t_type = tr_entry->type;
    //   t_sReg_a = tr_entry->sReg_a;
    //   t_sReg_b = tr_entry->sReg_b;
    //   t_dReg = tr_entry->dReg;
    //   t_PC = tr_entry->PC;
    //   t_Addr = tr_entry->Addr;
    // }  

// SIMULATION OF A SINGLE CYCLE cpu IS TRIVIAL - EACH INSTRUCTION IS EXECUTED
// IN ONE CYCLE, EXCEPT IF THERE IS A CACHE MISS.

  latency = cache_access(I_cache, tr_entry->PC, 0);
	cycle_number = cycle_number + latency; /* simulate instruction fetch */
	// update I_access and I_misses
    I_accesses++;
    if(latency)
      I_misses++;


    revert--;
    if(trace_view_on == 2)
      printf("\n--------------------------------------------------------------------------\n");

    //1. analyize pipe to determine fetch, stall or squash
    if(data_hazard(pipeline, trace_view_on))
    {
      if(trace_view_on == 2)
        printf("\nDATA HAZARD - DONT FETCH\n");
    }
    else if(PRED_METH == 0 && branch_taken(pipeline))
    {
        if(trace_view_on == 2)
          printf("\nBRANCH0 - SQUASH\n");
    }
    else if (PRED_METH == 1 && pipeline[2].type == ti_BRANCH && cannot_predict(hashmap, pipeline[2].PC) && pipeline[2].Addr == pipeline[1].PC)
    {
      if(trace_view_on == 2)
        printf("\nBRANCH1 - SQUASH\n");
    }
    else if (PRED_METH == 1 && (pipeline[3].type == ti_BRANCH) && pipeline[2].type == ti_NOP)
    {
      if(trace_view_on == 2)
        printf("\nBRANCH1 - SQUASHME\n");
    }
    else //otherwise try to fetch an instruction
    {
      if(trace_view_on == 2)
        printf("\nINSTR FETCH\n");
      size = trace_get_item(&tr_entry); //fetch
    }
    printCantPredictBranch(PRED_METH, pipeline, hashmap, trace_view_on);

    //2. Print the pipeline

    //3. Check if the simulation is completed
    if (!size && flush == 0) /* no more instructions (trace_items) to simulate */
    {       
      //end simulation
      printf("+ Simulation terminates at cycle : %u \nEmpty Pipe:", cycle_number-1);
      break;
    }
    else if(!size && flush !=0) //no more fresh instructions, but pipeline is not flushed
    {
      if(flush != 0)    //malloc a no-op instruction to keep the pipeline running till flushed
      {
        tr_entry = malloc(sizeof(struct trace_item));
        tr_entry->type = 0;
        flush--;
      }
    }

    if (trace_view_on)
    {
      printPipe(pipeline, trace_view_on);
    }
    printOutput(pipeline, trace_view_on, tr_entry, cycle_number, f);

    //3. parse the next instruction to simulate; print
    cycle_number++;
    t_type = tr_entry->type;
    t_sReg_a = tr_entry->sReg_a;
    t_sReg_b = tr_entry->sReg_b;
    t_dReg = tr_entry->dReg;
    t_PC = tr_entry->PC;
    t_Addr = tr_entry->Addr;

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
        printf("\nC3: stall...\n");
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
        printf("\nC4: stall...\n");
      int result = 0;
      if (pipeline[2].Addr == pipeline[1].PC) 
        result = 1;
      hash_update(hashmap, pipeline[2].PC, result, trace_view_on);
      temp = stall_flow(pipeline);
    }
    else if(PRED_METH == 0 && branch_taken(pipeline))
    {
      if(trace_view_on == 2)
        printf("\nC5: stall...\n");
      temp = stall_flow(pipeline); 
    }
    else //no hazards
    {
      if (PRED_METH == 1 && pipeline[2].type == ti_BRANCH && cannot_predict(hashmap, pipeline[2].PC) && pipeline[2].Addr != pipeline[1].PC)
        SPECIAL_METH = 1;
      if(trace_view_on == 2)
        printf("\nC6: flow...\n");
      temp = normal_flow(pipeline, &tr_entry); //move pipe
    }  
  }

  fclose(f);
  trace_uninit();
  exit(0);
}


