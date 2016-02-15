
#include <pshmem.h>
#include <gasnet.h>

#define SHMEMX_PAGESIZE GASNET_PAGESIZE

/* Macro to check return codes and terminate with useful message. */
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

#define GASNET_TO_SHMEM(ret_typ,g_fn,s_fn,param_list,arg_list) \
	ret_typ s_fn param_list {      \
		return g_fn arg_list ; \
	}
GASNET_TO_SHMEM(size_t,gasnet_AMMaxMedium,shmemx_AMMaxRequest,(),())


typedef void (*shmemx_AM_Handler)  (void *buf, size_t nbytes, int is_request)

struct shmemx_AM_Handler2Id_Map {
     int id; /* user defined handler */
     shmemx_AM_Handler fn_ptr;
     UT_hash_handle hh;
};
struct shmemx_AM_Handler2Id_Map *AM_MapHashPtr = NULL;


/* collective operation */
int 
shmemx_attach (int function_id, shmemx_AM_Handler function_handler)
{
   static int gasnet_attach_called = 0;
   if(!gasnet_attach_called) {
      gasnet_handlerentry_t  gasnet_htable[] = {{128, global_request_handler},
      						{129, global_reply_handler}};
      GASNET_SAFE(gasnet_attach(table, 2, sizeof(gasnet_htable)/sizeof(gasnet_handlerentry_t), gasnet_getMaxLocalSegmentSize(), 1048576*2));
      gasnet_attach_called = 1;
   }
   struct shmemx_AM_Handler2Id_Map* temp_handler_entry;
   temp_handler_entry = (struct shmemx_AM_Handler2Id_Map*) malloc(sizeof(struct shmemx_AM_Handler2Id_Map));
   temp_handler_entry->id = function_id;
   temp_handler_entry->fn_ptr = function_handler;
   HASH_ADD_INT(AM_MapHashPtr, id, temp_handler_entry);
}


shmemx_detach(int function_id)
{
   struct shmemx_AM_Handler2Id_Map* temp_handler_entry;
   temp_handler_entry = (struct shmemx_AM_Handler2Id_Map*) malloc(sizeof(struct shmemx_AM_Handler2Id_Map));
   HASH_DEL( AM_MapHashPtr, temp_handler_entry);  /* delete - pointer to handler */
   free(AM_MapHashPtr);                           /* optional */
}

static request_cnt = 0;

static void 
global_request_handler (gasnet_token_t token, void *buf, size_t nbytes, gasnet_handlerarg_t is_request, gasnet_handlerarg_t handler_id)
{
      struct shmemx_AM_Handler2Id_Map* temp_handler_entry;
      HASH_FIND_INT( AM_MapHashPtr, &handler_id, temp_handler_entry );  /* s: output pointer */
      temp_handler_entry->fn_ptr(buf, nbytes, is_request);
      GASNET_SAFE(gasnet_AMReplyShort0 (token, 129));
}

static void 
global_reply_handler (gasnet_token_t token)
{
	request_cnt--;
}

void 
shmemx_AMLaunch(int dest, void* source_addr, size_t nbytes, int is_request, int handler_id)
{
	request_cnt++;
	GASNET_SAFE(gasnet_AMRequestMedium2 (dest, 128, source_addr, nbytes, is_request, handler_id));
}


void
shmemx_AMQuiet()
{
	GASNET_BLOCKUNTIL(request_cnt==0);	
}


void 
start_pes(int temp)
{
	GASNET_SAFE(gasnet_init(NULL, NULL));
	pstart_pes(0);
}


void 
shmem_init()
{
	GASNET_SAFE(gasnet_init(NULL, NULL));
	pstart_pes(0);
}



#define PREPEND_GASNET_AMPOLL(ret_typ,fn_name,c_fn_name,param_list,arg_list) \
	ret_typ s_fn param_list {      \
		gasnet_AMPoll();       \
		p##c_fn_name arg_list;
	}

PREPEND_GASNET_AMPOLL(void, shmem_barrier_all, shmem_barrier_all, (), ());
PREPEND_GASNET_AMPOLL(void, shmem_quiet, shmem_quiet, (), ());

void shmem_finalize()
{
	GASNET_SAFE(gasnet_AMPoll());;
	gasnet_exit(0);
	pshmem_finalize();
}
