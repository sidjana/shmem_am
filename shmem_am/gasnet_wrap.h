
#include <pshmem.h>
#include <gasnet.h>


typedef gasnet_token_t  shmemx_token_t;
typedef gasnet_handler_t shmemx_handler_t;
typedef gasnet_node_t shmemx_node_t;
typedef gasnet_handlerentry_t shmemx_handlerentry_t;

enum shmemx_AMType{SHMEM_AM_REQUEST, SHMEM_AM_ONESIDED};

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

#define BARRIER() do {                                              \
	  gasnet_barrier_notify(0,GASNET_BARRIERFLAG_ANONYMOUS);            \
	  GASNET_SAFE(gasnet_barrier_wait(0,GASNET_BARRIERFLAG_ANONYMOUS)); \
} while (0)

#define GASNET_TO_SHMEM(ret_typ,g_fn,s_fn,param_list,arg_list) \
	ret_typ s_fn param_list {      \
		return g_fn arg_list ; \
	}


GASNET_TO_SHMEM(size_t,gasnet_AMMaxMedium,shmemx_AMMaxRequest,(),())
GASNET_TO_SHMEM(int, gasnet_AMPoll, shmemx_AMPoll, (), ())
GASNET_TO_SHMEM(int, gasnet_attach, shmemx_attach, (shmemx_handlerentry_t* table, int numentries, uintptr_t segsize, uintptr_t minheapoffset), (table, numentries, segsize, minheapoffset))


#define shmemx_getMaxLocalSegmentSize() gasnet_getMaxLocalSegmentSize()


/* handler prototype:
   void handler (shmemx_token_t token, void *buf, size_t nbytes, int is_request);
*/

int shmemx_AMSend(shmemx_node_t dest, shmemx_handler_t handler, void* source_addr, size_t nbytes, enum shmemx_AMType amtype)
{
	if(handler<128 || handler>255) {
		fprintf(stderr, "Error: Handler index needs to be 128..255\n"); 
	        exit(1);
	}

	if(amtype == SHMEM_AM_REQUEST) {
		return gasnet_AMRequestMedium1 (dest, handler, source_addr, nbytes, 1);
	}
	else if(amtype == SHMEM_AM_ONESIDED) {
		return gasnet_AMRequestMedium1 (dest, handler, source_addr, nbytes, 0);
	}
	else {
		fprintf(stderr, "Error: Wrong value of SHMEM_AMType\n"); 
	        exit(1);
	}
}


void start_pes(int temp)
{
	//GASNET_SAFE(gasnet_init(NULL, NULL));
	pstart_pes(0);
}

void shmem_barrier_all()
{
	shmemx_AMPoll();
	pshmem_barrier_all();
}


void shmem_barrier(int PE_start, int logPE_stride, int PE_size, long *pSync)
{
	shmemx_AMPoll();
	pshmem_barrier(PE_start, logPE_stride, PE_size, pSync);
}


void shmem_quiet()
{
	GASNET_SAFE(gasnet_AMPoll());
	pshmem_quiet();
}


void shmem_finalize()
{
	gasnet_AMPoll();
	gasnet_exit(0);
	pshmem_finalize();
}
