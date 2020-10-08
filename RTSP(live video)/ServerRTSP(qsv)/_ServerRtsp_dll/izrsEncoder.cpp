#include "stdafx.h"
#include "ffmpegInclude.h"
#include "opencv2/opencv.hpp"
#include "common/include/common_utils.h"	
#include "izrsEncoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <liveMedia.hh>
#include <GroupsockHelper.hh>
mfxHDL deviceHandle=0;
izrsEncoder::izrsEncoder() 
{
	pOutFormat = NULL;
	pFormatContext = NULL;
	pVideoStream = NULL;
	rtspServerPortNum = 554;
	static_pts=0;
	ClrTrigger=false;
	av_codec_type=0;
}
izrsEncoder::~izrsEncoder()
{
	Finish();
}
bool izrsEncoder::Initialize(std::string& container,void* real_Streamer)
{
	bool res = false;
	m_streamer=(Streamer*)real_Streamer;
	std::string codec_name;
	VideoCodec=NULL;
	TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" %s",container.c_str());
	codec_name=container.substr(1,3);
	if(memcmp(codec_name.c_str(),"264",3)==0)
	{
		I264=false;
		if( (memcmp(m_streamer->srv_app->srvParam.encParam.accel.c_str(),"i264S",5)==0) )
		{
			m_impl =MFX_IMPL_SOFTWARE;
			I264=true;
		}
		else
			if( (memcmp(m_streamer->srv_app->srvParam.encParam.accel.c_str(),"i264H",5)==0) )
			{
				m_impl=MFX_IMPL_HARDWARE;
				I264=true;
			}
			codec_name=container;
			container="mp4";
			VideoCodec=avcodec_find_encoder_by_name("libx264");

	}
	avformat_alloc_output_context2(&pFormatContext, NULL,container.c_str(),NULL);
	if(pFormatContext)
	{   

		pOutFormat=pFormatContext->oformat;
		if(VideoCodec!=NULL)
		{
			pOutFormat->video_codec=VideoCodec->id;
			TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__"avcodec_find_encoder_by_name %s OK codec_ID %d",codec_name.c_str(),pOutFormat->video_codec);
			fprintf(stderr,"avcodec_find_encoder_by_name(libx264) OK,id -%d in container %s\n",pOutFormat->video_codec,container.c_str());
		}
		else
		{
			VideoCodec = avcodec_find_encoder(pOutFormat->video_codec);
			if(!VideoCodec)
			{
				TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" error,not found codec ");
				fprintf(stderr,"Initialize  error,not found codec  \n");
				return STATE_ERR;
			}
			TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__"Ok, Set codec by container",container.c_str());
			fprintf(stderr,"Set codec by container,id -%d \n",pOutFormat->video_codec);
		}
		return STATE_OK;
	}
	TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__"  error,not found codec");
	return STATE_ERR;
}
void izrsEncoder::InitialAcceleration()
{
	AVCodecContext *pVCodecCtx=pVideoStream->codec;
	InitQSVEnc(pVCodecCtx);
	if(QSV_enable)
	{	 
		TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__"after InitQSVEnc");
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
		fprintf(stderr, "Acceleration mode %s set, encoder ID= 0x%x \n",s_impl.c_str(),m_streamer->StreamID);
	}
	else  fprintf(stderr, "FFMPEG encoder mode set, encoder ID= 0x%x \n",m_streamer->StreamID);
}
AVStream *izrsEncoder::AddVideoStream(AVFormatContext *pContext)
{
	AVCodecContext *p_VCodecCtx = NULL;
	AVStream *st    = NULL;
	if(m_streamer==NULL)return NULL;
	HRESULT res=STATE_OK;
	st = avformat_new_stream(pContext,VideoCodec);
	if (!st) return NULL;

	QSV_enable=false;

	int w=m_streamer->srv_app->srvParam.encParam.real_time_prm.Width;
	int h=m_streamer->srv_app->srvParam.encParam.real_time_prm.Height;
	int fps=m_streamer->srv_app->srvParam.encParam.real_time_prm.FPS;
	int bitrate=m_streamer->srv_app->srvParam.encParam.real_time_prm.BR; 
	st->id =pContext->nb_streams-1; 

	p_VCodecCtx = st->codec;  
	p_VCodecCtx->codec=VideoCodec;
	p_VCodecCtx->codec_id =VideoCodec->id;
	p_VCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	p_VCodecCtx->height=h;
	p_VCodecCtx->width=w;
	p_VCodecCtx->pix_fmt =AV_PIX_FMT_YUVJ420P;
	p_VCodecCtx->framerate.den=1;
	p_VCodecCtx->framerate.num=fps;
	p_VCodecCtx->time_base.den =fps;
	p_VCodecCtx->time_base.num =1;
	st->r_frame_rate.den=1;
	st->r_frame_rate.num=fps;
	st->time_base.den =90000;
	st->time_base.den =1;
	p_VCodecCtx->thread_count=2;//было  2
	p_VCodecCtx->bit_rate=bitrate;   
	av_codec_type=VideoCodec->id; 
	switch(av_codec_type)
	{
	case AV_CODEC_ID_MJPEG:
		p_VCodecCtx->qmax=15;
		p_VCodecCtx->qmin=5;
		p_VCodecCtx->mb_lmin = p_VCodecCtx->qmin * FF_QP2LAMBDA;
		p_VCodecCtx->mb_lmax = p_VCodecCtx->qmax * FF_QP2LAMBDA;
		p_VCodecCtx->global_quality= p_VCodecCtx->qmin * FF_QP2LAMBDA; 
		p_VCodecCtx->gop_size =1; 
		p_VCodecCtx->keyint_min =0; 
		p_VCodecCtx->thread_count=1;///не трогай очень важный для jpeg
		break;
	case AV_CODEC_ID_H264: 
		p_VCodecCtx->coder_type =FF_CODER_TYPE_VLC;// FF_CODER_TYPE_RAW;//
		p_VCodecCtx->pix_fmt =AV_PIX_FMT_YUV420P; 
		p_VCodecCtx->thread_type=FF_THREAD_FRAME;
		p_VCodecCtx->bit_rate_tolerance=bitrate;
		p_VCodecCtx->rc_max_rate=(int)(bitrate*1.75);
		p_VCodecCtx->rc_min_rate=(int)(bitrate*0.5);
		p_VCodecCtx->rc_buffer_size=bitrate*2;
		p_VCodecCtx->rc_max_available_vbv_use=1;
		p_VCodecCtx->rc_min_vbv_overflow_use=1;
		p_VCodecCtx->profile=m_streamer->srv_app->srvParam.encParam.profile;
		p_VCodecCtx->level=m_streamer->srv_app->srvParam.encParam.level;
		p_VCodecCtx->max_b_frames =0;
		p_VCodecCtx->flags=CODEC_FLAG_UNALIGNED|CODEC_FLAG_LOW_DELAY|CODEC_FLAG_LOOP_FILTER;
		p_VCodecCtx->rc_initial_buffer_occupancy=p_VCodecCtx->rc_buffer_size * 3 / 4;
		p_VCodecCtx->gop_size=5; 
		p_VCodecCtx->keyint_min =5;
		p_VCodecCtx->max_qdiff = 4; 
		p_VCodecCtx->refs=1;

		if(fps<5)
		{
			p_VCodecCtx->max_b_frames =0;
			p_VCodecCtx->rc_buffer_size=bitrate;
			p_VCodecCtx->rc_initial_buffer_occupancy=p_VCodecCtx->rc_buffer_size;
			p_VCodecCtx->thread_count=1;
			p_VCodecCtx->gop_size =fps; 
			p_VCodecCtx->keyint_min =fps; 
		}
		else 
		{
			p_VCodecCtx->max_qdiff = 4;
			p_VCodecCtx->trellis = 0;		   
			p_VCodecCtx->qblur = 0.5;
			p_VCodecCtx->me_range=0;
		}
		break;
	}
	return st;
}

HRESULT izrsEncoder::InitQSVEnc(AVCodecContext *p_VCodecCtx)
{
	// Create Media SDK encoder
	mfxStatus sts;
	mfxVersion m_ver={{4,1}};
	memset(&m_mfxAllocator, 0, sizeof(mfxFrameAllocator));
	sts=InitializeMfx(m_impl, m_ver, &m_mfx_session, &m_mfxAllocator,deviceHandle);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE,MFX_ERR_UNKNOWN);
	mfxVideoParam mfxEncParams;
	m_mfxENC=new MFXVideoENCODE(m_mfx_session) ;
	memset(&mfxEncParams, 0, sizeof(mfxEncParams));
	mfxEncParams.mfx.CodecId = MFX_CODEC_AVC;
	mfxEncParams.mfx.TargetUsage =MFX_TARGETUSAGE_BEST_SPEED;//MFX_TARGETUSAGE_BALANCED;//  
	mfxEncParams.mfx.TargetKbps = p_VCodecCtx->bit_rate/1000;
	mfxEncParams.mfx.InitialDelayInKB =p_VCodecCtx->bit_rate/1000;
	mfxEncParams.mfx.MaxKbps=p_VCodecCtx->bit_rate/1000;
	mfxEncParams.mfx.RateControlMethod =MFX_RATECONTROL_VBR;
	mfxEncParams.AsyncDepth= 4;
	mfxEncParams.mfx.EncodedOrder=0; 	
	mfxEncParams.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
	mfxEncParams.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
	mfxEncParams.mfx.FrameInfo.BitDepthLuma   = 8;
	mfxEncParams.mfx.FrameInfo.BitDepthChroma = 8;
	mfxEncParams.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
	mfxEncParams.mfx.FrameInfo.CropX = 0;
	mfxEncParams.mfx.FrameInfo.CropY = 0;
	mfxEncParams.mfx.FrameInfo.CropW = p_VCodecCtx->width;
	mfxEncParams.mfx.FrameInfo.CropH = p_VCodecCtx->height;
	mfxEncParams.mfx.FrameInfo.Width = MSDK_ALIGN16(p_VCodecCtx->width);
	mfxEncParams.mfx.FrameInfo.Height =(MFX_PICSTRUCT_PROGRESSIVE == mfxEncParams.mfx.FrameInfo.PicStruct) ? MSDK_ALIGN16(p_VCodecCtx->height) : MSDK_ALIGN32(p_VCodecCtx->height);
	//int fps=p_VCodecCtx->framerate.num/p_VCodecCtx->framerate.den;
	mfxEncParams.mfx.GopPicSize =5;
	mfxEncParams.mfx.GopRefDist=1;
	mfxEncParams.mfx.FrameInfo.FrameRateExtD =1;
	mfxEncParams.mfx.FrameInfo.FrameRateExtN =p_VCodecCtx->framerate.num;
	mfxEncParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;//MFX_IOPATTERN_IN_OPAQUE_MEMORY;//MFX_IOPATTERN_IN_SYSTEM_MEMORY;; //

	mfxExtBuffer *extBuffersInit[1];
	mfxExtCodingOption extCO;
	memset( &extCO, 0, sizeof( extCO)) ;
	extCO.Header.BufferId = MFX_EXTBUFF_CODING_OPTION;
	extCO.Header.BufferSz = sizeof(mfxExtCodingOption);
	extCO.MaxDecFrameBuffering = 1;
	extBuffersInit[0] = reinterpret_cast<mfxExtBuffer*>(&extCO);



	mfxEncParams.NumExtParam = 1;
	mfxEncParams.ExtParam = extBuffersInit;
	mfxEncParams.mfx.NumRefFrame  = 1;
	//
	//
	//sts = m_mfxENC->Query(&mfxEncParams, &mfxEncParams);
	//MSDK_IGNORE_MFX_STS(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
	//MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	// Query number of required surfaces for encoder
	sts = m_mfxENC->Init(&mfxEncParams); 
	MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	memset(&m_EncRequest, 0, sizeof(mfxFrameAllocRequest));
	sts = m_mfxENC->QueryIOSurf(&mfxEncParams, &m_EncRequest);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
	m_EncRequest.NumFrameSuggested=m_EncRequest.NumFrameSuggested + mfxEncParams.AsyncDepth;

	// Allocate required surfaces

	sts = m_mfxAllocator.Alloc(m_mfxAllocator.pthis, &m_EncRequest, &mfxResponse);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
	m_nEncSurfNum = mfxResponse.NumFrameActual;
	m_pEncSurfaces = new mfxFrameSurface1*[m_nEncSurfNum];

	MSDK_CHECK_POINTER(m_pEncSurfaces, MFX_ERR_MEMORY_ALLOC);

	for (int i = 0; i < m_nEncSurfNum; i++) 
	{
		m_pEncSurfaces[i] = new mfxFrameSurface1;
		MSDK_CHECK_POINTER(m_pEncSurfaces[i], MFX_ERR_MEMORY_ALLOC);
		memset(m_pEncSurfaces[i], 0, sizeof(mfxFrameSurface1));
		memcpy(&(m_pEncSurfaces[i]->Info), &(mfxEncParams.mfx.FrameInfo), sizeof(mfxFrameInfo));
		m_pEncSurfaces[i]->Data.MemId = mfxResponse.mids[i]; 
		ClearYUVSurfaceVMem(m_pEncSurfaces[i]->Data.MemId);
	}

	memset(&encParams, 0, sizeof(encParams));
	sts =m_mfxENC->GetVideoParam(&encParams);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	memset(&m_mfxBS, 0, sizeof(m_mfxBS));
	m_mfxBS.MaxLength = encParams.mfx.BufferSizeInKB * 1000;
	m_mfxBS.Data =(mfxU8*)av_malloc(m_mfxBS.MaxLength);
	MSDK_CHECK_POINTER(m_mfxBS.Data, MFX_ERR_MEMORY_ALLOC);
	QSV_enable=true;

	return STATE_OK;
}
bool izrsEncoder::InitEncodeR(int fps,int bitrate)
{
	bool res = STATE_OK;
	pVideoStream=AddVideoStream(pFormatContext);
	if (pVideoStream)
	{
		if(QSV_enable==false)
		{
			res = OpenVideo(pVideoStream);
			if (res==STATE_ERR)
			{
				TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__" Exeption Streamer err: OpenVideo");
			}
		}
		else 
			TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__" QSV_enabled OK !");
	}    
	return res;
}
bool izrsEncoder::isFramesAvailableInOutputQueue()
{
	return !(frameQueue.empty());
}
bool izrsEncoder::AvPacketFillbyFFMPEG(AVCodecContext *_p_VCodecCtx,AVFrame * p_YUVFrame,AVPacket &_packet)
{
	int nSizeVideoEncodeBuffer =avpicture_get_size(_p_VCodecCtx->pix_fmt,_p_VCodecCtx->width,_p_VCodecCtx->height);
	_packet.data=(uint8_t *)av_malloc(nSizeVideoEncodeBuffer);
	_packet.size= nSizeVideoEncodeBuffer;

	int nOutputSize =0;
	int ret=0;
	_packet.pts = _packet.dts = 1;
	try
	{
		p_YUVFrame->quality = _p_VCodecCtx->global_quality; 
		p_YUVFrame->format=AV_PIX_FMT_YUV420P;
		p_YUVFrame->width=_p_VCodecCtx->width;
		p_YUVFrame->height=_p_VCodecCtx->height;

		int error = avcodec_encode_video2(_p_VCodecCtx, &_packet, p_YUVFrame, &nOutputSize);

		bool dropped_frame=STATE_OK;
		if(av_codec_type==AV_CODEC_ID_H264)
			if(nOutputSize<=0)dropped_frame=STATE_ERR;

		if (error==0 && dropped_frame==STATE_OK && _packet.size>0)
		{
			if(p_YUVFrame->key_frame)
			{
				_packet.flags|= AV_PKT_FLAG_KEY;
			}
			return STATE_OK;
		}
		else 
			return STATE_ERR;

	}

	catch(...)
	{
		TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__",stream ID 0x%x avcodec_encode_video2 generate exception " , m_streamer->StreamID); 
		fprintf(stderr,"Streamer err:avcodec_encode_video2 throw\n");
		return STATE_ERR;
	}
	return STATE_OK;
}

bool izrsEncoder::AvPacketFillbyFMX(AVCodecContext *p_VCodecCtx,AVFrame *p_YUVFrame,AVPacket &_packet)
{
	m_nEncSurfIdx = GetFreeSurfaceIndex(m_pEncSurfaces, m_nEncSurfNum);
	MSDK_CHECK_ERROR_BOOL(MFX_ERR_NOT_FOUND, m_nEncSurfIdx, MFX_ERR_MEMORY_ALLOC);
	mfxStatus sts=MFX_ERR_NONE;
	mfxU8* p_mfxYUV=m_mfxBS.Data;
	mfxFrameSurface1* pSurf=m_pEncSurfaces[m_nEncSurfIdx];
	mfxSyncPoint m_syncp(0); 
	mfxFrameData* pData=&m_pEncSurfaces[m_nEncSurfIdx]->Data;	
	sts = m_mfxAllocator.Lock(m_mfxAllocator.pthis, pData->MemId, pData);
	MSDK_CHECK_ERROR_BOOL(sts, MFX_ERR_NONE, sts);
	LoadYUVFrame(pSurf,p_YUVFrame);
	sts = m_mfxAllocator.Unlock(m_mfxAllocator.pthis, pData->MemId, pData);
	MSDK_CHECK_ERROR_BOOL(sts, MFX_ERR_NONE, sts);
	do
	{
		sts = m_mfxENC->EncodeFrameAsync(NULL,pSurf, &m_mfxBS, &m_syncp);
		if (MFX_ERR_NONE < sts && !m_syncp) 
		{ 
			if (MFX_WRN_DEVICE_BUSY == sts)
				MSDK_SLEEP(1); 
		}
		else 
			if (MFX_ERR_NONE < sts && m_syncp) 
			{
				sts = MFX_ERR_NONE; 
				break;
			} 
			else 
				if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) 
				{
					memset(&m_mfxBS, 0, sizeof(m_mfxBS)) ; 
					m_mfxBS.MaxLength = encParams.mfx.BufferSizeInKB * 1000;
					m_mfxBS.Data = p_mfxYUV;
					return STATE_ERR;
				} 
				else
					break;
	}
	while (true) ;	
	MSDK_CHECK_ERROR_BOOL(sts, MFX_ERR_NONE, sts);
	sts =m_mfx_session.SyncOperation(m_syncp,QSV_SYNCPOINT_WAIT);      // Synchronize. Wait until encoded frame is ready
	int tm_wait=0;
	while(sts!=MFX_ERR_NONE)
	{
		av_sleep(10);
		sts =m_mfx_session.SyncOperation(m_syncp,QSV_SYNCPOINT_WAIT); 
		if(tm_wait>20)
		{
			memset(&m_mfxBS, 0, sizeof(m_mfxBS)) ; 
			m_mfxBS.MaxLength = encParams.mfx.BufferSizeInKB * 1000;
			m_mfxBS.Data = p_mfxYUV;
			return STATE_ERR;
		}
		tm_wait++;  
	}
	MSDK_CHECK_ERROR_BOOL(sts, MFX_ERR_NONE, sts);	
	_packet.data=(uint8_t *)av_malloc(m_mfxBS.DataLength);
	memcpy(_packet.data,m_mfxBS.Data,m_mfxBS.DataLength);
	AVRational bq={1, 90000};
	if (m_mfxBS.FrameType & MFX_FRAMETYPE_IDR || m_mfxBS.FrameType & MFX_FRAMETYPE_xIDR)_packet.flags |= AV_PKT_FLAG_KEY;
	_packet.dts  = av_rescale_q(m_mfxBS.DecodeTimeStamp, p_VCodecCtx->time_base,bq);
	_packet.pts  = av_rescale_q(m_mfxBS.TimeStamp, p_VCodecCtx->time_base,bq);
	_packet.size=m_mfxBS.DataLength;
	_packet.pts=m_mfxBS.TimeStamp;

	memset(&m_mfxBS, 0, sizeof(m_mfxBS)) ; 
	m_mfxBS.MaxLength = encParams.mfx.BufferSizeInKB * 1000;
	m_mfxBS.Data = p_mfxYUV;
	MSDK_CHECK_POINTER_BOOL(m_mfxBS.Data, MFX_ERR_MEMORY_ALLOC);
	return STATE_OK;
}

void izrsEncoder::PushPacketFFMpeg(AVPacket &_packet)
{
	if (_packet.pts != AV_NOPTS_VALUE)
		_packet.pts= av_rescale_q(_packet.pts,pVideoStream->codec->time_base, pVideoStream->time_base);
	if (_packet.dts != AV_NOPTS_VALUE)
		_packet.dts = av_rescale_q(_packet.pts-1,pVideoStream->codec->time_base, pVideoStream->time_base);

	_packet.flags = _packet.flags;
	_packet.stream_index = pVideoStream->index;
	frameQueue.push_back(_packet);
}
void izrsEncoder::PushPacketMfx(AVPacket &_packet)
{
	_packet.stream_index = pVideoStream->index;
	frameQueue.push_back(_packet);
}
void izrsEncoder::AddVideoFrame(AVFrame * pOutputAVFrame,AVCodecContext *p_VCodecCtx)
{    

	bool res = STATE_OK;
	AVPacket packet={0};
	av_init_packet(&packet);
	if(QSV_enable)
	{
		AvPacketFill = &izrsEncoder::AvPacketFillbyFMX;
		PushPacket = &izrsEncoder::PushPacketMfx;
	}
	else
	{
		AvPacketFill = &izrsEncoder::AvPacketFillbyFFMPEG;
		PushPacket = &izrsEncoder::PushPacketMfx;
	}
	res =(this->*AvPacketFill)(p_VCodecCtx,pOutputAVFrame,packet);
	if(res!=STATE_OK)
	{
		av_free_packet(&packet);
		return;
	}
	(this->*PushPacket)(packet);

}


int izrsEncoder::nextPTS()
{
	return  static_pts++;
}
bool izrsEncoder::AddFrame(AVFrame* frame,int w,int h)
{
	bool res = true;
	int nOutputSize = 0;
	AVCodecContext *p_VCodecCtx = NULL;
	if (pVideoStream && frame && frame->data[0])
	{ 
		AVFrame *pCurrentAVFrame=NULL;
		p_VCodecCtx = pVideoStream->codec;
		struct SwsContext *pImgConvertCtx=NULL;

		if(p_VCodecCtx->pix_fmt==PIX_FMT_YUVJ420P)
		{
			p_VCodecCtx->pix_fmt=PIX_FMT_YUV420P;
		}

		if (NeedConvert()) 
		{
			// RGB to YUV420P.
			pImgConvertCtx = sws_getContext(w, h,PIX_FMT_RGB24,p_VCodecCtx->width, p_VCodecCtx->height,p_VCodecCtx->pix_fmt,SWS_BICUBLIN, NULL, NULL, NULL);

		}
		try
		{
			pCurrentAVFrame = CreateFFmpegPicture(p_VCodecCtx->pix_fmt, p_VCodecCtx->width,p_VCodecCtx->height);
			if (NeedConvert() && pImgConvertCtx&& pCurrentAVFrame) 
			{
				// Convert RGB to YUV.
				sws_scale(pImgConvertCtx,frame->data,frame->linesize,0,h, pCurrentAVFrame->data, pCurrentAVFrame->linesize); 

			}
			if(pCurrentAVFrame)
			{
				pCurrentAVFrame->pts=nextPTS(); 
				AddVideoFrame(pCurrentAVFrame, pVideoStream->codec);
				av_free(pCurrentAVFrame->data[0]);
				av_frame_free(&pCurrentAVFrame);
				if(pImgConvertCtx)sws_freeContext(pImgConvertCtx);
				pCurrentAVFrame = NULL;
			}

		}
		catch(...)
		{
			TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__",stream ID 0x%x CreateFFmpegPicture generate exception " , m_streamer->StreamID); 
		}


	}
	return res;
}

AVFrame * izrsEncoder::CreateFFmpegPicture(AVPixelFormat pix_fmt, int nWidth, int nHeight)
{
	AVFrame *picture     = NULL;
	uint8_t *picture_buf = NULL;
	int size;

	//avcodec_alloc_frame was changed to av_frame_alloc
	picture = av_frame_alloc();
	if ( !picture)
	{
		TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__",stream ID 0x%x  avcodec_alloc_frame() return NULL ERROR " , m_streamer->StreamID); 
		return NULL;
	}
	size = avpicture_get_size(pix_fmt, nWidth, nHeight);
	picture_buf = (uint8_t *) av_malloc(size);
	if (!picture_buf) 
	{
		av_free(picture);
		TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__",stream ID 0x%x av_malloc(size) return NULL ERROR "     , m_streamer->StreamID); 
		return NULL;
	}
	avpicture_fill((AVPicture *)picture, picture_buf,pix_fmt, nWidth, nHeight);
	return picture;
}


bool izrsEncoder::OpenVideo(AVStream *pStream)
{
	AVCodecContext *pContext=pStream->codec; 
	AVDictionary *opts = NULL;
	int  res=STATE_OK;
	av_dict_set(&opts, "preset","ultrafast",0);
	av_dict_set(&opts, "tune", "zerolatency", 0);

	res=avcodec_open2(pContext,VideoCodec,&opts);

	av_dict_free(&opts);
	if(res<STATE_OK)
	{
		fprintf(stderr,"Streamer err:avcodec_open2 throw\n");
		TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_ERROR,"stream ID 0x%x Streamer err:avcodec_open2 throw" , m_streamer->StreamID); 
		return STATE_ERR;
	}
	TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_DEBUG,__FUNCTION__",stream ID 0x%x Exit Ok " , m_streamer->StreamID); 

	return STATE_OK;
}


void izrsEncoder::CloseVideo(AVFormatContext *pContext, AVStream *pStream)
{
	avcodec_close(pStream->codec);
}

bool izrsEncoder::NeedConvert()
{
	bool res = false;
	if (pVideoStream && pVideoStream->codec)
	{
		res = (pVideoStream->codec->pix_fmt != PIX_FMT_RGB24);
	}
	return res;
}



izrsEncoder::ImgPrm izrsEncoder::getCvImage(IplImage* rawImage,int scale)
{ 
	ImgPrm prm_in={0};
	if(rawImage->imageData==NULL)
	{
		TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",Frame empty,rawImage->imageData=NULL m_streamer ID= 0x%x",m_streamer->StreamID);
		fprintf(stderr, "Frame empty,rawImage->imageData=NULL m_streamer ID= 0x%x\n",m_streamer->StreamID);
		prm_in.QA=STATE_ERR;
		return prm_in;
	}

	try
	{
		cvConvertImage(rawImage,rawImage, CV_CVTIMG_SWAP_RB);
		prm_in=IPL_to_Picture(rawImage,scale);
	}
	catch (...)
	{
		TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",Receive invalid frame, m_streamer ID= 0x%x",m_streamer->StreamID);
		fprintf(stderr, "Receive invalid frame, m_streamer ID= 0x%x\n",m_streamer->StreamID);
		prm_in.QA=STATE_ERR;
	}
	if(prm_in.picture==NULL)
	{
		TracerOutputText(m_streamer->srv_app,ZLOG_LEVEL_ERROR,__FUNCTION__",Frame empty, prm_in.picture=NULL m_streamer ID= 0x%x",m_streamer->StreamID);
		fprintf(stderr, "Frame empty, prm_in.picture=NULL m_streamer ID= 0x%x\n",m_streamer->StreamID);
		prm_in.QA=STATE_ERR;
	}
	return prm_in;
}
izrsEncoder::ImgPrm izrsEncoder::IPL_to_Picture(IplImage* img_src,int scale=0)
{

	IplImage* _RSrc=NULL;
	if(scale!=0)
	{
		_RSrc=cvCreateImage(cvSize(img_src->width/scale,img_src->height/scale),img_src->depth, img_src->nChannels );
		cvResize(img_src,_RSrc,CV_INTER_LINEAR);
	}
	else 
		_RSrc=img_src;

	int bufferImgSize=avpicture_get_size(PIX_FMT_RGB24, _RSrc->width,_RSrc->height);
	uint8_t* buffer=(uint8_t*)av_mallocz(bufferImgSize);
	char *App=(char*)buffer;
	memcpy(App,_RSrc->imageDataOrigin,_RSrc->imageSize);
	ImgPrm prm={0};
	prm.picture=buffer;
	prm.height=_RSrc->height;
	prm.width=_RSrc->width;
	if(scale!=0)CleanMemoryIm(_RSrc);
	return prm;
}
void izrsEncoder::CleanMemoryIm(IplImage* img)
{
	cvReleaseData(img);
	cvReleaseImage(&img);
	img=NULL;
}
bool izrsEncoder::Finish()
{
	bool res = true;
	if (pFormatContext)
	{
		Free();
	}
	return res;
}


void izrsEncoder::Free()
{
	bool res = true;
	if (pFormatContext)
	{
		// close video stream
		if (pVideoStream)
		{
			CloseVideo(pFormatContext, pVideoStream);
		}

		// Free the streams.
		for(size_t i = 0; i < pFormatContext->nb_streams; i++) 
		{
			av_freep(&pFormatContext->streams[i]->codec);
			av_freep(&pFormatContext->streams[i]);
		}
		// Free the stream.
		av_freep(pFormatContext);
		pFormatContext = NULL;
	}
	if(m_mfxENC)
	{
		m_mfxENC->Close();
		for (int i = 0; i < m_nEncSurfNum; i++)
			delete m_pEncSurfaces[i];
		MSDK_SAFE_DELETE_ARRAY(m_pEncSurfaces);
		MSDK_SAFE_DELETE_ARRAY(m_mfxBS.Data);
	}
}
