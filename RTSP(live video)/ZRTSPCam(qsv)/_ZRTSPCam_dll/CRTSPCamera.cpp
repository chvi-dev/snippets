#include "CRTSPCamera.h"
#include "common/include/common_utils.h"
#include <WinBase.h>

#define DEBUG_ACCEL 0
mfxHDL deviceHandle=0;
#define AV_CODEC_FLAG_INTERLACED_DCT   (1 << 18)
#define AV_CODEC_FLAG_CLOSED_GOP   (1U << 31)
#define AV_CODEC_FLAG_QSCALE   (1 << 1)
extern void TracerOutputText(CRTSPCamera* dvs, ZLOG_LEVEL level, LPCTSTR Format, ...);
//------------------------Создание обьекта pDevice-------------------------------------------

//State M------------in DS_ALLOCATED ; out DS_ALLOCATED---------------------
CRTSPCamera* CRTSPCamera::CreateDecoder()
{
	CRTSPCamera* objDecoder=new CRTSPCamera();
	return objDecoder;
}
CRTSPCamera::CRTSPCamera()
{

	DecoderID=(DWORD)this;   
	pCodecCtx=NULL;
	pCodec=NULL;
	pFrame=NULL;
	videoStream=1;
	StartReceiveFrame=0;
	delta_time_rcv=0;
	m_count_drop_frame=0;
	m_count_corrupt_frame=0;
	m_ipaddr.S_un.S_addr = 0;
	m_camera_handle = 0;
	m_device_is_running = false;
	m_cam_start_time=-1;
	m_PartNumber = 0;
	m_PartVersion.clear();
	m_PartRevision.clear();
	m_SerialNumber = 0;
	m_UniqueId = 0;
	m_ModelName.clear();
	m_frame_arrival = 0;
	m_fPts=0;
	m_d_pts=0;
	m_ts_r=0;
	m_id = 0;
	m_type = 0x01;
	IP_addr="";
	m_sid=0;
	pts=0;
	Stopped=false;

}
void  CRTSPCamera::InitObject()
{
	srv=new Services();
	std::string evName;
	std::string id;
	std::stringstream str_stream;
	str_stream<<this;
	str_stream>>id;
	str_stream.clear();


	evName="EvDevAllocated:#"+id;
	ev_DvAllocated=evDSArray[0]=CreateEvent(NULL,false,false,evName.c_str());
	evName="EvDisconnect:#"+id;
	ev_DvDisconnect=evDSArray[1]=CreateEvent(NULL,false,false,evName.c_str());
	evName="EvDevPaused:#"+id;
	ev_DvPaused=evDSArray[2]=CreateEvent(NULL,false,false,evName.c_str());
	evName="EvDevStopped:#"+id;
	ev_DvStop=evDSArray[3]=CreateEvent(NULL,false,false,evName.c_str());
	evName="EvDevRunning:#"+id;
	ev_DvRunning=evDSArray[4]=CreateEvent(NULL,false,false,evName.c_str());
	evName="EvStopMonitor:#"+id;
	ev_StopMonitor=evDSArray[5]=CreateEvent(NULL,false,false,evName.c_str()); 
	evName="EvCtrlAVframe:#"+id;
	ev_CtrlAVframe=CreateEvent(NULL,false,false,evName.c_str());
	evName="EvCameraReady:#"+id;
	ev_CamReady=evDSArray[6]=CreateEvent(NULL,false,false,evName.c_str());
	evName="EvInvalidLink:#"+id;
	ev_InvalidLink=evDSArray[7]=CreateEvent(NULL,false,false,evName.c_str());
	evName="EvDvResetConnect:#"+id;
	ev_DvResetConnect=evDSArray[8]=CreateEvent(NULL,false,false,evName.c_str());


	m_Thread_Status_Monitor_end=false;
	m_Thread_add_frame_end=false;
	m_Thread_decoder_end=false;
	hthread_frame_add=NULL;
	hStatus_Monitor=NULL;
	hDecoderStart=NULL;

	m_YearDelta=0;
	m_MonthDelta=0;
	m_DayOfWeekDelta=0;
	m_DayDelta=0;
	m_HourDelta=0;  
	m_MinuteDelta=0;
	m_SecondDelta=0;  
	m_MillisecondsDelta=0;
	pFormatCtx=NULL;
	pFrame=NULL;
	m_Monitoring=true; 
	InterlockedExchange((LONG*)&m_receive,false);
	InterlockedExchange((LONG*)&link_on,false);
	hStatus_Monitor=(HANDLE)_beginthread(CRTSPCamera::Thread_Status_Monitor, 0,this); 
	SetThreadPriority(hStatus_Monitor,THREAD_PRIORITY_LOWEST);
	SetEvent(ev_DvAllocated);
	link_on=false;
	QSV_enable=false;
	m_mfxDEC=NULL;
	accel_mode=DEBUG_ACCEL;
	num_obj++;
	srv->m_logger=NULL;
	TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__" Done OK, device id 0x%x"  ,DecoderID); 
}
void  __cdecl  CRTSPCamera::Thread_Status_Monitor(void* _this)
{
	if( _this == NULL ) return;

	CRTSPCamera* device=reinterpret_cast<CRTSPCamera*>(_this);
	TracerOutputText(device,ZLOG_LEVEL_INFO,__FUNCTION__" start, device id 0x%x enter "  ,device->DecoderID); 
	device->m_Thread_Status_Monitor_end=false;
	while(device->m_Monitoring)
	{

		DWORD AlertEv=WaitForMultipleObjects(9,device->evDSArray,false,WAIT_LINK_TIMEOUT);
		switch(AlertEv)
		{
		case WAIT_OBJECT_0:  // ev_DvAllocated
			device->m_device_state=DS_ALLOCATED;
			break;
		case WAIT_OBJECT_0+1:// ev_DvDisconnect
			device->m_device_state=DS_DISCONNECTED;
			break;
		case WAIT_OBJECT_0+2:// ev_DvPaused
			device->m_device_state=DS_PAUSED;
			device->pause_stop=true;
			break;
		case WAIT_OBJECT_0+3://ev_DvStop
			device->m_device_state=DS_STOPPED; 
			InterlockedExchange((LONG*)&device->m_receive,false); //Дима ,здесь останавливается runtime потока декодирования 
			TracerOutputText(device,ZLOG_LEVEL_INFO,__FUNCTION__",Thread \"StartDecoderThreads()\" will be close,event \"ev_DvStop\"" );
			break;
		case WAIT_OBJECT_0+4: //ev_DvRunning 
			device->m_device_state=DS_RUNNING;
			TracerOutputText(device,ZLOG_LEVEL_DEBUG,__FUNCTION__"m_device_state=DS_RUNNING" );
			break;
		case WAIT_OBJECT_0+5: //ev_StopMonitor 
			device->m_device_state=DS_DISCONNECTED; 
			device->m_Monitoring=false;
			break;
		case WAIT_OBJECT_0+6://ev_CamReady
			{
				CZcoreMQueue* mqueue=NULL;	
				std::vector<CZcoreMQueue*> ::iterator imqueue;
				for( imqueue= device->mqueue_array.begin();imqueue != device->mqueue_array.end();imqueue++)
				{
					mqueue=*imqueue;
					if(mqueue==NULL)continue;
					//zcore_MQueue_SetMaxSize(mqueue,1);
					int gt_sz=zcore_MQueue_GetMaxSize(mqueue);
					if (gt_sz>5)
					{				
					 fprintf(stderr,"MQueue Error maxsize=%d ,be reinit\n",gt_sz);
					// zcore_MQueue_SetMaxSize(mqueue,1);
					// gt_sz=zcore_MQueue_GetMaxSize(mqueue);
					}
					fprintf(stderr,"MQueue maxsize=%d \n",gt_sz);
				} 
				device->m_device_state=DS_ALLOCATED;
			}
			break;
		case WAIT_OBJECT_0+7:     //ev_InvalidLink
			device->m_device_state=DS_DISCONNECTED;
			TracerOutputText(device,ZLOG_LEVEL_ERROR,__FUNCTION__",Invalid url,m_device_state=DS_DISCONNECTED" );
			break;
		case WAIT_OBJECT_0+8:// ev_DvResetConnect
			device->m_device_state=DS_ALLOCATED;
			TracerOutputText(device,ZLOG_LEVEL_INFO,__FUNCTION__",m_device_state=DS_ALLOCATED" );
			break; 
		case WAIT_TIMEOUT:
			if(device->m_device_state==DS_RUNNING||device->m_device_state==DS_ALLOCATED)
			{
				device->m_device_state=DS_DISCONNECTED;
				TracerOutputText(device,ZLOG_LEVEL_INFO,__FUNCTION__",m_device_state=DS_DISCONNECTED,link lost within %d seconds ",WAIT_LINK_TIMEOUT/1000 );
			}
			break;
		}
		if(device->m_Monitoring==false)
			break;
	}
	if(device->m_receive==true)InterlockedExchange((LONG*)&device->m_receive,false);	
	device->m_Thread_Status_Monitor_end=true;
	TracerOutputText(device,ZLOG_LEVEL_INFO,__FUNCTION__"decoder ID 0x%x,Thread_Status_Monitor END!!!",device->DecoderID); 
}
//State M-------------in DS_ALLOCATED ; out DS_DISCONNECTED-------------------
int  __cdecl  CRTSPCamera::interrupt_cb(void* ctx)
{
	CRTSPCamera* device = reinterpret_cast<CRTSPCamera*>(ctx);
	if(device)
	{
		fprintf(stderr,"interrupt_cb flush\n");
		return 1;
	}
	return 0;
}
bool CRTSPCamera::InitDecoder()
{
	try
	{
		TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__" enter, URL= %s",URLCam.c_str() );

		av_FreeObj();

		pFormatCtx = avformat_alloc_context();
		if(pFormatCtx==0)
		{
			TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" avformat_alloc_context error,URLCam=%s "   ,URLCam);
			Sleep(100);
			return STATE_ERR;
		}

		AVDictionary *opts = 0;	
#ifdef RTSP_TCP	

		size_t found_http = URLCam.find("http://");
		if(found_http!=std::string::npos)
		{ 
			av_dict_set(&opts, "rtsp_transport", "http", 0);
			fprintf(stderr,"http transport %s\n",URLCam.c_str());
		}
	     else 
			   {
				   av_dict_set(&opts, "rtsp_transport", "tcp", 0);
		  	   } 
		
		av_dict_set(&opts, "rtsp_flags", "prefer_tcp", 0);
#else	
		av_dict_set(&opts, "rtsp_transport", "udp", 0);	
#endif 	

		av_dict_set(&opts, "max_delay", "500000", 0);
		av_dict_set(&opts, "allowed_media_types", "video", 0);
		av_dict_set(&opts, "stimeout", "2000000", 0); 
		av_dict_set(&opts, "reorder_queue_size", "10", 0);
		//av_dict_set(&opts, "probesize", "100000000", 0);
		//av_dict_set(&opts, "max_analyze_duration", "100000000", 0);
		//URLCam="rtsp://10.10.3.245:554/cam0_0";
		HRESULT rdy=avformat_open_input(&pFormatCtx,URLCam.c_str(),NULL,&opts);
		av_dict_free(&opts);
		if(rdy != 0)
		{

			TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" avformat_open_input error,URLCam=%s "   ,URLCam);

			av_FreeObj();

			return STATE_ERR; 
		}
		//pFormatCtx->interrupt_callback.callback = interrupt_cb;
		//pFormatCtx->interrupt_callback.opaque =this;
		rdy=avformat_find_stream_info(pFormatCtx,NULL);
		if( rdy < 0)
		{

			av_FreeObj();

			TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" avformat_find_stream_info error "   );

			return STATE_ERR; 
		}
		av_dump_format(pFormatCtx,0,URLCam.c_str(), 0);

		videoStream =-1;
		for(int k=0; k <(int) pFormatCtx->nb_streams; k++)
		{
			if(pFormatCtx->streams[k]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
			{
				videoStream = k;
				TracerOutputText(this,ZLOG_LEVEL_DEBUG,__FUNCTION__" videoStream =%d "  ,videoStream );
				VideoStream=pFormatCtx->streams[k];
				pCodecCtx = pFormatCtx->streams[k]->codec;
				break;
			}
		}
		if(videoStream == -1)
		{
			TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" videoStream =%d "  ,videoStream );
			av_FreeObj();

			fprintf(stderr,"No video stream \n");
			return STATE_ERR; 
		}


		pCodec =avcodec_find_decoder(pCodecCtx->codec_id);
		if(pCodec==NULL)
		{

			TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" avcodec_find_decoder error "   );

			av_FreeObj();

			fprintf(stderr,"Set by default codec error \n");

			return STATE_ERR; 
		}

		double Pts=0;	   
		m_delay=0;
		m_cam_fps=0;
		AVRational fps=av_guess_frame_rate(NULL,VideoStream,NULL);
		int fr=fps.num/fps.den;
		fprintf(stderr,"\nFPS %d\n",fr);
		fprintf(stderr,"VideoStream->r_frame_rate.den %d\n",fps.den);
		fprintf(stderr,"VideoStream->r_frame_rate.num %d\n",fps.num);
		if(fr>=90000)fr=30;
		Pts = 1000/fr;
		m_delay=(int)Pts;	
		if(m_delay!=0&&fr<32)
		{

			m_cam_fps=fr;
			fprintf(stderr,"m_delay %d\n",m_delay);


		}
		else
		{
			fprintf(stderr,"Invalid av_guess_frame_rate \n");
			m_delay=33;
			m_cam_fps=30;

		}

		rdy=avcodec_open2(pCodecCtx,pCodec,NULL); 
		if(rdy < 0)
		{
			TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" avcodec_open2 error "   );

			av_FreeObj();

			return STATE_ERR;
		}
		if(pFrame)
		{
			av_frame_free(&pFrame);
			pFrame = NULL;
		}
		pFrame= av_frame_alloc();
		if(pFrame == NULL)
		{
			TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" avcodec_alloc_frame error "   );

			av_FreeObj();

			return STATE_ERR;
		}


	}

	catch (...)
	{
		if(pFrame)
		{
			av_frame_free(&pFrame);
			pFrame=NULL;
		}

		av_FreeObj();
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" avcodec_alloc_frame error "   );
		return STATE_ERR;
	}

	//====================================== init QSW =================================================	
	if(accel_mode)
	{	
		if(QSV_enable)
		{
			m_mfxAllocator.Free(m_mfxAllocator.pthis,&mfxResponse);
			delete []m_pDecSurfaces;
			m_mfxDEC->Close();
		}
		AVPacket packet;
		QSV_enable=false;
		while(!QSV_enable)
		{
			av_init_packet(&packet);
			int res = av_read_frame(pFormatCtx,&packet);
			if(res>=0)
				InitQSVDec(pCodecCtx,&packet);
			av_free_packet(&packet);
			Sleep(10);
		}
	}
	//=================================================================================================	
	fprintf(stderr,"avcodec_find_decoder() OK,id -%d ,camera  ID - 0x%x \n",pCodec->id,DecoderID);

	TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__" Done OK,URLCam=%s ",URLCam);

	SetEvent(ev_DvRunning);	

	InterlockedExchange((LONG*)&link_on,true);
	//=================================set point to decoder===========================================	
	if(QSV_enable)
	{
		AVDecodeVideo = &CRTSPCamera::AVDecodeVideoMFX;
	}
	else
	{
		AVDecodeVideo = &CRTSPCamera::AVDecodeVideoFFM;
	}
	InitialAcceleration();
	//=================================================================================================

	stream_tm_base.den=90000;
	stream_tm_base.num=1;
	fprintf(stderr,"stream_tm_base den=%d,num=%d\n",stream_tm_base.den,stream_tm_base.num);

	codec_tm_base.den=m_cam_fps;
	codec_tm_base.num=1;
	FPS=m_cam_fps;
	fprintf(stderr,"codec_tm_base den=%d,num=%d\n",codec_tm_base.den,codec_tm_base.num);
	return STATE_OK;	
}

void CRTSPCamera::CleanRawData(RawData* r_data)
{
	if(r_data)
	{
		delete []r_data->data;
		delete r_data;
		r_data=NULL;
	}

}

//State M-------------in DS_DISCONNECTED,DS_PAUSED ; out DS_RUNNING------------------------

void  __cdecl  CRTSPCamera::Thread_add_frame(void* _this)
{   
	if( _this == NULL ) return; 
	
	CRTSPCamera* queue=reinterpret_cast<CRTSPCamera*>(_this);
	TracerOutputText(queue,ZLOG_LEVEL_INFO,__FUNCTION__" start zCore control device ID 0x%x,",queue->DecoderID); 
	queue->m_Thread_add_frame_end=false;
	while(queue->m_receive)
	{
		DWORD AlertEv=WaitForSingleObject(queue->ev_CtrlAVframe, WAIT_FRAME_IN_QUEUE);
		if(queue->m_receive==false)
		{
			queue->m_Thread_add_frame_end=true;
			TracerOutputText(queue,ZLOG_LEVEL_INFO,__FUNCTION__" decoder ID 0x%x,Thread_add_frame END!!!"  ,queue->DecoderID); 
			return;
		}
		switch(AlertEv)
		{
		case WAIT_OBJECT_0:
			SetEvent(queue->ev_DvRunning);
			break;
		case WAIT_TIMEOUT:
			if(queue->link_on==false)
			{
				TracerOutputText(queue,ZLOG_LEVEL_ERROR,__FUNCTION__" device ID 0x%x,CONNECTION IS NOT RESTORED... %d sec",queue->DecoderID ,WAIT_FRAME_IN_QUEUE/1000);
				continue;
			}
			InterlockedExchange((LONG*)&queue->link_on,false);
			SetEvent(queue->ev_DvResetConnect);
			fprintf(stderr,"\nRESET CONNECTION!!! %d sec\n",WAIT_FRAME_IN_QUEUE/1000);
			TracerOutputText(queue,ZLOG_LEVEL_ERROR,__FUNCTION__" device ID 0x%x,RESET CONNECTION!!! %d sec",queue->DecoderID ,WAIT_FRAME_IN_QUEUE/1000);
			break;
		}
	}
	queue->m_Thread_add_frame_end=true;
	TracerOutputText(queue,ZLOG_LEVEL_INFO,__FUNCTION__"decoder ID 0x%x,Thread_add_frame END!!!"  ,queue->DecoderID);
}
//State M-----------in DS_DISCONNECTED work DS_RUNNING  out DS_STOPPED -> DS_DISCONNECTED ----

bool CRTSPCamera::IsDecoderReady()
{
	if(link_on==false)
	{
		if(InitDecoder()!=STATE_OK)
		{
			TracerOutputText(this,ZLOG_LEVEL_ERROR,"DID 0x%x,"__FUNCTION__",reinit decoder failure,may be error in connection",DecoderID);
			fprintf(stderr,"reinit decoder failure,may be error in connection \n");
			if(inBegining==false)
			{
				SetEvent(ev_InvalidLink);
			}
			return STATE_ERR;	 
		}
		m_fPts=0;
		m_d_pts=0;
		m_ts_r=0;
		m_cam_start_time=0;
		transmit_ready=false;
		pts=0;
		if(inBegining==false)
		{
			fprintf(stderr,"SetEvent(ev_CamReady)\n");
			SetEvent(ev_CamReady);
			inBegining=true;
		}

	}
	m_device_state=DS_RUNNING;
	return STATE_OK;
}
bool CRTSPCamera::IsVideoStream(AVPacket &stream_packet)
{
	if(stream_packet.stream_index != videoStream)
	{
		av_free_packet(&stream_packet);
		Sleep(m_delay);
		duration_T=(clock()- start_DC_tr);
		if(duration_T>=1000)
		{
			TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__",stream_packet.stream_index != videoStream!!!");
			start_DC_tr=clock();
		}
		return STATE_ERR;
	}
	return STATE_OK;
}
bool CRTSPCamera::IsValidStart_time( AVFormatContext *pFtCtx,AVPacket &stream_packet)
{
	if(m_cam_start_time<=0)
	{
		if(pFtCtx->start_time_realtime==AV_NOPTS_VALUE)
		{
			if((stream_packet.flags & AV_PKT_FLAG_KEY)==1)
			{
				m_cam_start_time=clock();
			}
			else
			{
				av_free_packet(&stream_packet);
				Sleep(m_delay*10);
				fprintf(stderr,"pFormatCtx->start_time_realtime AV_NOPTS_VALUE\n");
				duration_T=(clock()- start_DC_tr);
				if(duration_T>=1000)
				{
					TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__",pFormatCtx->start_time_realtime=AV_NOPTS_VALUE!!!");
					start_DC_tr=clock();
				}
				return STATE_ERR;
			}
		}
		else
			m_cam_start_time=pFtCtx->start_time_realtime;
	}
	return STATE_OK;
}
bool CRTSPCamera::IsFirstPtsSet(AVPacket &stream_packet)
{
	if(m_fPts==0)
	{
		if(stream_packet.flags!=AV_NOPTS_VALUE)
		{
			if(stream_packet.dts!=AV_NOPTS_VALUE)
			{
				m_fPts=stream_packet.dts;
				return STATE_OK;
			}
		}
	}
	else  
		return STATE_OK;

	av_free_packet(&stream_packet); 
	Sleep(m_delay);
	duration_T=(clock()- start_DC_tr);
	if(duration_T>=1000)
	{
		TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__",AV_NOPTS_VALUE in PTS!!!");
		start_DC_tr=clock();
	}
	return STATE_ERR;
}
bool CRTSPCamera::IsValidDuration(AVPacket &stream_packet)
{
	if(m_d_pts==0)
	{
		av_free_packet(&stream_packet);
		duration_T=(clock()- start_DC_tr);
		if(duration_T>=1000)
		{
			TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__",Calculation new delta PTS!!!");
			start_DC_tr=clock();
		}
		return STATE_ERR;
	}
	return STATE_OK;
}
void  CRTSPCamera::BuildTimeStamp()
{
	if(m_ts_r==0)
	{
#ifdef ZRTSP_USE_SYSTEM_TIME
		GetSystemTime(&m_cam_ts);
		SetFrameTimeStamp(&m_cam_ts,-m_MillisecondsDelta);


#else
		ts_r=m_cam_start_time/1000000;
		srv->UnixTimeToSystemTime((time_t)ts_r,&m_cam_ts);
		m_ts_r=1;
#endif

	} 
	else 
		SetFrameTimeStamp(&m_cam_ts,m_delay);
	TracerOutputText(this,ZLOG_LEVEL_DEBUG,__FUNCTION__",delta pts=%d , obj-> 0x%x",m_delay,this->DecoderID); 
}
void  CRTSPCamera::MQueue_Send(CZcoreVFrame* v_frame)
{
	CZcoreMQueue* mqueue=NULL;
	std::vector<CZcoreMQueue*> ::iterator imqueue;

	for(imqueue= mqueue_array.begin();imqueue!= mqueue_array.end();imqueue++)
	{
		if(!m_receive)
		{
			zcore_Destroy(v_frame);
			v_frame=NULL;
			return;
		}
		mqueue=*imqueue;
		if(mqueue==NULL)continue;
		int frames_in_zcore=zcore_MQueue_GetSize(mqueue);
		if(frames_in_zcore)continue;
		if (zcore_MQueue_Add(mqueue, ZCORE_MT_SDATA, v_frame)!=0)
		{
			TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" zcore_MQueue_Add Error!!! "  ); 
		}

	}
	Sleep(m_delay);
	zcore_Destroy(v_frame);
	v_frame=NULL;
}

void CRTSPCamera::CheckQualityFrame(AVPacket &ch_packet)
{
#ifdef DBG_FILE	
	time_t t = time(0);
	struct tm * now = localtime( & t );
	if(srv->time_start->tm_yday<now->tm_yday)
	{
		cn_bdFr=0;
		srv->time_start=now;
	}
	if(cn_bdFr<20)
	{

		std::stringstream tm;
		std::string Fr_fileNm;
		tm<<now->tm_mday<<"-"<<now->tm_hour<<"_"<<now->tm_min<<"_"<<now->tm_sec<<".jpg";
		tm>>Fr_fileNm;
		std::ofstream dbgfile;
		dbgfile.open(Fr_fileNm.c_str());
		dbgfile.write((char*)ch_packet.data,ch_packet.size);
		dbgfile.flush();
		dbgfile.close();
		cn_bdFr++;
	}
#endif
	av_free_packet(&ch_packet);
	Sleep(m_delay);
}

#define SOI         0xD8FF
#define APP0	    0xE0FF
#define JFIF        0x4649464A
#define DQT         0xDBFF
#define DHT         0xC4FF
#define SOF0        0xC0FF
#define SOF1        0xC1FF
#define SOF2        0xC2FF
#define HEAD_END    0xDAFF
#define PICT_END    0xD9FF
#define MAX_HDR_LEN 0x266
#define MARKER_JPG   (*(u_int16*)tmp)
#define MARKER_JFIF  (*(u_int32*)tmp)
#define SOFХ		((MARKER_JPG != SOF0) && (MARKER_JPG != SOF1) &&  (MARKER_JPG != SOF2))

BYTE* JumpOverTable(BYTE *c_point,int &c_offset,int &frLen)
{
	c_point+=2;
	c_offset+=2;
	WORD offset=(*c_point)<<8;
	offset|=(*(c_point+1));
	if(offset>=frLen-c_offset)
	{
		return NULL;
	}
	c_point+=offset;
	c_offset+=offset;
	return c_point;
}
int CheckFrameParams(AVCodecContext *avctx, AVPacket *avpkt)
{
	if(avctx==NULL) return -11;
	if ((avctx->coded_width || avctx->coded_height) && av_image_check_size(avctx->coded_width, avctx->coded_height, 0, avctx))
		                                                                                                                 	 return -12;
	return 0;
}
int CRTSPCamera::CheckJFrame(AVPacket &_packet)
{		
	int ret=CheckFrameParams(pCodecCtx,&_packet);
    if (ret < 0)
	{
		std::string err_ms=__FUNCTION__;
		switch(ret)
		{
		case -11:
			err_ms+=", DID 0x%x:CheckFrameParams generate  error; Invalid J - Frame!!! codec_context is NULL";
			break;
		 case -12:
			err_ms+=", DID 0x%x:CheckFrameParams generate  error; Invalid J - Frame!!! av_image_check_size() error";
		 break;
		 default:
		    err_ms+=	", DID 0x%x:CheckFrameParams generate  error;Invalid J - Frame!!!";
		}
		TracerOutputText(this,
			ZLOG_LEVEL_ERROR,
			err_ms.c_str(),
			DecoderID
			);
		return -1;
	}
  	BYTE* pdata=_packet.data;
	int frame_len=_packet.size;
	int pic_height=0;
	int pic_width=0;
	BYTE *tmp = pdata;
	unsigned short end_pict=*((unsigned short*)(pdata+(frame_len-2)));
	if(end_pict!=PICT_END) 
	{
		TracerOutputText(this,
			ZLOG_LEVEL_ERROR,
			__FUNCTION__", DID 0x%x:CheckJFrame() return -1;No marker #0xFFD9.Invalid J - Frame!!!",
			DecoderID
			);
		return -1;
	}
	if(MARKER_JPG!=SOI)
	{
		TracerOutputText(this,
			ZLOG_LEVEL_ERROR,
			__FUNCTION__", DID 0x%x:CheckJFrame() return -1; #SOI(0xD8FF)= 0x%x;Invalid J - Frame!!!",
			DecoderID,
			*(unsigned short*)tmp
			);
		return -1;
	}
	tmp+=2;
	if(MARKER_JPG!=APP0)
	{
		TracerOutputText(this,
			ZLOG_LEVEL_ERROR,
			__FUNCTION__", DID 0x%x:CheckJFrame() return -1; #APP0(0xFEFF)= 0x%x;Invalid J - Frame!!!",
			DecoderID,
			*(unsigned short*)tmp
			);
		return -1;
	}
	tmp+=4;
	if( MARKER_JFIF != JFIF)
	{
		TracerOutputText(this,
			ZLOG_LEVEL_ERROR,
			__FUNCTION__", DID 0x%x:CheckJFrame() return -1; #JFIF(0x4649464A)= 0x%x;Invalid J - Frame!!!",
			DecoderID,
			*(unsigned int*)tmp
			);
		return -1;
	}
	tmp+=4;
	int off=10;
	while(SOFХ)
	{
		if(MARKER_JPG==DQT)
		{
			tmp=JumpOverTable(tmp,off,frame_len);
			if(tmp!=NULL)continue;
			TracerOutputText(this,
				ZLOG_LEVEL_ERROR,
				__FUNCTION__", DID 0x%x:CheckJFrame() return -1; DQT - invalid len .J - Frame is corrupt !!!",
				DecoderID
				);
			return -1;
		}
		if(MARKER_JPG==DHT)
		{
			tmp=JumpOverTable(tmp,off,frame_len);
			if(tmp!=NULL)continue;
			TracerOutputText(this,
				ZLOG_LEVEL_ERROR,
				__FUNCTION__", DID 0x%x:CheckJFrame() return -1; DQT - invalid len .J - Frame is corrupt !!!",
				DecoderID
				);
			return -1;
		}
		if(tmp==pdata+MAX_HDR_LEN)
		{
			TracerOutputText(this,
				ZLOG_LEVEL_ERROR,
				__FUNCTION__", DID 0x%x:CheckJFrame() return -1;JFIF HDR out of data.J - Frame is corrupt !!!",
				DecoderID
				);
			return -1;
		}
		if(MARKER_JPG==HEAD_END)
		{
			TracerOutputText(this,
				ZLOG_LEVEL_ERROR,
				__FUNCTION__", DID 0x%x:CheckJFrame() return -1;No marker #SOF0.Invalid J - Frame!!!",
				DecoderID
				);
			return -1;
		}
		tmp++;
		off++;
	}
	tmp+=4;
	if(*tmp!=8)
	{
		TracerOutputText(this,
			ZLOG_LEVEL_ERROR,
			__FUNCTION__", DID 0x%x:CheckJFrame() return -1; Inalid presition %d;Invalid J - Frame!!!",
			DecoderID,
			*tmp
			);
		return -1;
	}
	tmp++;
	pic_height=(*tmp)<<8;
	pic_height|=(*++tmp);
	tmp++;
	pic_width=(*tmp)<<8;
	pic_width|=(*++tmp);
	if(pic_width<=0||pic_height<=0)
	{
		TracerOutputText(this,
			ZLOG_LEVEL_ERROR,
			__FUNCTION__", DID 0x%x:CheckJFrame() return -1; invalid pic param width=%d,height=%d;Invalid J - Frame!!!",
			DecoderID,
			pic_height,
			pic_width
			);
		return-1;
	}
	//int s_frameFinished=0;
	//int res = (this->*AVDecodeVideo)(pCodecCtx,pFrame,&s_frameFinished,&_packet);
	//if(res<0||!s_frameFinished||pause_stop)
	//{	
    //  TracerOutputText(this,
	//	ZLOG_LEVEL_ERROR,
	//	__FUNCTION__", DID 0x%x: av_decode_video2() return error %d,s_frameFinished is %d ,pause %d;Invalid J - Frame!!!",
	//	DecoderID,res,
	//	s_frameFinished,
	//	pause_stop);
	// return -1;
	//}
	//res=av_frame_get_decode_error_flags(pFrame);
	//if(res!=ERROR_SUCCESS)
	//{
	//	TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__", DID 0x%x: #%d av_frame_get_decode_error_flags() return error %d;Invalid J - Frame!!!",DecoderID,res);
	//	return -1;
	//}
	return 0;
}
bool CRTSPCamera::MJPEGFrameDecode(AVCodecID codec_id,AVPacket &s_packet)
{ 
	if(codec_id!=AV_CODEC_ID_MJPEG)return STATE_ERR;
	int res=0;
	if(s_packet.size>0 && s_packet.data!=NULL && s_packet.flags == AV_PKT_FLAG_KEY)
	{

		try
		{	
			CZcoreVFrame* vframe=NULL;
			res = CheckJFrame(s_packet);
			if(res < 0)
			{
				m_sid++;
				InterlockedExchange((long*)&m_count_corrupt_frame,++m_count_corrupt_frame);
				CheckQualityFrame(s_packet);
				return STATE_OK;
			}

			vframe = zcore_CreateVFrame();
			if(vframe==NULL)
			{
				TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__", DID 0x%x: #%d Create  vframe is NULL!!!",DecoderID);
				return STATE_OK;
			}

			long jpeg_size =s_packet.size;

			long res = zcore_VFrame_CloneJPEGImage(vframe,(long)s_packet.data,jpeg_size,100);
			if(res<0)
			{
				zcore_Destroy(vframe);
				TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__", DID 0x%x: #%d zcore_VFrame_CloneJPEGImage return -1!!!",DecoderID);
				return STATE_OK;
			}

			vframe->image_type = m_type;
			vframe->frame_type=ZCORE_FT_FRAME;

			zcore_SetSystemTimestamp(vframe, &m_cam_ts);

			MQueue_Send(vframe);

			j_fps++;
			duration_T=(clock()- start_DC_tr);
			if(duration_T>=1000)
			{
				TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__", DID 0x%x: #%d  J-frames added in M_Queue, pkt size =%d ",DecoderID,j_fps,s_packet.size);
				start_DC_tr=clock();
				j_fps=0;
			} 
		}
		catch (...)
		{
			TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__", DID 0x%x: #%d Error,BAD vframe value !!!",DecoderID);
			return STATE_OK;
		}

	}
	av_free_packet(&s_packet);
	return STATE_OK;
}
void CRTSPCamera::Clean_Invalid_Im(RawData* rawdata,IplImage *idata)
{
	CleanMemoryIm(idata);
	CleanRawData(rawdata);
}
void CRTSPCamera::Clean_Invalid_Vfr(RawData* rdata,IplImage *data,CZcoreVFrame* _vframe)
{
	zcore_Destroy(_vframe);
	_vframe=NULL;
	Clean_Invalid_Im(rdata,data);
}
void CRTSPCamera::addDecDataInQueue(RawData* raw_data,IplImage *i_data,int tm_delay) 
{ 

	if(i_data==NULL)
	{

		CleanRawData(raw_data);
		TracerOutputText(this,ZLOG_LEVEL_DEBUG,__FUNCTION__" i_data->imageData=NULL "  );
		return;
	}

	int mq_size = mqueue_array.size();
	if(mq_size==0)
	{

		Clean_Invalid_Im(raw_data,i_data);
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__"MQueue was removed!!!,Critical error "  );
		return;
	}
	CZcoreMQueue* mqueue=NULL;
	CZcoreVFrame* vframe = zcore_CreateVFrame();
	if(vframe==NULL)
	{
		Clean_Invalid_Im(raw_data,i_data);
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" zcore_CreateVFrame Error!!!,vframe =NULL " );
		return;
	}
	vframe->image_type = m_type;
	zcore_SetSystemTimestamp(vframe, &m_cam_ts);
	try
	{

		vframe->IPL_handle=xiplCreateImage(XIPL_RGB_IMAGE,i_data->width,i_data->height,i_data->origin);
		if(vframe->IPL_handle==NULL)
		{
			Clean_Invalid_Vfr(raw_data,i_data,vframe);
			TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" vframe->IPL_handle create error "  );
			return;
		}
		long res=xiplCopyImage(i_data,vframe->IPL_handle,0,0); 
		if(res!=0)
		{
			Clean_Invalid_Vfr(raw_data,i_data,vframe);
			TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" xiplCopyImage return  error "  );
			return;
		}
		CleanMemoryIm(i_data);

	}

	catch (...)
	{
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" vframe->IPL_handle exept error "  );
		CleanRawData(raw_data);
		return;
	}

	vframe->id =m_sid++;
	vframe->frame_type = raw_data->frametype;

	MQueue_Send(vframe);

	CleanRawData(raw_data);
	duration_T=(clock()- start_DC_tr);
	h_fps++;	
	if(duration_T>=1000)
	{

		TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__", DID 0x%x: #%d H-frames added in M_Queue ",DecoderID,h_fps);
		start_DC_tr=clock();
		h_fps=0;
	}

}
void CRTSPCamera::Clean_Imfifo(AVPacket &_packet)
{
	CleanMemoryIm(_fifo);
	_fifo=NULL;
	av_free_packet(&_packet);
	Sleep(m_delay);
}
bool CRTSPCamera::H264FrameDecode(AVCodecID codec_id,AVPacket &s_packet)
{	
	if(codec_id!=AV_CODEC_ID_H264)return STATE_ERR;
	int s_frameFinished=0;
	int res = (this->*AVDecodeVideo)(pCodecCtx,pFrame,&s_frameFinished,&s_packet);
	
	if(res<0||!s_frameFinished||pause_stop)
	{	
		av_free_packet(&s_packet);
		Sleep(m_delay);
		return STATE_OK;
	}
	
	_fifo=xiplCreateImage(XIPL_RGB_IMAGE,pFrame->width,pFrame->height,0);  
	if(srv->CheckIfValidFrame(_fifo)!=IPL_StsOk)
	{	 

		Clean_Imfifo(s_packet);
		fprintf(stderr,"CheckIfValidFrame(_fifo)<0\n");
		return STATE_OK;
	}

	if(av2ipl(pFrame,_fifo))
	{

		Clean_Imfifo(s_packet);
		fprintf(stderr,"av2ipl(pFrame,_fifo) error run time\n");
		return STATE_OK;
	}

	//_fifo=Dn_Scale(_fifo,1);

	RawData* rw_data=new RawData;
	rw_data->data=new byte[s_packet.size];
	memcpy(rw_data->data,s_packet.data,s_packet.size);
	rw_data->realtime=m_cam_start_time;
	rw_data->av_codec_type=pCodecCtx->codec_id;
	if(pFrame->interlaced_frame)
	{

		switch(pFrame->top_field_first)
		{
		case 0:
			rw_data->frametype=ZCORE_FT_FIELD_0;
			break;
		case 1:
			rw_data->frametype=ZCORE_FT_FIELD_1;
			break;
		}
	}
	else 
		rw_data->frametype=ZCORE_FT_FRAME;
	rw_data->pause_stop=pause_stop;
	rw_data->frame_dur=(int)m_delay;
	FrameReceived=clock();
	rw_data->delta_time_rcv=delta_time_rcv=FrameReceived-StartReceiveFrame;
	StartReceiveFrame=FrameReceived;
	fParam *fparam=new fParam;
	fparam->delay=(int)m_delay;
	fparam->i_data=_fifo;
	fparam->raw_data=rw_data;
	fparam->obj=this;
	addDecDataInQueue(fparam->raw_data,fparam->i_data,fparam->delay);
	mFrameCount++;
	TracerOutputText(this,ZLOG_LEVEL_DEBUG,__FUNCTION__",m_delay=%d",m_delay);
	av_free_packet(&s_packet);
	return STATE_OK;
}

DWORD CRTSPCamera::StartDecoderThreads()
{   
	TracerOutputText(this,ZLOG_LEVEL_DEBUG,__FUNCTION__",decoder ID 0x%x enter "  ,DecoderID); 
	fprintf(stderr, "RTSP DID=0x%x ready\n",DecoderID);
	_fifo=NULL;
	AVPacket packet;
	hthread_frame_add=NULL;
	mFrameCount = 0;
	int res=STATE_OK;
	start_DC_tr=clock();
	duration_T=0;
	h_fps=0;
	j_fps=0;
	while(m_receive)
	{ 
		if(IsDecoderReady()==STATE_ERR)
		{
			duration_T=(clock()- start_DC_tr);
			if(duration_T>=1000)
			{
				TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__",IsDecoderReady()==STATE_ERR");
				start_DC_tr=clock();
			}
			continue;
		}

		if(mqueue_array.size()==0)
		{
			m_receive=false;
			TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__",decoder ID 0x%x  queue array EMPTY!!! Thread stopped  "  ,this->DecoderID); 
			fprintf(stderr, "RTSP DID=0x%x queue array CRASH!!!! Thread stopped \n",DecoderID);
			continue;
		}

		av_init_packet(&packet);

		res = av_read_frame(pFormatCtx,&packet);

		if( res<0 || link_on==false || (packet.pts < packet.dts) || (packet.flags == AV_PKT_FLAG_CORRUPT) )
		{
			if(res == AVERROR_EOF)
				{
				 InterlockedExchange((LONG*)&link_on,false);
				 SetEvent(ev_DvResetConnect);
			    }
			     else 
					 if (res>=0 && link_on==true)
				                     SetEvent(ev_CtrlAVframe);
			av_free_packet(&packet);
			Sleep(10);
			duration_T=(clock()- start_DC_tr);
			if(duration_T>=1000)
			{
				TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__",res = av_read_frame(); res= %d,link_on=%d",res,link_on);
				start_DC_tr=clock();
			}
			continue;
		}
		if(hthread_frame_add==NULL)
		{	 
			TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__",create thread for add frames in m-queue ");
			hthread_frame_add=(HANDLE)_beginthread(CRTSPCamera::Thread_add_frame, 0,this);  
			SetThreadPriority(hthread_frame_add,THREAD_PRIORITY_ABOVE_NORMAL );
			SetEvent(ev_DvRunning);
			m_fPts=packet.pts;
		}
		SetEvent(ev_CtrlAVframe);
		if(transmit_ready==false)
		{	  
			
			if(IsValidStart_time(pFormatCtx,packet)==STATE_ERR)
			{
				TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__",transmit_ready false ");
				continue;
			}

			if(IsVideoStream(packet)==STATE_ERR)
			{
				TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__",transmit_ready false ");
				continue;
			}

			if(IsFirstPtsSet(packet)==STATE_ERR)
			{
				TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__",transmit_ready false ");
				continue;
			}

			if (packet.pts != AV_NOPTS_VALUE)
			{	 
				m_d_pts=packet.pts-m_fPts;	
				m_d_pts/=90;
				m_fPts=packet.pts;
				if(m_d_pts>m_delay)
				{
					m_count_drop_frame+=(m_d_pts/m_delay);
				} 
			}
			if(IsValidDuration(packet)==STATE_ERR)
			{
				TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__",transmit_ready false ");
				continue;
			}
			TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__",transmit_ready OK ");
			transmit_ready=true;
		}

		BuildTimeStamp();

		if(MJPEGFrameDecode(VideoStream->codec->codec_id,packet)==STATE_OK )
		{
			continue;
		}

		if( H264FrameDecode(VideoStream->codec->codec_id,packet)==STATE_OK)
		{
			continue;
		}

		av_free_packet(&packet);
		Sleep(10);
		duration_T=(clock()- start_DC_tr);
		if(duration_T>=1000)
		{
			TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__",NO VALID  FARAMES!!!");
			start_DC_tr=clock();
		}
	}
	av_frame_free(&pFrame);
	pFrame=NULL;
	m_Monitoring=false;
	if(hthread_frame_add!=NULL)SetEvent(ev_CtrlAVframe);
	return STATE_OK;
}      



//State M-------------in DS_ALLOCATED ; out DS_DISCONNECTED-------------------
//---------------------------------поток приема кадров---------------------------------------

void  __cdecl  CRTSPCamera::Thread_decoder(void* _this)
{   
	if( _this == NULL ) return;

	CRTSPCamera* runDecoder=reinterpret_cast<CRTSPCamera*>(_this);
	TracerOutputText( runDecoder,ZLOG_LEVEL_INFO,__FUNCTION__" start, ID13 0x%x enter "  ,runDecoder->DecoderID); 

	HANDLE process = GetCurrentProcess(); 
	DWORD_PTR processAffinityMask;
	DWORD_PTR systemAffinityMask;
	DWORD_PTR currAffinityMask;
	if(runDecoder->srv->numCPU>1)
	{

		if (GetProcessAffinityMask(process, &processAffinityMask, &systemAffinityMask))
		{
			currAffinityMask=processAffinityMask;
			SetThreadAffinityMask(runDecoder->hDecoderStart, 1); 
		}

	}	
	if(runDecoder->StartReceiveFrame==0)runDecoder->StartReceiveFrame=clock();

	//=====================================assel mode set=======================================
	if(runDecoder->accel_mode)
	{
		switch(runDecoder->accel_mode)
		{
		case 1:
			runDecoder->m_impl = MFX_IMPL_SOFTWARE;
			break;
		case 2:
			runDecoder->m_impl = MFX_IMPL_HARDWARE;
			break;
		}
		runDecoder->CreateQSVDec(deviceHandle);
	}
	//==========================================================================================
	runDecoder->m_Thread_decoder_end=false;
	runDecoder->StartDecoderThreads();

	//==========================================================================================

	if(runDecoder->srv->numCPU>1)
	{
		SetThreadAffinityMask(runDecoder->hDecoderStart,currAffinityMask); 
	}
	InterlockedExchange((LONG*)&runDecoder->link_on,false);
	runDecoder->Camera_Close();
	runDecoder->av_FreeObj();
	TracerOutputText(runDecoder,ZLOG_LEVEL_INFO,__FUNCTION__"decoder ID 0x%x,Thread_decoder END!!!"  ,runDecoder->DecoderID); 
	runDecoder->m_Thread_decoder_end=true;
}
//----------------инициализация  потока "thread_decoder" -------------------------------------

//State M------------in DS_ALLOCATED ; out DS_RUNNING------------
HRESULT CRTSPCamera::Camera_Open()
{
	pause_stop=false;
	if(m_receive)
	{
		Device_Start();
		return STATE_OK;
	}


	inBegining=false;
	InterlockedExchange((LONG*)&m_receive,true);
	hDecoderStart=(HANDLE)_beginthread(CRTSPCamera::Thread_decoder, 0,this); 
	if(hDecoderStart==NULL)
	{
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" Done ERR,Camera down "  );
		return STATE_ERR;
	}
	SetThreadPriority(hDecoderStart,THREAD_PRIORITY_ABOVE_NORMAL); 
	m_device_is_running = true;
	Device_Start();
	TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__" Done OK,Camera running "  );
	return STATE_OK;
}
//--------------------разрушение  потока "thread_decoder" и обьекта pDevice--------------------

// разрушение потоков начинается с установки события  ev_DVStop
// и  далее по цепочке	
// case WAIT_OBJECT_0+3://ev_DvStop->InterlockedExchange((LONG*)&device->m_receive,false)->StartDecoderThreads()->m_Monitoring=false;
// InterlockedExchange((LONG*)&device->m_receive,false)завершает runtime двух потоков- Tread_decoder и Tread_add_frame
// 
// Поток МОЖНО завершить четырьмя способами:(Рихтер)
// 
// функция потока возвращает управление (рекомендуемый способ), используется в этом програмном модуле
// 
// поток самоуничтожяется вызовом функции ExitThread (нежелательный способ);
// один из потоков данного или стороннего процесса вызывает функцию TerminateThread (нежелательный способ);
// завершается процесс, содержащий данный поток (тоже нежелательно). 
// 


bool CRTSPCamera::WaitStateChange(bool &state,int time_out,HANDLE &h_thread)
{
	DWORD ExitCode;
	bool res=GetExitCodeThread(h_thread,&ExitCode);
	if(res &&  ExitCode==STILL_ACTIVE)
	  {
	   TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__",DID 0x%x ,wait for terminate thread...",DecoderID );
	   return WaitForSingleObject(h_thread,time_out);
	  }
	return STATE_OK;
}

void  CRTSPCamera::ThreadsCtrlStop()
{
	
	bool result=WaitStateChange(m_Thread_decoder_end,1000,hDecoderStart);
	if(result)
	{
		TerminateThread(hDecoderStart,STILL_ACTIVE);
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" thread \"Thread_decoder\" emergency stop "  );
		m_receive=false;
		if(m_Monitoring)
			SetEvent(ev_StopMonitor);
	} 
	else 
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" thread \"Thread_decoder\" stop normal "  );
	result=WaitStateChange(m_Thread_add_frame_end,1000,hthread_frame_add);
	if(result)
	{
		TerminateThread(hthread_frame_add,STILL_ACTIVE);
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" thread \"Thread_add_frame\" emergency stop "  );
	} 
	else 
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" thread \"Thread_add_frame\" stop normal "  );

	result=WaitStateChange(m_Thread_Status_Monitor_end,1000,hStatus_Monitor);
	if(result)
	{
		TerminateThread(hStatus_Monitor,STILL_ACTIVE);
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" thread \"Thread_Status_Monitor\" emergency stop "  );
	}
	else 
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" thread \"Thread_Status_Monitor\" stop normal "  );

	hthread_frame_add=NULL;
	hStatus_Monitor=NULL;
	hDecoderStart=NULL;
	TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__",DID 0x%x Done  OK "  ,DecoderID);
} 
//State M-----------in DS_STOPPED ; out DS_DISCONNECTED----------------------

void  CRTSPCamera::DestroyDecoder(void* _this)
{
	CRTSPCamera* dev=(CRTSPCamera*)_this; 
	if(dev->Stopped==false)
	{
		dev->Device_Stop();
		Sleep(1000);
	}
	dev->ThreadsCtrlStop();
	TracerOutputText(dev,ZLOG_LEVEL_INFO,__FUNCTION__",stream ID 0x%x All threads terminated OK " ,dev->DecoderID);
	delete dev;
	dev=NULL;
}
CRTSPCamera::~CRTSPCamera()
{
	if(Stopped==false)
	{
		Device_Stop();
		Sleep(1000);
	}
	m_PartRevision.clear();
	m_SerialNumber = 0;
	m_UniqueId = 0;
	m_ModelName.clear();
	URLCam.clear();
	num_obj--;
	if(num_obj==0)
	{
		avformat_network_deinit();
	}
	delete srv;
	srv=NULL;
	//====================	evDSArray[9] ============
	CloseHandle(ev_CamReady);
	CloseHandle(ev_InvalidLink);
	CloseHandle(ev_DvAllocated);
	CloseHandle(ev_DvDisconnect);
	CloseHandle(ev_DvResetConnect);
	CloseHandle(ev_DvPaused);
	CloseHandle(ev_DvStop);
	CloseHandle(ev_DvRunning);
	CloseHandle(ev_StopMonitor); 
	//===============================================
	CloseHandle(ev_CtrlAVframe);

}
//----------------------------------Services-----------------------------------------

void CRTSPCamera::av_FreeObj()
{

	if (pFormatCtx)
	{
		avformat_close_input(&pFormatCtx);
		avformat_free_context(pFormatCtx);
		pFormatCtx = NULL;
		pCodecCtx=NULL;
		pCodec=NULL;
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" pFormatCtx ");
		fprintf(stderr," free  pFormatCtx \n");
	}

}
void CRTSPCamera::CleanMemoryIm(IplImage* img)
{
	if(img==NULL)return;
	xiplFree(img);
	img=NULL;
}	

IplImage* CRTSPCamera::Dn_Scale(IplImage* img_src,byte scale)
{

	if(scale<=0)scale=1;
	if(scale==1)return img_src;
	int width=img_src->width/scale;
	int height=img_src->height/scale;
	IplImage* _RSrc=cvCreateImage(cvSize(width,height),img_src->depth,img_src->nChannels);
	cvResize(img_src,_RSrc,CV_INTER_LINEAR);
	cvReleaseData(img_src);
	cvReleaseImage(&img_src);
	img_src=NULL;
	return _RSrc;
}	
bool  CRTSPCamera::av2ipl(AVFrame *src, IplImage *dst)
{
	AVPixelFormat pixFormat;
	std::string pxlFmt;
	enum AVPixelFormat pf=(AVPixelFormat)src->format;
	switch (pCodecCtx->pix_fmt) 
	{
	case AV_PIX_FMT_YUVJ420P :
		pixFormat = AV_PIX_FMT_YUV420P;
		pxlFmt = "AV_PIX_FMT_YUV420P";
		break;
	case AV_PIX_FMT_YUVJ422P  :
		pixFormat = AV_PIX_FMT_YUV422P;
		pxlFmt = "AV_PIX_FMT_YUV420P";
		break;
	case AV_PIX_FMT_YUVJ444P   :
		pixFormat = AV_PIX_FMT_YUV444P;
		pxlFmt = "AV_PIX_FMT_YUV420P";
		break;
	case AV_PIX_FMT_YUVJ440P :
		pixFormat = AV_PIX_FMT_YUV440P;
		pxlFmt = "AV_PIX_FMT_YUV420P";
		break;
	default:
		pixFormat=pCodecCtx->pix_fmt;
		pxlFmt = "by pCodecCtx->pix_fmt";
		break;
	}
	struct SwsContext *cntxt =sws_getContext(src->width, src->height,pixFormat,src->width, src->height, PIX_FMT_BGR24,SWS_FAST_BILINEAR, 0, 0, 0);
	if (cntxt == 0)
	{
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" SwsContext exit err , px_fmt=%s " ,pxlFmt);
		return STATE_ERR;
	}
	int linesize[4] = {dst->widthStep, 0, 0, 0 };
	sws_scale(cntxt,src->data,src->linesize,0,src->height,(uint8_t **)&dst->imageData, linesize);
	sws_freeContext(cntxt);
	if(dst->imageData==NULL)
	{
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" imageData==NULL exit err, px_fmt=%s " ,pxlFmt);
		return STATE_ERR;
	}
	TracerOutputText(this,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " );
	return STATE_OK;
}
void CRTSPCamera::SetFrameTimeStamp(SYSTEMTIME* sys_tm,int synhro_time)
{
	static int cv_time=0;
	FILETIME  covn_tm;
	__int64 t_cv;
	SystemTimeToFileTime( sys_tm, &covn_tm );// in 100-nanosecs...

	t_cv = ( ( ( __int64 )covn_tm.dwHighDateTime ) << 32   ) | covn_tm.dwLowDateTime;
	t_cv+=(synhro_time*10000);

	memcpy(&covn_tm, &t_cv, sizeof(covn_tm));
	FileTimeToSystemTime(&covn_tm,sys_tm);
#if 0	
	if(cv_time>=1000)
	{

		cv_time=0;
		SYSTEMTIME sys_ts;
		GetSystemTime(&sys_ts);
		fprintf(stderr,"timestamp on cam 0x%x  time:%04d.%02d.%02d %02d:%02d:%02d:%03d\n",DecoderID,m_cam_ts.wYear,m_cam_ts.wMonth,m_cam_ts.wDay,m_cam_ts.wHour,m_cam_ts.wMinute,m_cam_ts.wSecond,m_cam_ts.wMilliseconds);
		fprintf(stderr,"system timestamp 0x%x  time:%04d.%02d.%02d %02d:%02d:%02d:%03d\n",DecoderID,sys_ts.wYear,sys_ts.wMonth,sys_ts.wDay,sys_ts.wHour,sys_ts.wMinute,sys_ts.wSecond,sys_ts.wMilliseconds);

	}
	cv_time+=duration;
#endif

}
//----------------------------------Параметры инициализации обьекта pDevice-------------
long CRTSPCamera::LoadDevice()
{
	SetEvent(ev_DvDisconnect);
	return STATE_OK;
}
bool CRTSPCamera::SetID(long id)
{
	if ( id<=0 ) return false;
	m_id=id;
	return true;
}
long CRTSPCamera::GetID()
{
	return m_id;
}
void CRTSPCamera::setTimeStampDelta(
	long YearDelta, 
	long MonthDelta, 
	long DayOfWeekDelta, 
	long DayDelta, 
	long HourDelta, 
	long MinuteDelta, 
	long SecondDelta, 
	long MillisecondsDelta
	)
{
	m_YearDelta          = YearDelta        ;
	m_MonthDelta         = MonthDelta       ;
	m_DayOfWeekDelta     = DayOfWeekDelta   ;
	m_DayDelta           = DayDelta         ;
	m_HourDelta          = HourDelta        ;  
	m_MinuteDelta        = MinuteDelta      ;
	m_SecondDelta        = SecondDelta      ;  
	m_MillisecondsDelta  = MillisecondsDelta;
	TracerOutputText(this,ZLOG_LEVEL_DEBUG,"config time %d msek ",m_MillisecondsDelta);
	fprintf(stderr,"CRTSPCamera::setTimeStampDelta time %d msek \n",m_MillisecondsDelta);
	return; 
}
long CRTSPCamera::Device_GetState()
{
	return m_device_state;
}
bool CRTSPCamera::Device_Stop()
{
	Stopped=true;
	TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__",DID 0x%x DONE OK "  ,this); 
	SetEvent(ev_DvStop);
	return true;
}
bool CRTSPCamera::Device_Pause()
{
	TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__",DID 0x%x DONE OK "  ,this);
	SetEvent(ev_DvPaused);
	return true;
}
bool CRTSPCamera::Device_Start()
{
	TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__",DID 0x%x DONE OK "  ,this);
	SetEvent(ev_DvRunning);
	return true;
}
bool CRTSPCamera::SetCameraType(long t)
{
	TracerOutputText(this,ZLOG_LEVEL_DEBUG,__FUNCTION__" Enter "  ,t);
	if ( t<=0 ) 
	{
		TracerOutputText(this,ZLOG_LEVEL_DEBUG,__FUNCTION__" Exit error ,t=%d"  ,t);
		return false;
	}
	m_type=t;
	TracerOutputText(this,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK ,t=%d"  ,t);
	return true;
}
long CRTSPCamera::GetCameraType()
{
	return m_type;
}
void CRTSPCamera::Camera_Close()
{
	TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__",DID 0x%x enter "  ,this->DecoderID); 
	m_device_state=DS_DISCONNECTED;
}
bool CRTSPCamera::SetIPAddrString(char *ipaddr_str)
{
	if (ipaddr_str==NULL) {return false;}

	unsigned long ipaddr_tmp = inet_addr(ipaddr_str);

	if ( ipaddr_tmp==INADDR_NONE || ipaddr_tmp==INADDR_ANY )
	{
		return false;
	}

	m_ipaddr.S_un.S_addr = ipaddr_tmp;
	return true;
}
bool CRTSPCamera::GetIPAddrString(char *ipaddr_str, long max_length)
{
	if (ipaddr_str==NULL) return false;
	if (max_length==0) return false;

	if ( m_ipaddr.S_un.S_addr==INADDR_NONE || m_ipaddr.S_un.S_addr==INADDR_ANY )
	{
		return false;
	}
	char ipaddr_str_tmp[120];
	sprintf(ipaddr_str_tmp, "%u.%u.%u.%u", m_ipaddr.S_un.S_un_b.s_b1, m_ipaddr.S_un.S_un_b.s_b2, m_ipaddr.S_un.S_un_b.s_b3, m_ipaddr.S_un.S_un_b.s_b4 );
	strncpy (ipaddr_str, ipaddr_str_tmp, max_length);
	ipaddr_str[max_length-1]='\0';

	return true;
}
bool CRTSPCamera::SetCameraNameString(char *camera_name)
{
	if (camera_name == NULL) {return false;}

	std::string t_url(camera_name); 
	std::transform(t_url.begin(), t_url.end(), t_url.begin(), ::tolower);
	int ln=t_url.length();
	std::string asin=t_url;
	size_t fRtsp = asin.find("rtsp");
	size_t fHttp = asin.find("http");
	if((fRtsp==std::string::npos || fHttp==std::string::npos) && t_url.length()>0 &&  t_url.length()<=7 && IP_addr.length()!=0)
	{
		URLCam="rtsp://" + IP_addr+ ":554/"+t_url.c_str(); 
		TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__" url = %s" ,t_url.c_str());
		return true;
	}
	else
	{

		char *c_name=new char[ln+1];
		t_url.copy(c_name,ln);
		c_name[ln]='\0';
		char *tmp_c_name=c_name;
		int cn=0;

		while(memcmp(tmp_c_name,"rtsp",4))
		{
			if(memcmp(tmp_c_name,"rtp",3)==0||memcmp(tmp_c_name,"udp",3)==0||memcmp(tmp_c_name,"http",4)==0)break;
			if(cn>ln)
			{
				TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit error,tmp_c_name=%s",tmp_c_name);
				return false;
			}
			tmp_c_name++;
			camera_name++;
			cn++;
		}
		URLCam=camera_name;
		std::string asq="?accel";
		std::string ctrl(camera_name);
		size_t found = ctrl.find(asq);
		if(found!=std::string::npos)
		{ 
			char *t_name=new char[found+1];
			memcpy(t_name,camera_name,found);
			t_name[found]='\0';
			URLCam=t_name;
			delete []t_name;
			int a_mode=atoi(ctrl.substr(found+7,1).c_str());
			SetAccelMode(a_mode);
		} 

		TracerOutputText(this,ZLOG_LEVEL_INFO,__FUNCTION__" Done OK,CmNm %s ",URLCam.c_str());
		delete []c_name;
	}
	return true;
}
bool CRTSPCamera::GetCameraNameString(char *camera_name)
{
	if (camera_name == NULL) return false;
	int ln=URLCam.length();
	URLCam.copy(camera_name,ln);
	camera_name[ln]='\0';
	return true;
}
unsigned long CRTSPCamera::GetCameraName(char* buffer,int len)
{
	if (buffer==NULL||URLCam.length()==0) {return 0;}
	URLCam.copy(buffer,len);
	buffer[len-1]='\0';
	return len;
}
bool CRTSPCamera::SetIPAddr(unsigned long ipaddr)
{
	m_ipaddr.S_un.S_addr = ipaddr;
	return true;
}
unsigned long CRTSPCamera::GetIPAddr()
{
	return m_ipaddr.S_un.S_addr;
}
unsigned long CRTSPCamera::GetPartNumber()
{
	return m_PartNumber;
}
unsigned long CRTSPCamera::GetPartVersion(char* buffer, long buffer_length)
{
	if (buffer==NULL) {return 0;}
	if (buffer_length<=0) {return 0;}

	memset(buffer, 0, buffer_length);

	return strlen(buffer)+1;
}
unsigned long CRTSPCamera::GetPartRevision(char* buffer, long buffer_length)
{
	if (buffer==NULL) {return 0;}
	if (buffer_length<=0) {return 0;}

	memset(buffer, 0, buffer_length);
	strncpy(buffer, m_PartRevision.c_str(),buffer_length);

	return strlen(buffer)+1;
}
unsigned long CRTSPCamera::GetSerialNumber()
{
	return m_SerialNumber;
}
unsigned long CRTSPCamera::GetUniqueId()
{
	return m_UniqueId;
}
unsigned long CRTSPCamera::GetModelName(char* buffer, long buffer_length)
{
	if (buffer==NULL) {return 0;}
	if (buffer_length<=0) {return 0;}

	memset(buffer, 0, buffer_length);
	strncpy(buffer, m_ModelName.c_str(), buffer_length-1);

	return strlen(buffer)+1;
}

bool CRTSPCamera::Device_GetVideoPresence()
{
	return true;
}

bool CRTSPCamera::LockDevice()
{
	return true;
}

bool CRTSPCamera::UnlockDevice()
{
	return true;
}

bool CRTSPCamera::AddMQueue(CZcoreMQueue* mqueue)
{
	if (mqueue==NULL) return false;
	int gt_sz=zcore_MQueue_GetMaxSize(mqueue);
	if (gt_sz>5)
	{				
		fprintf(stderr,"MQueue Error maxsize=%d ,be reinit\n",gt_sz);
		return false;
	}
	fprintf(stderr,"MQueue maxsize=%d \n",gt_sz);
	RemoveMQueue(mqueue);
	mqueue_array.push_back(mqueue);
	return true;
}

bool CRTSPCamera::RemoveMQueue(CZcoreMQueue* mqueue)
{
	bool res=false;
	int mq_size = mqueue_array.size();
	if (mq_size>0)
	{
		std::vector<CZcoreMQueue*>::iterator i_fElQueue;
		i_fElQueue=find(mqueue_array.begin(),mqueue_array.end(),mqueue);
		if(i_fElQueue!=mqueue_array.end())mqueue_array.erase(i_fElQueue);
		res=true;
	}
	return res;
}

bool CRTSPCamera::SetLoggerContext(ZLOG_CONTEXT logger)
{
	if(srv!=NULL)
		srv->m_logger=logger;
	else  
		srv->m_logger=NULL;
	return true;
}

ZLOG_CONTEXT CRTSPCamera::GetLoggerContext()
{
	return 0;
}

__int64 CRTSPCamera::GetCountDropedFrame()
{
	return m_count_drop_frame;
}
void CRTSPCamera::ResetCounterDropFrames()
{
 InterlockedExchange((long*)&m_count_drop_frame,0);
}

long CRTSPCamera::GetCountCorruptFrame()
{
  return m_count_corrupt_frame;
}
void CRTSPCamera::ResetCounterCorruptFrames()
{
  InterlockedExchange((long*)&m_count_corrupt_frame,0);
}




//=====================================================QSW=============================================================
void CRTSPCamera::InitialAcceleration()
{
	if(QSV_enable)
	{	 
		std::string s_impl;
		mfxIMPL impl;
		m_mfx_session.QueryIMPL(&impl);
		switch(impl)
		{
		case MFX_IMPL_HARDWARE:
			s_impl="MFX_IMPL_HARDWARE";
			break;
		case  MFX_IMPL_HARDWARE_ANY:
			s_impl="MFX_IMPL_HARDWARE_ANY";
			break;
		case  MFX_IMPL_SOFTWARE:
			s_impl="MFX_IMPL_SOFTWARE";
			break;
		default :
			s_impl="MFX_IMPL_UNSUPPORTED";
		}
		fprintf(stderr, "Acceleration mode %s set, camera ID= 0x%x \n",s_impl.c_str(),DecoderID);
	}
	else  fprintf(stderr, "FFMPEG encoder mode set, camera ID= 0x%x \n",DecoderID);
}
HRESULT CRTSPCamera::CreateQSVDec(mfxHDL &dvcHandle)
{

	mfxStatus sts;
	mfxVersion m_ver={{4,1}};
	memset(&m_mfxAllocator, 0, sizeof(mfxFrameAllocator));
	sts=InitializeMfx(m_impl, m_ver, &m_mfx_session, &m_mfxAllocator,dvcHandle);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE,MFX_ERR_UNKNOWN);
	m_mfxDEC=new MFXVideoDECODE(m_mfx_session) ;
	MSDK_CHECK_POINTER(m_mfxDEC, MFX_ERR_MEMORY_ALLOC);
	return STATE_OK;

}
int CRTSPCamera::ff_qsv_codec_id_to_mfx(enum AVCodecID codec_id)
{
	switch (codec_id)
	{
	case AV_CODEC_ID_H264:
		return MFX_CODEC_AVC;

	case AV_CODEC_ID_MPEG1VIDEO:
	case AV_CODEC_ID_MPEG2VIDEO:
		return MFX_CODEC_MPEG2;

	case AV_CODEC_ID_VC1:
		return MFX_CODEC_VC1;
	default:
		break;
	}
	return AVERROR(ENOSYS);
}
int CRTSPCamera::init_video_param(AVCodecContext *avctx, mfxVideoParam &param)
{
	const char *ratecontrol_desc;
	float quant;
	int ret;
	ret = ff_qsv_codec_id_to_mfx(avctx->codec_id);
	if (ret < 0)
		return AVERROR_BUG;
	memset(&param, 0, sizeof(mfxVideoParam));
	param.mfx.CodecId = ret;

	param.mfx.CodecLevel         = avctx->level;
	param.mfx.CodecProfile       = avctx->profile;
	param.mfx.GopPicSize         = FFMAX(0, avctx->gop_size);
	param.mfx.GopRefDist         = FFMAX(-1, avctx->max_b_frames) + 1;
	param.mfx.GopOptFlag         = avctx->flags & AV_CODEC_FLAG_CLOSED_GOP ? MFX_GOP_CLOSED : 0;

	param.mfx.NumSlice           = avctx->slices;
	param.mfx.NumRefFrame        = FFMAX(0, avctx->refs);

	param.ExtParam               = (mfxExtBuffer**)avctx->priv_data;
	param.mfx.BufferSizeInKB     = 25000;

	param.mfx.FrameInfo.FourCC         = MFX_FOURCC_NV12;
	param.mfx.FrameInfo.CropX          = 0;
	param.mfx.FrameInfo.CropY          = 0;
	param.mfx.FrameInfo.CropW          = avctx->width;
	param.mfx.FrameInfo.CropH          = avctx->height;
	param.mfx.FrameInfo.AspectRatioW   = avctx->sample_aspect_ratio.num;
	param.mfx.FrameInfo.AspectRatioH   = avctx->sample_aspect_ratio.den;
	param.mfx.FrameInfo.ChromaFormat   = MFX_CHROMAFORMAT_YUV420;
	param.mfx.FrameInfo.BitDepthLuma   = 8;
	param.mfx.FrameInfo.BitDepthChroma = 8;
	param.mfx.FrameInfo.Width          = FFALIGN(avctx->width, 16);
	param.mfx.Quality = 12;
	if (avctx->flags & AV_CODEC_FLAG_INTERLACED_DCT) 
	{
		param.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_FIELD_TFF;
	}
	else 
	{
		param.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
	}
	param.mfx.FrameInfo.Height    = FFALIGN(avctx->height,16);

	if (avctx->framerate.den > 0 && avctx->framerate.num > 0) 
	{
		param.mfx.FrameInfo.FrameRateExtN = avctx->framerate.num;
		param.mfx.FrameInfo.FrameRateExtD = avctx->framerate.den;
	} 
	else 
	{
		param.mfx.FrameInfo.FrameRateExtN  = avctx->time_base.den;
		param.mfx.FrameInfo.FrameRateExtD  = avctx->time_base.num;
	}

	if (avctx->flags & AV_CODEC_FLAG_QSCALE) 
	{
		param.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
		ratecontrol_desc = "constant quantization parameter (CQP)";
	} else 
		if (avctx->rc_max_rate == avctx->bit_rate) 
		{
			param.mfx.RateControlMethod = MFX_RATECONTROL_CBR;
			ratecontrol_desc = "constant bitrate (CBR)";
		} else 
			if (!avctx->rc_max_rate) 
			{
				param.mfx.RateControlMethod = MFX_RATECONTROL_AVBR;
				ratecontrol_desc = "average variable bitrate (AVBR)";
			} else 
			{
				param.mfx.RateControlMethod = MFX_RATECONTROL_VBR;
				ratecontrol_desc = "variable bitrate (VBR)";
			}
			av_log(avctx, AV_LOG_VERBOSE, "Using the %s ratecontrol method\n", ratecontrol_desc);
			switch (param.mfx.RateControlMethod)
			{
			case MFX_RATECONTROL_CBR:
			case MFX_RATECONTROL_VBR:
				param.mfx.InitialDelayInKB = avctx->rc_initial_buffer_occupancy / 1000;
				param.mfx.TargetKbps       = avctx->bit_rate / 1000;
				param.mfx.MaxKbps          = avctx->rc_max_rate / 1000;
				break;
			case MFX_RATECONTROL_CQP:
				quant = (float)avctx->global_quality / FF_QP2LAMBDA;

				param.mfx.QPI = av_clip(int(quant * fabs(avctx->i_quant_factor) + avctx->i_quant_offset), 0, 51);
				param.mfx.QPP = av_clip((int)quant, 0, 51);
				param.mfx.QPB = av_clip((int)(quant * fabs(avctx->b_quant_factor) + avctx->b_quant_offset), 0, 51);

				break;
			case MFX_RATECONTROL_AVBR:
				param.mfx.TargetKbps  = avctx->bit_rate / 1000;
				break;
			} 
			param.AsyncDepth=4;
			param.mfx.NumThread=2;
			param.ExtParam = (mfxExtBuffer**)avctx->extradata;	 

			param.IOPattern =  MFX_IOPATTERN_OUT_VIDEO_MEMORY;
			return 0;
}
HRESULT CRTSPCamera::InitQSVDec(AVCodecContext *p_VCodecCtx,AVPacket *pkt)
{	

	if (pkt->size==0)return -1; 
	mfxStatus sts;
	mfxBitstream bs = { { { 0 } } };
	memset(&mfxDecParams, 0, sizeof(mfxDecParams));
	init_video_param(p_VCodecCtx,mfxDecParams);
	bs.ExtParam   = (mfxExtBuffer **)pkt->priv;
	bs.Data       = pkt->data;
	bs.DataLength = pkt->size;
	bs.MaxLength  = pkt->size;
	bs.TimeStamp  =	pkt->pts;
	bs.DecodeTimeStamp=pkt->dts;
	bs.DataFlag = MFX_BITSTREAM_COMPLETE_FRAME;
	sts = m_mfxDEC->DecodeHeader(&bs, &mfxDecParams);
	MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
	if(sts == MFX_ERR_MORE_DATA)
	{
		return MFX_ERR_MORE_DATA;
	}
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	memset(&m_DecRequest, 0, sizeof(mfxFrameAllocRequest));
	sts = m_mfxDEC->QueryIOSurf(&mfxDecParams, &m_DecRequest);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
	m_DecRequest.NumFrameMin = MSDK_MAX(m_DecRequest.NumFrameSuggested, 1);
	m_DecRequest.NumFrameSuggested = m_DecRequest.NumFrameMin+1;
	m_DecRequest.Info = mfxDecParams.mfx.FrameInfo;
	m_DecRequest.Type = MFX_MEMTYPE_EXTERNAL_FRAME;
	m_DecRequest.Type |= MFX_MEMTYPE_FROM_DECODE; 
	m_DecRequest.Type |= MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET; 

	sts = m_mfxAllocator.Alloc(m_mfxAllocator.pthis, &m_DecRequest, &mfxResponse);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	m_nDecSurfNum = mfxResponse.NumFrameActual;
	m_pDecSurfaces = new mfxFrameSurface1*[m_nDecSurfNum];
	MSDK_CHECK_POINTER(m_pDecSurfaces, MFX_ERR_MEMORY_ALLOC);

	for (int i = 0; i < m_nDecSurfNum; i++) 
	{
		m_pDecSurfaces[i] = new mfxFrameSurface1;
		MSDK_CHECK_POINTER(m_pDecSurfaces[i], MFX_ERR_MEMORY_ALLOC);
		memset(m_pDecSurfaces[i], 0, sizeof(mfxFrameSurface1));
		memcpy(&(m_pDecSurfaces[i]->Info), &(mfxDecParams.mfx.FrameInfo), sizeof(mfxFrameInfo));
		m_pDecSurfaces[i]->Data.MemId = mfxResponse.mids[i]; 

	}

	sts = m_mfxDEC->Init(&mfxDecParams); 
	MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
	int w=mfxDecParams.mfx.FrameInfo.Width;
	int h=mfxDecParams.mfx.FrameInfo.Height;
	int size = avpicture_get_size(p_VCodecCtx->pix_fmt,w ,h);
	uint8_t *picture_buf = (uint8_t *) av_malloc(size);
	MSDK_CHECK_POINTER(picture_buf, MFX_ERR_MEMORY_ALLOC);
	avpicture_fill((AVPicture *)pFrame, picture_buf,p_VCodecCtx->pix_fmt,w ,h);
	pFrame->width=w;
	pFrame->height=w;
	QSV_enable=true;
	return STATE_OK;
}
int CRTSPCamera::AVDecodeVideoMFX(AVCodecContext *mf_ctx, AVFrame *mf_picture,int *mf_got_frame,AVPacket *mf_pkt)
{
	if (mf_pkt->size==0)
		return -1;	
	if(mf_ctx->has_b_frames)
		av_frame_set_pkt_pos(mf_picture, mf_pkt->pos);

	*mf_got_frame=0;
	mfxStatus sts = MFX_ERR_NONE;
	mfxSyncPoint syncp(0); 
	mfxBitstream mfxBS = { { { 0 } } };
	mfxBS.ExtParam   = (mfxExtBuffer **)mf_pkt->priv;
	mfxBS.Data       = mf_pkt->data;
	mfxBS.DataLength = mf_pkt->size;
	mfxBS.MaxLength  = mf_pkt->size;
	mfxBS.DataFlag = MFX_BITSTREAM_COMPLETE_FRAME;
	mfxBS.DecodeTimeStamp  =mf_pkt->dts;
	mfxBS.TimeStamp  = mf_pkt->pts;
	mfxFrameSurface1* pmfxOutSurface = NULL;
	do
	{	

		if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts)
		{
			m_nDecSurfIdx = GetFreeSurfaceIndex(m_pDecSurfaces, m_nDecSurfNum);
			if(MFX_ERR_NOT_FOUND == m_nDecSurfIdx)
			{
				continue;
			}
		}
		if(mf_ctx->has_b_frames)
			m_mfxDEC->SetSkipMode(MFX_SKIPMODE_MORE);
		sts = m_mfxDEC->DecodeFrameAsync(&mfxBS,m_pDecSurfaces[m_nDecSurfIdx], &pmfxOutSurface, &syncp);
		if (MFX_ERR_NONE < sts && !syncp) 
		{ 
			if (MFX_WRN_DEVICE_BUSY == sts)
				MSDK_SLEEP(1); 
		}
		else 
			if (MFX_ERR_NONE <=sts && syncp) 
			{
				sts = MFX_ERR_NONE; 
				break;
			} 
			else
			{

				return -1;
			}
	}
	while (true) ;	

	sts =m_mfx_session.SyncOperation(syncp,QSV_SYNCPOINT_WAIT);
	int tm_wait=0;
	while(sts!=MFX_ERR_NONE)
	{	 
		av_sleep(5);
		sts =m_mfx_session.SyncOperation(syncp,QSV_SYNCPOINT_WAIT); 
		if(tm_wait>400)
		{
			return -1;
		}

		tm_wait++;  
	}
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);	
	sts = m_mfxAllocator.Lock(m_mfxAllocator.pthis, pmfxOutSurface->Data.MemId, &(pmfxOutSurface->Data));
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	UnLoadYUVFrame(pmfxOutSurface,mf_picture);

	sts = m_mfxAllocator.Unlock(m_mfxAllocator.pthis, pmfxOutSurface->Data.MemId, &(pmfxOutSurface->Data));
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
	*mf_got_frame=1;
	return  STATE_OK;
}
int CRTSPCamera::AVDecodeVideoFFM(AVCodecContext *ff_ctx, AVFrame *ff_picture,int *ff_got_frame,AVPacket *ff_pkt)
{
	return avcodec_decode_video2(ff_ctx,ff_picture,ff_got_frame,ff_pkt);
}
long  CRTSPCamera::SetAccelMode(int index)
{
	//0-x264lib,1-i264(sw),2-i264(hw)
	accel_mode=index;
	TracerOutputText(this,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}
long  CRTSPCamera::GetAccelMode()
{

	int index=0;
	TracerOutputText(this,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return index;
}
//====================================================================================================================