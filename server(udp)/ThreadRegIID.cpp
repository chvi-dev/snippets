#include "../Common/GlobalVar.h"
#include "../Common/srv_sockets.h"
#include "../Common/srv_bkup_image.h"
#include <sys/times.h>
#include <time.h>
#include <errno.h>
#include "clientstoredimg.h"
#include "ThreadRegIID.h"
#include "helper.h"
pthread_t thread_id;
volatile sig_atomic_t server_stoped = 0;
void* ThreadWorker(void* ctx);
void signal_handler(int s_signal);

std::list<CLIENT_LIB* > stored_clients;
void signal_handler(int s_signal)
 {
  server_stoped = s_signal;
 }
struct TimeStruct
 {
  time_t sec;
  time_t nano;
 };
void* ThreadWorker(void* ctx)
 {
  THREAD_CONTEXT* c = (THREAD_CONTEXT*)ctx;

  char recv_buff[SZ_UDP_PKT ] =	{ 0 };
  size_t  buff_soct_size = SZ_UDP_PKT ;
  int iid_socket = 0;
  int err = create_socket(&iid_socket);
  if(err == SOCKET_ERROR)
   {
    free(ctx);
    printf ("VssCoreSS:/> select error  %s\n",strerror(errno));
    pthread_exit(nullptr);
   }
  err = init_socket(iid_socket,c->host_addr);
  if(err == SOCKET_ERROR)
   {
    free(ctx);
    printf ("VssCoreSS:/> select error  %s\n",strerror(errno));
    pthread_exit(nullptr);
   }
  //uint16_t clien_rv_port=0;
  //pthread_mutex_t *rcv_mtx = c->mtx;
  sockaddr_in client_addr;
  ssize_t recv_len = 0;
  fd_set s_get;
  timeval timeout;
  while (server_stoped == 0)
   {
    FD_ZERO(&s_get);
    FD_SET(iid_socket, &s_get);
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    int ret = select(iid_socket+1, &s_get, nullptr, nullptr, &timeout);
    if(ret <0)
     {
      free(ctx);
      printf ("VssCoreSS:/> select error  %s\n",strerror(errno));
      pthread_exit(nullptr);
     }
    if (FD_ISSET(iid_socket, &s_get))
     {
      memset(recv_buff,0,SZ_UDP_PKT);
      int cn_iterr=0;
      int ret = recv_data_raw(iid_socket, recv_buff, &recv_len, buff_soct_size, client_addr, slen);
      while(ret == SOCKET_ERROR)
       {
        close(iid_socket);
        ret = init_socket(iid_socket,c->host_addr);
        if(ret == SOCKET_OK)
         {
          memset(recv_buff,0,SZ_UDP_PKT);
          ret = recv_data_raw(iid_socket, recv_buff, &recv_len, buff_soct_size, client_addr, slen);
          cn_iterr++;
          if(cn_iterr>10)
           {
            free(ctx);
            fprintf(stderr, "VssCoreSS:/> recv_data_raw - SOCKET ERROR \n");
            pthread_exit(nullptr);
           }
         }
       }
      if (ret == 0)
       {
        FD_CLR(iid_socket, &s_get);
        continue;
       }
      Packet* pkt = (Packet*)recv_buff;
      if(pkt->pkt_head.key != 0xE2E4 || pkt->pkt_head.type_data != CMD )
       {
        FD_CLR(iid_socket, &s_get);
        continue;
       }
      uint8_t cmd = pkt->data[0];
      if(cmd != HI)
       {
        FD_CLR(iid_socket, &s_get);
        continue;
       }
      std::list<CLIENT_LIB*>::iterator findStorageClient = stored_clients.begin();
      std::list<CLIENT_LIB*>::iterator end_list = stored_clients.end();
      int err = 0;
      CLIENT_LIB* clientID = nullptr;
      bool busy = false;
      while(findStorageClient != end_list)
       {
         CLIENT_LIB* id = *findStorageClient;
         if(id != nullptr )
          {
            if(id->session_num_const == pkt->pkt_head.SESSION_NUM )
             {
               clientID = id;
               break;
             }
           }
          findStorageClient++;
       }
      if(findStorageClient == end_list)
       {
         CLIENT_LIB* new_client_id = (CLIENT_LIB*)calloc(1,sizeof(CLIENT_LIB));
         new_client_id->session_num_const = pkt->pkt_head.SESSION_NUM;
         new_client_id->sender = client_addr;
         //set worked port
         CLIENT_LIB* last_client_id = stored_clients.back();
         uint16_t port_rcv = 1024;
         new_client_id->work_port = 0;
         if(last_client_id != nullptr)
          {
            port_rcv = last_client_id->work_port+1;
          }

         for(port_rcv = 1025; port_rcv < 65535 ; port_rcv++)
          {
           sockaddr_in addr;
           int t_sock;
           setup_sockaddr_in(&addr, &port_rcv, (char*)(void*)hostIP.c_str());
           int err = create_socket(&t_sock);
           if(err == SOCKET_ERROR)
            {
             printf("VssCoreSS:/>create_socket->SOCKET_ERROR in init work port .Thread finished %s\n", strerror(errno));
             fflush(stdout);
             pthread_exit(nullptr);
            }
           err = bind(t_sock, (sockaddr*)&addr, sizeof(sockaddr));
           close(t_sock);
           if ( err == SOCKET_ERROR)
           {
             continue;
           }
           new_client_id->work_port = port_rcv;
           clientID = new_client_id;
           break;
         }
         if(clientID->work_port == 0)
          {
           busy = true;
           free(new_client_id);
          }
          else
          {

            char* host_ip = (char*)(void*)hostIP.c_str();
            ClientStoredImg* client = new (std::nothrow) ClientStoredImg(host_ip,
                                                                         clientID->work_port,
                                                                         clientID->session_num_const);
            if(client == nullptr)
             {
              continue;
             }
            err = client->InitClient();
            if(err == SOCKET_ERROR)
             {
              delete client;
             }
              else
             {
              clientID->client_obj = client;
              stored_clients.push_back(clientID);
             }
          }
       }
      Packet s_pkt;
      memset(&s_pkt,0,sizeof(s_pkt));
      s_pkt.pkt_head.SESSION_NUM = clientID->session_num_const;
      s_pkt.pkt_head.PACKET_NUM = pkt->pkt_head.PACKET_NUM+1;
      int8_t command = HI;
      uint8_t* p_portNm = (uint8_t*)&clientID->work_port;
      size_t sz_data = sizeof(uint16_t);
      if( busy == true )
       {
        command = BUSY;
        p_portNm = nullptr;
        sz_data = 0;
       }
      uint16_t data_len = BuildDataPacket(CMD,command,&s_pkt, p_portNm, sz_data);
      err = send_data_raw(iid_socket, (char*) &s_pkt, data_len, &client_addr ,slen);
      err = CheckdataSend(err,iid_socket,*c->host_addr,client_addr,data_len,s_pkt);
      if(err == SOCKET_ERROR)
       {
        printf("VssCoreSS:/>CheckdataSend -> SOCKET_ERROR  Thread finished %s\n", strerror(errno));
        fflush(stdout);
        pthread_exit(nullptr);
       }
    }
    else
     {
//      //timeout socket ready;
       printf ("VssCoreSS:/> thread for get IID clients Vss - timeout %s\n",strerror(errno));
       FD_CLR(iid_socket, &s_get);
     }

   }
  close(iid_socket);
  printf("VssCoreSS:/>  Thread finished %s\n", strerror(errno));
  fflush(stdout);
  pthread_exit(nullptr);
 }
