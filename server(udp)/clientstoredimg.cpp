
#include "../Common/srv_sockets.h"
#include "../Common/srv_bkup_image.h"
#include "clientstoredimg.h"

bool ClientStoredImg::data_recv_stoped;
bool ClientStoredImg::data_storage_stoped;
ClientStoredImg* ClientStoredImg::obj;
std::queue<RCV_SESSION_DATA*> ClientStoredImg::vss_packets;
char ClientStoredImg::SESSIONNAME[50];
ClientStoredImg::ClientStoredImg(char* ip_point,uint16_t port_resv,uint64_t iid_session_num)
{
  host_ip.clear();
  host_ip.append(ip_point);
  work_port = port_resv;
  wr_bytes = 0;
  were_drop = 0;
  dropp = 0 ;
  mb_bytes = 0;
  bytes_ps = 0;
  bit_ps = 0;
  cn_lost_blocks = 0;
  session_num_const = iid_session_num;
  packet_num = 0;
  offsef_by_indx = 0;
  count_pkts_receive = 0;
  wr_blocks = 0;
  file_len_const = 0;
  num_pages = 0;
  last_real_bit = 0;
  write_data_len = 0 ;
  im_f_des_const = 0;
  p_mark = nullptr;
  wait_close = false;
  data_recv_stoped = false;
  data_storage_stoped = false;
  start = false;
  mark_field_array = nullptr;
  poit2freePkt = nullptr;
  obj=this;
}
int ClientStoredImg::InitClient()
 {
  char* ip =  (char*)(void*)host_ip.c_str();
  setup_sockaddr_in(&host_addr, &work_port,ip );

  int err = create_socket(& send_socket);
  if(err == SOCKET_ERROR)
   {
    return err;
   }
  err = init_socket(send_socket,&host_addr);
  if(err == SOCKET_ERROR)
   {
    return err;
   }
  err = create_socket(&recv_socket);
  if(err == SOCKET_ERROR)
   {
    return err;
   }
  err = init_socket(recv_socket,&host_addr);
  if(err == SOCKET_ERROR)
   {
    return err;
   }
  pthread_mutexattr_t mAttr;
  pthread_mutex_init(&client_mtx, &mAttr);
  pthread_mutexattr_destroy(&mAttr);

  uint8_t* ctx = (uint8_t*)calloc(1,sizeof(THREAD_CONTEXT));
  THREAD_CONTEXT* rv_thread_ctx = (THREAD_CONTEXT*)(void*)ctx;
  rv_thread_ctx->snd_socket = send_socket;
  rv_thread_ctx->rcv_socket = recv_socket;
  rv_thread_ctx->host_addr = &host_addr;
  rv_thread_ctx->target_addr = nullptr;
  rv_thread_ctx->thread_id =&recv_thread_id;
  rv_thread_ctx->hlp_pointer = &vss_packets;
  rv_thread_ctx->mtx = &client_mtx;
  err = pthread_create( &recv_thread_id, nullptr, ThreadReceiveDataFromClient, (void*)rv_thread_ctx);
  if(err !=0 )
   {
    free(ctx);
    printf ("VssCoreSS:/> parser thread create  %s\n",strerror(errno));
    pthread_cancel(recv_thread_id);
    return err;
   }
  THREAD_CONTEXT* stg_thread_ctx = (THREAD_CONTEXT*)(void*)ctx;
  stg_thread_ctx->snd_socket = send_socket;
  stg_thread_ctx->rcv_socket = recv_socket;
  stg_thread_ctx->host_addr = &host_addr;
  stg_thread_ctx->target_addr = nullptr;
  stg_thread_ctx->thread_id =&recv_thread_id;
  stg_thread_ctx->hlp_pointer = &vss_packets;
  stg_thread_ctx->mtx = &client_mtx;
  err = pthread_create( &recv_thread_id, nullptr, ThreadStorageData2Image, (void*)stg_thread_ctx);
  if(err !=0 )
   {
    free(ctx);
    printf ("VssCoreSS:/> parser thread create  %s\n",strerror(errno));
    pthread_cancel(recv_thread_id);
    return err;
   }
  return 0;
 }

ClientStoredImg::~ClientStoredImg()
  {

  }

uint8_t* ClientStoredImg::CheckImageMap(size_t num_pages, size_t &cn_lost_blocks, uint8_t* imageMap, int8_t numRlbits)
  {
   fprintf(stdout, "VssCoreSS:/> CheckImageMap enter \n");
   if(num_pages == 0)
    {
     return nullptr;
    }
   uint8_t *pBlocks=nullptr;
   cn_lost_blocks = 0;
   size_t last_pg = num_pages-1;
   for(size_t pg=0; pg < num_pages ; pg++)
    {
      uint8_t field = imageMap[pg];
      if( field != 0xFF)
       {
         for(uint8_t b = 0; b < 8 ; b++)
          {
           uint8_t block_receve = (field>>b)&0x1;
           if( block_receve == 0 )
            {
              if(pg==last_pg && b > numRlbits)
               {
                break;
               }

             size_t num_bl_lost = (pg * 8)+ b;

             uint8_t* pBlocks_1=(uint8_t*)realloc(pBlocks,(cn_lost_blocks+1)*lenAtomSz_t);
             if(pBlocks_1 == nullptr)
              {
               if(pBlocks!=nullptr)
                {
                 free(pBlocks);
                 pBlocks=nullptr;
                }
               return nullptr;
              }
             pBlocks = pBlocks_1;
             uint8_t *tmp= pBlocks + (lenAtomSz_t*(cn_lost_blocks));
             memcpy(tmp,&num_bl_lost,lenAtomSz_t);
             cn_lost_blocks++;

            }
          }
       }
    }
   if(cn_lost_blocks > 0)
    {
     return pBlocks;
    }
    return (uint8_t*)1;
  }

 int  ClientStoredImg::Request4RestoredLostBl( uint8_t *blocks, int &sendSock,sockaddr_in &hostAddr,sockaddr_in &clientAddr,size_t cn_lostBlocks)
  {
    fprintf(stdout, "VssCoreSS:/> Request4RestoredLostBl enter request bloks for restored %zd\n",cn_lostBlocks);
    Packet pkt;
    ssize_t len=ssize_t(cn_lostBlocks * lenAtomSz_t);
    div_t len_a=div(SZ_FILE_DATA,lenAtomSz_t);
    uint16_t sz_file_data=uint16_t(len_a.quot * lenAtomSz_t);
    lldiv_t it=lldiv(len,sz_file_data);
    uint8_t *tmp = blocks;
    int8_t cmd = CMD;
    for(int i = 0; i<it.quot; i++)
     {
       memset(&pkt,0,sizeof(pkt));
       if(it.rem == 0 && i==it.quot - 1)
        {
         cmd = CMD_E;
         fprintf(stdout,"VssCoreSS:/>Send CMD_E RESTORE_BLOCKS in QUOT to %s:%d \n",
                 inet_ntoa(clientAddr.sin_addr),
                 ntohs(clientAddr.sin_port));
        }
       uint16_t data_len = BuildDataPacket(cmd,REPEATE_BLOCKS,&pkt,tmp,(size_t)sz_file_data);
       int err = send_data_raw(sendSock, (char*) &pkt, data_len, &clientAddr ,slen);
       err = CheckdataSend(err,sendSock,hostAddr,clientAddr,data_len,pkt);
       if(err == SOCKET_ERROR)
        {
         return err;
        }
       tmp+=sz_file_data;
      }
      if(it.rem>0)
       {
        memset(&pkt,0,sizeof(pkt));
        uint16_t data_len = BuildDataPacket(CMD_E,REPEATE_BLOCKS,&pkt,tmp,size_t(it.rem));
        int err = send_data_raw(sendSock, (char*) &pkt, data_len, &clientAddr ,slen);
        err = CheckdataSend(err,sendSock,hostAddr,clientAddr,data_len,pkt);
        if(err == SOCKET_ERROR)
         {
          return err;
         }
         fprintf(stdout,"VssCoreSS:/>Send CMD_E RESTORE_BLOCKS in REM to %s:%d \n",
                 inet_ntoa(clientAddr.sin_addr),
                  ntohs(clientAddr.sin_port));
       }

      return NOERR;
   }

int8_t ClientStoredImg::StorageData(RCV_SESSION_DATA* rv_pkt)
 {
  //clien_rv_port = rv_pkt->pkt_head.port_receive;
  switch (rv_pkt->rcv_packet.pkt_head.type_data)
   {
   case CMD:
    {
     uint8_t* pTmp = rv_pkt->rcv_packet.data;
     uint8_t cmd = rv_pkt->rcv_packet.data[0];
     pTmp++;
     switch (cmd)
      {
      case OPEN_BK_FILE:
       {
        memcpy(&file_len_const, pTmp, sizeof(uint64_t));
        lldiv_t pkts_must_be_receive = lldiv((ssize_t)file_len_const, SZ_FILE_DATA);
        count_pkts_receive = (size_t)pkts_must_be_receive.quot;
        if (pkts_must_be_receive.rem > 0)
         {
          count_pkts_receive++;
         }

        lldiv_t map_fields = lldiv((ssize_t)count_pkts_receive,8);
        num_pages = (size_t)map_fields.quot;
        if (map_fields.rem > 0)
         {
          num_pages++;
         }
         mark_field_array = (uint8_t*) malloc(num_pages);
        if (mark_field_array == nullptr)
         {
          free(rv_pkt);
          fprintf(stderr,  "VssCoreSS:/> Markers memory allocate - ERROR_INSUFFICIENT_BUFFER \n");
          return -1;
         }
        memset(mark_field_array,0,num_pages);
        last_real_bit=(int8_t)map_fields.rem;
        uint8_t *last_fld = &mark_field_array[num_pages-1];
        (*last_fld)=(uint8_t)(0xFF<<map_fields.rem);
        int64_t offset = 0;
        int mode = 0x0777;
        std::string img_file_name = "backup_";
        memset(SESSIONNAME,0,50);
        sprintf(SESSIONNAME,"%lx",(size_t)session_num_const);
        img_file_name+=SESSIONNAME;
        img_file_name+=".iso";
        im_f_des_const = open(img_file_name.c_str(), O_RDWR | O_CREAT | O_TRUNC, mode );
        int err = posix_fallocate64(im_f_des_const,offset,(int64_t)file_len_const);
        if(err)
         {
          close(im_f_des_const);
          free(rv_pkt);
          fprintf(stderr, "VssCoreSS:/> posix_fallocate(file for bkup) failed %s\n", strerror(errno));
          return -1;
         }
        fprintf(stdout, "VssCoreSS:/> FMap :# of packets mast be received %zd, size file(bytes) %zd\n",count_pkts_receive,file_len_const);
       }
      break;
      case CLOSE_BK_FILE:
       fprintf(stdout, "VssCoreSS:/>receive <End Of transmit data>");
       uint8_t* p_blocks = CheckImageMap(num_pages,cn_lost_blocks,mark_field_array,last_real_bit);
       if(p_blocks==nullptr)
        {
         close(im_f_des_const);
         free(rv_pkt);
         fprintf(stderr, "VssCoreSS:/>  CheckImageMap failed \n");
         return -1;
        }
       if(cn_lost_blocks > 0)
        {

         fprintf(stderr, "VssCoreSS:/>must be restored %ld pakets \n",cn_lost_blocks);
         int ret = Request4RestoredLostBl(p_blocks,rv_pkt->snd_socket,rv_pkt->host_addr,rv_pkt->sender,cn_lost_blocks);
         if(ret == -1)
          {
           fprintf(stderr,  "VssCoreSS:/> SendRepitNum - thread ID is nullptr \n");
          }
         wait_close = true;
         break;
        }
       fprintf(stdout, "VssCoreSS:/>CLOSE_BK_FILE exec without error.Map of image no error. map memory flush \n");
       num_pages = 0;
       wait_close = false;
       close(im_f_des_const);
      break;
     }
    }
   break;
   case STREAM:
    {
     ssize_t len_rcv = rv_pkt->recv_len;
     if (im_f_des_const < 0)
      {
       fprintf(stderr, "VssCoreSS:/>  CheckImageMap failed \n");
       close(im_f_des_const);
       free(rv_pkt);
       return -1;
      }

     rv_pkt->recv_len -= lenAtomPktHd_t;
     rv_pkt->recv_len -=lenAtom64_t;
     if (rv_pkt->recv_len < 1)
      {
       free(rv_pkt);
       fprintf(stdout, "VssCoreSS:/> write backUp error recv_len < 7\n");
       return -1;
      }
     uint8_t* pTmp = rv_pkt->rcv_packet.data;
     uint64_t block_index = 0;
     memcpy(&block_index, pTmp, lenAtom64_t);
     map_fields = lldiv((ssize_t)block_index,8);
     uint64_t num_page = (size_t)map_fields.quot;
     p_mark = &mark_field_array[num_page];
     uint8_t t=*p_mark;
     if(((t>>map_fields.rem)&1)>0 )
      {

       free(rv_pkt);
       return -1;
      }
     bit_ps+=len_rcv;
     pTmp += lenAtom64_t;
     wr_bytes += rv_pkt->recv_len;
     bytes_ps += rv_pkt->recv_len;
     wr_blocks++;

     clock_gettime(CLOCK_REALTIME,&curr_time);
     int dif = 0;
     if(start_time.tv_sec!=0)
      {
        dif = int (curr_time.tv_sec - start_time.tv_sec);
      }
       else
      {
       start_time = curr_time ;
      }
     if ( (dif >= 1) )
      {
       start_time = curr_time;
       double br_bits = bit_ps*8;
       bit_ps=0;
       char str[]=" bits/sec";
       double del = 1.0;
       if(br_bits > 1000 && br_bits < 1000000)
        {
         del = 1000.0;
         memcpy(str,"kbits/sec",4);
        }
       else
        {
         del= 1000000.0;
         memcpy(str,"mbits/sec",4);
        }
       br_bits /=del;

       std::string  dir_str ="VssCoreSS:/>rcv from client #";
       if(wait_close == true)
        {
         dir_str ="VssCoreSS:/>rst from client #";
        }
       dir_str += SESSIONNAME;
       dir_str += " p/sec:BR=";
       mb_bytes=(int64_t)((block_index+1)*(uint64_t)rv_pkt->recv_len);
       dropp =mb_bytes - wr_bytes;
       if(were_drop != dropp)
        {
         fprintf(stdout, "%d %s%7.3f %s,in total: bytes=%zd, pkts=%zd;!!!FIX dropped pkts=%zd\n",
                 dif,
                 dir_str.c_str(),
                 br_bits,
                 str,
                 wr_bytes,
                 wr_blocks,
                 dropp/SZ_FILE_DATA);

        }
       else
        {
         fprintf(stdout, "%s%7.3f %s,in total: bytes=%zd, pkts=%zd\n",
                 dir_str.c_str(),
                 br_bits,
                 str,
                 wr_bytes,
                 wr_blocks);
        }
       were_drop = dropp;
       bytes_ps = 0;
      }

     offsef_by_indx = block_index * SZ_FILE_DATA;
     if(block_index==count_pkts_receive-1)
      {
       printf("VssCoreSS:/> last packet rcv; offsef_by_indx %zd\n",offsef_by_indx);
      }
       poit2freePkt = rv_pkt;
       write_data_len = (size_t)rv_pkt->recv_len;
       WriteImgData(pTmp);

    }
    break;
   }
  return 0;
 }
void ClientStoredImg::WriteImgData(uint8_t* p_tmp)
 {

  int64_t res = lseek64(im_f_des_const,(int64_t)offsef_by_indx,SEEK_SET);
  if(res==-1)
   {
    fprintf(stderr, "VssCoreSS:/> lseek64 failed %s\n", strerror(errno));
   }
  else
   {
    if((res%SZ_FILE_DATA)!=0)
     {
      fprintf(stderr, "VssCoreSS:/> set offset for write last packet ok, result:%zd\n",res);
     }
      int8_t num_bl_pg = (int8_t)(1<<map_fields.rem);
      (*p_mark) |= num_bl_pg;
      struct iovec iov;
      iov.iov_base = p_tmp;
      iov.iov_len = write_data_len;
      sched_yield();
      writev(im_f_des_const,&iov,1);

      free(poit2freePkt);
   }

}
void* ClientStoredImg::ThreadReceiveDataFromClient(void* thread_ctx)
 {
  THREAD_CONTEXT* ctx = (THREAD_CONTEXT*) thread_ctx;
  char recv_buff[SZ_UDP_PKT ] =	{ 0 };
  size_t  buff_soct_size = SZ_UDP_PKT ;
  pthread_mutex_t *rcv_mtx = ctx->mtx;
  sockaddr_in client_addr;
  int &rcv_socket = ctx->rcv_socket;
  ssize_t recv_len = 0;
  fd_set s_set;
  timeval timeout;
  while (data_recv_stoped == 0)
   {
    FD_ZERO(&s_set);
    FD_SET(rcv_socket, &s_set);
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    int ret = select(rcv_socket+1, &s_set, nullptr, nullptr, &timeout);
    if(ret <0)
     {
      free(thread_ctx);
      printf ("VssCoreSS:/> select error  %s\n",strerror(errno));
      pthread_exit(nullptr);
     }
    if (FD_ISSET(rcv_socket, &s_set))
     {
      memset(recv_buff,0,SZ_UDP_PKT);
      int cn_iterr=0;
      int ret = recv_data_raw(rcv_socket, recv_buff, &recv_len, buff_soct_size, client_addr, slen);
      while(ret == SOCKET_ERROR)
       {
        close(rcv_socket);
        ret = init_socket(ctx->rcv_socket,ctx->host_addr);
        if(ret == SOCKET_OK)
         {
          memset(recv_buff,0,SZ_UDP_PKT);
          rcv_socket = ctx->rcv_socket;
          ret = recv_data_raw(rcv_socket, recv_buff, &recv_len, buff_soct_size, client_addr, slen);
          cn_iterr++;
          if(cn_iterr>10)
           {
            free(thread_ctx);
            fprintf(stderr, "VssCoreSS:/> recv_data_raw - SOCKET ERROR \n");
            pthread_exit(nullptr);
           }
         }
       }
      if (ret == 0)
       {
        FD_CLR(rcv_socket, &s_set);
        continue;
       }

      RCV_SESSION_DATA* rv_pkt = (RCV_SESSION_DATA*)calloc(1,sizeof(RCV_SESSION_DATA));
      memcpy(&rv_pkt->rcv_packet,recv_buff,(size_t)ret);
      memcpy(&rv_pkt->sender,&client_addr,sizeof(sockaddr_in));
      rv_pkt->recv_len = ret;
      rv_pkt->rcv_socket = rcv_socket;
      rv_pkt->snd_socket = ctx->snd_socket;
      rv_pkt->host_addr = *ctx->host_addr;
      pthread_mutex_lock(rcv_mtx);
            vss_packets.push(rv_pkt);
      pthread_mutex_unlock(rcv_mtx);
     }
    else
     {
      //timeout socket ready;
      printf ("VssCoreSS:/> recv timeout %s\n",strerror(errno));
      if(vss_packets.empty()!=true)
       {
          FD_CLR(rcv_socket, &s_set);
          continue;
       }
      uint8_t * p_blocks = obj->CheckImageMap(obj->num_pages,obj->cn_lost_blocks,obj->mark_field_array,obj->last_real_bit);
      if(p_blocks == nullptr)
       {
        FD_CLR(rcv_socket, &s_set);
        continue;
       }
      if(obj->cn_lost_blocks > 0)
       {
        fprintf(stdout, "VssCoreSS:/> packet with cmd CLOSE_BK_FILE Lost.Data begin restored \n");
        int rez = obj->SendRepitNum(p_blocks, obj->send_socket,obj->host_addr,client_addr,obj->cn_lost_blocks,&obj->client_mtx);
        if(rez != 0)
         {
          free(thread_ctx);
          fprintf(stderr,  "VssCoreSS:/> SendRepitNum - thread ID is nullptr \n");
          pthread_exit(nullptr);
         }
        continue;
       }
      if(obj->wait_close == true)
       {
        fprintf(stdout, "VssCoreSS:/>Map of image no error. map memory flush \n");
        obj->num_pages = 0;
        obj->wait_close = false;
        //munmap(bk_img,file_len);
        close(obj->im_f_des_const);
       }
      FD_CLR(rcv_socket, &s_set);
    }

   }

  printf("VssCoreSS:/>  Thread finished %s\n", strerror(errno));
  fflush(stdout);
  pthread_exit(nullptr);
 }
void* ClientStoredImg::ThreadStorageData2Image(void* thread_ctx)
{
  THREAD_CONTEXT* ctx = (THREAD_CONTEXT*) thread_ctx;
  void *data_recv = ctx->hlp_pointer;
  pthread_mutex_t &rcv_mtx = *reinterpret_cast<pthread_mutex_t*>(ctx->mtx);
  std::queue<RCV_SESSION_DATA*> *list_pointers=reinterpret_cast<std::queue<RCV_SESSION_DATA*>*>(data_recv);

  while (data_recv_stoped == false)
   {
    if(list_pointers->empty() == true)
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

    if(rv_pkt->rcv_packet.pkt_head.SESSION_NUM != obj->session_num_const)
     {
      free(rv_pkt);
      continue;
     }

    if(rv_pkt->rcv_packet.pkt_head.PACKET_NUM == obj->packet_num)
     {
      free(rv_pkt);
      continue;
     }
    obj->StorageData(rv_pkt);
  }
  printf("VssCoreSS:/>  parser Thread finished %s\n", strerror(errno));
  fflush(stdout);
  pthread_exit(nullptr);
}
void* ClientStoredImg::ThreadSendRepitNumersBB(void* thread_ctx)
 {
  THREAD_CONTEXT* ctx = (THREAD_CONTEXT*)((void*)thread_ctx);
  sockaddr_in hostAddr =*ctx->host_addr;
  sockaddr_in clientAddr = *ctx->target_addr;
  uint8_t* p_blocks = ctx->data;
  uint64_t cn_lost_blocks = ctx->imageFile_size;
  int ret = obj->Request4RestoredLostBl(p_blocks,
                                        obj->send_socket,
                                        hostAddr,
                                        clientAddr,
                                        cn_lost_blocks);
  if(ret == -1)
   {
    fprintf(stderr,  "VssCoreSS:/> SendRepitNum - thread ID is nullptr \n");
   }
  free(ctx->data);
  free(ctx);
  pthread_exit(nullptr);
 }

int ClientStoredImg::SendRepitNum(uint8_t *numOffblocks,// array of numbers of blocks for restored
                                  int &sendSock,
                                  sockaddr_in &hostAddr,
                                  sockaddr_in &clientAddr,
                                  size_t cn_lostBlocks,//counts blocks
                                  pthread_mutex_t *mtx
                                 )
 {
  uint8_t* ctx = (uint8_t*)calloc(1,sizeof(THREAD_CONTEXT));
  THREAD_CONTEXT* thread_ctx = (THREAD_CONTEXT*)((void*)ctx);
  thread_ctx->snd_socket = sendSock;
  thread_ctx->rcv_socket = 0;
  thread_ctx->host_addr = &hostAddr;
  thread_ctx->target_addr = &clientAddr;
  thread_ctx->thread_id =nullptr;
  thread_ctx->data = numOffblocks ;// array of numbers of blocks for restored
  thread_ctx->imageFile_size = cn_lostBlocks;
  thread_ctx->mtx=mtx;
  pthread_attr_t threadAttr;
  pthread_attr_init(&threadAttr);
  pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
  pthread_t *thread_id = nullptr ;
  int rez = pthread_create(thread_id,nullptr, ThreadSendRepitNumersBB, (void*)thread_ctx);
  return rez;
 }

