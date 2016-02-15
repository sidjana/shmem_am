
#include <stdio.h>
#include <shmem.h>
#include "gasnet_wrap.h"


#define hidx_ping_shorthandler   201
#define hidx_pong_shorthandler   202

volatile int flag = 0;
int rank_shmem, size_shmem;

void ping_shorthandler(shmemx_token_t token, void* temp_addr, size_t nbytes, int is_request)
{
        printf("PE%d: Inside handler. Received %dbytes\t isrequest=%d\n", rank_shmem, nbytes, is_request);
	flag = *((int*) temp_addr);
	printf("PE%d: Leaving handler\n", rank_shmem);
}


int main(int argc, char **argv)
{
  size_t segsz = SHMEMX_PAGESIZE;
  size_t heapsz = SHMEMX_PAGESIZE;
  int peer;
  int source_addr[1]; 

  shmemx_handlerentry_t htable[] = { 
      { hidx_ping_shorthandler,  ping_shorthandler  } 
  };  
  
  //GASNET_SAFE(gasnet_init(&argc, &argv));
  shmem_init();
  size_shmem = shmem_n_pes();
  rank_shmem = shmem_my_pe();

  printf("shmem: rank=%d, size=%d\n",rank_shmem, size_shmem);

  segsz = shmemx_getMaxLocalSegmentSize();
   
  shmemx_attach(htable, sizeof(htable)/sizeof(shmemx_handlerentry_t),
			                              segsz, 1048576*2);

  if (rank_shmem) peer = 0;
  else peer = 1;

  flag = 0;
  shmem_barrier_all();
  /* Send a 1-sided active message to peer */
  source_addr[0]=-8;
  shmemx_AMSend(peer, hidx_ping_shorthandler, source_addr, sizeof(int), SHMEM_AM_ONESIDED);
  while(flag!=-8) {
	shmemx_AMPoll();		
  }

  printf("%d: I received %d from %d\n", rank_shmem, flag, peer);
  shmem_barrier_all();

  if (!rank_shmem || (rank_shmem == size_shmem-1))
    printf("Hello from node %d of %d\n", (int)rank_shmem, (int)size_shmem);

  shmem_finalize();
}


