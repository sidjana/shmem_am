
typedef void (*shmemx_am_handler)  (void *buf, size_t nbytes, int req_pe);
int shmemx_am_maxrequest();
void shmemx_am_attach (int function_id, shmemx_am_handler function_handler);
void shmemx_am_detach(int function_id);
void shmemx_am_launch(int dest, int handler_id, void* source_addr, size_t nbytes);
void shmemx_am_quiet();

