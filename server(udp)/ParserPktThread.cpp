#include "../Common/GlobalVar.h"
#include "../Common/srv_sockets.h"
#include "../Common/srv_bkup_image.h"
#include <sys/times.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include "ThreadRegIID.h"
#include "ParserPktThread.h"
#include "helper.h"
#include "storageimage.h"



pthread_t prs_thread_id;

volatile sig_atomic_t prs_stoped = 0;
void prs_signal_handler(int s_signal);
void* ParserThread (void* thread_ctx);

void prs_signal_handler(int s_signal)
 {
  prs_stoped = s_signal;
 }

struct TimeStruct
 {
  time_t sec;
  time_t nano;
 };

void* ParserThread(void* thread_ctx)
 {
  THREAD_CONTEXT* ctx = (THREAD_CONTEXT*) thread_ctx;
  void *data_recv = ctx->hlp_pointer;
  pthread_mutex_t &rcv_mtx = *reinterpret_cast<pthread_mutex_t*>(ctx->mtx);
  std::queue<RCV_SESSION_DATA*> *list_pointers=reinterpret_cast<std::queue<RCV_SESSION_DATA*>*>(data_recv);

  pthread_mutex_t parse_mtx;
  pthread_mutexattr_t mAttr;
  pthread_mutex_init(&parse_mtx, &mAttr);
  pthread_mutexattr_destroy(&mAttr);

  while (prs_stoped == 0)
   {
    next:if(list_pointers->empty() == true)
     {
      usleep(40000);
      continue;
     }
    pthread_mutex_lock(&rcv_mtx);
    RCV_SESSION_DATA* rv_pkt = list_pointers->front();

    list_pointers->pop();
    if(rv_pkt == nullptr)
     {
      continue;
     }
    pthread_mutex_unlock(&rcv_mtx);
    std::list<CLIENT_LIB*>::iterator findStorageClient = stored_clients.begin();
    std::list<CLIENT_LIB*>::iterator end_list = stored_clients.end();
    while(findStorageClient != end_list)
     {
         CLIENT_LIB* pkt_id = *findStorageClient;
         if(pkt_id != nullptr )
          {
            if(pkt_id->session_num_const == rv_pkt->rcv_packet.pkt_head.SESSION_NUM )
            {
              if(pkt_id->packet_num == rv_pkt->rcv_packet.pkt_head.PACKET_NUM)
               {
                free(rv_pkt);
                goto next;
               }
              StorageData(parse_mtx,pkt_id,rv_pkt);
              break;
             }
          }
        findStorageClient++;
     }
    if(findStorageClient == end_list)
     {
       CLIENT_LIB* pkt_id = (CLIENT_LIB*)calloc(1,sizeof(CLIENT_LIB));
       pkt_id->session_num_const = rv_pkt->rcv_packet.pkt_head.SESSION_NUM;
       sprintf(pkt_id->SESSIONNAME,"%lx",(size_t)pkt_id->session_num_const);
       StorageData(parse_mtx,pkt_id,rv_pkt);
       stored_clients.push_back(pkt_id);
     }
  }
  printf("VssCoreSS:/>  parser Thread finished %s\n", strerror(errno));
  fflush(stdout);
  pthread_exit(nullptr);
 }

