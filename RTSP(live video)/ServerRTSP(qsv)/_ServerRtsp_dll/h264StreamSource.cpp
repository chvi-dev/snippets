#include "h264StreamSource.h"
#include "Streamer.h"
#include <stdio.h>
#include <iostream>
extern CvScalar cvColor[];
std::string num;
std::stringstream str_str;
h264FrameSource* h264FrameSource::createNew(UsageEnvironment& env,Streamer* streamer,int fps,int br,unsigned session_id)
{
	TracerOutputText(streamer->srv_app,ZLOG_LEVEL_INFO,__FUNCTION__",stream ID 0x%x .Begin session",streamer->StreamID);
	return new h264FrameSource(env,streamer,fps,br,session_id);
}
EventTriggerId h264FrameSource::eventTriggerId = 0;
unsigned h264FrameSource::referenceCount = 0;
h264FrameSource::h264FrameSource(UsageEnvironment& env,Streamer* streamer,int fps,int br,unsigned session_id):FramedSource(env)
{
	H_streamer=streamer;
	if(referenceCount == 0){}
	++referenceCount;
	fSecs=1.0f/fps;
	FPS=fps;
	fDurationInMicroseconds=(int)(fSecs*1000000);
	gettimeofday(&fPresentationTime, NULL);
	str_str.clear();
	str_str<<H_streamer->StreamID;
	str_str>>num;
	str_str.clear();
	std::string	debug_fileName("");
	if(streamer->srv_app->srvParam.dump_to_file)
	{
		debug_fileName="dump_"+num+".h264";
		streamer->dbg_data=fopen(debug_fileName.c_str(), "w"); 
		streamer->wf=true;
	}
	else debug_fileName="";
	if(eventTriggerId == 0)
	{
		eventTriggerId = envir().taskScheduler().createEventTrigger(deliverFrameEv);
	}
	streamer->srv_app->isTransmit=true;
	streamer->GetCurrentEncParam();

}
void h264FrameSource::ResetPicSetting()
{  
	H_streamer->srv_app->isTransmit=false;
	TracerOutputText(H_streamer->srv_app,ZLOG_LEVEL_INFO,__FUNCTION__",stream ID 0x%x End LV_session " ,H_streamer->StreamID);
	for(unsigned p=0;p<H_streamer->srv_app->frameSending.size();p++)
	{
		IplImage* from=H_streamer->srv_app->frameSending.front();
		H_streamer->srv_app->CleanMemoryIm(from);
		from=NULL;
		H_streamer->srv_app->frameSending.erase(H_streamer->srv_app->frameSending.begin());
		H_streamer->srv_app->frameSendingTS.erase(H_streamer->srv_app->frameSendingTS.begin());
		H_streamer->srv_app->frameSendingID.erase(H_streamer->srv_app->frameSendingID.begin());
	}
	std::vector<Service_app::enc_paramRT*>::iterator ssId;
	int i=0;
	for(ssId=H_streamer->v_curr_enc_param.begin();ssId!=H_streamer->v_curr_enc_param.end();ssId++,i++)
	{
		Service_app::enc_paramRT *e_prm=*ssId;
		if(e_prm->session_id==id_session)
		{
			int s=H_streamer->v_last_enc_param.size();
			Service_app::enc_paramRT *prev_e_prm=H_streamer->v_last_enc_param.at(i);
			Service_app::enc_paramRT *curr_e_prm=H_streamer->v_curr_enc_param.at(i);
			H_streamer->v_last_enc_param.erase(H_streamer->v_last_enc_param.begin()+i);
			H_streamer->v_curr_enc_param.erase(H_streamer->v_curr_enc_param.begin()+i);
			if(memcmp(prev_e_prm,curr_e_prm,sizeof(Service_app::enc_paramRT)))
			{
				memcpy(&H_streamer->srv_app->srvParam.encParam.real_time_prm,prev_e_prm,sizeof(Service_app::enc_paramRT));
			}
			delete prev_e_prm;
			delete curr_e_prm;
			break;
		}
	}
}
h264FrameSource::~h264FrameSource(void)
{
	ResetPicSetting();
	--referenceCount;
	if(referenceCount<=0)
	{
		envir().taskScheduler().deleteEventTrigger(eventTriggerId);
		eventTriggerId = 0;
	} 

}
void h264FrameSource::FrameCapture()
{ 
	AVFrame* frame = av_frame_alloc();
	izrsEncoder::ImgPrm img={0};
	if(H_streamer->srv_app->frameSending.size())
	{   
		if(H_streamer->srv_app->img_pause)
		{
			H_streamer->srv_app->CleanMemoryIm(H_streamer->srv_app->img_pause);
			H_streamer->srv_app->img_pause=NULL;
		} 

		if(H_streamer->g_watch || H_streamer->AbortTransmit)
		{
			for(unsigned p=0;p<H_streamer->srv_app->frameSending.size();p++)
			{
				IplImage* from=H_streamer->srv_app->frameSending.front();
				H_streamer->srv_app->CleanMemoryIm(from);
				from=NULL;
				H_streamer->srv_app->frameSending.erase(H_streamer->srv_app->frameSending.begin());
				H_streamer->srv_app->frameSendingTS.erase(H_streamer->srv_app->frameSendingTS.begin());
				H_streamer->srv_app->frameSendingID.erase(H_streamer->srv_app->frameSendingID.begin());
			}

			return;
		}
		EnterCriticalSection(&H_streamer->message_queue);
		H_streamer->srv_app->img_pause=H_streamer->srv_app->frameSending.front();
		ULONG vframe_id=H_streamer->srv_app->frameSendingID.front();
		SYSTEMTIME ts=H_streamer->srv_app->frameSendingTS.front();
		LeaveCriticalSection(&H_streamer->message_queue);
		H_streamer->srv_app->frameSending.erase(H_streamer->srv_app->frameSending.begin());
		H_streamer->srv_app->frameSendingID.erase(H_streamer->srv_app->frameSendingID.begin());
		H_streamer->srv_app->frameSendingTS.erase(H_streamer->srv_app->frameSendingTS.begin());
		if(H_streamer->OverlayEnabled)
		{
			std::vector<Service_app::FullOverLayLabelParam*>::iterator it;
			for(it=H_streamer->srv_app->OverlayLabels.begin();it!=H_streamer->srv_app->OverlayLabels.end();it++)
			{
				Service_app::FullOverLayLabelParam  *ObjLabel=*it;
				std::string TextOverImage="";
				if(ObjLabel->label.DataType!=1)
				{
					switch(ObjLabel->label.DataType)
					{
					case 0:
						TextOverImage=std::string(ObjLabel->label.TxtString);
						break;
					case 2:
						H_streamer->srv_app->TimeFromVframe(&ts,ObjLabel->label.LabelTimestampFormat.c_str(),TextOverImage);
						break;
					case 3:
						str_str.clear();
						str_str<<vframe_id;
						str_str>>num;
						TextOverImage=num;
						break;
					}

					H_streamer->srv_app->LabelOvrImage(
						H_streamer->srv_app->img_pause,
						TextOverImage,
						ObjLabel
						);
				}
				else
					H_streamer->srv_app->ImageOvrImage (
					H_streamer->srv_app->img_pause,
					ObjLabel
					);
			}
		}

		img=H_streamer->h264Enc->getCvImage(H_streamer->srv_app->img_pause,0); 
		if(img.QA==STATE_OK)
		{
			avpicture_fill((AVPicture*)frame,img.picture, PIX_FMT_RGB24, img.width,img.height);
			H_streamer->h264Enc->AddFrame(frame,img.width,img.height);
			av_free(img.picture);
		}
		else
		{
			TracerOutputText(H_streamer->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",stream ID 0x%x Bad image ERROR ",H_streamer->StreamID);
		}

	}
	else 
		if(H_streamer->srv_app->isPaused && H_streamer->srv_app->isTransmit)
		{
			if(H_streamer->srv_app->img_pause)
			{
				//cvShowImage("",H_streamer->srv_app->img_pause);
				//cvWaitKey(1);
				img=H_streamer->h264Enc->getCvImage(H_streamer->srv_app->img_pause,0);
				if(img.QA==STATE_OK)
				{
					avpicture_fill((AVPicture*)frame,img.picture, PIX_FMT_RGB24, img.width,img.height);
					H_streamer->h264Enc->AddFrame(frame,img.width,img.height);
					av_free(img.picture);
				}
				else
				{
					TracerOutputText(H_streamer->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",stream ID 0x%x Bad image ERROR " ,H_streamer->StreamID);
				}
			}
		}

		av_sleep(1);
		av_frame_free(&frame);

}
void h264FrameSource::deliverFrameEv(void* clientData)
{
	((h264FrameSource*)clientData)->deliverFrame();
}
void h264FrameSource::doGetNextFrame()
{ 
	if(H_streamer->g_watch || H_streamer->AbortTransmit)
	{
		for(unsigned p=0;p<H_streamer->h264Enc->frameQueue.size();p++)
		{
			AVPacket pkt=H_streamer->h264Enc->frameQueue.front();
			H_streamer->h264Enc->frameQueue.erase(H_streamer->h264Enc->frameQueue.begin());
			av_free(pkt.data);
			av_free_packet(&pkt);
		}
		return;
	}
	while(!H_streamer->h264Enc->isFramesAvailableInOutputQueue()) 
	{
		if(H_streamer->g_watch)return;
		FrameCapture();
	}
	deliverFrame(); 
	av_sleep(1);
}
void h264FrameSource::SetPts()
{
	if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) 
	{
		gettimeofday(&fPresentationTime, NULL);
	} 
	else 
	{

		int delay=msFrameDuration(FPS);
		if(delay>1000)
		{
			TracerOutputText(H_streamer->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",stream ID 0x%x duration ERROR " ,H_streamer->StreamID);
			gettimeofday(&fPresentationTime, NULL);
			delay=222;
		}
		uint64_t f_duration =(uint64_t)mksFrameDuration(FPS);
		uint64_t uSeconds   = fPresentationTime.tv_usec + f_duration;
		fPresentationTime.tv_sec += long(uSeconds/1000000);
		fPresentationTime.tv_usec = long(uSeconds%1000000);
		fDurationInMicroseconds=(int64_t)mksFrameDuration(FPS);
		H_streamer->h_fps++;
		H_streamer->duration_T=(clock() - H_streamer->start_EnC_tr);
		if(H_streamer->duration_T>=1000)
		{
			TracerOutputText(H_streamer->srv_app,ZLOG_LEVEL_INFO,__FUNCTION__",SID 0x%p: #%d H-frames(ch %d) may be deliver.Frame duration %d ms",H_streamer->StreamID,H_streamer->h_fps,H_streamer->nChannelsIn,delay);
			H_streamer->start_EnC_tr=clock();
			H_streamer->h_fps=0;
		}

	}  
}
void h264FrameSource::deliverFrame()
{
	if(H_streamer->g_watch)return;
	try
	{
		if(!isCurrentlyAwaitingData())
		{
			FrameCapture();
			FramedSource::afterGetting(this);
			return;
		}
		if(H_streamer->h264Enc->frameTrunkQueue.size())
		{
			izrsEncoder::TrunckPkt pkt;
			pkt=H_streamer->h264Enc->frameTrunkQueue.front();
			H_streamer->h264Enc->frameTrunkQueue.pop();
			if((unsigned)pkt.size_trunks>fMaxSize)
			{
				fFrameSize = fMaxSize;
				fNumTruncatedBytes =pkt.size_trunks - fMaxSize;
				izrsEncoder::TrunckPkt pkt_t;
				pkt_t.tr_bytes=new unsigned char[fNumTruncatedBytes];
				memcpy(pkt_t.tr_bytes,pkt.tr_bytes+fMaxSize,fNumTruncatedBytes);
				pkt_t.size_trunks=fNumTruncatedBytes;
				H_streamer->h264Enc->frameTrunkQueue.push(pkt_t);
				fprintf(stderr, "very big frame was deliver stream , ID= 0x%x ,fMaxSize=%s\n",H_streamer->StreamID,fMaxSize);
			}
			else
			{
				fNumTruncatedBytes=0;  
				fFrameSize = pkt.size_trunks;
			}
			SetPts();
			memmove(fTo, pkt.tr_bytes,fFrameSize);
			delete []pkt.tr_bytes;
		}
		else
		{  
			if(!H_streamer->h264Enc->frameQueue.size())
			{
				FramedSource::afterGetting(this);
				return;
			}
			AVPacket pkt=H_streamer->h264Enc->frameQueue.front();
			H_streamer->h264Enc->frameQueue.erase(H_streamer->h264Enc->frameQueue.begin());
			unsigned int newFrameSize = pkt.size;
			if(newFrameSize == 0) return;
			if(newFrameSize > fMaxSize)
			{
				fFrameSize = fMaxSize;
				fNumTruncatedBytes = newFrameSize - fMaxSize;
				izrsEncoder::TrunckPkt pkt_tr;
				pkt_tr.tr_bytes=new unsigned char[fNumTruncatedBytes];
				memcpy(pkt_tr.tr_bytes,pkt.data+fMaxSize,fNumTruncatedBytes);
				pkt_tr.size_trunks=fNumTruncatedBytes;
				H_streamer->h264Enc->frameTrunkQueue.push(pkt_tr);
			}
			else
			{
				fFrameSize =newFrameSize;
			} 
			SetPts();
			memmove(fTo,pkt.data, fFrameSize);
			av_free(pkt.data);
			av_free_packet(&pkt);
		}

		nextTask() = envir().taskScheduler().scheduleDelayedTask(fDurationInMicroseconds, (TaskFunc*) FramedSource::afterGetting, this);
	}
	catch (...)
	{
		TracerOutputText(H_streamer->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__", memmove(fTo, pkt.data, fFrameSize)set exception,stream ID 0x%x ,was catch" ,H_streamer->StreamID);
		nextTask() = envir().taskScheduler().scheduleDelayedTask(fDurationInMicroseconds*100, (TaskFunc*) FramedSource::afterGetting, this);
	}
}  
