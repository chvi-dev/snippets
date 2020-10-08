//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "mSeaDet.h"
#include "DbManager.h"
#include "Math.hpp"
#include <jpeg.hpp>
#include "Registry.hpp"
#include "systdate.h"
#include "neodg.h"
#include "LoggIn.h"
#include "ThreadSynchro.h"
#define MaxCountImages 500
#define maxMem 12000000
#pragma alias "@System@Win@Registry@TRegistry@SetRootKey$qqrpv" = \
"@System@Win@Registry@TRegistry@SetRootKey$qqrp6HKEY__"
#pragma alias "@Vcl@Dialogs@TPrintDialog@Execute$qqrpv" = \
"@Vcl@Dialogs@TPrintDialog@Execute$qqrp6HWND__"
#define db_key "\\SeaDetobg\\LoadDataBase\\"
#define md_key "\\SeaDetobg\\LoadPicture\\"
#define forcegoto(label) asm jmp label
//---------------SYSTEM_PERFORMANCE_INFORMATION-----------------------
#define SystemBasicInformation       0
#define SystemPerformanceInformation 2
#define SystemTimeInformation        3
#define Li2Double(x) ((double)((x).HighPart) * 4.294967296E9 + (double)((x).LowPart))
typedef struct
{
DWORD   dwUnknown1;
ULONG   uKeMaximumIncrement;
ULONG   uPageSize;
ULONG   uMmNumberOfPhysicalPages;
ULONG   uMmLowestPhysicalPage;
ULONG   uMmHighestPhysicalPage;
ULONG   uAllocationGranularity;
PVOID   pLowestUserAddress;
PVOID   pMmHighestUserAddress;
ULONG   uKeActiveProcessors;
BYTE    bKeNumberProcessors;
BYTE    bUnknown2;
WORD    wUnknown3;
} SYSTEM_BASIC_INFORMATION;

typedef struct
{
LARGE_INTEGER   liIdleTime;
DWORD           dwSpare[76];
} SYSTEM_PERFORMANCE_INFORMATION;

typedef struct
{
LARGE_INTEGER liKeBootTime;
LARGE_INTEGER liKeSystemTime;
LARGE_INTEGER liExpTimeZoneBias;
ULONG         uCurrentTimeZoneId;
DWORD         dwReserved;
} SYSTEM_TIME_INFORMATION;


typedef LONG (WINAPI *PROCNTQSI) (INT,PVOID,ULONG,PULONG);
PROCNTQSI NtQuerySystemInformation;

SYSTEM_PERFORMANCE_INFORMATION SysPerfInfo;
SYSTEM_TIME_INFORMATION        SysTimeInfo;
SYSTEM_BASIC_INFORMATION       SysBaseInfo;
double                         dbIdleTime;
double                         dbSystemTime;
LONG                           status;
LARGE_INTEGER                  liOldIdleTime = {0,0};
LARGE_INTEGER                  liOldSystemTime = {0,0};
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TSeaDetobg *SeaDetobg;
TPc_Vision *pcVision;
TPc_Vision::ImageInfo chkImgInfo;
TPc_Vision::ImageInfo ctrlImgInfo;
TPc_Vision::Work_srf w_size;
TOpenDialogEx *odDataBase;
TOpenDialogEx *odImageObj;
TSaveDialogEx *sdDataBase;
TSaveDialogEx *sdLogResult;
SynchroVCL *_synchro;
//using namespace std;
//---------------------------------------------------------------------------
void __fastcall TSeaDetobg::CPU_Usage()
{
 status = NtQuerySystemInformation(SystemTimeInformation,&SysTimeInfo,sizeof(SysTimeInfo),0);
 status = NtQuerySystemInformation (SystemPerformanceInformation,&SysPerfInfo,sizeof(SysPerfInfo),NULL);
 if (liOldIdleTime.QuadPart != 0)
       {
        dbIdleTime = Li2Double(SysPerfInfo.liIdleTime) - Li2Double(liOldIdleTime);
        dbSystemTime = Li2Double(SysTimeInfo.liKeSystemTime) - Li2Double(liOldSystemTime);
        dbIdleTime = dbIdleTime / dbSystemTime;
        dbIdleTime = 100.0 - dbIdleTime * 100.0 / (double)SysBaseInfo.bKeNumberProcessors + 0.5;
       }
 // cgProc1->Progress=(int)dbIdleTime;
 // Label49->Caption=IntToStr(cgProc1->Progress);
 liOldIdleTime = SysPerfInfo.liIdleTime;
 liOldSystemTime = SysTimeInfo.liKeSystemTime;
}
__fastcall TSeaDetobg::TSeaDetobg(TComponent* Owner)
	: TForm(Owner)
{

}
//---------------------------------------------------------------------------
void __fastcall TSeaDetobg::Decode_Jpg(WideString f_name)
 {

  /*if(SmPhotoImgIn->Picture->Bitmap->Handle)
   {
      SmPhotoImgIn->Picture->Bitmap->Dormant();
      SmPhotoImgIn->Picture->Bitmap->FreeImage();
      SmPhotoImgIn->Picture->Bitmap->Assign(NULL);
   }
  if(ImHist->Picture->Bitmap->Handle)
   {
    ImHist->Picture->Bitmap->Dormant();
    ImHist->Picture->Bitmap->FreeImage();
    ImHist->Picture->Bitmap->Assign(NULL);
   }
   if(ImHistCtrl->Picture->Bitmap->Handle)
    {
      ImHistCtrl->Picture->Bitmap->Dormant();
     ImHistCtrl->Picture->Bitmap->FreeImage();
     ImHistCtrl->Picture->Bitmap->Assign(NULL);
    }
   if(CheckedIm->Picture->Bitmap->Handle)
    {
     CheckedIm->Picture->Bitmap->Dormant();
     CheckedIm->Picture->Bitmap->FreeImage();
     CheckedIm->Picture->Bitmap->Assign(NULL);
    }
    if(ImDb->Picture->Bitmap->Handle)
     {
      ImDb->Picture->Bitmap->Dormant();
      ImDb->Picture->Bitmap->FreeImage();
      ImDb->Picture->Bitmap->Assign(NULL);
     }
    if(bmpX->Handle)
     {
      bmpX->Dormant();
      bmpX->FreeImage();
      bmpX->Assign(NULL);
     }*/
  EdSlName->Caption=ExtractFileName(AnsiString(f_name));
  IplImage *image=cvLoadImage(AnsiString(f_name).c_str(),CV_LOAD_IMAGE_UNCHANGED);
  TPc_Vision::Smooth_metods smooth={0};
  smooth.src=image;
  smooth.metod=3;
  image=pcVision->SetSmooth(smooth);
  SmPhotoImgIn->Picture->Bitmap->Handle=pcVision->IPL_to_BMP(image,4);
  ZeroMemory(&chkImgInfo,sizeof(chkImgInfo));
  pcVision->GetImageQuality(image,chkImgInfo);
  WideString val=FloatToStrF(chkImgInfo.Yv_Brigthness,ffFixed,6,3);
  StaticText1->Caption=val;
  val=FloatToStrF(chkImgInfo.Cv_Contrast,ffFixed,6,3);
  StaticText2->Caption=val;
  val=FloatToStrF(chkImgInfo.RichTone,ffFixed,6,3);
  StaticText3->Caption=val;
  SpeedButton1->Enabled=true;
  SpeedButton2->Enabled=true;
  if(loadImage)
   {
    loadImage=false;
    pcVision->CleanMemoryIm(image);
    return;
   }
  bool w=false;
  bool h=false;
  int a=((float)image->width)/jpgScale;
  int b=((float)image->height)/jpgScale;
  if((a&1)>0)w=true;
  if((b&1)>0)h=true;
  IplImage* _RSrc=cvCreateImage(cvSize(a-w,b-h), 8, 3 );
  cvResize(image,_RSrc,CV_INTER_LINEAR);
  pcVision->CleanMemoryIm(image);
  int z=0;
  if(jpgScale<3.8)z=2;
   else z=1;
  int ROI_x =9*z;
  int ROI_y =9*z;
  CvRect roi={ROI_x,ROI_y,_RSrc->width-2*ROI_x,_RSrc->height-2*ROI_y};
  cvSetImageROI(_RSrc,roi);
  image=cvCreateImage(cvGetSize(_RSrc),IPL_DEPTH_8U,3);
  cvCopy(_RSrc,image,NULL);
  cvResetImageROI(_RSrc);
  pcVision->CleanMemoryIm(_RSrc);
  bmpX->Handle=pcVision->IPL_to_BMP(image,0);
  pcVision->CleanMemoryIm(image);
 }

void __fastcall TSeaDetobg::FormCreate(TObject *Sender)
{
  t_min = 0;
  it = 4;
  jpgScale=3.0;
 _synchro=new SynchroVCL(true);
 _synchro->hEvent[0]=CreateEvent(NULL,false,false,"ev_Synchro");
 _synchro->hEvent[1]=CreateEvent(NULL,false,false,"ev_Post");
 _synchro->Resume();
  pcVision=new TPc_Vision(jpgScale);
  cvInitFont(&pcVision->mFont,CV_FONT_HERSHEY_DUPLEX, .2*4/jpgScale, .5*4/jpgScale, 0, 1, 0);  //CV_FONT_HERSHEY_COMPLEX_SMALL
  StaticText3->Caption="";
  odDataBase=new TOpenDialogEx(this);
  odDataBase->chBox->Caption="Remember root of connection";
  odDataBase->Title="Connect to Database";
  odImageObj=new TOpenDialogEx(this);
  odImageObj->chBox->Caption="Remember root of selection";
  odImageObj->Title="Open imaje for added to database";
  odImageObj->Options<<ofAllowMultiSelect;
  sdDataBase=new TSaveDialogEx(this);
  sdDataBase->chBox->Caption="Remember root of storage table";
  sdDataBase->Title="Save creating database";
  sdLogResult=new TSaveDialogEx(this);
  sdLogResult->panel->Visible=false;
  sdLogResult->chBox->Caption="Remember root of storage table";
  sdLogResult->Title="Save creating database";
  ZeroMemory(&mode,sizeof(mode));
  pMS=new TMemoryStream;
  r_Blob=new TMemoryStream;
  pMS->Clear();
  FilesX=new TStringList();
  FilesX->Sorted=true;
  Application->Title="Salamander° CV(v1.4)";
  bmpX=new Graphics::TBitmap();
  bmpX->PixelFormat=pf24bit;
  bmpX->HandleType=bmDIB;
  ControlStyle<<csOpaque;
  ImDb->ControlStyle<<csOpaque;
  SmPhotoImgIn->ControlStyle<<csOpaque;
  I_Panel->ControlStyle<<csOpaque;
  I_Left=I_Panel->Left;
 for(int i=0;i<4;i++)
 {
  BR_ObjkeyPoint.push_back(brObjkeyPoint[i]);
  BR_DbkeyPoint.push_back(brDbkeyPoint[i]);
  OR_ObjkeyPoint.push_back(orObjkeyPoint[i]);
  OR_DbkeyPoint.push_back(orDbkeyPoint[i]);
 }
 BR_detector=auto_ptr<cv::FeatureDetector>(new cv::BRISK(25,0,1.0));
 BR_extractor=auto_ptr<cv::DescriptorExtractor>(new cv::BRISK(25,0,1.0));

  //int _nfeatures
  //float _scaleFactor 1
  //int _nlevels       2
  //int _edgeThreshold 3
  //int _firstLevel    4
  //int _WTA_K         5
  //int _scoreType     6
  //int _patchSize     7


 OR_detector=auto_ptr<cv::FeatureDetector>(new cv::ORB(1000));
 OR_extractor=auto_ptr<cv::DescriptorExtractor>(new cv::ORB(1000));
 //CPU Usage
 NtQuerySystemInformation = (PROCNTQSI)GetProcAddress(GetModuleHandle ("ntdll"), "NtQuerySystemInformation");
 if (!NtQuerySystemInformation)return;
 status = NtQuerySystemInformation( SystemBasicInformation,&SysBaseInfo,sizeof(SysBaseInfo),NULL);
 if (status != NO_ERROR)
  {
    Close();
    return;
  }
 hProcess=GetCurrentProcess();
 hHeapProcess=GetProcessHeap();
}
void __fastcall TSeaDetobg::CheckWriteObj()
 {
   if(Logger->RichEdit1->Lines->Count>32767)
    {
     Logger->RichEdit1->Lines->BeginUpdate();
     Logger->SpeedButton1->Perform(WM_LBUTTONDOWN, 0, 0);
     Logger->SpeedButton1->Perform(WM_LBUTTONUP, 0, 0);
    }

 }
HRESULT __fastcall TSeaDetobg::RunTransAction(int &a)
{
  HRESULT reS=RECORD_IS_EQUAL;
  DWORD replay=IDYES;
  if(DbManager->UniTable1->Active==false)
   {
    DbManager->UniTable1->Open();
    if(!DbManager->UniTable1->Active)
     {
         //Error connect
         return ERROR_DISCARDED;
     }

   }
  DbManager->UniTransaction1->StartTransaction();
  int cnRecords=DbManager->UniTable1->RecordCount;
  bool lastObg=LoadPicture(a);
  Label12->Caption="Object under identification";
  LoadCheckedObject(&CheckedObject);
  if(CheckedObject.e==E_FAIL)
   {
    reS=-201;
    CheckedIm->Picture->Bitmap->Assign(ImageErCn->Picture->Graphic);
   }
  else
      {
       CheckedObject.lastObj=lastObg;
       ////CheckedIm->Picture->Bitmap->Handle=pcVision->IPL_to_BMP(CheckedObject.cmpImg_2D,0);
       CheckedIm->Picture->Bitmap->Handle=pcVision->IPL_to_BMP(CheckedObject.cmpImg_3ChDCT,0);
       reS=ReadObjectFromDB(&CheckedObject);
      }
  switch(reS)
  {
      case -201:
      {
        TColor clr=clRed;
        Label12->Caption="Error ctreate countor";
        Label12->Color=clBtnFace;
        Label12->Font->Color=clBlack;
        for(int i=0;i<4;i++)
         {
          Label12->Font->Color=clr;
          IdAntiFreeze1->Process();
          IdAntiFreeze1->Sleep(250);
          Label12->Font->Color=clBlack;
          IdAntiFreeze1->Process();
          IdAntiFreeze1->Sleep(250);
         }
       	AnsiString mess="\n•Checked image:\n"+EdSlName->Caption+": Detection contour error";
		Logger->WriteLn(mess,clRed,IS_TEXT_LOG);
		Logger->WriteLn("",clBlack,IS_TEXT_LOG);
        Logger->Tag=-13;
        IdAntiFreeze1->Process();
        break;
      }
      case ERROR_SIGNAL_PENDING:
      return ERROR_SIGNAL_PENDING;
      isEqual:
      case RECORD_IS_EQUAL:
       {
        if(StartIdBtn->Tag==false)
        {
         Application->MessageBox(L"Found an equal image",L"The result of the identification",MB_OK);
        }
        Logger->WriteLn(L"\nFound an equal image",clBlue,false);
        AnsiString mess="•Checked image:\n"+EdSlName->Caption;
        Logger->WriteLn(mess,clPurple,false);
        mess="•DataBase image\n"+ StNameObj->Caption;
        Logger->WriteLn(mess,clGreen,false);
        mess="Hamming len "+ AnsiString(distHamm);
        mess+="\n";
        Logger->WriteLn(mess,clPurple,false);
        mess="Feature detectors confirm equal images";
        mess+="\n";
        switch(CheckedObject.filter_type)
        {
          case 20:
            mess+="Full hash";
          break;
          case 21:
             mess+="BRISK & Full hash";
          break;

          case 22:
           mess+="ORB & Full hash";
          break;
          case 23:
           mess+="BRISK & ORB & Full hash";
          break;

        }
        Logger->WriteLn(mess,clOlive,false);
        GetAllocatedMemory(Logger->Label6);
       }
      break;
      case RECORD_IS_COINCIDES:
      if(StartIdBtn->Tag==false)
      {
       replay=MessageBoxW(Handle,L"Found an coincides image.\n\nAdd image into database?",
                                  L"The result of the identification",
                                   MB_ICONQUESTION|MB_YESNO);
      }
      Logger->WriteLn(L"\nFound an coincides image",clBlue,false);
       {
        AnsiString mess="•Checked image:\n"+EdSlName->Caption;
        Logger->WriteLn(mess,clPurple,false);
        mess="•DataBase image\n"+ StNameObj->Caption;
        Logger->WriteLn(mess,clGreen,false);
        mess="Hamming len "+ AnsiString(distHamm);
        Logger->WriteLn(mess,clPurple,false);
        mess="Detected with the  ";

        switch(CheckedObject.filter_type)
         {
          case 1:
           mess+="BRISK detector";
          break;
          case 2:
           mess+="ORB detector";
          break;
          case 3:
           mess+="BRISK & ORB detectors";
          break;
          case 10:
           mess+="Full hash & histograme";
          break;
          case 11:
            mess+="BRISK confirm \"FlHash & histCmp\"";
          break;
          case 12:
             mess+="ORB confirm \"FlHash & histCmp\"";
          break;
          case 13:
           mess="BRISK & ORB confirm \"FlHash & histCmp\"";
          break;
          case 14:
           mess+="Block hash & histogrames compare";
          break;
          case 15:
           mess+="BRISK confirm \"BlHash & histCmp\"";
          break;
          case 16:
             mess+="ORB confirm \"BlHash & histCmp\"";
          break;
          case 17:
           mess="BRISK & ORB confirm \"BlHash & histCmp\"";
          break;
         }
        Logger->WriteLn(mess,clOlive,false);
       }
      if(replay==IDYES)
          {
           TStringField* v_Number=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("Number"));
           AnsiString Val=v_Number->AsAnsiString;
           int ps=Val.LastDelimiter("“");
           Val=Val.SubString(1,ps-1);
           if(AddObjectCtrl(cnRecords,&CheckedObject,Val))
            {
              Logger->ProgressBar1->Progress--;
              Logger->StaticText2->Caption=WideString(Logger->ProgressBar1->Progress);
              StaticTextRmDw->Caption=WideString(Logger->ProgressBar1->Progress);
              ClearMemory(&CheckedObject);
              return S_OK;
            }
		   Logger->WriteLn(EdSlName->Caption+" added in group",clOlive,IS_TEXT_LOG);
           Logger->WriteLn("\n",clOlive,false);
          }
      break;
      default:
      if(StartIdBtn->Tag==false)
          {
           replay=MessageBoxW(Handle,L"Not found an identical image.\n\nAdd image into database?",
                                      L"The result of the identification",
                                       MB_ICONQUESTION|MB_YESNO);
          }
         if(replay==IDYES)
          {
		   Logger->WriteLn(EdSlName->Caption+" added as template",clBlack,IS_TEXT_LOG);
           AddObjectCtrl(-1,&CheckedObject,"");
		  }

  }

  IdAntiFreeze1->Process();
  DbManager->UniTransaction1->Commit();
  if(FilesX->Count==1)return S_OK;
  Logger->ProgressBar1->Progress--;
  Logger->StaticText2->Caption=WideString(Logger->ProgressBar1->Progress);
  StaticTextRmDw->Caption=WideString(Logger->ProgressBar1->Progress);
  Update();
  MemoryFree();

  return S_OK;
}
__int64 __fastcall TSeaDetobg::GetAllocatedMemory(TLabel *label)
{
 TMemoryManagerState  mSt;
 TSmallBlockTypeState mStb;
 GetMemoryManagerState(mSt);
 __int64 mVal=mSt.TotalAllocatedMediumBlockSize + mSt.TotalAllocatedLargeBlockSize;
 for(int i=0;i<54;i++)
  {
   mStb=mSt.SmallBlockTypeStates[i];
   int sz=mStb.UseableBlockSize * mStb.AllocatedBlockCount;
   mVal=mVal+ sz;
  }
  label->Caption=WideString(mVal/1024)+" K";
  return mVal;
}
void __fastcall TSeaDetobg::MemoryFree()
{
  MEMORY_BASIC_INFORMATION mem_inf={0};
  DWORD memVal=GetVirtualAllocSize(mem_inf,true);
}
DWORD __fastcall TSeaDetobg::GetVirtualAllocSize(MEMORY_BASIC_INFORMATION &memInfo,bool reset)
 {
  /* //GetMemoryMap(aMemoryMap);
   BYTE *Base=NULL;
   int sz=sizeof(MEMORY_BASIC_INFORMATION);
   PVOID AllocationBase=NULL;
   DWORD memVal=0;
   DWORD res=VirtualQuery(Base,&memInfo,sz);
   while(res==sz)
	{
	 if(memInfo.State==MEM_COMMIT&&memInfo.Type==MEM_PRIVATE)
	  {
	   if(memInfo.Protect==PAGE_READWRITE)
		 {

		   DWORD* oPr=&memInfo.Protect;
		   bool r;

             DWORD _Base=(DWORD)Base;
			 if(HIWORD(Base)>0x70000)
              {
               r=VirtualFree(Base,0,MEM_DECOMMIT);
			   if(r)
                {
                 int hpCount=HeapCompact(hHeapProcess,0);
                 Base=NULL;
				 res=VirtualQuery(Base,&memInfo,sz);
                 continue;
                }
              }


		 }
	  }
	 Base+=memInfo.RegionSize;
	 res=VirtualQuery(Base,&memInfo,sz);
	}
	return  memVal;  */
 }
void __fastcall TSeaDetobg::MatchDbImg()
{
 Imagesdatabase1->Enabled=false;
 int a=0;
 HRESULT reS=S_OK;
 int sIn;
 if(PnNavg->Visible)
   {
    StartIdBtn->Tag=1;
    PnNavg->Visible=false;
   }
 Logger->ProgressBar1->MaxValue=FilesX->Count;
 Logger->ProgressBar1->Progress=FilesX->Count;
 Logger->ProgressBar1->Visible=true;
 do
 {
  if(CloseMatch)
			  return;
  ClearMemory(&CheckedObject);
  int hpCount=HeapCompact(hHeapProcess,0);
  int memComitSz=GetAllocatedMemory(Logger->Label5);
  if(memComitSz>maxMem)
   {

   }

  HRESULT res;
  CheckWriteObj();
  res=RunTransAction(a);
  /*try
   {


    AnsiString message="Clear memory";
    throw message;
   }
   catch(AnsiString msg)
    {
     if(msg=="Clear memory")
      {

      }
    }*/
  if(res==ERROR_SIGNAL_PENDING)break;
  a++;
 }
  while(a<FilesX->Count);
  I_Procc+=a;
  if(DbManager->UniTable1->Active)
   {
    DbManager->UniTable1->ApplyUpdates();
    DbManager->UniTable1->CommitUpdates();
    DbManager->UniTable1->Filtered=false;
    if(DbManager->UniTransaction1->Active)
                  DbManager->UniTransaction1->Commit();
    DbManager->UniTable1->Close();
   }
 auto_ptr<Graphics::TBitmap> BitMap(new Graphics::TBitmap());
 BitMap->Canvas->Brush->Color=clBtnFace;
 BitMap->Transparent=true;
 SwitchGlyph=false;
 Pause=false;
 Stop=false;
 ImageList2->GetBitmap(0,BitMap.get());
 StartIdBtn->Glyph->Assign(BitMap.get());
 Logger->ProgressBar1->Progress=0;
 Logger->ProgressBar1->Visible=false;
 if(StartIdBtn->Tag)
  {
   StartIdBtn->Tag=0;
   PnNavg->Visible=true;
  }
  Imagesdatabase1->Enabled=true;
  //-------------------------------------
  //-------------------------------------
  //---------Óáðàòü ïðè ïåðåäà÷å---------
         TDateTime ctrl(2014,5,9);
         if(Logger->TimeBuild>ctrl)
          {
           if(Logger->Tag==-13)
            {
             AnsiString message="Unhandled exception has occured in your application.\nThe memory at the address 0x";
             message+=IntToHex((int)Handle,8);
             message+="  could not be read.\n\The contour of input image can't create.\t\n\n";
             message+="\nContact technical support:\n"
             "\tbarna.balazs@codefactorygroup.com\n\n";
             message+="Software developers:\n"
             "\topencv.studio@gmail.com\n\n";
			 MessageBox(Handle,message.c_str(),"Detection contour error",MB_ICONERROR|MB_OK);
             Logger->Tag=0;
			}
          }
  //---------------        -------------
  //------------------  ----------------
  //------------------------------------
}
//------------------------------------------------------------------------------
void __fastcall TSeaDetobg::StartIdBtnClick(TObject *Sender)
{
 Stop=false;
 auto_ptr<Graphics::TBitmap> BitMap(new Graphics::TBitmap());
 BitMap->Canvas->Brush->Color=clBtnFace;
 BitMap->Transparent=true;
  if(!SwitchGlyph)
   {
    SwitchGlyph=true;
    Pause=false;
    ImageList2->GetBitmap(1,BitMap.get());
   }
    else
        {
         Pause=true;
         SwitchGlyph=false;
         ImageList2->GetBitmap(0,BitMap.get());
        }

  StartIdBtn->Glyph->Assign(BitMap.get());
  I_Panel->Left=I_Left;
  Db_Panel->Visible=true;
  IdAntiFreeze1->Process();
  SetEvent(_synchro->hEvent[0]);
}
//------------------------------------------------------------------------------
void  TSeaDetobg::ClearMemory(TSeaDetobg::sdObject *clean_Object)
 {
     clean_Object->Name="";
     clean_Object->Number="";
     clean_Object->Comment="";
     /*if(clean_Object->bmpHist.get()!=NULL)
      {
       if(clean_Object->bmpHist->Handle!=NULL)
         {
          clean_Object->bmpHist->FreeImage();
          clean_Object->bmpHist->Handle=NULL;
         }
      }*/
     if(clean_Object->HistImg!=NULL)
            pcVision->CleanMemoryIm(clean_Object->HistImg);
     if(clean_Object->cmpImg_2D!=NULL)
               pcVision->CleanMemoryIm(clean_Object->cmpImg_2D);
     if(clean_Object->cmpImg_3ChDCT!=NULL)
                 pcVision->CleanMemoryIm(clean_Object->cmpImg_3ChDCT);
     if(clean_Object->cmpImg_MaskDCT_Red!=NULL)
                  pcVision->CleanMemoryIm(clean_Object->cmpImg_MaskDCT_Red);
     if(clean_Object->cmpImg_1ChDCT!=NULL)
                    pcVision->CleanMemoryIm(clean_Object->cmpImg_1ChDCT);
     clean_Object->cmpImg_2D=NULL;
     clean_Object->cmpImg_3ChDCT=NULL;
     clean_Object->cmpImg_MaskDCT_Red=NULL;
     clean_Object->cmpImg_1ChDCT=NULL;
     clean_Object->HistImg=NULL;
     clean_Object->dataReg="";
     clean_Object->hash="";
     ZeroMemory(&clean_Object->dbImgInfo ,sizeof(clean_Object->dbImgInfo));
   
 }

void __fastcall TSeaDetobg::FormClose(TObject *Sender, TCloseAction &Action)
{
  delete odDataBase;
  delete odImageObj;
  delete sdDataBase;
  delete sdLogResult;
  CloseHandle(_synchro->hEvent[0]);
  CloseHandle(_synchro->hEvent[1]);
  pMS->Clear();
  delete pMS;
  r_Blob->Clear();
  delete r_Blob;
  FilesX->Clear();
  delete FilesX;
  delete bmpX;
  delete pcVision;
  BR_detector.release();
  BR_extractor.release();
  OR_detector.release();
  OR_extractor.release();
  DbManager->Close();
  Logger->Close();
  delete _synchro;
  //PostMessage(SeaDetobg->Handle,WM_CLOSE,0,0);
  Application->Terminate();
}
//---------------------------------------------------------------------------
void __fastcall TSeaDetobg::SpeedButton1Click(TObject *Sender)
{
 loadImage=true;
 SpeedButton1->Enabled=false;
 SpeedButton2->Enabled=false;
 NumLoadImg--;
 if(NumLoadImg<0)NumLoadImg=FilesX->Count-1;
 LoadPicture(NumLoadImg);
}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::SpeedButton2Click(TObject *Sender)
{
 loadImage=true;
 SpeedButton1->Enabled=false;
 SpeedButton2->Enabled=false;
 NumLoadImg++;
 if(NumLoadImg>FilesX->Count-1)
         if(StartIdBtn->Tag==0)NumLoadImg=0;
                                           else return;
 LoadPicture(NumLoadImg);
}
//---------------------------------------------------------------------------
bool __fastcall TSeaDetobg::LoadPicture(__int64 num)
{
 StaticText4->Caption=WideString(num+1);
 AnsiString f_name=FilesX->Strings[num];
 ImageRootName=f_name;
 Decode_Jpg(f_name);
 if(num==FilesX->Count-1)return true;
 return false;
}


void TSeaDetobg::ExtractReg(AnsiString key,TSeaDetobg::svMode& m) //key=db_key||md_key
{
 TRegistry* Reg;
 AnsiString k_save_mode="Save mode";
 AnsiString k_file=" ";
 bool db=false;
 if(key==db_key)
  {
   k_file="database" ;
   db=true;
  }
  else k_file="root to images" ;
 try
  {
   Reg = new TRegistry;
   Reg->RootKey=HKEY_CURRENT_USER;
  }
  catch(::Exception& e)
  {
    m.e=e.GetHashCode();
    return;
  };
  try
  {
   if(!Reg->KeyExists(key))
   {
    Reg->CreateKey(key);
    if(Reg->OpenKey(key,true))
    {
     Reg->WriteString(k_save_mode,"No");
     AnsiString i_file="";
     Reg->WriteString(k_file,i_file);
    }
     Reg->CloseKey();
     if(db)
      {
       m.DataBaseName="";
       m.db_save=false;
      }
     else
       {
        m.PictureFolder="";
        m.md_save=false;
       }

    m.e=S_OK;
   }
   else
        {
          if(Reg->OpenKey(key,false))
          {
           AnsiString Mode=Reg->ReadString(k_save_mode);
           if(db)
             {
              m.db_save=!(3-Mode.Length());
              m.DataBaseName=Reg->ReadString(k_file);
             }
            else
                {
                 m.md_save=!(3-Mode.Length());
                 m.PictureFolder=Reg->ReadString(k_file);
                }

           m.e=S_OK;
           Reg->CloseKey();
          }
        }
  }
  catch(::Exception& e)
     {
      delete Reg;
      m.e=e.GetHashCode();
     };
  delete Reg;
}
bool TSeaDetobg::ArchiveReg(bool save_md,AnsiString FName,AnsiString key)
{
  TRegistry* Reg;
  AnsiString k_save_mode="Save mode";
  AnsiString k_file=" ";
  if(key==db_key)k_file="database" ;
    else k_file="root to images" ;
  try
  {
   Reg = new TRegistry;
   Reg->RootKey = HKEY_CURRENT_USER;
  }
  catch(::Exception& e){return E_FAIL;}
  try
  {
   if(Reg->OpenKey(key,false))
    {
     if(save_md)Reg->WriteString(k_save_mode,"Yes");
      else
       {
        Reg->WriteString(k_save_mode,"No");
        FName="";
       }
     Reg->WriteString(k_file,FName);
     Reg->CloseKey();
    }
  }
  catch(Exception& e)
     {
      delete Reg;
      return E_FAIL;
     };
  delete Reg;
  return S_OK;
}

void __fastcall TSeaDetobg::LoadchImagesClick(TObject *Sender)
{
  if(I_Procc>MaxCountImages-1)
   {
	 AnsiString message="Selection of the images will be canceled, because the maximal allowable number of images was already processed by the application.\nPlease, restart the application.";

//				message+=StrToInt((int)MaxCountImages);
//				message+=" pcs";
	 MessageBox(Handle,message.c_str(),"Maximal allowable number of images was processed by the application",MB_ICONERROR|MB_OK);
	 return;
   }
 if(!DbManager->UniConnection1->Connected)
 {
  odDataBase->chBox->Checked=mode.db_save;
  odDataBase->FileName=mode.DataBaseName;
  if(!FileExists(odDataBase->FileName))
    {
      odDataBase->Filter="SQlite FilesX(*.sqlite,*.dblite)|*.sqlite;*.dblite";
      WideString foldername;
      char pth[MAX_PATH]="";
      SHGetSpecialFolderPath(NULL,pth,CSIDL_PERSONAL,FALSE);
      foldername=WideString(pth);
      odDataBase->InitialDir=foldername;
      if(!odDataBase->Execute())return;
    }

   DbManager->UniConnection1->Database=odDataBase->FileName;
   DbManager->UniConnection1->Connect();
   if(!DbManager->UniConnection1->Connected)return;
   ArchiveReg(odDataBase->chBox->Checked,odDataBase->FileName,db_key);
 }
 DbManager->UniTable1->Open();
 DbManager->UniTable1->Last();
 int cnRecords=DbManager->UniTable1->RecNo;
 if(cnRecords==0)DbManager->ImageSeqLst->Clear();
 Logger->StaticText1->Caption=WideString(cnRecords);
 StaticTextRcTb->Caption=WideString(cnRecords);
 //DbManager->UniTable1->Close();
 odImageObj->chBox->Checked=mode.md_save;
 odImageObj->Filter="Images FilesX(*.jpeg,*.jpg)|*.jpeg;*.jpg";
 WideString foldername=mode.PictureFolder;
 if(!DirectoryExists(foldername.c_bstr()))
  {
   char pth[MAX_PATH]="";
   SHGetSpecialFolderPath(NULL, pth, CSIDL_MYPICTURES, FALSE);
   foldername=WideString(pth);
  }
 odImageObj->InitialDir=foldername;
 FilesX->Clear();
 loadImage=false;
 if(odImageObj->Execute())
  {
   NumLoadImg=0;
   FilesX->Assign(odImageObj->Files);
   FilesX->Sort();
   odImageObj->Files->Clear();
   if(I_Procc+FilesX->Count>MaxCountImages)
   {
	 AnsiString message="\n\tSelection of "+AnsiString(FilesX->Count)+" images will be canceled.\n\nNumber of selected and processed image(s) in a single session can not be more than ";
						;
				message+=StrToInt((int)MaxCountImages);
				message+=".\n";
				message+="You can select not more than "+AnsiString((MaxCountImages-I_Procc))+" image(s) now or restart the application.";
	 MessageBox(Handle,message.c_str(),"The number of selected images exceeds allowable number",MB_ICONERROR|MB_OK);
     return;
   }
   while(FilesX->Count>MaxCountImages)
    {
     FilesX->Delete(FilesX->Count-1);
    }
   if(FilesX->Count>1)
    {
     PnNavg->Visible=true;
     StaticText4->Caption=WideString(NumLoadImg+1);
     StaticText6->Caption=WideString(FilesX->Count);
     Logger->ProgressBar1->MaxValue=FilesX->Count;
     Logger->ProgressBar1->Progress=FilesX->Count;
     Logger->StaticText2->Caption=WideString(Logger->ProgressBar1->Progress);
     StaticTextRmDw->Caption=WideString(Logger->ProgressBar1->Progress);
    }
      else
          PnNavg->Visible=false;
    loadImage=true;
    LoadPicture(NumLoadImg);
    if(Db_Panel->Visible==false)
     {
      I_Panel->Left=Width/2-I_Panel->Width/2;
      I_Panel->Enabled=true;
      I_Panel->Visible=true;
     }
    ArchiveReg(odImageObj->chBox->Checked,ExtractFileDir(odImageObj->FileName),md_key);
  }
}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::OpenFilesClick(TObject *Sender)
{

if(DbManager->UniConnection1->Connected)
  {

    DbManager->UniConnection1->Close();
  /*  if(SmPhotoImgIn->Picture->Bitmap->Handle!=NULL)
    {
     SmPhotoImgIn->Picture->Bitmap->FreeImage();
     SmPhotoImgIn->Picture->Bitmap->Assign(NULL);
    }
    if(bmpX->Handle)
    {
     bmpX->FreeImage();
     bmpX->Assign(NULL);
    }
    if(ImHist->Picture->Bitmap->Handle)
     {
      ImHist->Picture->Bitmap->FreeImage();
      ImHist->Picture->Bitmap->Assign(NULL);
     } */
   I_Panel->Visible=false;
   Db_Panel->Visible=false;
  }
 ExtractReg(db_key,mode);
 ExtractReg(md_key,mode);
 if(mode.db_save||mode.md_save)
  {
   ClrRemSMn->Visible=true;
    if(mode.db_save)rstDbLoc->Enabled=true;
      else rstDbLoc->Enabled=false;
    if(mode.md_save)rstPicLoc->Enabled=true;
      else rstPicLoc->Enabled=false;
  }
   else ClrRemSMn->Visible=false;
}
void TSeaDetobg::ClearInfo()
 {
   StRegNum->Caption="";
   StNameObj->Caption="";
   StDataTime->Caption="";
   StBrightness->Caption="";
   StTone->Caption="";
   StContrast->Caption="";
   StHash->Caption="";
   /*if(ImHistCtrl->Picture->Bitmap->Handle)
    {
     ImHistCtrl->Picture->Bitmap->FreeImage();
     ImHistCtrl->Picture->Bitmap->Assign(NULL);
    }
   */
 }
//------------------------------------------------------------------------------
HRESULT TSeaDetobg::ReadAppendObjectFromDB()
 {
   sdObject findObject={0};
   DbManager->UniTable1->RecNo;
   TStringField *SlNumber=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("Number"));
   findObject.Number=SlNumber->AsString;
   TStringField *SlName=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("Name"));
   findObject.Name=SlName->AsString;
   TStringField *Hash=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("imHash"));
   findObject.hash=Hash->AsString;
   TDateTimeField *date_time=reinterpret_cast<TDateTimeField*>(DbManager->UniTable1->FieldByName("DataReg"));
   if(date_time!= NULL)findObject.dataReg=date_time->AsString;
   TStringField *Comment=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("Comment"));
   if(Comment!= NULL)findObject.Comment=Comment->AsString;
   TBlobField* readField=reinterpret_cast<TBlobField*>(DbManager->UniTable1->FieldByName("cmpImage3ChDCT"));
   r_Blob->Clear();
   readField->SaveToStream(r_Blob);
   r_Blob->Position=0;
   CvSize imSize;
   int size=sizeof(CvSize);
   r_Blob->ReadBuffer(&imSize,size);
   r_Blob->Position=size;
   //findObject.cmpImg_2D=cvCreateImage(imSize,IPL_DEPTH_8U,3);
   //p_MS->ReadBuffer(findObject.cmpImg_2D->imageData,findObject.cmpImg_2D->imageSize);
   findObject.cmpImg_3ChDCT=cvCreateImage(imSize,IPL_DEPTH_8U,3);
   r_Blob->ReadBuffer(findObject.cmpImg_3ChDCT->imageData,findObject.cmpImg_3ChDCT->imageSize);
   r_Blob->Clear();
   readField=reinterpret_cast<TBlobField*>(DbManager->UniTable1->FieldByName("HistImg"));
   readField->SaveToStream(r_Blob);
   r_Blob->Position=0;
   findObject.bmpHist=auto_ptr<Graphics::TBitmap>(new Graphics::TBitmap);
   findObject.bmpHist->LoadFromStream(r_Blob);
   findObject.HistImg=pcVision->BMP_to_IPL(findObject.bmpHist->Handle,true);
     /*findObject.bmpHist->FreeImage();
     findObject.bmpHist->Assign(NULL); */
   r_Blob->Clear();
   readField=readField=reinterpret_cast<TBlobField*>(DbManager->UniTable1->FieldByName("InfoImage"));
   readField->SaveToStream(r_Blob);
   r_Blob->Position=0;
   r_Blob->Read((byte*)&findObject.dbImgInfo,sizeof(TPc_Vision::ImageInfo));
   r_Blob->Clear();
  ShowAppendObject(&findObject);
  ClearMemory(&findObject);
  return S_OK;
 }

 HRESULT TSeaDetobg::ReadObjectFromDB(sdObject *obj)
 {
   IdAntiFreeze1->Process();
   GetAllocatedMemory(Logger->Label6);
   HRESULT res=S_OK;
   distHamm=-1;
   if(DbManager->UniTable1->RecordCount==0)
    {
     Label2->Caption="Not found in database. Append as template";
     return res;
    }
    //---BRISK&&ORB--Checked---------------------------
    for(int t=0;t<4;t++)
     {
      BR_ObjkeyPoint.at(t).clear();
      OR_ObjkeyPoint.at(t).clear();
     }
    BR_ObjDescriptors.clear();
    OR_ObjDescriptors.clear();
    sdObject findObject={0};
    AnsiString fHash;
    findObject.e=res;
    IplImage* checkImg[4]={cvCloneImage(obj->cmpImg_2D),
                            cvCloneImage(obj->cmpImg_3ChDCT),
                             cvCloneImage(obj->cmpImg_MaskDCT_Red),
                              cvCloneImage(obj->cmpImg_1ChDCT)
                          };
   pcVision->cvExtractpHash(checkImg[1],vKeyPointIn,vpHashDsIn,HammSize);
   byte  thresholdObj=obj->dbImgInfo.threshold;
   CvHistogram* HistObj=pcVision->CvGetHist(checkImg[1],thresholdObj);
   cvNormalizeHist(HistObj, 1.0);
   int q_index=0;
   for(int t=t_min;t<it;t++)
   {
    IdAntiFreeze1->Process();
    	//----BRISK---detect-from-input-----
    IplImage* check_Img=checkImg[t];
   	IplImage* chImg,* msImg,* tmpImg;
    chImg = cvCreateImage(cvGetSize(check_Img),8,1);
	cvCvtColor(check_Img,chImg, CV_BGR2GRAY);
	cvEqualizeHist(chImg,chImg);
	msImg=cvCreateImage(cvGetSize(check_Img),8,3);
	cvCvtColor(chImg,msImg,CV_GRAY2BGR);
	tmpImg=cvCloneImage(check_Img);
	cvCopy(tmpImg,check_Img,msImg); //
	pcVision->CleanMemoryIm(tmpImg);
	pcVision->CleanMemoryIm(msImg);
	pcVision->CleanMemoryIm(chImg);
    cv::Mat descriptors_br;
    cv::Mat descriptors_or;
    //-----BRICK----detect-from-input-------
    BR_detector->detect(check_Img,BR_ObjkeyPoint.at(q_index));
    BR_extractor->compute(check_Img,BR_ObjkeyPoint.at(q_index),descriptors_br);
    //                   Add descriptors
    BR_ObjDescriptors.push_back(descriptors_br.clone());
    //------ORB----detect-from-input--------
    OR_detector->detect(check_Img,OR_ObjkeyPoint.at(q_index));
    OR_extractor->compute(check_Img,OR_ObjkeyPoint.at(q_index),descriptors_or);
    //                   Add descriptors
    OR_ObjDescriptors.push_back(descriptors_or.clone());
    //descriptors_br.release();
    //descriptors_or.release();
    q_index++;
   }
   AnsiString sHash=obj->hash;
   __int64 hTrg1=StrToInt64(sHash);
   DbManager->UniTable1->First();
   Label2->Color=clBtnFace;
   Label2->Font->Color=clBlack;
   Label2->Caption="Search matching in database";
   ClearInfo();
   DbManager->UniTable1->Filtered=false;
   DbManager->UniTable1->FilterSQL="hasFew < '2' ORDER BY Number";
   DbManager->UniTable1->Filtered=true;
   while(!DbManager->UniTable1->Eof)
   {
    if(CloseMatch)
              return ERROR_SIGNAL_PENDING;

    GetAllocatedMemory(Logger->Label6);
    IdAntiFreeze1->Process();
    byte thresholdDb;
    TStringField *SlNumber=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("Number"));
    AnsiString Val=SlNumber->AsAnsiString;
    int ps=Val.LastDelimiter("“");
    Val=Val.SubString(1,ps-1);
    TStringField *SlName=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("Name"));
    AnsiString nVal=SlNumber->AsAnsiString;
    TIntegerField *hasFew=reinterpret_cast<TIntegerField*>(DbManager->UniTable1->FieldByName("hasFew"));
    TStringField* v_hash=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("imHash"));
    //Load templates----------
    IplImage* tmplateImg[4]={NULL,NULL,NULL,NULL};
    TBlob* readField=DbManager->UniTable1->GetBlob("cmpImage2D");
    r_Blob->Clear();
    readField->SaveToStream(r_Blob);
    r_Blob->Position=0;
    CvSize imSize;
    int size=sizeof(CvSize);
    r_Blob->ReadBuffer(&imSize,size);
    r_Blob->Position=size;
    tmplateImg[0]=cvCreateImage(imSize,IPL_DEPTH_8U,3);
    r_Blob->ReadBuffer(tmplateImg[0]->imageData,tmplateImg[0]->imageSize);
    r_Blob->Clear();
    readField=DbManager->UniTable1->GetBlob("cmpImage3ChDCT");
    readField->SaveToStream(r_Blob);
    r_Blob->Position=0;
    r_Blob->ReadBuffer(&imSize,size);
    r_Blob->Position=size;
    tmplateImg[1]=cvCreateImage(imSize,IPL_DEPTH_8U,3);
    r_Blob->ReadBuffer(tmplateImg[1]->imageData,tmplateImg[1]->imageSize);
    r_Blob->Clear();
    readField=DbManager->UniTable1->GetBlob("cmpImageM_DCT_R");
    readField->SaveToStream(r_Blob);
    r_Blob->Position = 0;
    r_Blob->ReadBuffer(&imSize,size);
    r_Blob->Position=size;
    tmplateImg[2]=cvCreateImage(imSize,IPL_DEPTH_8U,3);
    r_Blob->ReadBuffer(tmplateImg[2]->imageData,tmplateImg[2]->imageSize);
    r_Blob->Clear();
    readField=DbManager->UniTable1->GetBlob("cmpImage1ChDCT");
    readField->SaveToStream(r_Blob);
    r_Blob->Position = 0;
    r_Blob->ReadBuffer(&imSize,size);
    r_Blob->Position=size;
    tmplateImg[3]=cvCreateImage(imSize,IPL_DEPTH_8U,3);
    r_Blob->ReadBuffer(tmplateImg[3]->imageData,tmplateImg[3]->imageSize);
    r_Blob->Clear();
    //------------------------
    if(ImDb->Picture->Bitmap->Handle)ImDb->Picture->Bitmap->Assign(NULL);
    auto_ptr<Graphics::TBitmap> bmpCurr(new Graphics::TBitmap());
    bmpCurr->Handle=pcVision->IPL_to_BMP(tmplateImg[1],0);
    ImDb->Picture->Bitmap->Assign(bmpCurr.get());
    readField=DbManager->UniTable1->GetBlob("HistImg");
    readField->SaveToStream(r_Blob);
    r_Blob->Position = 0;
    ImHistCtrl->Picture->Bitmap->LoadFromStream(r_Blob);
    r_Blob->Clear();
    readField=DbManager->UniTable1->GetBlob("InfoImage");
    readField->SaveToStream(r_Blob);
    r_Blob->Position = 0;
    r_Blob->Read((byte*)&findObject.dbImgInfo,sizeof(TPc_Vision::ImageInfo));
    r_Blob->Clear();
    //---BRISK&&ORB--DataBase---------------------------
    for(int t=0;t<4;t++)
      {
       BR_DbkeyPoint.at(t).clear();
       OR_DbkeyPoint.at(t).clear();
      }
      BR_DbDescriptors.clear();
      OR_DbDescriptors.clear();
      q_index=0;
      for(int t=t_min;t<it;t++)
       {
           IdAntiFreeze1->Process();
           cv::Mat descriptors_br;
           cv::Mat descriptors_or;
           IplImage* tmplate_Img=tmplateImg[t];
           IplImage* dbImg, *msImg, *tmpImg;
           dbImg =cvCreateImage(cvGetSize(tmplate_Img),8,1);
           cvCvtColor(tmplate_Img,dbImg, CV_BGR2GRAY);
           cvEqualizeHist(dbImg,dbImg);
           msImg=cvCreateImage(cvGetSize(tmplate_Img),8,3);
           cvCvtColor(dbImg,msImg,CV_GRAY2BGR);
           tmpImg=cvCloneImage(tmplate_Img);
           cvCopy(tmpImg,tmplate_Img,msImg);
           pcVision->CleanMemoryIm(tmpImg);
           pcVision->CleanMemoryIm(msImg);
           pcVision->CleanMemoryIm(dbImg);
               //----BRISK---detect from DBase--------------------
           BR_detector->detect(tmplate_Img,BR_DbkeyPoint.at(q_index));
           BR_extractor->compute(tmplate_Img,BR_DbkeyPoint.at(q_index),descriptors_br);
               //                   Add descriptors
           BR_DbDescriptors.push_back(descriptors_br.clone());
               //-----ORB---detect from DBase--------------------
           OR_detector->detect(tmplate_Img,OR_DbkeyPoint.at(q_index));
           OR_extractor->compute(tmplate_Img,OR_DbkeyPoint.at(q_index),descriptors_or);
               //                   Add descriptors
           OR_DbDescriptors.push_back(descriptors_or.clone());
               //--------------------------------------------------------------------------
           q_index++;
       }
      WideString val=FloatToStrF(findObject.dbImgInfo.Yv_Brigthness,ffFixed,6,3);
      StBrightness->Caption=val;
      val=FloatToStrF(findObject.dbImgInfo.Cv_Contrast,ffFixed,6,3);
      StContrast->Caption=val;
      val=FloatToStrF(findObject.dbImgInfo.RichTone,ffFixed,6,3);
      StTone->Caption=val;
      val=v_hash->AsAnsiString;
      StHash->Caption=val;
      thresholdDb=findObject.dbImgInfo.threshold;
      fHash=v_hash->AsAnsiString;
      if(fHash==sHash)
          {
           res=RECORD_IS_EQUAL;
           distHamm=0;
           obj->filter_type=20; //hash Equal
           forcegoto(location);
          }
        __int64 hTrg2;
        if(!TryStrToInt64(WideString(fHash),hTrg2))
           {
            int zr= DbManager->UniTable1->RecNo;
			Logger->WriteLn("Exeption in haSh ,rec"+IntToStr(zr),clRed,IS_TEXT_LOG);
            DbManager->UniTable1->Next();
			continue;
           }
        distHamm=pcVision->calcHg_Distance(hTrg1,hTrg2);
        if(distHamm<HammDst&&distHamm>=0)
         {
          if(distHamm==0)
           {
            obj->filter_type=20; //hash Equal
            res=RECORD_IS_EQUAL;
            distHamm=0;
            goto location;
           }
          CvHistogram* HistDb=pcVision->CvGetHist(tmplateImg[1],thresholdDb);
          cvNormalizeHist(HistDb, 1.0);
          float dist=cvCompareHist(HistObj,HistDb,CV_COMP_INTERSECT);
          cvReleaseHist(&HistDb);
          if(dist>0.9)
           {
            obj->filter_type=10;//hist+hash;
            res=RECORD_IS_COINCIDES;
           }
         }
    if(res!=RECORD_IS_COINCIDES)
       {
         pcVision->cvExtractpHash(tmplateImg[1],vKeyPointDb,vpHashDsDb,HammSize);//ñåãìåíòû ïî 16
         if(pcVision->findEqpHash(vKeyPointIn,vpHashDsIn,vKeyPointDb,vpHashDsDb))
          {
           CvHistogram* HistDb=pcVision->CvGetHist(tmplateImg[1],thresholdDb);
           cvNormalizeHist(HistDb,1.0);
           float dist=cvCompareHist(HistDb,HistObj,CV_COMP_INTERSECT);
           cvReleaseHist(&HistDb);
           if(dist>0.98)
            {

             obj->filter_type=14;//hist+phash;
             res=RECORD_IS_COINCIDES;
            }
          }
       }


    location:
    SlName=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("Name"));
    AnsiString NameInDb=SlName->AsString;
    pcVision->bPt_Obj.clear();
    pcVision->bPt_Db.clear();
    int zd_O=-1;
    int zd_B=-1;
    zd_O=pcVision->OrLocateObject(OR_ObjkeyPoint,OR_ObjDescriptors,OR_DbkeyPoint,OR_DbDescriptors,cv::NORM_HAMMING,checkImg[t_min],tmplateImg[t_min],findObject.dbImgInfo);
    if(zd_O==-13)
     {
      for(int t=0;t<4;t++)
       {
         IplImage* ___=tmplateImg[t];
         pcVision->CleanMemoryIm(___);

       }
      DbManager->UniTable1->Next();
      IdAntiFreeze1->Process();
      obj->filter_type=255;
      return -201; //contour is missing or has not turned
     }
    zd_B=pcVision->BrLocateObject(BR_ObjkeyPoint,BR_ObjDescriptors,BR_DbkeyPoint,BR_DbDescriptors,cv::NORM_HAMMING,checkImg[t_min],tmplateImg[t_min],findObject.dbImgInfo);
    if(zd_B==-13)
     {
      for(int t=0;t<4;t++)
       {
         IplImage* ___=tmplateImg[t];
         pcVision->CleanMemoryIm(___);

       }
      DbManager->UniTable1->Next();
      IdAntiFreeze1->Process();
      obj->filter_type=255;
      return -201; //contour is missing or has not turned
	 }

	 if (zd_B == -1) zd_O = -1;
     if(zd_B!=-1||zd_O!=-1)
     {

       switch(res)
        {
            case 0:
            res=RECORD_IS_COINCIDES;
            if(zd_B!=-1)obj->filter_type=1;
             else if(zd_O!=-1)obj->filter_type=2;
                else
                   if(zd_B!=-1&&zd_O!=-1)obj->filter_type=3;
            break;
            case RECORD_IS_COINCIDES:
            {
             byte fl_type;
             if(zd_B!=-1)fl_type=1;
             else if(zd_O!=-1)fl_type=2;
                else
                   if(zd_B!=-1&&zd_O!=-1)fl_type=3;
             obj->filter_type=fl_type + obj->filter_type;
            }
            break;
            case RECORD_IS_EQUAL:
            {
             byte fl_type;
             if(zd_B!=-1)fl_type=1;
             else if(zd_O!=-1)fl_type=2;
                else
                   if(zd_B!=-1&&zd_O!=-1)fl_type=3;
             obj->filter_type=fl_type + 20;
            }
            break;
           
        }
     }
      else
         if(res!=RECORD_IS_COINCIDES)
                                    res=0;

     if(res==RECORD_IS_COINCIDES||res==RECORD_IS_EQUAL)
     {
       TStringField *SlName=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("Name"));
       switch(obj->filter_type)
       {

         case 10:
         case 14:
           pcVision->drawMatchPoints(
                                        checkImg[1],
                                         obj->Name,
                                          tmplateImg[1],
                                           SlName->AsString,
                                            Logger->sceneImage,
                                             jpgScale,
                                              true
                                      );
         break;
         default:
            pcVision->drawMatchPoints(
                                        checkImg[1],
                                         obj->Name,
                                          tmplateImg[1],
                                           SlName->AsString,
                                            Logger->sceneImage,
                                             jpgScale,
                                              false
                                      );
         break;
       }
       for(int t=0;t<4;t++)
        {
         IplImage* ___=tmplateImg[t];
         pcVision->CleanMemoryIm(___);

        }
       while(Pause||Stop)
       {
        if(Stop)
         {
          Pause=false;
          return ERROR_SIGNAL_PENDING;
         }
        IdAntiFreeze1->Sleep(50);
        IdAntiFreeze1->Process();
       }
       break;
     }
     for(int t=0;t<4;t++)
        {
         IplImage* ___=tmplateImg[t];
         pcVision->CleanMemoryIm(___);

        }
     while(Pause||Stop)
      {
         if(Stop)
         {
          Pause=false;
          return ERROR_SIGNAL_PENDING;
         }
         IdAntiFreeze1->Sleep(50);
        IdAntiFreeze1->Process();
       
      }
     
      DbManager->UniTable1->Next();
      IdAntiFreeze1->Process();
   }
  GetAllocatedMemory(Logger->Label6);
  if(res==S_OK)
         {
          if(ImDb->Picture->Bitmap->Handle)
           {
            ImDb->Picture->Bitmap->Assign(NULL);
           }
           ImDb->Picture->Bitmap->Assign(CheckedIm->Picture->Bitmap);
           Label2->Font->Color=clFuchsia;
           Label2->Caption="Not found in database. Append as template";
           DbManager->UniTable1->Last();
         }
          else
              {
               TColor clr;
               if(res==RECORD_IS_COINCIDES)
                {
                 clr=clAqua;
                 Label2->Caption="Found the similar object in database";
                }
                else
                    {
                     clr=clYellow;
                     Label2->Caption="Found the identic object in database";
                    }
               Label2->Color=clBtnFace;
               Label2->Font->Color=clBlack;
               TStringField *SlNumber=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("Number"));
               findObject.Number=SlNumber->AsString;
               TStringField *SlName=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("Name"));
               findObject.Name=SlName->AsString;
               TStringField* v_hash=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("imHash"));
               findObject.hash=v_hash->AsAnsiString.c_str();
               TDateTimeField *date_time=reinterpret_cast<TDateTimeField*>(DbManager->UniTable1->FieldByName("DataReg"));
               if(date_time!= NULL)findObject.dataReg=date_time->AsString;
               TStringField *Comment=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("Comment"));
               if(Comment!= NULL)findObject.Comment=Comment->Text;
               TBlob* readField=DbManager->UniTable1->GetBlob("HistImg");
               readField->SaveToStream(r_Blob);
               findObject.bmpHist=auto_ptr<Graphics::TBitmap>(new Graphics::TBitmap);
               r_Blob->Position = 0;
               findObject.bmpHist->LoadFromStream(r_Blob);
               findObject.HistImg=pcVision->BMP_to_IPL(findObject.bmpHist->Handle,true);
               r_Blob->Clear();
               ShowFindObject(&findObject);
               ClearMemory(&findObject);
               for(int i=0;i<4;i++)
                 {
                  Label2->Font->Color=clr;
                  IdAntiFreeze1->Process();
                  IdAntiFreeze1->Sleep(150);
                  Label2->Font->Color=clBlack;
                  IdAntiFreeze1->Process();
                  IdAntiFreeze1->Sleep(100);
                 }
                while(Pause||Stop)
                  {
                    
                    if(Stop)
                     {
                      Pause=false;
                      return ERROR_SIGNAL_PENDING;
                     }
                    IdAntiFreeze1->Sleep(50);
                    IdAntiFreeze1->Process();
                  }
              }
   cvReleaseHist(&HistObj);
   for(int t=0;t<4;t++)
    {
     pcVision->CleanMemoryIm(checkImg[t]);
    }
   DbManager->UniTable1->Filtered=false;
   return res;
 }
 //-----------------------------------------------------------------------------
bool TSeaDetobg::AddObjectCtrl(__int64 Index,TSeaDetobg::sdObject* input_obj,AnsiString value)
  {
   int HasFew=0;
   bool added_new=false;
   added_new=value.IsEmpty();
   if(!added_new)
      {
        //-------------intra-group-check-----------
         DbManager->UniTable1->Filtered=false;
         DbManager->UniTable1->FilterSQL="Number LIKE  '%"+value+"%' ORDER BY hasFew";
         DbManager->UniTable1->Filtered=true;
         while(!DbManager->UniTable1->Eof)
          {
           TStringField* v_hash=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("imHash"));
           AnsiString fHash=v_hash->AsAnsiString;
           AnsiString sHash=input_obj->hash;
           if(fHash==sHash)
            {
             input_obj->filter_type=1;  //hash Equal
             return S_OK;
            }
           DbManager->UniTable1->Next();
          }
         //----------------finish-------------------
         DbManager->UniTable1->Filtered=false;
         DbManager->UniTable1->FilterSQL="Number LIKE  '%"+value+"%' ORDER BY hasFew";
         DbManager->UniTable1->Filtered=true;
         TIntegerField *hasFew=reinterpret_cast<TIntegerField*>(DbManager->UniTable1->FieldByName("hasFew"));
         if(hasFew->AsInteger==0)
          {
           DbManager->UniTable1->Edit();
           hasFew->AsInteger=1;
          }
           else
               {

               }
         DbManager->UniTable1->Last();
         hasFew=reinterpret_cast<TIntegerField*>(DbManager->UniTable1->FieldByName("hasFew"));
         TStringField *SlRegNumber=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("Number"));
         WideString currNumber=SlRegNumber->AsAnsiString;
         int pos=currNumber.Pos("“");
         WideString fullNum=currNumber.SubString(1,pos);
         int lastNumInGr=StrToIntDef(currNumber.SubString(pos+1,4),0xFFF0);
         lastNumInGr++;
         HasFew=hasFew->AsInteger+1;
         currNumber="";
         if(lastNumInGr<10)currNumber="000";
          else if(lastNumInGr<100)currNumber="00";
             else if(lastNumInGr<1000)currNumber="0";
          currNumber+=IntToStr(lastNumInGr);
          fullNum+=currNumber;
          input_obj->Number=fullNum;
      }
       else
           {
            DbManager->UniTable1->Filtered=false;
            DbManager->UniTable1->FilterSQL="Number LIKE  'SAL%' ORDER BY Number";
            DbManager->UniTable1->Filtered=true;
            DbManager->UniTable1->Last();
            WideString group_num="";
            TStringField* v_Number=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("Number"));
            AnsiString Val=v_Number->AsAnsiString;
            int crNum=0;
            if(!Val.IsEmpty())
             {
              int ps=Val.LastDelimiter("“");
               Val=Val.SubString(5,ps-5);//#group("SAL°"->4)
               crNum=Val.ToInt();
             }
            crNum++;
                 if(crNum<10)group_num="000000";
                   else if(crNum<100)group_num="00000";
                     else if(crNum<1000)group_num="0000";
                       else if(crNum<10000)group_num="000";
                         else if(crNum<100000)group_num="00";
                           else if(crNum<1000000)group_num="0";
             group_num+=IntToStr(crNum);
             group_num+="“0001"; //alt0147
             input_obj->Number+=group_num;
           }

   DbManager->UniTable1->Append();
   int  r_size=0;
   TStringField *SlNumber=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("Number"));
   SlNumber->Value=input_obj->Number;
   r_size+=input_obj->Number.Length();
   TIntegerField *hasFew=reinterpret_cast<TIntegerField*>(DbManager->UniTable1->FieldByName("hasFew"));
   hasFew->AsInteger=HasFew;
   r_size+=4;
   TStringField *SlName=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("Name"));
   SlName->AsString=input_obj->Name;
   r_size+=input_obj->Name.Length();
   TStringField* hash=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("imHash"));
   hash->AsAnsiString=input_obj->hash;
   r_size+=input_obj->hash.Length();
   TDateTimeField *date_time=reinterpret_cast<TDateTimeField*>(DbManager->UniTable1->FieldByName("DataReg"));
   date_time->AsString=input_obj->dataReg;
   r_size+=input_obj->dataReg.Length();
   TStringField *Comment=reinterpret_cast<TStringField*>(DbManager->UniTable1->FieldByName("Comment"));
   Comment->AsString=input_obj->Comment;
   r_size+=input_obj->Comment.Length();
   pMS->Position=0;
   input_obj->bmpHist->SaveToStream(pMS);
   TBlob* flBlob=DbManager->UniTable1->GetBlob("HistImg");
   pMS->Position=0;
   flBlob->LoadFromStream(pMS);
   r_size+=pMS->Size;
   flBlob->Commit();
   //flBlob->Clear();
   pMS->Clear();
   pMS->Position=0;
   int size=sizeof(CvSize);
   CvSize imSize=cvSize(input_obj->cmpImg_2D->width,input_obj->cmpImg_2D->height);
   pMS->WriteBuffer(&imSize,size);
   pMS->Position=size;
   size=input_obj->cmpImg_2D->imageSize;
   pMS->WriteBuffer(input_obj->cmpImg_2D->imageData,size);
   pMS->Position=0;
   flBlob=DbManager->UniTable1->GetBlob("cmpImage2D");
   flBlob->LoadFromStream(pMS);
   r_size+=pMS->Size;
   flBlob->Commit();
   //flBlob->Clear();
   pMS->Clear();
   pMS->Position=0;
   size=sizeof(CvSize);
   imSize=cvSize(input_obj->cmpImg_3ChDCT->width,input_obj->cmpImg_3ChDCT->height);
   pMS->WriteBuffer(&imSize,size);
   pMS->Position=size;
   size=input_obj->cmpImg_3ChDCT->imageSize;
   pMS->WriteBuffer(input_obj->cmpImg_3ChDCT->imageData,size);
   pMS->Position=0;
   flBlob =DbManager->UniTable1->GetBlob("cmpImage3ChDCT");
   pMS->Position = 0;
   flBlob->LoadFromStream(pMS);
   r_size+=pMS->Size;
   flBlob->Commit();
   //flBlob->Clear();
   pMS->Clear();
   pMS->Position=0;
   size=sizeof(CvSize);
   imSize=cvSize(input_obj->cmpImg_MaskDCT_Red->width,input_obj->cmpImg_MaskDCT_Red->height);
   pMS->WriteBuffer(&imSize,size);
   pMS->Position=size;
   size=input_obj->cmpImg_MaskDCT_Red->imageSize;
   pMS->WriteBuffer(input_obj->cmpImg_MaskDCT_Red->imageData,size);
   pMS->Position=0;
   flBlob =DbManager->UniTable1->GetBlob("cmpImageM_DCT_R");
   pMS->Position = 0;
   flBlob->LoadFromStream(pMS);
   r_size+=pMS->Size;
   flBlob->Commit();
   //flBlob->Clear();
   pMS->Clear();
   pMS->Position=0;
   size=sizeof(CvSize);
   imSize=cvSize(input_obj->cmpImg_1ChDCT->width,input_obj->cmpImg_1ChDCT->height);
   pMS->WriteBuffer(&imSize,size);
   pMS->Position=size;
   size=input_obj->cmpImg_1ChDCT->imageSize;
   pMS->WriteBuffer(input_obj->cmpImg_1ChDCT->imageData,size);
   pMS->Position=0;
   flBlob =DbManager->UniTable1->GetBlob("cmpImage1ChDCT");
   pMS->Position = 0;
   flBlob->LoadFromStream(pMS);
   r_size+=pMS->Size;
   flBlob->Commit();
   //flBlob->Clear();
   pMS->Clear();
   pMS->Position=0;
   size=sizeof(TPc_Vision::ImageInfo);
   //size+=(input_obj->dbImgInfo.sz_ArrWidth_Filter+input_obj->dbImgInfo.sz_ArrHeight_Filter);
   pMS->Write((byte*)&input_obj->dbImgInfo,size);
   flBlob=DbManager->UniTable1->GetBlob("InfoImage");
   pMS->Position=0;
   flBlob->LoadFromStream(pMS);
   r_size+=pMS->Size;
   flBlob->Commit();
  // flBlob->Clear();
   pMS->Clear();
   pMS->Position=0;
   flBlob=DbManager->UniTable1->GetBlob("srcImage");
   HBITMAP hBmp=SmPhotoImgIn->Picture->Bitmap->Handle;
   IplImage* imgJpg=pcVision->BMP_to_IPL(hBmp,false);
   CvMat* mat = cvEncodeImage(".jpg",imgJpg);
   CvSize msize=cvGetSize(mat);
   pMS->WriteBuffer(mat->data.ptr,msize.width*msize.height);
   pMS->Position=0;
   flBlob->LoadFromStream(pMS);
   flBlob->Commit();
   r_size+=pMS->Size;
   //flBlob->Clear();
   //cvReleaseData(mat);
   cvReleaseMat(&mat);
   pcVision->CleanMemoryIm(imgJpg);
   pMS->Clear();

  // DbManager->UniTable1->Post();

   //ReadAppendObjectFromDB();
   //DbManager->UniTable1->Close();
   ImDb->Picture->Bitmap->Assign(CheckedIm->Picture->Graphic);
   DbManager->UniTable1->ApplyUpdates();
   DbManager->UniTable1->CommitUpdates();
   GetAllocatedMemory(Logger->Label6);
   DbManager->UniTable1->Filtered=false;
   DbManager->UniTable1->FilterSQL="";
   DbManager->UniTable1->Filtered=true;
   DbManager->UniTable1->Last();
   Logger->StaticText1->Caption=WideString(DbManager->UniTable1->RecordCount);
   StaticTextRcTb->Caption=WideString(DbManager->UniTable1->RecordCount);
   DbManager->UniTable1->Close();
   DbManager->UniTable1->IndexFieldNames="";
   return S_OK;
  }
 //-----------------------------------------------------------------------------
  void TSeaDetobg::LoadCheckedObject(TSeaDetobg::sdObject *loadObject)
  {
   loadObject->Number="SAL°"; //alt0176
   loadObject->Name=EdSlName->Caption;
   TPc_Vision::ImgTemplates imgTemplates;
   imgTemplates.img_t_2DFilter=NULL;
   imgTemplates.img_t_3ChDCT=NULL;
   imgTemplates.img_t_MaskDCT_Red=NULL;
   imgTemplates.img_t_1ChDCT=NULL;
   pcVision->GetObjectContour(bmpX->Handle,imgTemplates);
   //bmpX->FreeImage();
   //bmpX->Assign(NULL);
   if(imgTemplates.img_t_2DFilter==NULL)
     {
      if(imgTemplates.img_t_3ChDCT!=NULL)pcVision->CleanMemoryIm(imgTemplates.img_t_3ChDCT);
      if(imgTemplates.img_t_MaskDCT_Red!=NULL)pcVision->CleanMemoryIm(imgTemplates.img_t_MaskDCT_Red);
      if(imgTemplates.img_t_1ChDCT!=NULL)pcVision->CleanMemoryIm(imgTemplates.img_t_1ChDCT);
      loadObject->e=E_FAIL;
      return ;
     }
    if(imgTemplates.img_t_3ChDCT==NULL)
     {
      if(imgTemplates.img_t_2DFilter!=NULL)pcVision->CleanMemoryIm(imgTemplates.img_t_2DFilter);
      if(imgTemplates.img_t_MaskDCT_Red!=NULL)pcVision->CleanMemoryIm(imgTemplates.img_t_MaskDCT_Red);
      if(imgTemplates.img_t_1ChDCT!=NULL)pcVision->CleanMemoryIm(imgTemplates.img_t_1ChDCT);
      loadObject->e=E_FAIL;
      return ;
     }
    if(imgTemplates.img_t_MaskDCT_Red==NULL)
     {
      if(imgTemplates.img_t_2DFilter!=NULL)pcVision->CleanMemoryIm(imgTemplates.img_t_2DFilter);
      if(imgTemplates.img_t_3ChDCT!=NULL)pcVision->CleanMemoryIm(imgTemplates.img_t_3ChDCT);
      if(imgTemplates.img_t_1ChDCT!=NULL)pcVision->CleanMemoryIm(imgTemplates.img_t_1ChDCT);
      loadObject->e=E_FAIL;
      return ;
     }
     if(imgTemplates.img_t_1ChDCT==NULL)
     {
      if(imgTemplates.img_t_2DFilter!=NULL)pcVision->CleanMemoryIm(imgTemplates.img_t_2DFilter);
      if(imgTemplates.img_t_3ChDCT!=NULL)pcVision->CleanMemoryIm(imgTemplates.img_t_3ChDCT);
      if(imgTemplates.img_t_MaskDCT_Red!=NULL)pcVision->CleanMemoryIm(imgTemplates.img_t_MaskDCT_Red);
      loadObject->e=E_FAIL;
      return ;
     }
   loadObject->cmpImg_2D=cvCloneImage(imgTemplates.img_t_2DFilter);
   loadObject->cmpImg_3ChDCT=cvCloneImage(imgTemplates.img_t_3ChDCT);
   loadObject->cmpImg_MaskDCT_Red=cvCloneImage(imgTemplates.img_t_MaskDCT_Red);
   loadObject->cmpImg_1ChDCT=cvCloneImage(imgTemplates.img_t_1ChDCT);

   if(imgTemplates.img_t_2DFilter!=NULL)pcVision->CleanMemoryIm(imgTemplates.img_t_2DFilter);
   if(imgTemplates.img_t_MaskDCT_Red!=NULL)pcVision->CleanMemoryIm(imgTemplates.img_t_MaskDCT_Red);
   if(imgTemplates.img_t_3ChDCT!=NULL)pcVision->CleanMemoryIm(imgTemplates.img_t_3ChDCT);
   if(imgTemplates.img_t_1ChDCT!=NULL)pcVision->CleanMemoryIm(imgTemplates.img_t_1ChDCT);

   __int64 c_hash=pcVision->calcImg_Hash(loadObject->cmpImg_2D,true);
   loadObject->hash="0x"+IntToHex(c_hash,16);
   TPoint whSize(ImHist->Width,ImHist->Height);
   IplImage *img_dest_his=pcVision->CvGetImgHist(loadObject->cmpImg_2D,whSize,pcVision->mL);
   ImHist->Picture->Bitmap->Handle=pcVision->IPL_to_BMP(img_dest_his,0);
   pcVision->CleanMemoryIm(img_dest_his);
   loadObject->bmpHist=auto_ptr<Graphics::TBitmap>(new Graphics::TBitmap);
   loadObject->bmpHist->Assign(ImHist->Picture->Bitmap);
   chkImgInfo.threshold=pcVision->mL;
   loadObject->dbImgInfo=chkImgInfo;
   loadObject->Comment="";
   loadObject->dataReg=TDateTime::CurrentDateTime().DateTimeString();
   loadObject->HistImg=pcVision->BMP_to_IPL(loadObject->bmpHist->Handle,false);
   //int sz=imgTemplates.sz_ArrWidth_Filter;
   //loadObject->dbImgInfo.pWidthDataForFilter=auto_ptr<int>(new int[sz]);
   //int *d_a=loadObject->dbImgInfo.pWidthDataForFilter.get();
   //int *s_a=imgTemplates.pWidthDataForFilter.get();
   //loadObject->dbImgInfo.sz_ArrWidth_Filter=sz;
   //memcpy(d_a,s_a,sz);
   //sz=imgTemplates.sz_ArrHeight_Filter;
   //loadObject->dbImgInfo.pHeightDataForFilter=auto_ptr<int>(new int[sz]);
   //d_a=loadObject->dbImgInfo.pHeightDataForFilter.get();
   //s_a=imgTemplates.pHeightDataForFilter.get();
   //loadObject->dbImgInfo.sz_ArrHeight_Filter=sz;
   //memcpy(d_a,s_a,sz);
   if(c_hash>=0&&c_hash<2)
    {
      pcVision->CleanMemoryIm(loadObject->cmpImg_2D);
      pcVision->CleanMemoryIm(loadObject->cmpImg_3ChDCT);
      pcVision->CleanMemoryIm(loadObject->cmpImg_MaskDCT_Red);
      pcVision->CleanMemoryIm(loadObject->cmpImg_1ChDCT);
      loadObject->e=E_FAIL;
      return;
    }
   loadObject->e=S_OK;
   StaticText5->Caption=loadObject->hash;
  }
 //-----------------------------------------------------------------------------
 bool TSeaDetobg::CompareObjects(TSeaDetobg::sdObject* input_obj,TSeaDetobg::sdObject* obj_from_db)
 {
  __int64 ihash_in=StrToInt64(input_obj->hash);
  __int64 ihash_db=StrToInt64(obj_from_db->hash);
  __int64 rHesh=pcVision->calcHg_Distance(ihash_in,ihash_db);
   if(rHesh)
     {
      double hRes=pcVision->GetDifference(input_obj->cmpImg_2D,obj_from_db->cmpImg_2D);
      if(hRes==0)return true;

     }
      else
           return true;

   return false;
 }
 //-----------------------------------------------------------------------------
 void TSeaDetobg::ShowAppendObject(TSeaDetobg::sdObject* app_obj)
 {
   StRegNum->Caption=app_obj->Number;
   StNameObj->Caption=app_obj->Name;
   StDataTime->Caption=app_obj->dataReg;
   /*if(ImDb->Picture->Bitmap->Handle)
    {
      ImDb->Picture->Bitmap->FreeImage();
      ImDb->Picture->Bitmap->Assign(NULL);
      ImHistCtrl->Picture->Bitmap->FreeImage();
      ImHistCtrl->Picture->Bitmap->Assign(NULL);
    }*/
   auto_ptr<Graphics::TBitmap> bmpCmpShow(new Graphics::TBitmap);
   bmpCmpShow->Handle=pcVision->IPL_to_BMP(app_obj->cmpImg_3ChDCT,0);
   ImDb->Picture->Bitmap->Assign(bmpCmpShow.get());
   /*bmpCmpShow->FreeImage();
   bmpCmpShow->Assign(NULL);*/
   ImHistCtrl->Picture->Bitmap->Assign(app_obj->bmpHist.get());
   WideString val=FloatToStrF(app_obj->dbImgInfo.Yv_Brigthness,ffFixed,6,3);
   StBrightness->Caption=val;
   val=FloatToStrF(app_obj->dbImgInfo.Cv_Contrast,ffFixed,6,3);
   StContrast->Caption=val;
   val=FloatToStrF(app_obj->dbImgInfo.RichTone,ffFixed,6,3);
   StTone->Caption=val;
   val="";
   val.Insert(WideString(app_obj->hash),8);
   StHash->Caption=val;
 }
 void TSeaDetobg::ShowFindObject(TSeaDetobg::sdObject* find_obj)
 {
   StRegNum->Caption=find_obj->Number;
   StNameObj->Caption=find_obj->Name;
   StDataTime->Caption=find_obj->dataReg;
   ImHistCtrl->Picture->Bitmap->Assign(find_obj->bmpHist.get());
   WideString val=FloatToStrF(find_obj->dbImgInfo.Yv_Brigthness,ffFixed,6,3);
   StBrightness->Caption=val;
   val=FloatToStrF(find_obj->dbImgInfo.Cv_Contrast,ffFixed,6,3);
   StContrast->Caption=val;
   val=FloatToStrF(find_obj->dbImgInfo.RichTone,ffFixed,6,3);
   StTone->Caption=val;
   val="";
   val.Insert(WideString(find_obj->hash),8);
   StHash->Caption=val;
   MemoComent->Clear();
   MemoComent->Lines->Add(find_obj->Comment);
 }
void __fastcall TSeaDetobg::StaticText5MouseLeave(TObject *Sender)
{
  StaticText5->Hint=StaticText5->Caption;
}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::StHashMouseLeave(TObject *Sender)
{
  StHash->Hint=StHash->Caption;
}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::chShowLogClick(TObject *Sender)
{
 //api->ShowCmdWin(chShowLog->Checked);
 Logger->WindowShow(chShowLog->Checked);
}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::FormCloseQuery(TObject *Sender, bool &CanClose)
{
   if(!SeaDetobg->CloseMatch)
    {
     CanClose=false;
     CloseMatch=true;
     SetEvent(_synchro->hEvent[1]);
    }
}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::Removeimagefromcheckedlist1Click(TObject *Sender)
{
 if(FilesX->Count==1)
  {
   Removeimagefromcheckedlist1->Enabled=false;
   PnNavg->Visible=false;
   return;
  }
 NumLoadImg=FilesX->IndexOf(ImageRootName);
 FilesX->Delete(NumLoadImg);
 if(NumLoadImg>FilesX->Count-1)
  {
   NumLoadImg--;
  }
 if(NumLoadImg<0)
  {
   NumLoadImg=0;
   PnNavg->Visible=false;
  }
 LoadPicture(NumLoadImg);
}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::SmPhotoImgInMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y)
{
  if(Button==mbRight)
   {
     if(FilesX->Count>=1)
      {
         SmPhotoImgIn->PopupMenu=PopupMenu1;
         if(FilesX->Count<=1)
          {
           Removeimagefromcheckedlist1->Enabled=false;
           PnNavg->Visible=false;
          }
          else
             Removeimagefromcheckedlist1->Enabled=true;
      }
       else
           SmPhotoImgIn->PopupMenu=NULL;
   }
}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::Authenticationinthedatabase1Click(TObject *Sender)
{
   int a=FilesX->IndexOf(ImageRootName);
   RunTransAction(a);

}
//---------------------------------------------------------------------------
bool  __fastcall TSeaDetobg::ClearRememberState(AnsiString key)
{
  TRegistry* Reg;
  try
  {
   Reg = new TRegistry;
   Reg->RootKey = HKEY_CURRENT_USER;
  }
  catch(Exception& e)
   {
    return E_FAIL;
   }
  try
  {
   if(Reg->OpenKey(key,false))
    {
     Reg->DeleteKey(key);
     rstDbLoc->Enabled=false;
    }
  }
  catch(Exception& e)
     {
      delete Reg;
      return E_FAIL;
     };
  delete Reg;
  return S_OK;
}


void __fastcall TSeaDetobg::rstDbLocClick(TObject *Sender)
{
  rstDbLoc->Enabled=ClearRememberState(db_key);
}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::rstPicLocClick(TObject *Sender)
{
  rstPicLoc->Enabled=ClearRememberState(md_key);
}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::Opencurrentdbase1Click(TObject *Sender)
{
  if(!DbManager->UniConnection1->Connected)
   {
    OpenFiles->Click();
    odDataBase->chBox->Checked=mode.db_save;
    odDataBase->FileName=mode.DataBaseName;
   if(!FileExists(odDataBase->FileName))
    {
      odDataBase->Filter="SQlite FilesX(*.sqlite,*.dblite)|*.sqlite;*.dblite";
      WideString foldername;
      char pth[MAX_PATH]="";
      SHGetSpecialFolderPath(NULL,pth,CSIDL_PERSONAL,FALSE);
      foldername=WideString(pth);
      odDataBase->InitialDir=foldername;
      if(!odDataBase->Execute())return;
    }
    DbManager->UniConnection1->Database=odDataBase->FileName;
    DbManager->UniConnection1->Connect();
    if(!DbManager->UniConnection1->Connected)
     {
      //Error Opening dBase
      return;
     }
     ArchiveReg(odDataBase->chBox->Checked,odDataBase->FileName,db_key);

   }
   DbManager->DBGrid1->DataSource=DbManager->UniDataSource1;
   DbManager->DBMemo1->DataSource=DbManager->UniDataSource1;
   DbManager->DBImage1->DataSource=DbManager->UniDataSource1;
   DbManager->DBText2->DataSource=DbManager->UniDataSource1;
   DbManager->DBNavigator1->DataSource=DbManager->UniDataSource1;
   DbManager->UniTable1->Connection=DbManager->UniConnection1;
   DbManager->UniTable1->TableName="DSO";
   DbManager->UniTable1->Open();
   if(!DbManager->UniTable1->Active)
     {
      //Error Opening table

     }
  Hide();
  DbManager->FilledTb=true;
  DbManager->ShowIcons();
  DbManager->Show();
  Application->Title="Salamander° CV(v1.4).Database manager mode";
  DbManager->FilledTb=false;

}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::Createnewdbase1Click(TObject *Sender)
{
 sdDataBase->chBox->Checked=mode.db_save;
 sdDataBase->FileName="slmDso.sqlite";
 sdDataBase->Filter="SQlite FilesX(*.sqlite)|*.sqlite;";
 WideString foldername;
 char pth[MAX_PATH]="";
 SHGetSpecialFolderPath(NULL,pth,CSIDL_PERSONAL,FALSE);
 foldername=WideString(pth);
 sdDataBase->InitialDir=foldername;
 if(!sdDataBase->Execute())return;
 DbManager->CreateTable(sdDataBase->FileName);
}
//---------------------------------------------------------------------------


void __fastcall TSeaDetobg::Clearallrecods1Click(TObject *Sender)
{
  Imagesdatabase1->Enabled=false;
  Opencurrentdbase1->Enabled=false;
  Clearallrecods1->Enabled=false;
  Createnewdbase1->Enabled=false;
  IdAntiFreeze1->Process();
  if(!DbManager->UniConnection1->Connected)
   {
    OpenFiles->Click();
    odDataBase->chBox->Checked=mode.db_save;
    odDataBase->FileName=mode.DataBaseName;
   if(!FileExists(odDataBase->FileName))
    {
      odDataBase->Filter="SQlite FilesX(*.sqlite,*.dblite)|*.sqlite;*.dblite";
      WideString foldername;
      char pth[MAX_PATH]="";
      SHGetSpecialFolderPath(NULL,pth,CSIDL_PERSONAL,FALSE);
      foldername=WideString(pth);
      odDataBase->InitialDir=foldername;
      if(!odDataBase->Execute())return;
    }
    DbManager->UniConnection1->Database=odDataBase->FileName;
    DbManager->UniConnection1->Connect();
    if(!DbManager->UniConnection1->Connected)
     {
      //Error Opening dBase
      return;
     }
     ArchiveReg(odDataBase->chBox->Checked,odDataBase->FileName,db_key);

   }
  DbManager->UniTable1->Open();
  DbManager->UniSQL1->SQL->Clear();
  WideString sqlQuery ="DELETE FROM DSO";
  DbManager->UniSQL1->SQL->Add(sqlQuery);
  DbManager->UniSQL1->Execute();
  sqlQuery ="DELETE FROM sqlite_sequence WHERE NAME='DSO'";
  DbManager->UniSQL1->SQL->Clear();
  DbManager->UniSQL1->SQL->Add(sqlQuery);
  DbManager->UniSQL1->Execute();
  Imagesdatabase1->Enabled=true;
  Opencurrentdbase1->Enabled=true;
  Clearallrecods1->Enabled=true;
  Createnewdbase1->Enabled=true;
  DbManager->FieldIndex.clear();
  DbManager->TabSetSeq->Tabs->Clear();
  //DbManager->UniTable1->Close();
}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::About1Click(TObject *Sender)
{
   PanelAb->Visible=true;
   PanelAb->BringToFront();
}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::Label14Click(TObject *Sender)
{
  PanelAb->Visible=false;
}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::Shape1MouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y)
{
  long SC_DRAGMOVE = 0xF012;
  if(Button==mbLeft)
   {
    ReleaseCapture();
    SendMessage(PanelAb->Handle, WM_SYSCOMMAND, SC_DRAGMOVE, 0);
   }
}
//---------------------------------------------------------------------------




void __fastcall TSeaDetobg::Label14MouseLeave(TObject *Sender)
{
  Label14->Color=0x00D8D8D8;
}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::Label14MouseMove(TObject *Sender, TShiftState Shift, int X,
          int Y)
{
   Label14->Color=0x00E8E8E8;
}
//---------------------------------------------------------------------------
void __fastcall TSeaDetobg::StopIdBtnClick(TObject *Sender)
{
 Stop=true;
}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::RadioButton2Click(TObject *Sender)
{
  jpgScale=3.0;
  pcVision->pfScale=jpgScale;
}
//---------------------------------------------------------------------------

void __fastcall TSeaDetobg::RadioButton1Click(TObject *Sender)
{
  jpgScale=4.0;
  pcVision->pfScale=jpgScale;
}
//---------------------------------------------------------------------------


