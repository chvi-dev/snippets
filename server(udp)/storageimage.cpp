#include "../Common/GlobalVar.h"
#include "../Common/srv_sockets.h"
#include "../Common/srv_bkup_image.h"
#include "storageimage.h"
#include "helper.h"
#include <sys/uio.h>
#include <QThreadPool>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
typedef struct
 {
  pthread_mutex_t *mtx;
  CLIENT_LIB* pkt_id;
 }THREAD_IMG_CONTEXT;

int8_t StorageData(pthread_mutex_t &prs_mtx,CLIENT_LIB* pkt_id,RCV_SESSION_DATA* rv_pkt);
void WriteData(CLIENT_LIB* pkt_id);
void* ThreadWriteData2Image(void* thread_ctx);

int8_t StorageData(pthread_mutex_t &prs_mtx , CLIENT_LIB* pkt_id,RCV_SESSION_DATA* rv_pkt)
 {
  (void)prs_mtx;
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
        memcpy(&obj->file_len_const, pTmp, sizeof(uint64_t));
        lldiv_t pkts_must_be_receive = lldiv((ssize_t)pkt_id->file_len_const, SZ_FILE_DATA);
        pkt_id->count_pkts_receive = (size_t)pkts_must_be_receive.quot;
        if (pkts_must_be_receive.rem > 0)
         {
          pkt_id->count_pkts_receive++;
         }

        lldiv_t map_fields = lldiv((ssize_t)pkt_id->count_pkts_receive,8);
        pkt_id->num_pages = (size_t)map_fields.quot;
        if (map_fields.rem > 0)
         {
          pkt_id->num_pages++;
         }
        pkt_id->mark_field_array = (uint8_t*) malloc(pkt_id->num_pages);
        if (pkt_id->mark_field_array == nullptr)
         {
          free(rv_pkt);
          fprintf(stderr,  "VssCoreSS:/> Markers memory allocate - ERROR_INSUFFICIENT_BUFFER \n");
          return -1;
         }
        memset(pkt_id->mark_field_array,0,pkt_id->num_pages);
        pkt_id->last_real_bit=(int8_t)map_fields.rem;
        uint8_t *last_fld = &pkt_id->mark_field_array[pkt_id->num_pages-1];
        (*last_fld)=(uint8_t)(0xFF<<map_fields.rem);
        int64_t offset = 0;
        int mode = 0x0777;
        std::string img_file_name = "backup_";
        img_file_name+=pkt_id->SESSIONNAME;
        img_file_name+=".iso";
        pkt_id->im_f_des_const = open(img_file_name.c_str(), O_RDWR | O_CREAT | O_TRUNC, mode );
        int err = posix_fallocate64(pkt_id->im_f_des_const,offset,(int64_t)pkt_id->file_len_const);
        if(err)
         {
          close(pkt_id->im_f_des_const);
          free(rv_pkt);
          fprintf(stderr, "VssCoreSS:/> posix_fallocate(file for bkup) failed %s\n", strerror(errno));
          return -1;
         }
        fprintf(stdout, "VssCoreSS:/> FMap :# of packets mast be received %zd, size file(bytes) %zd\n",pkt_id->count_pkts_receive,pkt_id->file_len_const);
       }
      break;
      case CLOSE_BK_FILE:
       fprintf(stdout, "VssCoreSS:/>receive <End Of transmit data>");
       uint8_t* p_blocks = CheckImageMap(pkt_id->num_pages,pkt_id->cn_lost_blocks,pkt_id->mark_field_array,pkt_id->last_real_bit);
       if(p_blocks==nullptr)
        {
         close(pkt_id->im_f_des_const);
         free(rv_pkt);
         fprintf(stderr, "VssCoreSS:/>  CheckImageMap failed \n");
         return -1;
        }
       if(pkt_id->cn_lost_blocks > 0)
        {
         //client_addr.sin_port=clien_rv_port;
         fprintf(stderr, "VssCoreSS:/>must be restored %ld pakets \n",pkt_id->cn_lost_blocks);
         int ret = Request4RestoredLostBl(p_blocks,rv_pkt->snd_socket,rv_pkt->host_addr,rv_pkt->sender,pkt_id->cn_lost_blocks);
         if(ret == -1)
          {
           fprintf(stderr,  "VssCoreSS:/> SendRepitNum - thread ID is nullptr \n");
          }
         pkt_id->wait_close = true;
         break;
        }
       fprintf(stdout, "VssCoreSS:/>CLOSE_BK_FILE exec without error.Map of image no error. map memory flush \n");
       pkt_id->num_pages = 0;
       pkt_id->wait_close = false;
       close(pkt_id->im_f_des_const);
      break;
     }
    }
   break;
   case STREAM:
    {
     ssize_t len_rcv = rv_pkt->recv_len;
     if (pkt_id->im_f_des_const < 0)
      {
       fprintf(stderr, "VssCoreSS:/>  CheckImageMap failed \n");
       close(pkt_id->im_f_des_const);
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
     pkt_id->pTmp = rv_pkt->rcv_packet.data;
     uint64_t block_index = 0;
     memcpy(&block_index, pkt_id->pTmp, lenAtom64_t);
     pkt_id->map_fields = lldiv((ssize_t)block_index,8);
     uint64_t num_page = (size_t)pkt_id->map_fields.quot;
     pkt_id->p_mark = &pkt_id->mark_field_array[num_page];
     uint8_t t=*pkt_id->p_mark;
     if(((t>>pkt_id->map_fields.rem)&1)>0 )
      {
       //printf("VssCoreSS:/> map dublicate  \n");
       free(rv_pkt);
       return -1;
      }
     pkt_id->bit_ps+=len_rcv;
     pkt_id->pTmp += lenAtom64_t;
     pkt_id->wr_bytes += rv_pkt->recv_len;
     pkt_id->bytes_ps += rv_pkt->recv_len;
     pkt_id->wr_blocks++;
     //fprintf(stdout, "#blocks: %zd , bytes: %lu \n",block_index+1 , recv_len);
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
       double br_bits = pkt_id->bit_ps*8;
       pkt_id->bit_ps=0;
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
       if(pkt_id->wait_close == true)
        {
         dir_str ="VssCoreSS:/>rst from client #";
        }
       dir_str += pkt_id->SESSIONNAME;
       dir_str += " p/sec:BR=";
       //					    fprintf(stdout, "VssCoreSS:/>Packets recv from %s:%d ,number blocks: %zd , bytes: %lu \n",
       //						                 inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
       //						                 block_index , wr_bytes);
       pkt_id->mb_bytes=(int64_t)((block_index+1)*(uint64_t)rv_pkt->recv_len);
       pkt_id->dropp =pkt_id->mb_bytes - pkt_id->wr_bytes;
       if(pkt_id->were_drop != pkt_id->dropp)
        {
         fprintf(stdout, "%d %s%7.3f %s,in total: bytes=%zd, pkts=%zd;!!!FIX dropped pkts=%zd\n",
                 dif,
                 dir_str.c_str(),
                 br_bits,
                 str,
                 pkt_id->wr_bytes,
                 pkt_id->wr_blocks,
                 pkt_id->dropp/SZ_FILE_DATA);

        }
       else
        {
         fprintf(stdout, "%s%7.3f %s,in total: bytes=%zd, pkts=%zd\n",
                 dir_str.c_str(),
                 br_bits,
                 str,
                 pkt_id->wr_bytes,
                 pkt_id->wr_blocks);
        }
       pkt_id->were_drop = pkt_id->dropp;
       pkt_id->bytes_ps = 0;
      }

     pkt_id->offsef_by_indx = block_index * SZ_FILE_DATA;
     if(block_index==pkt_id->count_pkts_receive-1)
      {
       printf("VssCoreSS:/> last packet rcv; offsef_by_indx %zd\n", pkt_id->offsef_by_indx);
      }
       pkt_id->poit2freePkt = rv_pkt;
       pkt_id->write_data_len = (size_t)rv_pkt->recv_len;

//       pthread_t thread_id;
//       THREAD_IMG_CONTEXT* ctx =(THREAD_IMG_CONTEXT*)calloc(1,sizeof(THREAD_IMG_CONTEXT));
//       ctx->mtx = &prs_mtx;
//       ctx->pkt_id = pkt_id;

//       int result = pthread_create( &thread_id, nullptr, ThreadWriteData2Image, (void*)ctx);
//       if(result !=0 )
//        {
//         free(ctx);
//         printf ("VssCoreSS:/> parser thread create  %s\n",strerror(errno));
//         return -1;
//        }

      WriteData (pkt_id);

       //QThreadPool pool;
       //QFuture<void> future = QtConcurrent::run(&pool,WriteData,prs_mtx,pkt_id);
    }
    break;
   }
  return 0;
 }
void WriteData(CLIENT_LIB* pkt_id)
 {

  int64_t res = lseek64(pkt_id->im_f_des_const,(int64_t)pkt_id->offsef_by_indx,SEEK_SET);
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
     int8_t num_bl_pg = (int8_t)(1<<pkt_id->map_fields.rem);
     (*pkt_id->p_mark) |= num_bl_pg;
      struct iovec iov;
      iov.iov_base = pkt_id->pTmp;
      iov.iov_len = pkt_id->write_data_len;
      sched_yield();
      writev(pkt_id->im_f_des_const,&iov,1);

      free(pkt_id->poit2freePkt);
   }

}
void* ThreadWriteData2Image(void* thread_ctx)
 {
  THREAD_IMG_CONTEXT* ctx = (THREAD_IMG_CONTEXT*)thread_ctx;
  //pthread_mutex_lock(ctx->mtx);
  WriteData (ctx->pkt_id);
  //pthread_mutex_unlock(ctx->mtx);
  free(ctx);
  pthread_exit(nullptr);
 }
