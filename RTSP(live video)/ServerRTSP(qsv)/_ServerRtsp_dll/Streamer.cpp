#include "Streamer.h"
#include "h264LvSrvMediaSession.h"
#include "jpegLvSrvMediaSession.h"
#include "izRTSPServer.h"
#include "Validator.h"
#include "izParser.h"
#include "common/include/common_utils.h"
#include <windows.h>

#include <map>
#define DEVICE_MGR_TYPE MFX_HANDLE_DIRECT3D_DEVICE_MANAGER9
#define deliverTimeOut 0
#define A_CONTROL 0
std::string num_srv;
std::stringstream str_srv;
mfxFrameAllocator* mfxAllocator;
int Streamer::num_obj = 0;
extern CvScalar cvColor[];
Streamer::Streamer()
{
	srv_app=NULL;
	StreamID=NULL;
	queue_data=NULL;
	getFrame=NULL;
	rtspServer=NULL;
	video_state=0;
	g_watch=0; 
	Fr_type=ZCORE_MT_NONE;
	sessionName="";
	StreamerErr=STATE_OK;
	wf=false;
	user_login="IZuser";
	user_password="IZpass";
	authenticate_enable=false;
	firstReuseSourceEnable=false;
	imgHeight=0;
	imgWidth=0;
	TracerOutputText(NULL,ZLOG_LEVEL_INFO,"Streamer::Streamer(){Done OK!}" );
}
bool Streamer::Init()
{
	StreamID=(DWORD)this;
	isServerStart=false;
	str_srv<<StreamID;
	str_srv>>num_srv;
	str_srv.clear();
	srv_app=new Service_app();
	if(srv_app==NULL)
	{
		TracerOutputText(NULL,ZLOG_LEVEL_ERROR,__FUNCTION__"Streamer err:Cannot init encoder");
		return STATE_ERR;
	}
	srv_app->Init(StreamID);
	MSDK_CHECK_POINTER_BOOL(srv_app, MFX_ERR_MEMORY_ALLOC);
	srv_app->srvParam.encParam.real_time_prm.m_cycle = 0;
	srv_app->srvParam.encParam.real_time_prm.m_cycleLenght = 0;
	srv_app->srvParam.encParam.real_time_prm.m_roiEnabled = false;
	num_obj++;
	InitializeCriticalSection(&message_queue);
	mfxStatus sts=MFX_ERR_NONE;
	if(num_obj==1)
	{
		time_t lt;
		lt = time(NULL);
		fprintf(stderr,"\n\nServer start at: %s",ctime(&lt));
		av_register_all();
		av_log_set_level(AV_LOG_ERROR);//AV_LOG_QUIET;AV_LOG_PANIC	
		fprintf(stderr, "\n%s\n\n",(srv_app->GetVersionInfo("ProductVersion")).c_str());
	}	
	encoder=new izrsEncoder();
	TracerOutputText(srv_app,ZLOG_LEVEL_INFO,__FUNCTION__",SID 0x%x DONE OK " ,StreamID);
	return STATE_OK;
}

Streamer::~Streamer()
{
	num_obj--;
	if(num_obj==0){}
	if(wf)fclose(dbg_data);
	TracerOutputText(srv_app,ZLOG_LEVEL_INFO,"Streamer::~Streamer(){DONE OK} ");
}

HRESULT Streamer::StartServer(Service_app::srv_param Param)
{   

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__",SID 0x%x enter ",StreamID); 
	ReceivingInterfaceAddr=SendingInterfaceAddr=Param.ipAddr;
	OutPacketBuffer::maxSize =10000000;//Param.encParam.real_time_prm.BR/8;
	TaskScheduler* scheduler= BasicTaskScheduler::createNew();
	BasicUsageEnvironment* env=BasicUsageEnvironment::createNew(*scheduler);
	UserAuthenticationDatabase* authDB = NULL;

#if A_CONTROL
	if(authenticate_enable)
	{
		authDB = new UserAuthenticationDatabase;
		authDB->addUserRecord(user_login.c_str(),user_password.c_str());
	}
#endif

	rtspServer = IZRTSPServer::createNew(this,*env,(portNumBits)Param.port_rtp,authDB,deliverTimeOut);
	h264LvSrvMediaSession* hLvSession=NULL;
	jpegLvSrvMediaSession* jLvSession=NULL;
	if(rtspServer == NULL)
	{
		TracerOutputText(srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x Create server object fault ERROR "     ,StreamID); 
		return STATE_SERVER_ERR;
	}
	str_srv<<StreamID;
	str_srv>>num_srv;
	str_srv.clear();
	sessionName="SessionStream"+num_srv;
	ServerMediaSession* sms = ServerMediaSession::createNew(*env, Param.streamName.c_str(), Param.streamName.c_str(),sessionName.c_str());
    
	bool res=encoder->Initialize(srv_app->srvParam.encParam.codec,(void*)StreamID); 
	if(res==STATE_ERR)
	{
		TracerOutputText(srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__"Streamer err:Cannot init encoder");
		return STATE_ERR;
	} 
	res=encoder->InitEncodeR(srv_app->srvParam.encParam.real_time_prm.FPS,Param.encParam.real_time_prm.BR);
	if(res==STATE_ERR)
	{
		TracerOutputText(srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__"Streamer err:Cannot open  encoder");
		return STATE_ERR;
	}
	switch(srv_app->srvParam.encParam.codec_type)
	{
	case 0:
		h264Enc=encoder;
		if (h264Enc->I264)
		{
			h264Enc->InitialAcceleration();
			TracerOutputText(srv_app,ZLOG_LEVEL_INFO,__FUNCTION__" Start Init Accel");
		}
		hLvSession=h264LvSrvMediaSession::createNew(*env,firstReuseSourceEnable,Param.encParam.real_time_prm.FPS,Param.encParam.real_time_prm.BR,(Streamer*)StreamID);//было true
		if(hLvSession==NULL)
		{	
			env->taskScheduler().deleteEventTrigger(NULL);
			Medium::close(rtspServer);
			isServerStart=false;
			g_watch=0;
			TracerOutputText(srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__"Streamer err:h264LvSrvMediaSession create");
			return STATE_ERR;
		} 
		sms->addSubsession(hLvSession);
		break;
	case 1:
		jpegEnc=encoder;
		jLvSession=jpegLvSrvMediaSession::createNew(*env,firstReuseSourceEnable,srv_app->srvParam.encParam.real_time_prm.FPS,Param.encParam.real_time_prm.BR,(Streamer*)StreamID);//было true
		if(jLvSession==NULL)
		{	
			env->taskScheduler().deleteEventTrigger(NULL);
			Medium::close(rtspServer);
			isServerStart=false;
			g_watch=0;
			TracerOutputText(srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__"Streamer err:j264LvSrvMediaSession create");
			return STATE_ERR;
		} 
		sms->addSubsession(jLvSession);
		break;
	}

	rtspServer->addServerMediaSession(sms);
	std::string message;
	if (rtspServer->setUpTunnelingOverHTTP((portNumBits)Param.port_rtsp))
	{
		isServerStart=true;
		message=rtspServer->rtspURL(sms);
		TracerOutputText(srv_app,ZLOG_LEVEL_INFO,__FUNCTION__" SID 0x%x.RTSP over HTTP tunneling OK. URL streaming onDemand %s ", StreamID,message.c_str()); 
		fprintf(stderr, "\n\nRTSP streamer ready, ID= 0x%x ,%s\n",StreamID,rtspServer->rtspURL(sms));

		env->taskScheduler().doEventLoop(&g_watch);
	}
	else 
	{
		message="\nRTSP-over-HTTP tunneling is not available\n";
		TracerOutputText(srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__" SID 0x%x RTSP over HTTP tunneling Error %s \n"  , StreamID,message.c_str()); 
	}
	Medium::close(rtspServer);

	switch(srv_app->srvParam.encParam.codec_type)
	{
	case 0:
		h264Enc=NULL;
		break;
	case 1:
		jpegEnc=NULL;
		break;
	}
	TracerOutputText(srv_app,ZLOG_LEVEL_INFO,__FUNCTION__",SID 0x%x DONE OK " ,StreamID); 
	srv_app->OverlayLabels.clear();
	isServerStart=false;
	mQueueStart=false;
	g_watch=0;
	delete encoder;
	encoder=NULL;
	rtspServer=(IZRTSPServer*)INVALID_HANDLE_VALUE;
	TracerOutputText(srv_app,ZLOG_LEVEL_INFO,__FUNCTION__",SID 0x%x thread \"Thread_server\" stop normal" ,StreamID); 
	srv_app->OverlayLabels.clear();
	return STATE_OK;
}

void  __cdecl  Streamer::Thread_server(void* _this)
{   

	if( _this == NULL ) return;
	Streamer* streamer=reinterpret_cast<Streamer*>(_this);
	TracerOutputText(streamer->srv_app,ZLOG_LEVEL_INFO,__FUNCTION__" start,SID 0x%x",streamer->StreamID); 
	if(streamer->srv_app->numCPU>1)
	{
		TracerOutputText(streamer->srv_app,ZLOG_LEVEL_INFO,__FUNCTION__" # core = %d",streamer->srv_app->numCPU); 
		HANDLE process =GetCurrentProcess(); 
		DWORD_PTR processAffinityMask;
		DWORD_PTR systemAffinityMask;
		if (GetProcessAffinityMask(process, &processAffinityMask, &systemAffinityMask))
		{
			streamer->currAffinityMask=processAffinityMask;
			DWORD_PTR  success = SetThreadAffinityMask(streamer->hServerStart,1); 
			TracerOutputText(streamer->srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" SetThreadAffinityMask = %d , result= %d , ID=0x%x",processAffinityMask,success,streamer->StreamID);
		}
	}	
	HRESULT res=streamer->StartServer(streamer->srv_app->srvParam);
	if(res)
	{
		if(res!=STATE_SERVER_ERR)
			streamer->StopServer();
		TracerOutputText(streamer->srv_app,ZLOG_LEVEL_ERROR,"Initialize  server RTSP ERROR");
		fprintf(stderr,"Initialize  server RTSP ERROR  \n");
		return;
	}
	if(streamer->srv_app->numCPU>1)
	{
		DWORD_PTR  success =SetThreadAffinityMask(streamer->hServerStart,streamer->currAffinityMask); 
		TracerOutputText(streamer->srv_app,ZLOG_LEVEL_DEBUG," Restore AffinityMask= %d , result= %d , ID=0x%x",streamer->currAffinityMask,success,streamer->StreamID);
	} 
	streamer->srv_app->CleanMemoryIm(streamer->_pause_black);
	TracerOutputText(streamer->srv_app,ZLOG_LEVEL_INFO,__FUNCTION__",SID 0x%x DONE OK "     ,streamer->StreamID);
}
DWORD CALLBACK Streamer::WaitEndServerThread(PVOID pvContext)
{ 
 Streamer* streamer=reinterpret_cast<Streamer*>(pvContext);
 if(streamer->rtspServer==NULL)return 0; 

 int cn=0;
 while(streamer->g_watch!=0)
 {
  Sleep(10);
  cn++;
  if(cn>2000)
	  {
	   TracerOutputText(streamer->srv_app,ZLOG_LEVEL_INFO,"Err End,SID 0x%x",streamer->StreamID);
	   return 0;
      }
 }
 TracerOutputText(streamer->srv_app,ZLOG_LEVEL_INFO,__FUNCTION__",SID 0x%x Wait End %d (sec.) Ok",streamer->StreamID,cn);
 return 0;
}
void Streamer::StopServer()
{
	if(rtspServer==NULL)
	{
		mQueueStart=false;
		TracerOutputText(srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",rtspServer==NULL ");
		return;
	}
    g_watch=1;
	if(isServerStart)
	{
		rtspServer->deleteServerMediaSession(sessionName.c_str());
		rtspServer->envir().reclaim();
		isServerStart=false;
	}
	AbortTransmit=true;
    QueueUserWorkItem(WaitEndServerThread,this,WT_EXECUTELONGFUNCTION);
	bool res=srv_app->WaitStateThread(hmqueue_start,1000);
	if(res)
		{
			TerminateThread(hmqueue_start,STILL_ACTIVE);
			TracerOutputText(srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x thread \"Thread_mqueue\" emergency stop ",StreamID);
		}
		else
		    TracerOutputText(srv_app,ZLOG_LEVEL_INFO,__FUNCTION__",SID 0x%x thread \"Thread_mqueue\" stop normal" ,StreamID); 
   
}

void CALLBACK Streamer::CountFPS(PVOID lpParam, BOOL TimerOrWaitFired)
{
	Streamer* real_Streamer=reinterpret_cast<Streamer*>(lpParam);
	if(!real_Streamer)
	{
		TracerOutputText(NULL,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x ERR ",real_Streamer->StreamID); 
		return;
	}
	TracerOutputText(real_Streamer->srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__",SID 0x%x enter "     ,real_Streamer->StreamID); 
	if(real_Streamer->StreamerErr==STATE_ERR)
	{
		TracerOutputText(real_Streamer->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x Frame dropped,bad image handle ",real_Streamer->StreamID);
		return;
	}
	int r_fps=real_Streamer->srv_app->calcFPS;
	double fps=((float)r_fps)/(CALC_FRAME/1000.0);
	double odd=fps - (int)fps;
	if (odd>0)fps++;
	real_Streamer->srv_app->calcFPS=(int)fps;
	int &calcFPS = real_Streamer->srv_app->calcFPS;

	if(calcFPS > 12 && calcFPS < 16) calcFPS=15;
	else 
		if(calcFPS > 15 && calcFPS < 26) calcFPS=25;
		else 
			if(calcFPS > 25 && calcFPS < 31) calcFPS=30;  //

	TracerOutputText(real_Streamer->srv_app,ZLOG_LEVEL_INFO,__FUNCTION__",SID 0x%x ,real number frames(3s)=%d,calculated FPS=%d ." ,real_Streamer->StreamID,r_fps,calcFPS);
	real_Streamer->srv_app->srvParam.encParam.real_time_prm.FPS=(real_Streamer->srv_app->calcFPS/real_Streamer->srv_app->srvParam.encParam.real_time_prm.DvFc);
	if(real_Streamer->srv_app->srvParam.encParam.real_time_prm.FPS==0)
	{
		if(real_Streamer->mQueueStart)return;
		TracerOutputText(real_Streamer->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x ,FPS ERROR (0)",real_Streamer->StreamID);
		real_Streamer->StartTimer();
		return ;
	}
	if(real_Streamer->srv_app->srvParam.encParam.real_time_prm.FPS<5)
		       real_Streamer->srv_app->srvParam.encParam.real_time_prm.FPS=5;
	real_Streamer->latensy=1000/real_Streamer->srv_app->srvParam.encParam.real_time_prm.FPS;   
	if(real_Streamer->latensy<15)real_Streamer->latensy=33;
	if(real_Streamer->latensy>500)
	{
		real_Streamer->latensy=200;
		real_Streamer->srv_app->srvParam.encParam.real_time_prm.FPS=5;
	}
	real_Streamer->frame_tm_out=real_Streamer->frame_tm_out + real_Streamer->latensy*0.75;
	std::vector<Service_app::FullOverLayLabelParam*>::iterator it;
	real_Streamer->imgHeight=real_Streamer->srv_app->srvParam.encParam.real_time_prm.Height;
	real_Streamer->imgWidth=real_Streamer->srv_app->srvParam.encParam.real_time_prm.Width;
	real_Streamer->imgDepth=real_Streamer->srv_app->srvParam.encParam.real_time_prm.Depth;
	real_Streamer->_pause_black=cvCreateImage(cvSize(real_Streamer->imgWidth,real_Streamer->imgHeight),8,3);
	cvSet(real_Streamer->_pause_black,cvScalar(0,0,0));

	for(it=real_Streamer->srv_app->OverlayLabels.begin();it!=real_Streamer->srv_app->OverlayLabels.end();it++)
	{
		Service_app::FullOverLayLabelParam  *ObjLabel=*it;
		ObjLabel->label.CalcObgRect.left=(long)((ObjLabel->label.ObgRect.X* real_Streamer->imgWidth)/100.0);
		ObjLabel->label.CalcObgRect.left=(long)(ObjLabel->label.CalcObgRect.left/real_Streamer->srv_app->srvParam.encParam.real_time_prm.res_scale);
		ObjLabel->label.CalcObgRect.top= (long)((ObjLabel->label.ObgRect.Y* real_Streamer->imgHeight)/100.0);
		ObjLabel->label.CalcObgRect.top= (long)(ObjLabel->label.CalcObgRect.top/real_Streamer->srv_app->srvParam.encParam.real_time_prm.res_scale);
		ObjLabel->label.CalcObgRect.right=(long)((ObjLabel->label.ObgRect.Width* real_Streamer->imgWidth)/100.0);
		ObjLabel->label.CalcObgRect.right=(long)(ObjLabel->label.CalcObgRect.right/real_Streamer->srv_app->srvParam.encParam.real_time_prm.res_scale);
		ObjLabel->label.CalcObgRect.bottom=(long)((ObjLabel->label.ObgRect.Height* real_Streamer->imgHeight)/100.0);
		ObjLabel->label.CalcObgRect.bottom=(long)(ObjLabel->label.CalcObgRect.bottom/real_Streamer->srv_app->srvParam.encParam.real_time_prm.res_scale);
		if(ObjLabel->label.DataType==1)
		{
			IplImage* _RSrc=cvCreateImage(cvSize(ObjLabel->label.CalcObgRect.right,ObjLabel->label.CalcObgRect.bottom),ObjLabel->label.IplDataImage->depth, ObjLabel->label.IplDataImage->nChannels );
			cvResize(ObjLabel->label.IplDataImage,_RSrc,CV_INTER_LINEAR);
			real_Streamer->srv_app->CleanMemoryIm(ObjLabel->label.IplDataImage);
			ObjLabel->label.IplDataImage=_RSrc;
		}
		else
			if(ObjLabel->label.DataType!=1)
			{		 
				ObjLabel->font_scaleH=(double)real_Streamer->imgHeight/576.0;
				ObjLabel->font_scaleW=(double)real_Streamer->imgWidth/720.0;
				real_Streamer->srv_app->initOvrFont(ObjLabel);
			}


	} 
	real_Streamer->hServerStart=(HANDLE)_beginthread(Streamer::Thread_server, 0,real_Streamer);  
	if(real_Streamer->hServerStart!=NULL)
	{
		SetThreadPriority(real_Streamer->hServerStart,THREAD_PRIORITY_ABOVE_NORMAL ); 
		DeleteTimerQueueTimer(real_Streamer->srv_app->hTimerQueue, real_Streamer->srv_app->hTimer, NULL);
		DeleteTimerQueue(real_Streamer->srv_app->hTimerQueue);
		real_Streamer->StreamerErr=STATE_OK;
		TracerOutputText(real_Streamer->srv_app,ZLOG_LEVEL_INFO,__FUNCTION__",SID 0x%x DONE OK "     ,real_Streamer->StreamID);
		fprintf(stderr,"Streamer::real receive fps %d,\n",calcFPS);
	}
	else
	{
		TracerOutputText(real_Streamer->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__"SID 0x%x , main thread ERR create",real_Streamer->StreamID);
		real_Streamer->StreamerErr=STATE_ERR;
		real_Streamer->StartTimer();
	}

}

void __cdecl  Streamer::Thread_mqueue(void* _this)
{

	Streamer* tr_stream = reinterpret_cast<Streamer*>(_this);
	if( _this == NULL ) return;
	TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_INFO,__FUNCTION__",SID 0x%x enter "     ,tr_stream->StreamID); 
	int qSizePr=0;
	int qSizeCr=0;
	tr_stream->_pause_black=NULL;
	tr_stream->srv_app->img_pause=NULL;
	tr_stream->latensy=510;
	tr_stream->start_EnC_tr=clock();
	tr_stream->duration_T=0;
	tr_stream->h_fps=0;
	tr_stream->j_fps=0;
	tr_stream->p_fps=0;
	tr_stream->frame_tm_out=tr_stream->latensy;
	int size_queue=0;
	int dropp_frames_count=0;
	while(tr_stream->mQueueStart)
	{
		DWORD 	AlertEv=WaitForMultipleObjects(2,tr_stream->hEvent,false,tr_stream->frame_tm_out);
		switch(AlertEv)
		{
		case WAIT_TIMEOUT:
			if(tr_stream->mQueueStart==false)return;
			if(tr_stream->_pause_black==NULL||!tr_stream->isServerStart)

			{
				tr_stream->p_fps++;
				tr_stream->duration_T=(clock()- tr_stream->start_EnC_tr);
				if(tr_stream->duration_T>=1000)
				{
					TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x NO ZCORE_M_QUEU EVENT!!! ,timeout %d(ms)",tr_stream->StreamID,tr_stream->p_fps,tr_stream->latensy);
					tr_stream->start_EnC_tr=clock();
					tr_stream->p_fps=0;
				}
				break;
			}

			size_queue=tr_stream->srv_app->frameSending.size();

			if(size_queue>tr_stream->srv_app->calcFPS)
			{
				TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x NO ZCORE_M_QUEU EVENT!!! ,size of out queue = FPS",tr_stream->StreamID);
				break;	
			}
			EnterCriticalSection(&tr_stream->message_queue);
			IplImage* pause_im;
			SYSTEMTIME cam_ts;
			GetSystemTime(&cam_ts);
			try
			{ 
				pause_im=cvCloneImage(tr_stream->_pause_black);
			}
			catch (...)
			{
				TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x SET %d COPY FRAMES !!!throw,timeout %d(ms)",tr_stream->StreamID,tr_stream->p_fps,tr_stream->latensy);
				LeaveCriticalSection(&tr_stream->message_queue);
				break;
			}


			tr_stream->srv_app->frameSending.push_back(pause_im);
			tr_stream->srv_app->frameSendingID.push_back(ZCORE_FT_FRAME);
			tr_stream->srv_app->frameSendingTS.push_back(cam_ts);
			LeaveCriticalSection(&tr_stream->message_queue);
			tr_stream->p_fps++;
			tr_stream->duration_T=(clock()- tr_stream->start_EnC_tr);
			if(tr_stream->duration_T>=1000)
			{
				TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x SET %d COPY FRAMES,timeout %d(ms)",tr_stream->StreamID,tr_stream->p_fps,tr_stream->latensy);
				tr_stream->start_EnC_tr=clock();
				tr_stream->p_fps=0;
			}
			break;
		case WAIT_OBJECT_0:
			if(tr_stream->srv_app->isTransmit==false && tr_stream->srv_app->isFirstSecond  && !tr_stream-> isServerStart)
			{
				tr_stream->srv_app->calcFPS++;
			}
			if(zcore_MQueue_GetSize(tr_stream->srv_queue))
			{   
				tr_stream->queue_data=NULL;
				long fr_type=ZCORE_MT_NONE;
				DWORD err=zcore_MQueue_GetFirst(tr_stream->srv_queue, &fr_type,&tr_stream->queue_data);
				if(err!=STATE_OK)
				{
					if(tr_stream->queue_data!=NULL)zcore_Destroy(tr_stream->queue_data);
					TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x in ERROR continue"     ,tr_stream->StreamID); 
					continue;
				}
				if(tr_stream->queue_data==NULL)
				{
					TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x (CZcoreBase*)queue_data=NULL "     ,tr_stream->StreamID); 
					continue;
				}
				if(tr_stream->srv_app->isTransmit==false)
				{
					if(fr_type==ZCORE_MT_SDATA)
					{   

						tr_stream->v_frame=(CZcoreVFrame*)tr_stream->queue_data;
						tr_stream->getFrame=zcore_VFrame_GetIplImage(tr_stream->v_frame);

						if(!tr_stream->getFrame)
						{
							TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x (IplImage)getFrame=NULL "     ,tr_stream->StreamID); 
							zcore_Destroy(tr_stream->queue_data);
							av_sleep(750);
							tr_stream->StreamerErr=STATE_ERR;
							continue;
						}
						tr_stream->StreamerErr=STATE_OK;
						tr_stream->Fr_type=zcore_VFrame_GetFrameType(tr_stream->v_frame);
						if(tr_stream->srv_app->isFirstSecond==false&&tr_stream->AbortTransmit==false)
						{

							tr_stream->srv_app->calcFPS=0;
							tr_stream->srv_app->hTimerQueue=CreateTimerQueue();
							tr_stream->StartTimer();
							tr_stream->srv_app->isFirstSecond=true;
							str_srv<<tr_stream->Fr_type;
							str_srv>>num_srv;
							str_srv.clear();
							tr_stream->srv_app->srvParam.encParam.real_time_prm.Height=(int)(tr_stream->getFrame->height *tr_stream->srv_app->srvParam.encParam.real_time_prm.res_scale);
							tr_stream->srv_app->srvParam.encParam.real_time_prm.Width=(int)(tr_stream->getFrame->width *tr_stream->srv_app->srvParam.encParam.real_time_prm.res_scale);
							tr_stream->srv_app->srvParam.encParam.real_time_prm.Depth=(int)(tr_stream->getFrame->depth);
							if((tr_stream->srv_app->srvParam.encParam.real_time_prm.Height%2)!=0)
								tr_stream->srv_app->srvParam.encParam.real_time_prm.Height++;
							if((tr_stream->srv_app->srvParam.encParam.real_time_prm.Width%2)!=0)
								tr_stream->srv_app->srvParam.encParam.real_time_prm.Width++;
							if(tr_stream->Fr_type==0||tr_stream->Fr_type==1)
							{
								tr_stream->srv_app->srvParam.encParam.real_time_prm.Width/=2;
							}
						}  
						//if(tr_stream->srv_app->isFirstSecond  && !tr_stream-> isServerStart)
						//{
						//	tr_stream->srv_app->calcFPS++;
						//	//str_srv<<tr_stream->StreamID;
						//	//str_srv>>num_srv;
						//	//std::string nw="Streamer "+num_srv;
						//	//cvShowImage(nw.c_str(),tr_stream->getFrame);
						//	//cvWaitKey(1);
						//}

					} 

					zcore_Destroy(tr_stream->queue_data);
					str_srv.clear();
					continue;
				}
				if(tr_stream->StreamerErr)
				{
					tr_stream->srv_app->isFirstSecond=false;
					zcore_Destroy(tr_stream->queue_data);
					tr_stream->StreamerErr=0;
					continue;
				}
				if(fr_type==ZCORE_MT_SDATA)
				{
					size_queue=tr_stream->srv_app->frameSending.size();
					if(size_queue>0)
					{
						tr_stream->duration_T=(clock()- tr_stream->start_EnC_tr);
						if(tr_stream->duration_T>=1000)
						{

							TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x frames dropped %d",tr_stream->StreamID,dropp_frames_count);
							tr_stream->start_EnC_tr=clock();
							dropp_frames_count=0;
						}
						else 
							dropp_frames_count+=size_queue;
						zcore_Destroy(tr_stream->queue_data);	
						continue;
					}
					if(zcore_GetTimestamp(tr_stream->queue_data,&tr_stream->ts))
					{
						TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x DONE ERROR "     ,tr_stream->StreamID); 
						zcore_Destroy(tr_stream->queue_data);
						continue;
					}
					if(tr_stream->srv_app->isPaused)
					{
						zcore_Destroy(tr_stream->queue_data);
						continue;
					}
					if((tr_stream->srv_app->countRframes%tr_stream->srv_app->srvParam.encParam.real_time_prm.DvFc)!=STATE_OK)
					{
						zcore_Destroy(tr_stream->queue_data);
						tr_stream->srv_app->countRframes++;
						continue;
					}
					tr_stream->v_frame=(CZcoreVFrame*)tr_stream->queue_data;
					if(tr_stream->v_frame==NULL)
					{
						TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x CZcoreVFrame=NULL,ERROR "     ,tr_stream->StreamID);
						continue;
					}
					tr_stream->frId=zcore_VFrame_GetID(tr_stream->v_frame);
					if(tr_stream->frId==0xdddddddd)
					{
						TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x CZcoreVFrame dropped "     ,tr_stream->StreamID); 
						zcore_Destroy(tr_stream->queue_data);
						continue;
					}

					IplImage* Frame=zcore_VFrame_GetIplImage(tr_stream->v_frame);

					if(!Frame)
					{
						TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x (IplImage)Frame=NULL "     ,tr_stream->StreamID);
						zcore_Destroy(tr_stream->queue_data);
						continue;
					}
					if(tr_stream->srv_app->CheckIfValidFrame(Frame))
					{
						zcore_Destroy(tr_stream->queue_data);
						continue;
					}
					tr_stream->nChannelsIn=Frame->nChannels;

					if(tr_stream->IsNeedToSkipCycle())
					{
						zcore_Destroy(tr_stream->queue_data);
						continue;
					}

					try
					{
						if(Frame->width!=tr_stream->imgWidth||Frame->height!=tr_stream->imgHeight||Frame->depth!=tr_stream->imgDepth)
						{
							SYSTEMTIME cam_ts;
							GetSystemTime(&cam_ts);
							CvFont font;
							int x=30;
							int y=tr_stream->imgHeight/2-10;
							EnterCriticalSection(&tr_stream->message_queue);
							IplImage* pause_im=	cvCloneImage(tr_stream->_pause_black);
							cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1.0, 1.0);
							cvPutText( pause_im,"Frame size was change.Restart service", cvPoint(x, y), &font,cvColor[2]);
							tr_stream->srv_app->frameSending.push_back( pause_im);
							tr_stream->srv_app->frameSendingID.push_back(ZCORE_FT_FRAME);
							tr_stream->srv_app->frameSendingTS.push_back(cam_ts);
							LeaveCriticalSection(&tr_stream->message_queue);
							tr_stream->p_fps++;
							tr_stream->duration_T=(clock()- tr_stream->start_EnC_tr);
							if(tr_stream->duration_T>=1000)
							{
								TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%p parameters of frame was change!!!! ",tr_stream->StreamID);
								tr_stream->start_EnC_tr=clock();
								tr_stream->p_fps=0;
							}
							zcore_Destroy(tr_stream->queue_data);
							continue;
						}
						IplImage *_RSrc=cvCreateImage(cvSize(Frame->width,Frame->height),Frame->depth,3);
						if(tr_stream->srv_app->CheckIfValidFrame(_RSrc))
						{
							TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x (IplImage)_RSrc=NULL ",tr_stream->StreamID);
							zcore_Destroy(tr_stream->queue_data);
							continue;
						}

						int flip=0;	
						if(Frame->origin)
							flip= CV_CVTIMG_FLIP;
						if(Frame->nChannels==1)
						{
							IplImage *_rSrc=cvCreateImage(cvSize(Frame->width,Frame->height),Frame->depth,3);
							if(tr_stream->srv_app->CheckIfValidFrame(_rSrc))
							{
								TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x (IplImage)_RSrc=NULL ",tr_stream->StreamID);
								tr_stream->srv_app->CleanMemoryIm(_rSrc);
								zcore_Destroy(tr_stream->queue_data);
								continue;
							}
							cvCvtColor(Frame,_rSrc,CV_GRAY2RGB);
							cvConvertImage(_rSrc , _RSrc ,flip);
							tr_stream->srv_app->CleanMemoryIm(_rSrc);
						}
						else
						{
							cvConvertImage(Frame , _RSrc ,flip );
						}
						tr_stream->getFrame=_RSrc;
						zcore_Destroy(tr_stream->queue_data);
						tr_stream->srv_app->countRframes++;  
						switch(tr_stream->Fr_type)
						{
						case 0://FIELD 0
							tr_stream->getFrame=tr_stream->srv_app->W_UpScale(tr_stream->getFrame,2);
							break;
						case 1://FIELD 1
							tr_stream->getFrame=tr_stream->srv_app->W_UpScale(tr_stream->getFrame,2);
							break;
						case 2://FRAME
							break;
						}
						tr_stream->ApplyROI(_RSrc);
						//cvShowImage("",_RSrc);
						//cvWaitKey(1);
					}

					catch(...)
					{
						TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x UpScale(tr_stream->getFrame,2)call exception, was catch "     ,tr_stream->StreamID);
						continue;
					}

					EnterCriticalSection(&tr_stream->message_queue);

					tr_stream->srv_app->frameSending.push_back(tr_stream->getFrame);
					tr_stream->srv_app->frameSendingID.push_back(tr_stream->frId);
					tr_stream->srv_app->frameSendingTS.push_back(tr_stream->ts);
					if(
						(tr_stream->_pause_black->width!=tr_stream->getFrame->width)||
						(tr_stream->_pause_black->height!=tr_stream->getFrame->height)||
						(tr_stream->_pause_black->depth!=tr_stream->getFrame->depth)
						)
						cvResize(tr_stream->getFrame,tr_stream->_pause_black,CV_INTER_LINEAR);
					else 
						cvCopy(tr_stream->getFrame,tr_stream->_pause_black,NULL);

					LeaveCriticalSection(&tr_stream->message_queue);
				}
			} 

			break;
		case WAIT_OBJECT_0+1:
			tr_stream->srv_app->isPaused=true;
			if(zcore_MQueue_GetSize(tr_stream->srv_queue))
			{   
				tr_stream->queue_data=NULL;
				long fr_type=ZCORE_MT_NONE;
				DWORD err=zcore_MQueue_GetFirst(tr_stream->srv_queue,&fr_type,&tr_stream->queue_data);
				if(tr_stream->queue_data)
				{
					zcore_Destroy(tr_stream->queue_data);
					tr_stream->queue_data = NULL;
				}
			}
			break;
		}


	}
	TracerOutputText(tr_stream->srv_app,ZLOG_LEVEL_INFO,__FUNCTION__",SID 0x%x thread DONE OK "     ,tr_stream->StreamID);
} 

bool Streamer::IsNeedToSkipCycle()
{
	if(srv_app->srvParam.encParam.real_time_prm.m_cycleLenght > 0)
	{
		int needToShow = v_frame->id % srv_app->srvParam.encParam.real_time_prm.m_cycleLenght;
		if(needToShow != srv_app->srvParam.encParam.real_time_prm.m_cycle) 
		{
			return true;
		}
	}

	return false;
}

bool Streamer::ApplyROI(IplImage * image)
{
	try
	{
		if(srv_app->srvParam.encParam.real_time_prm.m_roiEnabled)
		{
			if (
				!(
				srv_app->srvParam.encParam.real_time_prm.m_roi.width >= 0 && 
				srv_app->srvParam.encParam.real_time_prm.m_roi.height >= 0 &&
				srv_app->srvParam.encParam.real_time_prm.m_roi.x < image->width &&
				srv_app->srvParam.encParam.real_time_prm.m_roi.y < image->height && 
				(srv_app->srvParam.encParam.real_time_prm.m_roi.x + srv_app->srvParam.encParam.real_time_prm.m_roi.width) >=(srv_app->srvParam.encParam.real_time_prm.m_roi.width > 0) && 
				(srv_app->srvParam.encParam.real_time_prm.m_roi.y + srv_app->srvParam.encParam.real_time_prm.m_roi.height) >=(srv_app->srvParam.encParam.real_time_prm.m_roi.height > 0)
				)
				)
				return STATE_ERR;
			cvSetImageROI(image, srv_app->srvParam.encParam.real_time_prm.m_roi);
			IplImage *roiImage=cvCreateImage(cvSize(srv_app->srvParam.encParam.real_time_prm.m_roi.width,srv_app->srvParam.encParam.real_time_prm.m_roi.height), image->depth,3);
			cvCopy(image, roiImage, NULL);
			cvResetImageROI(image);
			cvResize(roiImage, image, CV_INTER_LINEAR );
			cvReleaseImage(&roiImage);
			roiImage = NULL;
		}
	}
	catch (...)
	{
		TracerOutputText(srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x call exception, was catch ",StreamID);
		fprintf(stderr,__FUNCTION__" call exception, was catch \n");
	}
	return STATE_OK;
}

//Streamer API
long  Streamer::SetStreamIP(char* ipAddr,long len)
{
	std::string url=ipAddr;
	if(url == "ANY")url = "0.0.0.0";
	else
	{
		Validator validator;
		if(!validator.ValidateIpAddress(url))
		{
			TracerOutputText(srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__" DONE ERROR ,  wrong IP address" ); 
			return STATE_ERR;
		}
	}
	srv_app->srvParam.ipAddr=our_inet_addr(url.c_str());
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " " ipAddr=%d",srv_app->srvParam.ipAddr); 
	return STATE_OK;
}
long  Streamer::GetStreamIP()
{
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return srv_app->srvParam.ipAddr;
}
long  Streamer::SetStreamPort(long port)
{

	srv_app->srvParam.port_rtp=port;
	srv_app->srvParam.port_rtsp=port+1;
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}

long  Streamer::GetStreamPort()
{
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return srv_app->srvParam.port_rtp;
}

long  Streamer::SetStreamName(char* name,long len)
{
	srv_app->srvParam.streamName=name;
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK "     ); 
	return STATE_OK;
}

long  Streamer::GetStreamName(char* name)
{

	memcpy(name,srv_app->srvParam.streamName.c_str(),srv_app->srvParam.streamName.length());
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return srv_app->srvParam.streamName.length();
}

long  Streamer::SetStreamType(long index)
{
	switch(index)
	{
	case 0:srv_app->srvParam.streamType="stream";
		break;
	case 1:srv_app->srvParam.streamType="file";
		break;
	} 
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}

long  Streamer::GetStreamType()
{
	long param=-1;
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	if(srv_app->srvParam.streamType=="stream") param=0;
	else if(srv_app->srvParam.streamType=="file")param=1;
	return param; 
}

Service_app::FullOverLayLabelParam*  Streamer::FindLabel(std::vector<Service_app::FullOverLayLabelParam* > v_layLabels,long f_ID)
{
	std::vector<Service_app::FullOverLayLabelParam*>::iterator it;
	for(it=v_layLabels.begin();it!=v_layLabels.end();it++)
	{
		Service_app::FullOverLayLabelParam  *ObjLabel=*it;
		if(ObjLabel->label.ID==f_ID)return *it;
	} 
	return NULL;
}

bool  Streamer::compareByZOrder(const Service_app::FullOverLayLabelParam* a, const Service_app::FullOverLayLabelParam* b)
{
	return a->label.z_Order < b->label.z_Order;
}

long  Streamer::LoadStreamer()
{
	srv_app->GetNcPU();
	srv_app->isFirstSecond=false;
	g_watch=0;
	mQueueStart=true;
	AbortTransmit=false;
	srv_app->isTransmit=false;
	srv_app->countRframes=0;
	std::vector<Service_app::FullOverLayLabelParam*>::iterator it;
	std::sort(srv_app->OverlayLabels.begin(), srv_app->OverlayLabels.end(),compareByZOrder);
	for(it=srv_app->OverlayLabels.begin();it!=srv_app->OverlayLabels.end();it++)
	{
		Service_app::FullOverLayLabelParam  *ObjLabel=*it;
		ObjLabel->label.begin_show_time=0;
	}
	TracerOutputText(srv_app,ZLOG_LEVEL_INFO,__FUNCTION__" Done OK " ); 
	return STATE_OK;

}

long  Streamer::UnloadStreamer()
{

	StopServer();
	TracerOutputText(srv_app,ZLOG_LEVEL_INFO,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}
long  Streamer::StartStreamer()
{

	srv_app->isPaused=false;
	TracerOutputText(srv_app,ZLOG_LEVEL_INFO,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}

long  Streamer::StopStreamer()
{

	if(hEvent[1]!=0)
		SetEvent(hEvent[1]);
	TracerOutputText(srv_app,ZLOG_LEVEL_INFO,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}

long  Streamer::DeleteStreamer()
{

	if(srv_queue!=NULL)
	{
		g_watch=1;
		zcore_DestroyMQueue(srv_queue);
		srv_queue=NULL;
		DeleteCriticalSection(&message_queue);
	}
	if(mQueueStart)
	{
		StopServer();
	}
	Sleep(40);
	TracerOutputText(srv_app,ZLOG_LEVEL_INFO,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}

//Codec
//Only on create
long  Streamer::SetCodec(int index)//0-"h264",1-"mjpeg"
{

	switch(index)
	{
	case 0:
		srv_app->srvParam.encParam.codec="h264";
		break;
	case 1:
		srv_app->srvParam.encParam.codec="mjpeg";
		break;
	}
	srv_app->srvParam.encParam.codec_type=index;
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}
long  Streamer::GetCodec()
{

	int index=0;
	if(srv_app->srvParam.encParam.codec=="h264")
		index=0;
	else 
		if(srv_app->srvParam.encParam.codec=="mjpeg")index=1;

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return index;
}
long  Streamer::SetAccelMode(int index)
{
	//0-x264lib,1-i264(sw),2-i264(hw)
	switch(index)
	{
	case 0:
		srv_app->srvParam.encParam.accel="x264";
		break;
	case 1:
		srv_app->srvParam.encParam.accel="i264S";
		break;
	case 2:
		srv_app->srvParam.encParam.accel="i264H";
		break;
	}
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK ,mode= %d ",index); 
	return STATE_OK;
}

long  Streamer::GetAccelMode()
{

	int index=0;
	if(srv_app->srvParam.encParam.codec=="x264")
		index=0;
	else 
		if(srv_app->srvParam.encParam.codec=="i264S")index=1;
		else 
			if(srv_app->srvParam.encParam.codec=="i264H")index=2;

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return index;
}
//Only on create
long  Streamer::SetProfile(long profile)         
	//only h264 0- baseline;1 -main; 2 -heigh  See https://en.wikipedia.org/wiki/H.264/MPEG-4_AVC Profiles
{

	int codec_prof=FF_PROFILE_H264_BASELINE;
	switch(profile)
	{
	case 1:
		codec_prof=FF_PROFILE_H264_MAIN;
		break;
	case 2:
		codec_prof=FF_PROFILE_H264_HIGH;
		break;
	default: codec_prof=FF_PROFILE_H264_BASELINE;
	} 
	srv_app->srvParam.encParam.profile=codec_prof;
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK profile=%d",profile); 
	return STATE_OK;
}
long  Streamer::GetProfile()//only h264
{

	int codec_prof=0;
	switch(srv_app->srvParam.encParam.profile)
	{
	case FF_PROFILE_H264_MAIN:
		codec_prof=1;
		break;
	case FF_PROFILE_H264_HIGH:
		codec_prof=2;
		break;
	default: codec_prof=0;
	} 
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " );      
	return codec_prof;
}

//Only on create
long  Streamer::SetProfileLevel(long level)//only h264 20 -2.0 22- 2.1 See https://en.wikipedia.org/wiki/H.264/MPEG-4_AVC Levels
{

	switch(level)
	{
	case 0:
		level=20;
		break;
	case 1:
		level=21;
		break; 
	case 2:
		level=22;
		break;
	case 3:
		level=30;
		break;
	case 4:
		level=31;
		break;
	case 5:
		level=32;
		break;
	case 6:
		level=40;
		break; 
	case 7:
		level=41;
		break;
	case 8:
		level=42;
		break;
	case 9:
		level=50;
		break;
	case 10:
		level=51;
		break;
	}
	srv_app->srvParam.encParam.level=level;
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}
long  Streamer::GetProfileLevel()
{
	int level=0;
	switch(srv_app->srvParam.encParam.level)
	{
	case 20:
		level=0;
		break;
	case 21:
		level=1;
		break; 
	case 30:
		level=2;
		break;
	case 31:
		level=3;
		break;
	case 40:
		level=4;
		break;
	case 41:
		level=5;
		break;
	}
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return level;
}

//Only on create
long  Streamer::SetBitrate(long br)
{
	srv_app->srvParam.encParam.real_time_prm.BR=br;
	int accel_mode=br&0x3;
	SetAccelMode(accel_mode);
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}
long  Streamer::GetBitrate()
{

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return srv_app->srvParam.encParam.real_time_prm.BR;
}
izrs_Resolution Streamer::GetResolution()
{   
	izrs_Resolution res={0};
	res.Height=srv_app->srvParam.encParam.real_time_prm.Height;
	res.Width=srv_app->srvParam.encParam.real_time_prm.Width;
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return res;
}
void Streamer::GetLastEncParam()
{
	if(srv_app->srvParam.encParam.real_time_prm.session_id==0)return;
	Service_app::enc_paramRT *e_rt_param=new Service_app::enc_paramRT;
	memcpy(e_rt_param,&srv_app->srvParam.encParam.real_time_prm,sizeof(Service_app::enc_paramRT));
	v_last_enc_param.push_back(e_rt_param);
}
void Streamer::GetCurrentEncParam()
{
	if(srv_app->srvParam.encParam.real_time_prm.session_id==0)return;
	if(v_last_enc_param.size()==0)return;
	Service_app::enc_paramRT *e_rt_param=new Service_app::enc_paramRT;
	memcpy(e_rt_param,&srv_app->srvParam.encParam.real_time_prm,sizeof(Service_app::enc_paramRT));
	Service_app::enc_paramRT *prev_e_prm=v_last_enc_param.back();
	prev_e_prm->session_id=e_rt_param->session_id;
	v_curr_enc_param.push_back(e_rt_param);
}

//may be change RT
long  Streamer::SetResolutionScaleFactor(long scale)
	//out W & H=in(Width & Height)*scale	0-full x1, 1-half x0.5, 2- quarter x0.25, 3- one-eighth x0.125
{

	switch(scale)
	{
	case 0:
		srv_app->srvParam.encParam.real_time_prm.res_scale=1;
		break;
	case 1:
		srv_app->srvParam.encParam.real_time_prm.res_scale=0.5;
		break;
	case 2:
		srv_app->srvParam.encParam.real_time_prm.res_scale=0.25;   
		break;
	case 3:
		srv_app->srvParam.encParam.real_time_prm.res_scale=0.125;
		break;
	}

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}
long  Streamer::GetResolutionScaleFactor()
{

	int scale=0;
	int cAse=(int)(srv_app->srvParam.encParam.real_time_prm.res_scale*1000);
	switch(cAse)
	{
	case 1000:
		scale=0;
		break;
	case 500:
		scale=1;
		break;
	case 250:
		scale=2;   
		break;
	case 125:
		scale=3;
		break;
	} 
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return scale;
}

long  Streamer::GetFPS()
{
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return srv_app->srvParam.encParam.real_time_prm.FPS;
}

//may be change RT
long  Streamer::SetFPSDivider(long each)
{

	srv_app->srvParam.encParam.real_time_prm.DvFc=each;
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}
long  Streamer::GetFPSDivider()
{

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return srv_app->srvParam.encParam.real_time_prm.DvFc;
}
long  Streamer::GetWidth()
{
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return srv_app->srvParam.encParam.real_time_prm.Width;
}
long  Streamer::GetHeight()
{
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return srv_app->srvParam.encParam.real_time_prm.Height;
}

long  Streamer::DumpToFile(long val)
{

	srv_app->srvParam.dump_to_file = (val > 0);
	return STATE_OK;
}
//Overlay
//may be change RT
long  Streamer::SetOverlayEnabled(long Enabled)
{

	EnterCriticalSection(&message_queue);
	OverlayEnabled = (Enabled > 0);
	LeaveCriticalSection(&message_queue);

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK , enable=%d ,ID=0x%x",Enabled,StreamID); 
	return STATE_OK;
}

long  Streamer::GetOverlayEnabled()
{
	long state=OverlayEnabled;
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return state;
}

void Streamer::SetCycle(long cycle)
{
	srv_app->srvParam.encParam.real_time_prm.m_cycle = cycle;
}

long Streamer::GetCycle()
{
	return srv_app->srvParam.encParam.real_time_prm.m_cycle;
}

void Streamer::SetRoi(CvRect roi)
{
	//m_roi = roi;
	srv_app->srvParam.encParam.real_time_prm.m_roi=roi;
}

CvRect Streamer::GetRoi()
{
	return srv_app->srvParam.encParam.real_time_prm.m_roi;//m_roi ;
}

void Streamer::SetRoiEnabled(bool enabled)
{
	srv_app->srvParam.encParam.real_time_prm.m_roiEnabled = true;
}

bool Streamer::GetRoiEnabled()
{
	return srv_app->srvParam.encParam.real_time_prm.m_roiEnabled;
}

long Streamer::SetCycleLenght(long cycleLenght)
{
	srv_app->srvParam.encParam.real_time_prm.m_cycleLenght = cycleLenght; 
	return STATE_OK;
}

long Streamer::GetCycleLenght()
{
	return srv_app->srvParam.encParam.real_time_prm.m_cycleLenght;
}

//only create stream
long  Streamer::CreateLabel()//create label,add to stream, return ID
{ 
	Service_app::FullOverLayLabelParam* OverLayLabel=new Service_app::FullOverLayLabelParam;
	ZeroMemory(&OverLayLabel->label,sizeof(OverLayLabel->label));
	EnterCriticalSection(&message_queue);
	srv_app->OverlayLabels.push_back(OverLayLabel);
	long IdLb=(long)OverLayLabel;
	OverLayLabel->label.ID=IdLb;
	LeaveCriticalSection(&message_queue);
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return IdLb;
}

//may be change RT
long  Streamer::DeleteLabel(long ID)
{

	std::vector<Service_app::FullOverLayLabelParam*>::iterator it;
	for(it=srv_app->OverlayLabels.begin();it!=srv_app->OverlayLabels.end();it++)
	{
		Service_app::FullOverLayLabelParam  *ObjLabel=*it;
		if(ObjLabel->label.ID==ID)
		{
			EnterCriticalSection(&message_queue);
			delete  ObjLabel;

			srv_app->OverlayLabels.erase(it);
			LeaveCriticalSection(&message_queue);

			TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
			return STATE_OK;
		}
	} 

	TracerOutputText(srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__" Done.Label 0x%p not found " ,ID); 
	return STATE_ERR;
}
//only create stream
long  Streamer::Label_SetDataType(long ID,long type)
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL) return STATE_ERR;
	EnterCriticalSection(&message_queue);
	ObjLabel->label.DataType=type;
	LeaveCriticalSection(&message_queue);

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}

long  Streamer::Label_GetDataType(long ID)
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	long type= STATE_ERR;
	if(ObjLabel)type=ObjLabel->label.DataType;
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return type;
}

//may be change RT
long  Streamer::Label_SetDataText(long ID,char *text,int len)
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL) return STATE_ERR;
	EnterCriticalSection(&message_queue);
	ObjLabel->label.TxtString=new char[len+2];
	memset(ObjLabel->label.TxtString,'\0',len+2);
	memcpy(ObjLabel->label.TxtString,text,len);
	ObjLabel->disableTS = false;
	ObjLabel->label.begin_show_time=clock();//0
	LeaveCriticalSection(&message_queue);
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;

}

char * Streamer::Label_GetDataText(long ID)
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL) return NULL;
	char *txt=ObjLabel->label.TxtString;
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return txt;
}

//may be change RT
long  Streamer::Label_SetDataImage(long ID,long ipl)
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	EnterCriticalSection(&message_queue); 
	srv_app->CleanMemoryIm(ObjLabel->label.IplDataImage);
	try
	{

		ObjLabel->label.IplDataImage=cvCloneImage((IplImage*)ipl);
	}
	catch (...)
	{
		TracerOutputText(srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x THROW!!!",StreamID);
		LeaveCriticalSection(&message_queue);
		return STATE_ERR;
	}
	IplImage *_Src=cvCreateImage(cvGetSize(ObjLabel->label.IplDataImage),ObjLabel->label.IplDataImage->depth,ObjLabel->label.IplDataImage->nChannels);
	cvConvertImage(ObjLabel->label.IplDataImage,_Src ,CV_CVTIMG_FLIP);
	srv_app->CleanMemoryIm(ObjLabel->label.IplDataImage);
	ObjLabel->label.IplDataImage=_Src;
	if(isServerStart)
	{
		IplImage* _RSrc=cvCreateImage(cvSize(ObjLabel->label.CalcObgRect.right,ObjLabel->label.CalcObgRect.bottom),ObjLabel->label.IplDataImage->depth, ObjLabel->label.IplDataImage->nChannels );
		cvResize(ObjLabel->label.IplDataImage,_RSrc,CV_INTER_LINEAR);
		srv_app->CleanMemoryIm(ObjLabel->label.IplDataImage);
		ObjLabel->label.IplDataImage=_RSrc;
		//cvShowImage("Hist",ObjLabel->label.IplDataImage);
		//cvWaitKey(1);
	}
	LeaveCriticalSection(&message_queue);

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}

long  Streamer::Label_GetDataImage(long ID)
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return 0;
	IplImage *ipl=ObjLabel->label.IplDataImage;

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return (long)ipl;
}

//may be change RT
long  Streamer::SetLabelTimestampFormat(long ID,char* format_string)
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;

	EnterCriticalSection(&message_queue);
	ObjLabel->label.LabelTimestampFormat=format_string;
	ObjLabel->label.begin_show_time=0;
	LeaveCriticalSection(&message_queue);

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK "     ); 
	return STATE_OK;
}

char *Streamer::GetLabelTimestampFormat(long ID)//http://www.w3.org/TR/NOTE-datetime 
{

	EnterCriticalSection(&message_queue);
	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return NULL;
	char*  formatStr=(char*)ObjLabel->label.LabelTimestampFormat.c_str();
	LeaveCriticalSection(&message_queue);

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return formatStr;
}

//may be change RT
long  Streamer::Label_SetRECT(long ID,izrs_Rect &rect)
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	EnterCriticalSection(&message_queue);
	memcpy(&ObjLabel->label.ObgRect,&rect,sizeof(rect));
	LeaveCriticalSection(&message_queue);

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}

void* Streamer::Label_GetRECT(long ID)
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return NULL;
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return (void*)&ObjLabel->label.ObgRect;
}

//may be change RT
long  Streamer::Label_SetBackgroundColor(long ID,long index_color)//index 0-14
{
	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	EnterCriticalSection(&message_queue);
	ObjLabel->label.bckColorId=index_color;
	LeaveCriticalSection(&message_queue);
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}

long  Streamer::Label_GetBackgroundColor(long ID)
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	long ind=ObjLabel->label.bckColorId;

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return ind;
}

//may be change RT
long  Streamer::Label_SetBackgroundTransparency(long ID,long transp_val)// [0- solid; 100 - transparent]
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	EnterCriticalSection(&message_queue);
	ObjLabel->label.bckTrnCyVal=transp_val;
	LeaveCriticalSection(&message_queue);

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}

long  Streamer::Label_GetBackgroundTransparency(long ID)
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	long ind=ObjLabel->label.bckTrnCyVal;

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return ind;
}
//may be change RT
long  Streamer::Label_SetFontName(long ID,long ind_fontname)
{
	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	EnterCriticalSection(&message_queue);
	ObjLabel->label.fontId=ind_fontname;
	LeaveCriticalSection(&message_queue);

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}

long  Streamer::Label_GetFontName(long ID)
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	long ind=ObjLabel->label.fontId;

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return ind;
}
//may be change RT
long  Streamer::Label_SetFontSize(long ID, long size) //Ц Font Height % of RECT height;
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	EnterCriticalSection(&message_queue);
	ObjLabel->label.fontHeight=size;
	ObjLabel->label.fontWidth=size;
	LeaveCriticalSection(&message_queue);

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}

long  Streamer::Label_GetFontSize(long ID)
{
	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	int size=ObjLabel->label.fontHeight;
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return size;
}
//may be change RT
long  Streamer::Label_SetFontColor(long ID, long ind_color)
{
	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	EnterCriticalSection(&message_queue);
	ObjLabel->label.fontColorId=ind_color;
	LeaveCriticalSection(&message_queue);
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}

long  Streamer::Label_GetFontColor(long ID, long ind_color)
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	long ind=ObjLabel->label.fontColorId;
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return ind;
}
//may be change RT
long  Streamer::Label_SetTimeout(long ID, long msec) //Ц erase data when timeout occurred       врем€ отображени€ 
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	EnterCriticalSection(&message_queue);
	ObjLabel->label.durationShow=msec;
	LeaveCriticalSection(&message_queue);

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK,duration= %d, " ,msec); 
	return STATE_OK;
}

long  Streamer::Label_GetTimeout(long ID)
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	long msec=ObjLabel->label.durationShow;

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}
//may be change RT
long  Streamer::Label_SetZOrder(long ID, long order)// Ц to resolve overlapping between labels 
{

	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	EnterCriticalSection(&message_queue);
	ObjLabel->label.z_Order=order;
	LeaveCriticalSection(&message_queue);

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;
}

long  Streamer::Label_GetZOrder(long ID)
{
	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	long order=ObjLabel->label.z_Order;
	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return order;
}
//may be change RT
long  Streamer::Label_SetBorder(long ID,boolean show)
{
	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	EnterCriticalSection(&message_queue);
	ObjLabel->label.SetBorder=show;
	LeaveCriticalSection(&message_queue);

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return STATE_OK;  
}

long  Streamer::Label_GetBorder(long ID)
{
	Service_app::FullOverLayLabelParam  *ObjLabel=FindLabel(srv_app->OverlayLabels,ID);
	if(ObjLabel==NULL)return STATE_ERR;
	long show=ObjLabel->label.SetBorder;

	TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK " ); 
	return show;
}

HRESULT Streamer::SetParamByURL(char const * urlQuery)
{
	std::string uQuery = urlQuery;
	std::string paramName;

	IZParser parser;
	std::map<std::string, std::string> parameters = parser.Parse(uQuery);
	bool ext_param=false;
	for(int i=0; i < Service_app::last; i++)
	{
		paramName=srv_app->GetTextByEnum(i);

		std::map<std::string, std::string>::iterator it = parameters.find(paramName);
		if(it == parameters.end())
		{
			continue;
		}
		ext_param=true;
		SetParamFromEnum(i, parameters[paramName]);

	}
	if(ext_param)
	{

	}
	return 0;
}

void Streamer::SetParamFromEnum(int enumVal, std::string &val)
{
	switch( enumVal )
	{
	case Service_app::res_scale:
		{
			int intVal = atoi(val.c_str());
			SetResolutionScaleFactor(intVal);
			break;
		}
	case Service_app::fps_dv:
		{
			int intVal = atoi(val.c_str());
			SetFPSDivider(intVal);
			break;
		}
	case Service_app::overlay:
		{
			int intVal = atoi(val.c_str());
			SetOverlayEnabled(intVal);
			break;
		}
	case Service_app::lb_delete:
		{
			int intVal = atoi(val.c_str());
			DeleteLabel(intVal);
			break;
		}
	case Service_app::cycle:
		{
			int cycle = 0;
			int cycleLenght = 0;

			Validator validator;
			if(validator.ValidateNumbersCommaSeparated(val, 2))
			{
				std::vector<std::string> roiArr;
				roiArr = Streamer::ParseStringSeparetedByChar(val, ',', roiArr); 

				cycle = atoi(roiArr[0].c_str());
				cycleLenght = atoi(roiArr[1].c_str());

				if(cycle < 0)
				{
					cycle = 0;
				}

				if(cycle > cycleLenght)
				{
					cycle = cycleLenght;
				}

				if(cycleLenght < 0)
				{
					cycleLenght = 0;
				}
			}

			SetCycle(cycle);
			SetCycleLenght(cycleLenght);

			break;
		}
	case Service_app::roi:
		{
			Validator validator;
			if(!validator.ValidateNumbersCommaSeparated(val, 4))
			{
				SetRoiEnabled(false);
				break;
			}

			std::vector<std::string> roiArr;
			roiArr = Streamer::ParseStringSeparetedByChar(val, ',', roiArr); 

			CvRect cvRect;
			cvRect.x = atoi(roiArr[0].c_str());
			cvRect.y = atoi(roiArr[1].c_str());
			cvRect.width = atoi(roiArr[2].c_str());
			cvRect.height = atoi(roiArr[3].c_str());

			long width =  GetWidth();
			long height = GetHeight();

			if(cvRect.x < 0)
			{
				cvRect.x = 0;
			}

			if(cvRect.x >= width - cvRect.width)
			{
				cvRect.x = 0; 
			}

			if(cvRect.y < 0)
			{
				cvRect.y = 0;
			}

			if(cvRect.y >= height - cvRect.height)
			{
				cvRect.y = 0; 
			}

			if(cvRect.width < 1)
			{
				cvRect.width = 1;
			}

			if(cvRect.width > width)
			{
				cvRect.width = width;
			}

			if(cvRect.height < 1)
			{
				cvRect.height = 1;
			}

			if(cvRect.height > height)
			{
				cvRect.height = GetHeight();
			}

			SetRoi(cvRect);
			SetRoiEnabled(true);
			break;
		}
	case Service_app::roi_pct:
		{
			Validator validator;
			if(!validator.ValidateNumbersCommaSeparated(val, 4))
			{
				SetRoiEnabled(false);
				break;
			}

			std::vector<std::string> roiArr;
			roiArr = Streamer::ParseStringSeparetedByChar(val, ',', roiArr); 

			long xPct = atoi(roiArr[0].c_str());
			long yPct = atoi(roiArr[1].c_str());
			long widthPct = atoi(roiArr[2].c_str());
			long heightPct = atoi(roiArr[3].c_str());

			if(xPct < 0)
			{
				xPct = 0;
			}

			if(xPct >= 100 - widthPct)
			{
				xPct = 0; 
			}

			if(yPct < 0)
			{
				yPct = 0;
			}

			if(yPct >= 100 - heightPct)
			{
				yPct = 0; 
			}

			if(widthPct < 1)
			{
				widthPct = 1;
			}

			if(widthPct > 100)
			{
				widthPct = 100;
			}

			if(heightPct < 1)
			{
				heightPct = 1;
			}

			if(heightPct > 100)
			{
				heightPct = 100;
			}

			long width =  GetWidth();
			long height = GetHeight();

			CvRect cvRect;
			cvRect.x = xPct * width / 100;
			cvRect.y = yPct * height / 100;
			cvRect.width = widthPct * width / 100;
			cvRect.height = heightPct * height / 100;

			if(cvRect.x < 0)
			{
				cvRect.x = 0;
			}

			if(cvRect.x >= width - cvRect.width)
			{
				cvRect.x = 0; 
			}

			if(cvRect.y < 0)
			{
				cvRect.y = 0;
			}

			if(cvRect.y >= height - cvRect.height)
			{
				cvRect.y = 0; 
			}

			if(cvRect.width < 1)
			{
				cvRect.width = 1;
			}

			if(cvRect.width > width)
			{
				cvRect.width = width;
			}

			if(cvRect.height < 1)
			{
				cvRect.height = 1;
			}

			if(cvRect.height > height)
			{
				cvRect.height = GetHeight();
			}

			SetRoi(cvRect);
			SetRoiEnabled(true);
			break;
		}


	default:
		break;
	}
}

std::vector<std::string> &Streamer::ParseStringSeparetedByChar(const std::string &s, char delim, std::vector<std::string> &elems) 
{
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) 
	{
		elems.push_back(item);
	}
	return elems;
}

long  Streamer::UserAuth_SetLogin(char *login,int len)
{
	user_login=login;
	return 0;
}

char * Streamer::UserAuth_GetLogin()
{
	return (char*)user_login.c_str();
}

long  Streamer::UserAuth_SetPassword(char *password,int len)
{
	user_password=password;
	return 0;
}

char * Streamer::UserAuth_GetPassword()
{
	return (char*)user_password.c_str();
}

long Streamer::UserAuth_SetEnabled(long Enabled)
{ 
	authenticate_enable = (Enabled != false);
	return 0;
}
long Streamer::UserAuth_GetEnabled()
{
	return authenticate_enable;
} 
long Streamer::SetReuseFirstSource(long firstReuseEnable)
{
	firstReuseSourceEnable=static_cast<bool>(firstReuseEnable);
	return 0;
}

long Streamer::SetChangedParam(char const * str_param)
{
	changed_param_str=str_param;
	SetParamByURL(str_param);
	GetCurrentEncParam();
	video_state=1;
	return STATE_OK;
}
char *Streamer::GetChangedParam()
{
	return (char*)changed_param_str.c_str();
}
void Streamer::SetLastPicSetting()
{
	if(v_last_enc_param.size())
	{

		Service_app::enc_paramRT *prev_e_prm=v_last_enc_param.back();
		Service_app::enc_paramRT *curr_e_prm=v_curr_enc_param.back();
		v_last_enc_param.erase(v_last_enc_param.end()-1);
		v_curr_enc_param.erase(v_curr_enc_param.end()-1);
		int  res= memcmp(prev_e_prm,curr_e_prm,sizeof(Service_app::enc_paramRT));
		if(res != 0)
		{	
			TracerOutputText(srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" Stream parameters was changed  " ); 
			memcpy(&srv_app->srvParam.encParam.real_time_prm,prev_e_prm,sizeof(Service_app::enc_paramRT));
		}
		delete prev_e_prm;
		delete curr_e_prm;
	}
	video_state=1;
}
bool Streamer::SetLoggerContext(ZLOG_CONTEXT logger)
{
	if(srv_app!=NULL)
		srv_app->m_logger=logger;
	else  
		srv_app->m_logger=NULL;
	return true;
}

ZLOG_CONTEXT Streamer::GetLoggerContext()
{
	return srv_app->m_logger;
}