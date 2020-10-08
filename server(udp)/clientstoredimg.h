#pragma once
#include "../Common/GlobalVar.h"
#include "helper.h"
#include <sys/uio.h>
#include <QThreadPool>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
class ClientStoredImg
 {
 protected:
  static bool data_recv_stoped;
  static bool data_storage_stoped;
  static ClientStoredImg *obj;
  static std::queue<RCV_SESSION_DATA*> vss_packets;
  static char SESSIONNAME[];
  int SendRepitNum(uint8_t *numOffblocks,// array of numbers of blocks for restored
                   int &sendSock,
                   sockaddr_in &hostAddr,
                   sockaddr_in &clientAddr,
                   size_t cn_lostBlocks,//counts blocks
                   pthread_mutex_t *mtx
                  );
  static void* ThreadReceiveDataFromClient(void* thread_ctx);
  static void* ThreadStorageData2Image(void* thread_ctx);
  static void* ThreadSendRepitNumersBB(void* thread_ctx);
 public:

  int InitClient();

  ClientStoredImg(char* ip_point, uint16_t port_resv,uint64_t iid_session_num);
  ~ClientStoredImg();
  uint8_t* CheckImageMap(size_t num_pages,
                           size_t &cn_lost_blocks,
                           uint8_t* imageMap,
                           int8_t numRlbits);
  int  Request4RestoredLostBl(uint8_t *blocks,
                              int &sendSock,
                              sockaddr_in &hostAddr,
                              sockaddr_in &clientAddr,
                              size_t cn_lostBlocks
                              );

  int8_t StorageData(RCV_SESSION_DATA* rv_pkt);
  void WriteImgData(uint8_t* p_tmp);

  std::string host_ip;
  uint16_t work_port;
  int send_socket ;
  int recv_socket ;
  sockaddr_in host_addr;
  sockaddr_in client_addr;
  pthread_t recv_thread_id;
  pthread_mutex_t client_mtx;

  //client unic worked pararmeters
  int im_f_des_const;
  int64_t  wr_bytes ;
  int64_t  were_drop;
  ssize_t dropp;
  int64_t  mb_bytes;
  int64_t  bytes_ps ;
  int64_t  bit_ps ;
  size_t cn_lost_blocks;
  uint64_t session_num_const;
  uint64_t packet_num;
  uint64_t offsef_by_indx ;
  uint64_t count_pkts_receive;
  uint64_t wr_blocks;
  uint64_t file_len_const;
  uint64_t num_pages;
  int8_t last_real_bit;
  size_t write_data_len;
  lldiv_t map_fields;
  bool wait_close;
  bool start;
  uint8_t* p_mark;
  uint8_t* mark_field_array;
  uint8_t* pArrayOfNumbersLB;
  RCV_SESSION_DATA* poit2freePkt;

 };
