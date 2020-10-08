#include "xipl.h"
#include "ServiceApp.h"
#include "Streamer.h"
#define CRC32_POLYNOMIAL 0x04c11db7L
extern CvScalar cvColor[];
#pragma comment(lib, "version.lib")
#pragma comment(lib, "user32.lib")
Service_app::Service_app ()
{

}
void  Service_app::Init(DWORD id)
{
	isPaused=NULL;
	img_pause=NULL;
	srvParam.encParam.level=21;
	srvParam.encParam.profile=0;
	srvParam.encParam.real_time_prm.res_scale=1;
	srvParam.encParam.real_time_prm.FPS=25;
	srvParam.encParam.real_time_prm.BR=900000;
	srvParam.encParam.real_time_prm.DvFc=1;
	srvParam.ipAddr=0;
	srvParam.port_rtp=0;
	srvParam.port_rtsp=0;
	srvParam.loadDefault=true;
	srvParam.dump_to_file=false;
	srvParam.streamName="video";
	srvParam.streamType="stream";
	OverlayLabels.clear();
	textSize.height=240;
	textSize.width=320;
	countRframes=0;
	calcFPS=0;
	baseline=0;
	numCPU=-1;
	m_logger=NULL;
	parent_streamID=id;
}

Service_app::~Service_app ()
{

}
char *Service_app::GetTextByEnum( int enumVal )
{
	switch( enumVal )
	{

	case res_scale:
		return "res_scale";
	case fps_dv:
		return "fps_dv";
	case overlay:
		return "overlay";
	case lb_delete:
		return "lb_delete";
	case cycle:
		return "cycle";
	case roi:
		return "roi";
	case roi_pct:
		return "roi_pct";
	default:
		return "Not recognized..";
	}
}

void Service_app::GetNcPU()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	numCPU = sysinfo.dwNumberOfProcessors;
}
IplImage* Service_app::W_UpScale(IplImage* img_src,byte scale)
{
	IplImage* _tRSrc;
	if(scale>1)
	{
		_tRSrc=cvCreateImage(cvSize(img_src->width/scale,img_src->height), 8, 3 );
		if(_tRSrc==NULL) 
			return  img_src;
		cvResize(img_src,_tRSrc,CV_INTER_LINEAR); 
		cvReleaseData(img_src);
		cvReleaseImage(&img_src);
		img_src=NULL;
		return _tRSrc;
	}
	else 
		return  img_src;
}
char* Service_app::getCurrenTime()
{
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	return asctime (timeinfo);
}
void Service_app::initOvrFont(FullOverLayLabelParam  *ObjLabel)
{

	CvFont *font=&ObjLabel->labelFont;
	int FontVal =ObjLabel->label.fontId;
	float scale=srvParam.encParam.real_time_prm.res_scale;
	if(scale==NULL)scale=1;
	double dH=(ObjLabel->label.fontHeight*scale*ObjLabel->font_scaleH);
	double dW=(ObjLabel->label.fontWidth*scale*ObjLabel->font_scaleW);
	double h=dH/100.0;
	double w=dW/100.0;
	int tn= 1;
	if(ObjLabel->font_scaleW>1)tn=2;
	cvInitFont(font,FontVal, h, w, 0, tn, 0);

}
void Service_app::LabelOvrImage(     
	IplImage* img_src,
	std::string txtOver,
	FullOverLayLabelParam *label
	)

{  

	CvRect   Obj_Rect={label->label.CalcObgRect.left,label->label.CalcObgRect.top,label->label.CalcObgRect.right,label->label.CalcObgRect.bottom};
	if( 
		!( Obj_Rect.width >= 0 && Obj_Rect.height >= 0 && Obj_Rect.x < img_src->width && Obj_Rect.y < img_src->height &&
		(Obj_Rect.x + Obj_Rect.width) >= (Obj_Rect.width > 0) && (Obj_Rect.y + Obj_Rect.height) >=(Obj_Rect.height > 0) )
		)
	{
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__"Invalid select ROI rect");
		return ;
	}
	CvScalar Fn_cvColor=cvColor[label->label.fontColorId];
	CvFont cr_font=label->labelFont;
	int Transparency=label->label.bckTrnCyVal;
	CvScalar Bg_cvColor=cvColor[label->label.bckColorId];
	long durationShow=label->label.durationShow;


	//http://robocraft.ru/blog/computervision/352.html  

	// этот код накладывает delay порядка 2х секунд
#if 0
	if(Transparency>0 && Transparency<100)
	{
		IplImage* overlay=cvCreateImage(cvGetSize(img_src),img_src->depth,img_src->nChannels); 
		cvCopy(img_src,overlay, NULL);
		cvRectangleR(img_src, Obj_Rect,Bg_cvColor,CV_FILLED );
		float opacity =Transparency/100.0;
		cvAddWeighted(overlay, opacity,img_src, 1 - opacity, 0, img_src);
		cvReleaseData(overlay);
		cvReleaseImage(&overlay);
	}
	else 
		if(Transparency==0)
#endif 
			try
		{
			cvRectangleR(img_src, Obj_Rect,Bg_cvColor,CV_FILLED );
			if(durationShow!=0 && !label->disableTS)
			{
				int curr_time=clock();
				int diff=curr_time-label->label.begin_show_time;
				if((diff-durationShow)>1000&&!label->disableTS)
				{
					label->disableTS=true;
					return;
				}
			}
			if(label->disableTS)return;
			cvGetTextSize(txtOver.c_str(),&cr_font,&textSize, &baseline);
			baseline += cr_font.thickness;
			cvSetImageROI(img_src,Obj_Rect);
			int posY=Obj_Rect.height/2;
			int posX=Obj_Rect.width-textSize.width;
			if(posX>=0)posX/=2;
			else posX=2;
			CvPoint textOrg =cvPoint( 0,(posY+textSize.height/2));
			cvPutText(img_src,txtOver.c_str(),textOrg,&cr_font,Fn_cvColor);
			cvResetImageROI(img_src);
		}
		catch (...)
		{
			TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" call exception, was catch ");
			fprintf(stderr,__FUNCTION__" call exception, was catch \n");
		}
}

void Service_app::ImageOvrImage (
	IplImage* img_src,
	FullOverLayLabelParam *label
	)
{
	IplImage* img_mux=label->label.IplDataImage;
	CvRect   Obj_Rect={label->label.CalcObgRect.left,label->label.CalcObgRect.top,label->label.CalcObgRect.right,label->label.CalcObgRect.bottom};
	if( 
		!( Obj_Rect.width >= 0 && Obj_Rect.height >= 0 && Obj_Rect.x < img_src->width && Obj_Rect.y < img_src->height &&
		(Obj_Rect.x + Obj_Rect.width) >= (Obj_Rect.width > 0) && (Obj_Rect.y + Obj_Rect.height) >=(Obj_Rect.height > 0) )
		)
	{
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__"Invalid select ROI rect");
		return ; 
	}
	try
	{
		cvSetImageROI(img_src,Obj_Rect);
		cvZero(img_src);
		cvCopy(img_mux, img_src,NULL);
		cvResetImageROI(img_src); 
	}
	catch (...)
	{
		TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__" call exception, was catch ");
		fprintf(stderr,__FUNCTION__" call exception, was catch \n");
	}
}
void Service_app::CleanMemoryIm(IplImage* img)
{
	if(img==NULL)return;
	cvReleaseData(img);
	cvReleaseImage(&img);
	img=NULL;
}
void  Service_app::set_slice(std::string &infrm,std::string &outfrm,int ps,int numf)
{
	if(numf==0)
	{
		outfrm+=" ";
		return;
	}
	std::string c=infrm.substr(ps+numf,1);
	if(c==" "||c=="-"||c==":"||c=="/"||c==".")outfrm+=c;
}
void  Service_app::parse_date(const char *input, struct tm *tm,std::string &out_frm)
{
	out_frm="";
	memset (tm, '\0', sizeof (*tm));
	std::string substring(input);
	int pos=substring.find("YYYY");
	if(pos!=-1)
	{
		out_frm+="%Y";
		set_slice(substring,out_frm,pos,4);
	}
	else
	{
		pos=substring.find("YY");
		if(pos!=-1)
		{
			out_frm+="%y";
			set_slice(substring,out_frm,pos,2);
		}
	}
	pos=substring.find("MM");
	if(pos!=-1)
	{
		out_frm+="%m";
		set_slice(substring,out_frm,pos,2);
	}
	pos=substring.find("DD");
	if(pos!=-1)
	{
		out_frm+="%d";
		set_slice(substring,out_frm,pos,2);
	}
	pos=substring.find("T");
	if(pos!=-1)
	{
		out_frm+=" ";
		set_slice(substring,out_frm,pos,0);
	}
	pos=substring.find("hh");
	if(pos!=-1)
	{
		out_frm+="%H";
		set_slice(substring,out_frm,pos,2);
	}
	pos=substring.find("mm");
	if(pos!=-1)
	{
		out_frm+="%M";
		set_slice(substring,out_frm,pos,2);
	}
	pos=substring.find("ss");
	if(pos!=-1)
	{
		out_frm+="%S";
		set_slice(substring,out_frm,pos,2);
	}

}
void Service_app::TimeFromVframe(const SYSTEMTIME * pTime,const char *input,std::string &tm_vfr)
{
	struct tm tm;
	std::string outFmr;
	parse_date (input, &tm,outFmr);
	tm.tm_year = pTime->wYear-1900;
	tm.tm_mon = pTime->wMonth - 1;
	tm.tm_mday = pTime->wDay;
	tm.tm_hour = pTime->wHour;
	tm.tm_min = pTime->wMinute;
	tm.tm_sec = pTime->wSecond;
	char str_tm[255];
	memset(str_tm,'\0',255);
	if(outFmr.empty())outFmr=input;
	strftime(str_tm,255,outFmr.c_str(),&tm);
	tm_vfr=str_tm;
	///*std::string mes="RTSP->timestamp  "+tm_vfr;
	// OutputDebugString(mes.c_str());*/
}
bool Service_app::WaitStateThread(HANDLE h_thread,int time_out)
{
 Streamer* streamer=reinterpret_cast<Streamer*>(parent_streamID);
 int cn=0;
 while(streamer->mQueueStart!=false)
 {
  Sleep(10);
  cn++;
  if(cn>2000)
	  {
	   TracerOutputText(streamer->srv_app,ZLOG_LEVEL_INFO,"Err End,SID 0x%x",streamer->StreamID);
       DWORD ExitCode;
	   bool res=GetExitCodeThread(h_thread,&ExitCode);
		if(res &&  ExitCode==STILL_ACTIVE)
		{
			TracerOutputText(this,ZLOG_LEVEL_ERROR,__FUNCTION__",SID 0x%x ,wait for terminate thread...",parent_streamID );
			return (bool)WaitForSingleObject(h_thread,time_out);
		}
      }
 }

 return STATE_OK;
}
long Service_app::CheckIfValidFrame( IplImage* frame )
{
	return  xiplCheckImageHeader(frame );
}
void Service_app::WriteIPLToFile(IplImage* pSaveImg)
{
#ifdef DBG_FILE	
	time_t t = time(0);
	struct tm * now = localtime( & t );
	if(time_start->tm_yday<now->tm_yday)
	{
		cn_bdFr=0;
		time_start=now;
	}
	if(cn_bdFr<1)
	{
		std::stringstream tm;
		std::string Fr_fileNm;
		tm<<fullfilename<<now->tm_mday<<"-"<<now->tm_hour<<"_"<<now->tm_min<<"_"<<now->tm_sec<<".png";
		tm>>Fr_fileNm;
		int lev=3;
		cvSaveImage(Fr_fileNm.c_str(),pSaveImg,&lev);

		//cvShowImage(Fr_fileNm.c_str(),pSaveImg);
		//cvWaitKey(1);

		cn_bdFr++;
	}
#endif
}
void removeExe(char *s)
{
	int i;
	for(i = 0; s[i]; i++)
	{}
	while(s[--i] != '\\')
	{}
	s[i+1] = '\0';
}
std::string Service_app::GetVersionInfo(TCHAR *szValue)
{
#ifdef DBG_FILE
	char path[255];
	GetModuleFileName(NULL,path,sizeof(path));
	removeExe(path);
	fullfilename.assign(path);
#endif
	std::string csRet="rtsp streamer load ver. ";
	HMODULE hServerRTSP=GetModuleHandle("ServerRTSP");
	if(hServerRTSP==NULL)return NULL;
	HRSRC hVersion = FindResource(hServerRTSP,MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
	DWORD dwSize = SizeofResource(hServerRTSP,hVersion);
	if (hVersion != NULL)
	{
		HGLOBAL hGlobal = LoadResource(hServerRTSP, hVersion);

		if (hGlobal != NULL)
		{
			LPVOID ver = LocalAlloc(LPTR, dwSize*2);
			if (ver != NULL)
			{
				memcpy(ver, hGlobal, dwSize);
				DWORD *codepage;
				UINT len;
				char fmt[0x40];
				PVOID ptr = 0;
				if (VerQueryValue(ver, "\\VarFileInfo\\Translation", (LPVOID*) & codepage, &len))
				{
					wsprintf(fmt, "\\StringFileInfo\\%08x\\%s", (*codepage) << 16 | (*codepage) >> 16, szValue);
					if (VerQueryValue(ver, fmt, &ptr, &len))
					{
						std::string	vers=(char*)ptr;
						csRet.append(vers);
					}
				}
				LocalFree(ver);
			}
			FreeResource(hGlobal);
		}
	}
	return csRet;
}
VOID Service_app::InitTableCRC32()
{
	for(int i=0; i < 256; i++ )
	{
		DWORD crc_accum = (i << 24);
		for(int j=0;  j < 8;  j++)
		{
			if(crc_accum & 0x80000000L)
				crc_accum = (crc_accum << 1) ^ CRC32_POLYNOMIAL;
			else crc_accum = (crc_accum << 1);
		}
		CrcTable32[i] = crc_accum;
	}
}

DWORD Service_app::Get_CRC32(PVOID data,int len)
{
	DWORD crc=0xFFFFFFFF;
	PUCHAR buffer=(PUCHAR)data;
	for(int j=0; j < len; j++)
	{
		int i = ((int)(crc >> 24) ^ *buffer++) & 0xFF;
		crc = (crc << 8) ^ CrcTable32[i];
	}
	return crc;
}