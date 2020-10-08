#define  DEBUG 1
#include <stdio.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "interface.h"
#include "Services.h"
#include "Validator.h"

void TracerOutputText(CRTSPCamera* dvs, ZLOG_LEVEL level, LPCTSTR Format, ...);
int CRTSPCamera::num_obj=0;
//----------------------------------- Обьект типа камера-----(DS_ALLOCATED)---------------------------

CRTSPCamera* __stdcall zrtspCreateDevice()
{


	CRTSPCamera* dev=CRTSPCamera::CreateDecoder();
	if(dev==NULL)
	{
		TracerOutputText(dev,ZLOG_LEVEL_ERROR,__FUNCTION__" Error create stream CRTSPCamera ");
		return NULL;
	}
	dev->InitObject();
	if(dev->num_obj==1)
		{
			
			av_register_all();
			avformat_network_init();
			av_log_set_level(AV_LOG_QUIET);//V_LOG_PANIC;AV_LOG_FATAL; (AV_LOG_ERROR);
			dev->srv->GetNcPU();
			fprintf(stderr, "\n%s\n\n",(dev->srv->GetVersionInfo("ProductVersion")).c_str());
		}
	TracerOutputText(dev,ZLOG_LEVEL_INFO,__FUNCTION__" stream CRTSPCamera 0x%x create Ok.",dev->DecoderID);
    fprintf(stderr, "RTSP camera ID=0x%x build start\n",dev->DecoderID);
 return dev;
}
//-------------------------------------DS_DISCONNECTED------------------------------------------------

long __stdcall zrtspLoadDevice(CRTSPCamera* pDevice)
{
	
	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit err = -1"     );
		return -1;
	}
	HRESULT res=pDevice->LoadDevice();
	TracerOutputText(pDevice,ZLOG_LEVEL_INFO,__FUNCTION__" Done OK");
	return res;	
}
//---------------------------DS_RUNNING---------------------------------------
long __stdcall zrtspStartGrab(CRTSPCamera* pDevice)
{
	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	HRESULT res=pDevice->Camera_Open();
	return res;
}
//---------------------------DS_PAUSE-----------------------------------------
long __stdcall zrtspPauseGrab(CRTSPCamera* pDevice)
{

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return STATE_ERR;
	}

	bool res=false;
	try
	{
  		res=pDevice->Device_Pause();
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" catch(...) " );
		return STATE_ERR;
	}
	return STATE_OK; 
}
//---------------------------DS_STOP------------------------------------------
long __stdcall zrtspStopGrab(CRTSPCamera* pDevice)
{
 	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return STATE_ERR;
	}

	pDevice->LockDevice();
	try
	{
		if(pDevice->Stopped==false)pDevice->Device_Stop();
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" catch(...) "     );
		return STATE_ERR;
	}
	pDevice->UnlockDevice();

	TracerOutputText(pDevice,ZLOG_LEVEL_INFO,__FUNCTION__" Done OK" );
	return STATE_OK; 
}




// ----------------Установка сервисной информации,инициализация драйвера------------------------------

long __stdcall zrtspSetCameraID(CRTSPCamera* pDevice, long id)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	bool res=false;
	pDevice->LockDevice();
	try
	{
		res = pDevice->SetID(id);
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) "     );
		return -1;
	}
	pDevice->UnlockDevice();
	if (!res) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit Error(-1) "     );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK"     );
	return STATE_OK;
}
long __stdcall zrtspGetCameraID(CRTSPCamera* pDevice)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}

	long res = pDevice->GetID();
	if (!res) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit Error(-1) "     );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK"     );
	return res;
}
long __stdcall zrtspSetCameraType(CRTSPCamera* pDevice,long t)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	bool res=false;
	pDevice->LockDevice();
	try
	{
		res = pDevice->SetCameraType(t);
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) "     );
		return -1;
	}
	pDevice->UnlockDevice();
	if (!res) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit Error(-1) "     );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return STATE_OK;
}
long __stdcall zrtspGetCameraType(CRTSPCamera* pDevice)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}

	long res = pDevice->GetCameraType();
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return res;
}
long __stdcall zrtspSetIPAddrString(CRTSPCamera* pDevice, char* ipaddr_str)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	bool res=false;
	pDevice->LockDevice();
	try
	{
		res = pDevice->SetIPAddrString(ipaddr_str);
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) "     );
		return -1;
	}
	pDevice->UnlockDevice();
	if (!res) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit Error(-1) "     );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_INFO,__FUNCTION__"   Done OK ip addr %s",ipaddr_str);
	return STATE_OK;
}
long __stdcall zrtspGetIPAddrString(CRTSPCamera* pDevice, char *ipaddr_str, long max_length)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	bool res = pDevice->GetIPAddrString(ipaddr_str, max_length);
	if (!res) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit Error(-1) "     );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return STATE_OK;
}
long __stdcall zrtspAddMQueue(CRTSPCamera* pDevice, CZcoreMQueue* mqueue)
{

	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	bool res=false;
	pDevice->LockDevice();
	try
	{
		res=pDevice->AddMQueue(mqueue);
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) "     );
		return -1;
	}
	pDevice->UnlockDevice();
	if (!res) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" pDevice->AddMQueue()return (-1) " );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_INFO,__FUNCTION__"   Done OK"     );
	return STATE_OK;
}
long __stdcall zrtspRemoveMQueue(CRTSPCamera* pDevice, CZcoreMQueue* mqueue)
{

	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	bool res=false;
	pDevice->LockDevice();
	try
	{
		res=pDevice->RemoveMQueue(mqueue);
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) "     );
		return -1;
	}
	pDevice->UnlockDevice();
	if (!res) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" pDevice->RemoveMQueue()return (-1) ");
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_INFO,__FUNCTION__"   Done OK" );
	return STATE_OK;
}
long __stdcall zrtsp_SetLoggerContext(CRTSPCamera* pDevice, ZLOG_CONTEXT logger)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	bool res=false;
	res = pDevice->SetLoggerContext(logger);
	if (!res) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" pDevice->SetLoggerContext()return (-1) "     );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_INFO,__FUNCTION__"   Done OK"     );
	return STATE_OK;
}
ZLOG_CONTEXT __stdcall zrtsp_GetLoggerContext(CRTSPCamera* pDevice)
{
	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit err = -1"     );
		return NULL;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return pDevice->GetLoggerContext();
}
taskmaster_type __stdcall zrtspGetTaskmaster()
{
	TracerOutputText(NULL,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return zrtspTaskmaster;
}
long __stdcall zrtspTaskmaster(long dev, long task, long data)
{
	 
    CRTSPCamera* pDevice= (CRTSPCamera*)dev;
	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
 
   long res=false;

   try
	{
		switch (task)
		{
			case ZCORE_TM_ADDMQUEUE:
			{
 		     TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_ADDMQUEUE "     );
			 if (data!=0)
			 {
				 pDevice->LockDevice();
			     bool bres = pDevice->AddMQueue((CZcoreMQueue*)data);
			     if (!bres)
			     {
				  res=-1;
			     } 
			    pDevice->UnlockDevice();
			 }
			  else res=-1;
			 break;
			}

			case ZCORE_TM_REMOVEMQUEUE:
			{
			   TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_REMOVEMQUEUE "     ); 

			   if (data!=0)
			   {
				   pDevice->LockDevice();
				   bool bres = pDevice->RemoveMQueue((CZcoreMQueue*)data);
				   if (!bres)
				    {
						res=-1;
				    }
				   pDevice->UnlockDevice();
			   }
			   else  
				   res=-1;
				break;
			}

			case ZCORE_TM_GETSID:
			{
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_GETSID "     ); 

				long* pLong = (long*)data;
				
				if (data!=0)
				{	
					*pLong = ZCORE_SID_VFRAME|ZCORE_SID_TRIGGER;
				}
				else
				{
					res=-1;
				}
 				break;
			}

			case ZCORE_TM_GETPREFRAMES:
			{
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_GETSID "     ); 

				long* pLong = (long*)data;
				if (data!=0)
				{
					long* pLong = (long*)data;
					*pLong =0;
				}
				else
				{
					res=-1;
				}
		    	break;
			}

			case ZCORE_TM_GETPOSTFRAMES:
			{
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_GETPOSTFRAMES "     ); 

				long* pLong = (long*)data;
			
				if (data!=0)
				{
			  	 *pLong = 0;
				}
				else
				{
					res=-1;
				}
				break;
			}

			case ZCORE_TM_GETTRIGGERTIMEOUT:
			{
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_GETTRIGGERTIMEOUT "     ); 

				// ne need to lock
				if (data!=0)
				{
					long* pLong = (long*)data;
					*pLong=0;					// don't support trigger 
				}
				else
				{
					res=-1;
				}
				break;
			}

			case ZCORE_TM_GETTRIGGERTIMEOUT_MSEC:
			{
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_GETTRIGGERTIMEOUT_MSEC "     ); 
				
				// no need to lock
				if (data!=0)
				{
					long* pLong = (long*)data;
					*pLong=0;					// don't support trigger 
				}
				else
				{
					res=-1;
				}
				break;
			}

			case ZCORE_TM_GETVFRAMETIMEOUT:
			{
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_GETVFRAMETIMEOUT "     ); 
							
				if (data==0)	
				{
					res=-1;
					break;
				}
				long* pLong = (long*)data; 
				*pLong =1;
		        /*
				
				LARGE_INTEGER ftmr;
				ftmr.QuadPart = 0;
				QueryPerformanceFrequency(&ftmr);
				if (ftmr.QuadPart==0)	{res=-1;break;}
				
				// get capture rate and calculate the timeout
				double fps = pDevice->GetFrameRate();
				//if ( fps<0.5 ) {res=-1;break;}	// the system is stucked, capture rate is below 0.5 fps
				if ( fps<0.5 ) {fps=0.5;}	// the system is stucked, capture rate is below 0.5 fps
				*pLong = (long)( double(1/fps) * ftmr.QuadPart);
							// safety margin
	            */				
				break;
			}

			case ZCORE_TM_GETVFRAMETIMEOUT_MSEC:
			{
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_GETVFRAMETIMEOUT_MSEC "     ); 

				if (data==0)
				{
					res=-1;
					break;
				}

				long* pLong = (long*)data;

				 *pLong = 500;

				break;
			}

			case ZCORE_TM_GETFRAMERATE:
			{
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_GETFRAMERATE "     ); 

				if (data==0)
				{
					res=-1;
					break;
				}

				long* pLong = (long*)data;
			   
				*pLong = 15;

                break;
			}

			case ZCORE_TM_GETCEVENTTIMEOUT:
			{
                TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_GETCEVENTTIMEOUT "     );

				if (data!=0)
				{
					long* pLong = (long*)data;
					*pLong=0;				// does not support hard-trigger
				}
				else
				{
					res=-1;
				}
				break;
			}

			case ZCORE_TM_GETCEVENTTIMEOUT_MSEC:
			{
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_GETCEVENTTIMEOUT_MSEC "     );

				if (data!=0)
				{
					long* pLong = (long*)data;
					*pLong=1000;				
				}
				else
				{
					res=-1;
				}
				break;
			}

			case ZCORE_TM_ISSUPPORTTRIGGER:
			{
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_ISSUPPORTTRIGGER "     );

				if (data!=0)
				{
					long* pLong = (long*)data;
					*pLong = 0;
				}
				else
				{
					res=-1;
				}
				break;
			}

			case ZCORE_TM_TRIGGER:
			{
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_TRIGGER "     );

				res=-1;
				break;
			}

			case ZCORE_TM_LOAD:
			{
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_LOAD "     );

				pDevice->LockDevice();
					res = zrtspLoadDevice(pDevice);
				pDevice->UnlockDevice();

				break;
			}

			case ZCORE_TM_UNLOAD:
			{
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_UNLOAD "     );

				pDevice->LockDevice();
					res = zrtspUnloadDevice(pDevice);
				pDevice->UnlockDevice();

				break;
			}

			case ZCORE_TM_ISRUNNING:
			{
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_ISRUNNING "     );
				
				if (data!=0)
				{
					long* pLong = (long*)data;
					if(pDevice->Device_GetState()==pDevice->DS_RUNNING)
					{
				     *pLong = 1;
					}
					 else  
					     {
						  *pLong = 0;
					     }
				}
				else
				{
					res=-1;
				}
				break;


			}

			case ZCORE_TM_GETDEVICETYPE:
			{
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_GETDEVICETYPE "     );

				if (data!=0)
				{
					long* pLong = (long*)data;
					*pLong = pDevice->GetCameraType();
				}
				else
				{
					res=-1;
				}
				break;
			}

			case ZCORE_TM_GETDEVICEID:
			{
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_GETDEVICEID "     );

				if (data!=0)
				{
					long* pLong = (long*)data;
					*pLong = pDevice->GetID();
				}
				else
				{
					res=-1;
				}
				break;
			}

			case ZCORE_TM_START:
			{

            
				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_START "     );

				zrtspStartGrab(pDevice);

				
				break;
			}

			case ZCORE_TM_PAUSE:
			{

               	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_PAUSE "     );
				
				zrtspStopGrab(pDevice);

				break;
			}

			case ZCORE_TM_STOP:
			{

				TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" ZRTSP - zrtspTaskmaster - ZCORE_TM_STOP "     );

				zrtspStopGrab(pDevice);

				break;
			}

			default:
				TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__",Error(-1) " );
				res=-1;
		}
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) "     );
		return -1;
	}


	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );

	return res;
}

//       ===================================   Type of decoder   =====================================

long __stdcall zrtsp_SetAccelMode(CRTSPCamera* pDevice,int index)//0-x264lib,1-i264(sw),2-i264(hw)
{
	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit err = -1"     );
		return -1;
	}
    TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return pDevice->SetAccelMode(index);

}
long __stdcall zrtsp_GetAccelMode(CRTSPCamera* pDevice)
{

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit err = -1"     );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return pDevice->GetAccelMode();
}

// ----------------------------Определение состояния устройства---------------------------------------


long __stdcall zrtspGetDeviceState(CRTSPCamera* pDevice)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}

	long dv_state=pDevice->Device_GetState();
	std::string state_d;
	switch(dv_state)
	{
	case pDevice->DS_ALLOCATED: 
			state_d="DS_ALLOCATED";
			break;
		case pDevice->DS_DISCONNECTED:
			state_d="DS_DISCONNECTED";
			break;
		case pDevice->DS_CONNECTING:
			state_d="RESET_CONNECTED";
			break;
		case pDevice->DS_PAUSED:     
			state_d="DS_PAUSED";
			break;
		case pDevice->DS_STOPPED:    
			state_d="DS_STOPPED"; 
			break;
		case pDevice->DS_RUNNING: 
			state_d="DS_RUNNING";
			break;
		default:
			state_d="DS_UNKNOW";
	}
	if(pDevice->m_Monitoring)
		{
		  TracerOutputText(pDevice,ZLOG_LEVEL_INFO,__FUNCTION__",device 0x%x state is  %s, link_on=%d ,m_receive=%d",pDevice->DecoderID,state_d.c_str(),pDevice->link_on,pDevice->m_receive);
		}
	    else 
		  TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,"Decoder thread switsh OFF, device 0x%x state is  %s, link_on=%d ,m_receive=%d",pDevice->DecoderID,state_d.c_str(),pDevice->link_on,pDevice->m_receive);
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK");
	return dv_state;
}
long __stdcall zrtspGetVideoState(CRTSPCamera* pDevice)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}

	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK" );
	return pDevice->VS_OK;
}

//-------------------------------------DS_DISCONNECTED------------------------------------------------

long __stdcall zrtspUnloadDevice(CRTSPCamera* pDevice)
{
	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit err = -1"     );
		return -1;
	}
	pDevice->LockDevice();
	try
	{
		if(pDevice->Stopped==false)pDevice->Device_Stop();
        pDevice->LoadDevice();
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) ");
		return -1;
	}

	pDevice->UnlockDevice();
	TracerOutputText(pDevice,ZLOG_LEVEL_INFO,__FUNCTION__"   Done OK"     );
	return STATE_OK;
}
long __stdcall zrtspDestroyDevice(CRTSPCamera* pDevice)
{

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit err = -1" );
		return -1;
	} 
	try
	{
		CRTSPCamera::DestroyDecoder(pDevice);
		pDevice=NULL;
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" exception destroy "  );
		return -1;
	}
	TracerOutputText(NULL,ZLOG_LEVEL_DEBUG,__FUNCTION__" Enter & destroy OK" ); 
	return 0;
}

//------------------------------------------заглушки--------------------------------------------------

long __stdcall zrtspGetLoadDeviceCount(CRTSPCamera* pDevice)
{
     
	if (pDevice==NULL) 
		{
			TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit err = -1"     );
			return -1;
	    }
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return STATE_OK;
}
long __stdcall zrtspSetIPAddr(CRTSPCamera* pDevice, unsigned long ipaddr)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	bool res=false;
	pDevice->LockDevice();
	try
	{
		res = pDevice->SetIPAddr(ipaddr);
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) "     );
		return -1;
	}
	pDevice->UnlockDevice();
	if (!res) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit Error(-1) "     );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return STATE_OK;
}
unsigned long __stdcall zrtspGetIPAddr(CRTSPCamera* pDevice)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	unsigned long res = pDevice->GetIPAddr();
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return res;
}
long __stdcall zrtspSetTriggerTO(CRTSPCamera* pDevice, DWORD to)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	bool res=true;
	pDevice->LockDevice();
	try
	{
	 //res = pDevice->SetTriggerTO(to);
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) "     );
		return -1;
	}
	pDevice->UnlockDevice();
	if (!res) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit Error(-1) "     );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return STATE_OK;
}
DWORD __stdcall zrtspGetTriggerTO(CRTSPCamera* pDevice)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	long res = 0; //pDevice->GetTriggerTO();
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return res;
}
long __stdcall zrtspSetLastTCount(CRTSPCamera* pDevice, DWORD tc)
{

	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	bool res=false;
	pDevice->LockDevice();
	try
	{
		//res = pDevice->SetLastTCount(tc);
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) "     );
		return -1;
	}
	pDevice->UnlockDevice();
	if (!res) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit Error(-1) "     );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return STATE_OK;
}
DWORD __stdcall zrtspGetLastTCount(CRTSPCamera* pDevice)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	DWORD res = 0;//pDevice->GetLastTCount();
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return res;
}
long __stdcall zrtspGetTriggerState(CRTSPCamera* pDevice)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}

	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return (long)0;//(pDevice->GetTriggerState());
}
long __stdcall zrtspGetEventState(CRTSPCamera* pDevice)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}

	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	//return (long)(pDevice->GetEventState()); TODO
	return 0;
}
long __stdcall zrtspIsCameraConnected(CRTSPCamera* pDevice)
{
	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return 0;
	//return pDevice->IsCameraConnected(); //TODO
	return 0;
}
long __stdcall zrtspSetPreFrames(CRTSPCamera* pDevice, long pre_frames)
{
	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return 0;
}
long __stdcall zrtspGetPreFrames(CRTSPCamera* pDevice)
{
	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return 0;
}
long __stdcall zrtspSetPostFrames(CRTSPCamera* pDevice,long post_frames)
{
	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return 0;
}
long __stdcall zrtspGetPostFrames(CRTSPCamera* pDevice)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return 0;
}
unsigned long __stdcall zrtspGetPartNumber(CRTSPCamera* pDevice)
{
  
 
 if (pDevice==NULL) 
	 {
		 TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		 return 0;
     }

  unsigned long res=0;
  pDevice->LockDevice();
  try
	{
		res = pDevice->GetPartNumber();
	}
	catch(...)
	{

	}
   pDevice->UnlockDevice();
   
   TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
   return res; 
}
unsigned long __stdcall zrtspGetPartVersion(CRTSPCamera* pDevice, char* buffer, long buffer_length)
{

	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return 0;
	}

	unsigned long res=0;
	pDevice->LockDevice();
	try
	{
		res = pDevice->GetPartVersion(buffer, buffer_length);
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) "     );
		return -1;
	}
	pDevice->UnlockDevice();

	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return res; 
}
unsigned long __stdcall zrtspGetPartRevision(CRTSPCamera* pDevice, char* buffer, long buffer_length)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return 0;
	}

	unsigned long res=0;
	pDevice->LockDevice();
	try
	{
		res = pDevice->GetPartRevision(buffer, buffer_length);
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) "     );
		return -1;
	}
	pDevice->UnlockDevice();

	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return res; 
}
unsigned long __stdcall zrtspGetSerialNumber(CRTSPCamera* pDevice)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return 0;
	}

	unsigned long res=0;
	pDevice->LockDevice();
	try
	{
		res = pDevice->GetSerialNumber();
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) "     );
		return -1;
	}
	pDevice->UnlockDevice();

	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return res; 
}
unsigned long __stdcall zrtspGetUniqueId(CRTSPCamera* pDevice)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return 0;
	}

	unsigned long res=0;
	pDevice->LockDevice();
	try
	{
		res = pDevice->GetUniqueId();
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) "     );
		return -1;
	}
	pDevice->UnlockDevice();

	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return res; 
}
unsigned long __stdcall zrtspGetCameraName(CRTSPCamera* pDevice, char* buffer, long buffer_length)
{
	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL " );
		return 0;
	}

	if(buffer_length<0)
  	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.buffer_length < 0 " );
		return 0;
	}

	unsigned long res=0;
	pDevice->LockDevice();
	try
	{
		res = pDevice->GetCameraName(buffer,buffer_length);
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) "     );
		return -1;
	}
	pDevice->UnlockDevice();

	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return res; 
}
unsigned long __stdcall zrtspGetModelName(CRTSPCamera* pDevice, char* buffer, long buffer_length)
{
	 

	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return 0;
	}

	unsigned long res=0;
	pDevice->LockDevice();
	try
	{
		res = pDevice->GetModelName(buffer, buffer_length);
	}
	catch(...)
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) "     );
		return -1;
	}
	pDevice->UnlockDevice();

	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return res; 
}
long __stdcall zrtspIsZamirCamera(CRTSPCamera* pDevice)
{
	if (pDevice==NULL) return -1;
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return STATE_OK;
}
long __stdcall zrtspSetPacketSize(CRTSPCamera* pDevice, long  packetSize)
{
	if (pDevice==NULL) return -1;
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
   	return 0;
}
long __stdcall zrtspGetPacketSize(CRTSPCamera* pDevice, long* pPacketSize)
{
	if (pDevice==NULL) return -1;
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
	return 0;
}
long __stdcall zrtspSetStreamBytesPerSecond(CRTSPCamera* pDevice, long  streamBytesPerSecond)
{
	if (pDevice==NULL) return -1;
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
  	return 0;
}
long __stdcall zrtspGetStreamBytesPerSecond(CRTSPCamera* pDevice, long* pStreamBytesPerSecond)
{
	if (pDevice==NULL) return -1;
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
   	return 0;
}
long __stdcall zrtspSetStreamFrameRateConstrain(CRTSPCamera* pDevice, long streamFrameRateConstrain)
{
	if (pDevice==NULL) return -1;
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
 	return 0;
}
long __stdcall zrtspGetStreamFrameRateConstrain(CRTSPCamera* pDevice, long* pStreamFrameRateConstrain)
{
	if (pDevice==NULL) return -1;
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__"   Done OK"     );
  	return 0;
}
long __stdcall zrtspSetCameraNameString(CRTSPCamera* pDevice, char* camera_name)
{
	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}

	bool res=false;
	pDevice->LockDevice();

	try
	{
		res = pDevice->SetCameraNameString(camera_name);
	}
	catch(...)
	{

	}

	pDevice->UnlockDevice();
	if (!res) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit Error(-1) "     );
		return -1;
	}
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK,Camera name %s "     ,camera_name);
	return STATE_OK;
}
long __stdcall zrtspGetCameraNameString(CRTSPCamera* pDevice, char* camera_name, long max_length)
{
	if (pDevice==NULL) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Error.pDevice=NULL "     );
		return -1;
	}
	
	bool res = pDevice->GetCameraNameString(camera_name);
	if (!res) 
	{
		TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__" Exit Error(-1) "     );
		return -1;
	}

	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK,Camera name %s ",camera_name);
	return STATE_OK;
}
long __stdcall zrtspSetTimeStampDelta( 
										CRTSPCamera* pDevice,
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
    if (pDevice==NULL) return -1; 
	pDevice->setTimeStampDelta(YearDelta, MonthDelta, DayOfWeekDelta, DayDelta, HourDelta, MinuteDelta, SecondDelta, MillisecondsDelta);
	TracerOutputText(pDevice,ZLOG_LEVEL_DEBUG,__FUNCTION__" Done OK"     );
	return 0;
}

long __stdcall zrtspSetTimeStampOffset(CRTSPCamera* pDevice, long millisecondsOffset)
{
	if (pDevice==NULL) return -1;
	fprintf(stderr,"API::zrtspSetTimeStampOffset %d msek \n", millisecondsOffset);
 	pDevice->setTimeStampDelta(0, 0, 0, 0, 0, 0, 0, millisecondsOffset);
	TracerOutputText(pDevice,ZLOG_LEVEL_INFO,__FUNCTION__" Done OK"     );
	return 0;
}

long __stdcall zrtspSetGainMode(CRTSPCamera* pDevice, long gainMode)
{
	
	if (pDevice==NULL) return -1;

	//bool res = pDevice->SetGainMode( (CRTSPCamera::GainMode) gainMode);
	//return (res ? 0 : -1);

	return STATE_OK;
}
long __stdcall zrtspGetGainMode(CRTSPCamera* pDevice, long* pGainMode)
{
	if (pDevice==NULL) return -1;
	if (pGainMode==NULL) return -1;

	(*pGainMode) = 1;//pDevice->GetGainAutoMax();

	return STATE_OK;
}
long __stdcall zrtspSetGainAutoMax(CRTSPCamera* pDevice, long  gainAutoMax)
{
	
	if (pDevice==NULL) return -1;

	//bool res = pDevice->SetGainAutoMax(gainAutoMax);
	//return (res ? 0 : -1);

	return STATE_OK;
}
long __stdcall zrtspGetGainAutoMax(CRTSPCamera* pDevice, long* pGainAutoMax)
{
	
	if (pDevice==NULL) return -1;
	if (pGainAutoMax==NULL) return -1;

	(*pGainAutoMax) = 1;//pDevice->GetGainAutoMax();
	
	return STATE_OK;
}
long __stdcall zrtspSetGainAutoMin(CRTSPCamera* pDevice, long  gainAutoMin)
{
	if (pDevice==NULL) return -1;

	//bool res = pDevice->SetGainAutoMin(gainAutoMin);
	//return (res ? 0 : -1);

	return STATE_OK;
}
long __stdcall zrtspGetGainAutoMin(CRTSPCamera* pDevice, long* pGainAutoMin)
{
	
	if (pDevice==NULL) return -1;
	if (pGainAutoMin==NULL) return -1;

	(*pGainAutoMin) = 1;//pDevice->GetGainAutoMin();
	
	return STATE_OK;
}
long __stdcall zrtspSetGainValue(CRTSPCamera* pDevice, long  gainValue)
{
	if (pDevice==NULL) return -1;
	//bool res = pDevice->SetGainValue(gainValue);
    //return (res ? 0 : -1);
	return STATE_OK;
}
long __stdcall zrtspGetGainValue(CRTSPCamera* pDevice, long* pGainValue)
{
	if (pDevice==NULL) return -1;
	if (pGainValue==NULL) return -1;
	(*pGainValue) =1;// pDevice->GetGainValue();
    return STATE_OK;
}

long __stdcall zrtspSetAcquisitionMode(CRTSPCamera* pDevice,AcquisitionMode am)
{
	if (pDevice==NULL) return false;
	bool res=false;
	pDevice->LockDevice();
		try
		{
			res = true;//pDevice->SetAcquisitionMode(am);
		}
		catch(...)
		{
			TracerOutputText(pDevice,ZLOG_LEVEL_ERROR,__FUNCTION__"catch Error(-1) "     );
			return -1;
		}
	
	pDevice->UnlockDevice();

	if (!res) return -1;
	return 0; // OK
}
long  __stdcall zrtspGetAcquisitionMode(CRTSPCamera* pDevice,AcquisitionMode* pAM)
{
	return 0; 
}
long __stdcall zrtspGetCountDropedFrames(CRTSPCamera* pDevice,int64_t *count_drop_frame)
{
 	if (pDevice==NULL) return -1;
	if (count_drop_frame==NULL) return -1;

	(*count_drop_frame) =pDevice->GetCountDropedFrame();

	return STATE_OK;
}
long __stdcall zrtspResetCounterDropFrames(CRTSPCamera* pDevice)
{
 	if (pDevice==NULL) return -1;
    pDevice->ResetCounterDropFrames();
	return STATE_OK;
}
long __stdcall zrtspGetCorruptFramesCount(CRTSPCamera *pDevice,long *pCount)
{
	if (pDevice == NULL) return -1;
	if (pCount == NULL) return -1;

	(*pCount) = pDevice->GetCountCorruptFrame();

	return STATE_OK;
}
long __stdcall zrtspResetCorruptFramesCounter(CRTSPCamera* pDevice)
{
	if (pDevice==NULL) return -1;
	pDevice->ResetCounterCorruptFrames();
	return STATE_OK;
}


//#define DBG_FILE 1 
//#define DBG_VIEW 0 
//#define DBG_LM 0
void TracerOutputText(CRTSPCamera* dvs, ZLOG_LEVEL level, LPCTSTR Format, ...)
{

#ifdef DBG_FILE
	if(level!=ZLOG_LEVEL_INFO&&level!=ZLOG_LEVEL_ERROR)return;
	time_t t = time(0);
	struct tm * now = localtime( & t );
	std::stringstream tm;
	std::string h;
	std::string m;
	std::string s;
	tm<<now->tm_hour;
	tm>>h;
	if(now->tm_hour<10)h.insert(0,"0",1);
	tm.clear();
	tm<<now->tm_min;
	tm>>m;
	if(now->tm_min<10)m.insert(0,"0",1);
	tm.clear();
	tm<<now->tm_sec;
	tm>>s;
	if(now->tm_sec<10)s.insert(0,"0",1);
	tm.clear();	 
	std::string status="debug/>";
	switch(level)
	{
	 case ZLOG_LEVEL_INFO:
		status="inform/>";
		break;
	 case ZLOG_LEVEL_ERROR:
		status="ERROR!/>";
		break;
	}
	std::string full_str_id(h+":"+m+":"+s+"  "+	"RTSP_CLN:" + status );
	CHAR str[MAX_STRING_LENGTH]={0};
	memcpy(str,full_str_id.c_str(),full_str_id.length());
	int ln=full_str_id.length();
	CHAR* nStr=&str[ln];
	va_list vaList;
	ZLOG_CONTEXT _logger={0};
	va_start(vaList, Format);
	_vstprintf(nStr, Format, vaList);
	std::string filtr=str;
	//size_t found = filtr.find("zrtsp");
	//if(found!=std::string::npos)
	//	{
	//		va_end(vaList);
	//		return;
	//    }
	ln=strlen(str);
	str[ln]='\n';
	FILE *f = fopen("A1_izClntRtsp.log", "a");
	fprintf(f,"%s",str);
	fflush(f);
	fclose(f);
#else
	CHAR str[MAX_STRING_LENGTH]={"RTSP_CLN:"};
	CHAR* nStr=&str[9];
	va_list vaList;
	ZLOG_CONTEXT _logger={0};
	va_start(vaList, Format);
	_vstprintf(nStr, Format, vaList);
	int ln=strlen(str);
	str[ln]='\n';
  #ifdef DBG_VIEW
	  OutputDebugString(str);
  #else
	if(dvs!=NULL)
		_logger= dvs->srv->m_logger;
  #endif
#endif
	va_end(vaList);
}
BOOL APIENTRY DllMain( HMODULE hModule,DWORD  ul_reason_for_call,LPVOID lpReserved)
{
	
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		   TracerOutputText(NULL,ZLOG_LEVEL_INFO,__FUNCTION__" DLL_PROCESS_ATTACH "     ); 
        break;
		case DLL_THREAD_ATTACH:
        break;
       	case DLL_THREAD_DETACH:
        break;
		case DLL_PROCESS_DETACH:
           TracerOutputText(NULL,ZLOG_LEVEL_INFO,__FUNCTION__" DLL_PROCESS_DEATTACH "     ); 
       	break;
	}

	return TRUE;
}


