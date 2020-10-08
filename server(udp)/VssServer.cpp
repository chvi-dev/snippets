#include "../Common/GlobalVar.h"
#include "../Common/srv_sockets.h"
#include "helper.h"
#include "ThreadRegIID.h"
#include "ParserPktThread.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

static int array_signals[]={SIGABRT,SIGBUS,SIGFPE,SIGHUP,SIGSTKFLT,SIGKILL,SIGINT,SIGQUIT,SIGSEGV,SIGTERM,SIGTSTP,SIGUSR1,SIGUSR2,SIGSYS};

int main (int argc, char **argv)
{
  int count_signals = sizeof(array_signals)/sizeof(array_signals[0]);

  for(int i =0 ; i < count_signals ; i++ )
  {
	signal(array_signals[i], signal_handler);
  }
  sockaddr_in host_addr;
  printf("Server/> : main loop enter\n");
  setbuf(stdout,nullptr );
  fflush( stdout);
  ParseArg(argc, argv);
  char* ip_point= (char*)((void*)hostIP.c_str());

  setup_sockaddr_in(&host_addr, &host_port,ip_point);

  pthread_mutex_t main_mtx;
  pthread_mutexattr_t mAttr;
  pthread_mutex_init(&main_mtx, &mAttr);
  pthread_mutexattr_destroy(&mAttr);


  void* c = calloc(1,sizeof(THREAD_CONTEXT));
  THREAD_CONTEXT* thread_ctx = (THREAD_CONTEXT*)c;
  thread_ctx->rcv_socket = 0;
  thread_ctx->snd_socket = 0;
  thread_ctx->host_addr = &host_addr;
  thread_ctx->target_addr = nullptr;
  thread_ctx->thread_id = &thread_id;
  thread_ctx->mtx = &main_mtx;

  pthread_attr_t threadAttr;
  pthread_attr_init(&threadAttr);
  pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);

  int result = pthread_create( &thread_id, nullptr, ThreadWorker, (void*)thread_ctx);
  if(result !=0 )
   {
    free(thread_ctx);
    return -1;
   }

  pthread_join(thread_id, nullptr);

  fprintf(stderr, "VssCoreSS:/> Server finished , error status - %s\n",strerror(errno));
  fflush( stdout);
  sched_yield();

  return 0;
}
