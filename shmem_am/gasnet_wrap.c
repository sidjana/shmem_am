#include <pshmem.h>
#include <shmem.h>
#include <gasnet.h>
#include "../uthash/uthash.h"


/* Macro to check return codes and terminate 
 * with useful message. 
 */
#define GASNET_SAFE(fncall) do {                                     \
    int _retval;                                                     \
    if ((_retval = fncall) != GASNET_OK) {                           \
      fprintf(stderr, "ERROR calling: %s\n"                          \
                      " at: %s:%i\n"                                 \
                      " error: %s (%s)\n",                           \
              #fncall, __FILE__, __LINE__,                           \
              gasnet_ErrorName(_retval), gasnet_ErrorDesc(_retval)); \
      fflush(stderr);                                                \
      gasnet_exit(_retval);                                          \
    }                                                                \
  } while(0)


#define PREPEND_GASNET_AMPOLL(ret_typ,fn_name,c_fn_name,param_list,arg_list) \
	ret_typ fn_name param_list {      \
		gasnet_AMPoll();       \
		p##c_fn_name arg_list; \
	}


#define SHMEMX_PAGESIZE GASNET_PAGESIZE

#define GASNET_TO_SHMEM(ret_typ,g_fn,s_fn,param_list,arg_list) \
	ret_typ s_fn param_list {      \
		return g_fn arg_list ; \
	}
GASNET_TO_SHMEM(size_t,gasnet_AMMaxMedium,shmemx_am_maxrequest,(),())


#define GASNET_MIN_OFFSET 1048576*2 
#define SHMEM_GLOBAL_AM_REQ_HANDLER 128
#define SHMEM_GLOBAL_AM_REPLY_HANDLER 129


typedef void (*shmemx_am_handler)  (void *buf, size_t nbytes, int req_pe);
struct shmemx_am_handler2id_map {
     int id; /* user defined handler */
     shmemx_am_handler fn_ptr;
     UT_hash_handle hh;
};

static int volatile request_cnt = 0;
static int global_pe_id = -1;
struct shmemx_am_handler2id_map *am_maphashptr = NULL;

static void 
global_request_handler (gasnet_token_t token, void *buf, size_t nbytes, gasnet_handlerarg_t handler_id, gasnet_handlerarg_t req_pe)
{
      struct shmemx_am_handler2id_map* temp_handler_entry;
      HASH_FIND_INT( am_maphashptr, &handler_id, temp_handler_entry );  /* s: output pointer */
      temp_handler_entry->fn_ptr(buf, nbytes, req_pe);
      GASNET_SAFE(gasnet_AMReplyShort0 (token, SHMEM_GLOBAL_AM_REPLY_HANDLER));
}

static void 
global_reply_handler (gasnet_token_t token)
{
	request_cnt--;
}



/* collective operation */
int 
shmemx_am_attach (int function_id, shmemx_am_handler function_handler)
{
   shmem_barrier_all();
   static int gasnet_attach_called = 0;
   if(!gasnet_attach_called) {
      gasnet_handlerentry_t  gasnet_htable[] = {{SHMEM_GLOBAL_AM_REQ_HANDLER, global_request_handler},
      						{SHMEM_GLOBAL_AM_REPLY_HANDLER, global_reply_handler}};
      GASNET_SAFE(gasnet_attach(gasnet_htable, 2, gasnet_getMaxLocalSegmentSize(), GASNET_MIN_OFFSET));
      gasnet_attach_called = 1;
   }
   struct shmemx_am_handler2id_map* temp_handler_entry;
   temp_handler_entry = (struct shmemx_am_handler2id_map*) malloc(sizeof(struct shmemx_am_handler2id_map));
   temp_handler_entry->id = function_id;
   temp_handler_entry->fn_ptr = function_handler;
   HASH_ADD_INT(am_maphashptr, id, temp_handler_entry);
   shmem_barrier_all();
}


int
shmemx_am_detach(int function_id)
{
   struct shmemx_am_handler2id_map* temp_handler_entry;
   temp_handler_entry = (struct shmemx_am_handler2id_map*) malloc(sizeof(struct shmemx_am_handler2id_map));
   HASH_DEL( am_maphashptr, temp_handler_entry);  /* delete - pointer to handler */
   free(am_maphashptr);                           /* optional */
}


void 
shmemx_am_launch(int dest, int handler_id, void* source_addr, size_t nbytes)
{
	request_cnt++;
	GASNET_SAFE(gasnet_AMRequestMedium2 (dest, SHMEM_GLOBAL_AM_REQ_HANDLER, source_addr, nbytes, handler_id, global_pe_id));
}


void
shmemx_am_quiet()
{
	GASNET_BLOCKUNTIL(request_cnt==0);	
}



void 
start_pes(int temp)
{
	//GASNET_SAFE(gasnet_init(NULL, NULL));
	pstart_pes(0);
	global_pe_id = p_my_pe();
}



void shmem_finalize()
{
	GASNET_SAFE(gasnet_AMPoll());;
	gasnet_exit(0);
	pshmem_finalize();
}


PREPEND_GASNET_AMPOLL(void, shmem_barrier_all, shmem_barrier_all, (), ());
PREPEND_GASNET_AMPOLL(void, shmem_quiet, shmem_quiet, (), ());

