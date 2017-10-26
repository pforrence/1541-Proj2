
#ifndef TRACE_ITEM_H
#define TRACE_ITEM_H

// this is tpts
enum trace_item_type {
	ti_NOP = 0,

	ti_RTYPE,
	ti_ITYPE,
	ti_LOAD,
	ti_STORE,
	ti_BRANCH,
	ti_JTYPE,
	ti_SPECIAL,
	ti_JRTYPE
};

struct trace_item {
	unsigned char type;			// see above
	unsigned char sReg_a;			// 1st operand
	unsigned char sReg_b;			// 2nd operand
	unsigned char dReg;			// dest. operand
	unsigned int PC;			// program counter
	unsigned int Addr;			// mem. address
};
#endif

#define TRACE_BUFSIZE 1024*1024

static FILE *trace_fd;
static int trace_buf_ptr;
static int trace_buf_end;
static struct trace_item *trace_buf;
static FILE *out_fd;

//METHODS-------------------------------------------------------

int is_big_endian(void)
{
	union {
		uint32_t i;
		char c[4];
	} bint = { 0x01020304 };

	return bint.c[0] == 1;
}

uint32_t my_ntohl(uint32_t x)
{
	u_char *s = (u_char *)&x;
	return (uint32_t)(s[3] << 24 | s[2] << 16 | s[1] << 8 | s[0]);
}

void trace_init()
{
	trace_buf = malloc(sizeof(struct trace_item) * TRACE_BUFSIZE);

	if (!trace_buf) {
		fprintf(stdout, "** trace_buf not allocated\n");
		exit(-1);
	}

	trace_buf_ptr = 0;
	trace_buf_end = 0;
}

void trace_uninit()
{
	free(trace_buf);
	fclose(trace_fd);
}

int trace_get_item(struct trace_item **item)
{
	int n_items;

	if (trace_buf_ptr == trace_buf_end) {	/* if no more unprocessed items in the trace buffer, get new data  */
		n_items = fread(trace_buf, sizeof(struct trace_item), TRACE_BUFSIZE, trace_fd);
		if (!n_items) return 0;				/* if no more items in the file, we are done */

		trace_buf_ptr = 0;
		trace_buf_end = n_items;			/* n_items were read and placed in trace buffer */
	}

	*item = &trace_buf[trace_buf_ptr];	/* read a new trace item for processing */
	trace_buf_ptr++;

	if (is_big_endian()) {
		(*item)->PC = my_ntohl((*item)->PC);
		(*item)->Addr = my_ntohl((*item)->Addr);
	}

	return 1;
}

int write_trace(struct trace_item item, char *fname)
{
	out_fd = fopen(fname, "a");
	int n_items;
	if (is_big_endian()) {
		(&item)->PC = my_ntohl((&item)->PC);
		(&item)->Addr = my_ntohl((&item)->Addr);
	}

	n_items = fwrite(&item, sizeof(struct trace_item), 1, out_fd);
	fclose(out_fd);
	if (!n_items) return 0;				/* if no more items in the file, we are done */

		
	return 1;
}
void fillPipe(struct trace_item *pipeline)
{
	struct trace_item* temp;
	for (int i = 0; i < 5; i++)
	{
		temp = malloc(sizeof(struct trace_item));
		temp->type = 0;
		pipeline[i] = *temp;
	}
	//temp = NULL;
}

void intToType(int type, char* type_str)
{
	if(type == 0)
	{
		strcpy(type_str, "no-op");
	}
	else if(type == 1)
	{
		strcpy(type_str,"rtype");
	}
	else if(type == 2)
	{
		strcpy(type_str,"itype");
	}
	else if(type == 3)
	{
		strcpy(type_str,"load");
	}
	else if(type == 4)
	{
		strcpy(type_str,"store");
	}
	else if(type == 5)
	{
		strcpy(type_str,"branch");
	}
	else if(type == 6)
	{
		strcpy(type_str,"jtype");
	}
	else if(type == 7)
	{
		strcpy(type_str,"special");
	}
	else if(type == 8)
	{
		strcpy(type_str,"jrtype");
	}
}


void printPipe(struct trace_item *pipeline, int trace_view_on)
{
	char* type_str = malloc(sizeof(char)*10);

	fprintf(stdout, "\n");

	intToType(pipeline[0].type, type_str);
	if(trace_view_on == 2)
		fprintf(stdout, "Fetch    :\t%s\n", type_str);
	
	intToType(pipeline[1].type, type_str);
	if(trace_view_on == 2)
		fprintf(stdout, "Decode   :\t%s\n", type_str);

	intToType(pipeline[2].type, type_str);
	if(trace_view_on == 2)
		fprintf(stdout, "Execute  :\t%s\n", type_str);
	
	intToType(pipeline[3].type, type_str);
	if(trace_view_on == 2)
		fprintf(stdout, "Memory   :\t%s\n", type_str);
	
	intToType(pipeline[4].type, type_str);
	if(trace_view_on == 2)
		fprintf(stdout, "Writeback:\t%s\n", type_str);

	free(type_str);
}


int isEmptyPipe(struct trace_item **pipeline)
{
	return 0;
}
struct trace_item normal_flow(struct trace_item *pipeline, struct trace_item **newInstr)
{
	struct trace_item ret = pipeline[4];
	for(int i = 4; i > 0; i--)
	{
		pipeline[i] = pipeline[i-1];
	}
	pipeline[0] = **newInstr;
	return ret;
}

struct trace_item stall_flow(struct trace_item *pipeline)
{
	struct trace_item ret = pipeline[4];

	for(int i = 4; i > 2; i--)
	{
		pipeline[i] = pipeline[i-1];
	}
	
	struct trace_item * temp = malloc(sizeof(struct trace_item));
	temp->type = 0;
	pipeline[2] = *temp;

	return ret;
}

int data_hazard(struct trace_item *pipeline, int trace_view_on)
{
	int haz = 0;

	if(pipeline[2].type == 3 && (pipeline[1].sReg_a == pipeline[2].dReg || pipeline[1].sReg_b == pipeline[2].dReg))
	{
		haz = 1;
	}
	return haz;
}

int branch_taken(struct trace_item *pipeline)
{
	if((pipeline[2].Addr == pipeline[1].PC && pipeline[2].type == 5) || (pipeline[3].Addr == pipeline[1].PC && pipeline[3].type == 5))
		return 1;
	return 0;
}

int* get_bits(int n, int bitswanted)
{
	int *bits = malloc(sizeof(int) * bitswanted);
	for (int k = 0; k < bitswanted; k++)
	{
		int mask = 1 << k;
		int masked_n = n & mask;
		int thebit = masked_n >> k;
		bits[k] = thebit;
	}
	return bits;
}

int hash_lookup(int *hashmap, int lookup)
{	
	return hashmap[lookup];
}

int hash_address(unsigned int address)
{
	address = address >> 4;
	int bitswanted = 6;
	int *bits = get_bits(address, bitswanted);

	int sum = 0;
	for(int i = 0; i < bitswanted; i++)
	{
		sum += bits[i]*pow(2, i);
	}
	free(bits);
	return sum;
}

int prediction(int* hashmap, int address)
{
	int h_addr, h_lookup;
	h_addr = hash_address(address);
	h_lookup = hash_lookup(hashmap, h_addr);
	return h_lookup;
}

int mispredict_branch(struct trace_item *pipeline, int *hashmap, int address)
{
	if(prediction(hashmap, address) == branch_taken(pipeline))
	{
		return 0; //correct prediction
	}
	else
	{
		return 1; //mispredict
	}
	return -1;
}

int cannot_predict(int *hashmap, int address)
{
	return prediction(hashmap, address) == -1;
}

void hash_update(int *hashmap, int address, int result, int trace_view_on)
{
	int h_addr;
	h_addr = hash_address(address);
	hashmap[h_addr] = result;

    	int i;
    	for(i = 0; i<64; i++)
    	{
    		if(trace_view_on == 2)
    			printf("\nHashTable[%d] = %d", i, hashmap[i]);
    	}
}

void printCantPredictBranch(unsigned char PRED_METH,struct trace_item pipeline[5], int hashmap[64], int trace_view_on)
{
    if (PRED_METH == 1 && pipeline[0].type == 5 && cannot_predict(hashmap, pipeline[0].PC) == 1)
    {

      if(trace_view_on == 2)
        printf("CANNOT PREDICT BRANCH 0 - ASSUME NOT TAKEN\n");
    }
    else if (PRED_METH == 1 && pipeline[1].type == 5 && cannot_predict(hashmap, pipeline[1].PC) == 1)
    {
      if(trace_view_on == 2)
        printf("CANNOT PREDICT BRANCH 1 - ASSUME NOT TAKEN\n");

    }
}

void printOutput(struct trace_item* pipeline, int trace_view_on, struct trace_item *tr_entry, unsigned int cycle_number)
{
  tr_entry = &pipeline[2];
if (trace_view_on) {/* print the executed instruction if trace_view_on=1 */
  tr_entry = &pipeline[2];
  switch(tr_entry->type) {
    case ti_NOP:
      printf("[cycle %d] NOP:\n", cycle_number) ;
      break;
    case ti_RTYPE:
      printf("[cycle %d] RTYPE:", cycle_number) ;
      printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->dReg);
      break;
    case ti_ITYPE:
      printf("[cycle %d] ITYPE:",cycle_number) ;
      printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
      break;
    case ti_LOAD:
      printf("[cycle %d] LOAD:",cycle_number) ;      
      printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
      break;
    case ti_STORE:
      printf("[cycle %d] STORE:",cycle_number) ;      
      printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
      break;
    case ti_BRANCH:
      printf("[cycle %d] BRANCH:",cycle_number) ;
      printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
      break;
    case ti_JTYPE:
      printf("[cycle %d] JTYPE:",cycle_number) ;
      printf(" (PC: %x)(addr: %x)\n", tr_entry->PC,tr_entry->Addr);
      break;
    case ti_SPECIAL:
      printf("[cycle %d] SPECIAL:\n",cycle_number) ;   
      break;
    case ti_JRTYPE:
      printf("[cycle %d] JRTYPE:",cycle_number) ;
      printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_entry->PC, tr_entry->dReg, tr_entry->Addr);
     break;
  }
}

}
void step1(int trace_view_on, size_t *size, unsigned char PRED_METH, struct trace_item* pipeline, int* hashmap, 
	struct cache_t *I_cache, struct trace_item **tr_entry, unsigned int* cycle_number)
{    
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
      *size = trace_get_item(tr_entry); //fetch
	  struct trace_item *temp = *tr_entry;
      *cycle_number = *cycle_number + cache_access(I_cache, temp->PC, 0); /* simulate instruction fetch */
      // update I_access and I_misses
      //updateAccessMiss();
    }
    printCantPredictBranch(PRED_METH, pipeline, hashmap, trace_view_on);
}
void step2(int trace_view_on, size_t size, int *flush, unsigned int cycle_number, unsigned int I_accesses, unsigned int I_misses, 
	unsigned int D_read_accesses, unsigned int D_read_misses, unsigned int D_write_accesses, unsigned int D_write_misses,
	struct trace_item *tr_entry, struct trace_item* pipeline)
{
    if (!size && *flush == 0) /* no more instructions (trace_items) to simulate */
    {       
      //end simulation
      printf("+ Simulation terminates at cycle : %u\n", cycle_number);
      printf("I-cache accesses %u and misses %u\n", I_accesses, I_misses);
      printf("D-cache Read accesses %u and misses %u\n", D_read_accesses, D_read_misses);
      printf("D-cache Write accesses %u and misses %u\n", D_write_accesses, D_write_misses);
      (*flush)--;
    }
    else if(!size && *flush !=0) //no more fresh instructions, but pipeline is not flushed
    {

      if(*flush != 0)    //malloc a no-op instruction to keep the pipeline running till flushed
      {
        tr_entry = malloc(sizeof(struct trace_item));
        tr_entry->type = 0;
        (*flush)--;
      }
    }
    if (trace_view_on)
    {
      printPipe(pipeline, trace_view_on);
    }
    printOutput(pipeline, trace_view_on, tr_entry, cycle_number);
}
