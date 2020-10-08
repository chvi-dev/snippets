//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include "DbManager.h"
#include "mSeaDet.h"
#include "neodg.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "DBAccess"
#pragma link "MemDS"
#pragma link "SQLiteUniProvider"
#pragma link "Uni"
#pragma link "UniProvider"
#pragma link "CRGrid"
#pragma resource "*.dfm"
TDbManager *DbManager;
//---------------------------------------------------------------------------
__fastcall TDbManager::TDbManager(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TDbManager::FormHide(TObject *Sender)
{
 PanelSetRange->Visible=false;
 DbManager->FieldIndex.clear();
 TabSetSeq->Tabs->Clear();
 DbManager->UniTable1->Active=false;
 Application->Title="Salamander° CV(v1.4)";
 SeaDetobg->Show();
}
//---------------------------------------------------------------------------
void __fastcall TDbManager::CreateTable(WideString newTable)
{
  UniConnection1->Close();
  CurrentDbTable=newTable;
  UniConnection1->Database=NULL;
  if(FileExists(sdDataBase->FileName))
	{
	 if (MessageDlg(
	        	("Do you really want to reCreate " + ExtractFileName(sdDataBase->FileName) + "?"),
                         mtConfirmation,
		         TMsgDlgButtons() << mbYes << mbNo,
	                 0,
		         mbNo) == mrYes)
                          {
                           if (MessageDlg(
	        	    ("All record in dbase file" + ExtractFileName(sdDataBase->FileName) + "will be lost"),
                              mtConfirmation,
		                TMsgDlgButtons() << mbNo<< mbOK ,
	                          0,
		                    mbNo) == mrOk)
                                    {
		                     DeleteFile(newTable);
                                     CreateTable(newTable);
                                    }
                           return;
                          }
	}
         else
              UniConnection1->SpecificOptions->Values["ForceCreateDatabase"]="True";
  UniConnection1->Database=CurrentDbTable;
  UniConnection1->Connect();
  UniSQL1->Connection=UniConnection1;
  UniSQL1->SQL->Clear();
  WideString sqlQuery ="CREATE TABLE IF NOT EXISTS DSO("\
                                                       "ObjInd integer PRIMARY KEY AUTOINCREMENT,"\
                                                       "HistImg blob,"\
                                                       "srcImage blob,"\
                                                       "cmpImage2D blob,"\
                                                       "cmpImage3ChDCT blob,"\
                                                       "cmpImageM_DCT_R blob,"\
                                                       "cmpImage1ChDCT blob,"\
                                                       "Name text,"\
                                                       "Number text,"\
                                                       "imHash text,"\
                                                       "InfoImage binary,"\
                                                       "DataReg timestamp,"\
                                                       "hasFew integer,"\
                                                       "Comment text);";

  UniSQL1->SQL->Add(sqlQuery);
  UniSQL1->Execute();
  UniConnection1->Close();
}
void __fastcall TDbManager::UniSQL1AfterExecute(TObject *Sender, bool Result)
{
   if(Result)
    {
     /* WideString StateM=UniSQL1->FinalSQL;
      if(StateM.Pos("DELETE FROM DSO"))
       {

          WideString sqlQuery ="VACUUM DSO";
          UniSQL1->SQL->Clear();
          UniSQL1->SQL->Add(sqlQuery);
          UniSQL1->Execute();
       }
        else
            if(StateM.Pos("VACUUM DSO"))
             {
               SeaDetobg->Imagesdatabase1->Enabled=true;
               SeaDetobg->Opencurrentdbase1->Enabled=true;
               SeaDetobg->Clearallrecods1->Enabled=true;
               SeaDetobg->Createnewdbase1->Enabled=true;
               FieldIndex.clear();
               TabSetSeq->Tabs->Clear();
               UniTable1->Close();
             }*/
    }
}
//---------------------------------------------------------------------------
void __fastcall TDbManager::FormShow(TObject *Sender)
{
  CurrentDbTable=UniConnection1->Database;
}
//---------------------------------------------------------------------------
void __fastcall TDbManager::FormCreate(TObject *Sender)
{
 DBGrid1->DoubleBuffered=true;
 DoubleBuffered=true;
 ControlStyle<<csOpaque;
 SrcImage->ControlStyle<<csOpaque;
 CmpImg->ControlStyle<<csOpaque;
 
}
//---------------------------------------------------------------------------
void __fastcall TDbManager::ShowCurrentRecordData()
 {
  TMemoryStream* p_MS;
  TSeaDetobg::sdObject obj_from_db;
  p_MS=reinterpret_cast<TMemoryStream*>(UniTable1->CreateBlobStream(UniTable1->FieldByName("InfoImage"),bmRead));
  if(p_MS!= NULL)
    {
     p_MS->Position=0;
     p_MS->Read((byte*)&obj_from_db.dbImgInfo,sizeof(TPc_Vision::ImageInfo));
     WideString val=FloatToStrF(obj_from_db.dbImgInfo.Yv_Brigthness,ffFixed,6,3);
     StBrightness->Caption=val;
     val=FloatToStrF(obj_from_db.dbImgInfo.Cv_Contrast,ffFixed,6,3);
     StContrast->Caption=val;
     val=FloatToStrF(obj_from_db.dbImgInfo.RichTone,ffFixed,6,3);
     StTone->Caption=val;
     p_MS->Free();
     p_MS=NULL;
    }
  p_MS=reinterpret_cast<TMemoryStream*>(UniTable1->CreateBlobStream(UniTable1->FieldByName("srcImage"),bmRead));
  if(p_MS!= NULL)
      {
       if(p_MS->Size)
        {
           p_MS->Position = 0;
           CvMat* mat =cvCreateMatHeader(1,p_MS->Size,CV_8UC1);
           auto_ptr<byte> mem(new byte[p_MS->Size]);
           byte* m=mem.get();
           p_MS->ReadBuffer(m,p_MS->Size);
           cvSetData(mat,m,0);
           IplImage *image=cvDecodeImage(mat,1);
           cvReleaseMat(&mat);
         /*  if(SrcImage->Picture->Bitmap->Handle!=NULL)
              {
               SrcImage->Picture->Bitmap->FreeImage();
               SrcImage->Picture->Bitmap->Handle=NULL;
              } */
           SrcImage->Picture->Bitmap->Handle=pcVision->IPL_to_BMP(image,0);
           pcVision->CleanMemoryIm(image);
           //mem.release();
        }
       p_MS->Free();
       p_MS=NULL;
      }
   p_MS=reinterpret_cast<TMemoryStream*>(UniTable1->CreateBlobStream(UniTable1->FieldByName("cmpImage2D"),bmRead));
   if(p_MS!= NULL)
      {
       if(p_MS->Size)
        {
         p_MS->Position = 0;
         CvSize imSize;
         int size=sizeof(CvSize);
         p_MS->ReadBuffer(&imSize,size);
         p_MS->Position =size;
         IplImage *Img=cvCreateImage(imSize,IPL_DEPTH_8U,3);
         p_MS->ReadBuffer((char*)Img->imageData,Img->imageSize);
         auto_ptr<Graphics::TBitmap> bmpCmpShow(new Graphics::TBitmap);
         bmpCmpShow->Handle=pcVision->IPL_to_BMP(Img,0);
         CmpImg->Picture->Bitmap->Assign(bmpCmpShow.get());
         //bmpCmpShow->Assign(NULL);
         //bmpCmpShow.release();
         pcVision->CleanMemoryIm(Img);
        }
       p_MS->Free();
       p_MS=NULL;
      }

 }
 void __fastcall TDbManager::ShowIcons()
  {
     if(UniTable1->RecordCount==0)return;
     ImageSeqLst->Clear();
     FieldIndex.clear();
     UniTable1->First();
     UniTable1->UpdateCursorPos();
     TabSetSeq->Tabs->BeginUpdate();
     TabSetSeq->Tabs->Add("All");
     HRSRC HRSRC = FindResourceEx(NULL, RT_RCDATA, "All_0", NULL);
     if(HRSRC)
     {
      int TSizeofResource = SizeofResource(NULL, HRSRC);
      if (TSizeofResource)
          {
           THandle *handle=(unsigned int*)LoadResource(NULL, HRSRC);
           if (handle)
           {
            auto_ptr<TMemoryStream> MemoryStream (new TMemoryStream());
            MemoryStream->WriteBuffer(handle, TSizeofResource);
            MemoryStream->Position=0;
            auto_ptr<Graphics::TBitmap> bmpI(new Graphics::TBitmap());
            bmpI->PixelFormat=pf24bit;
            bmpI->HandleType=bmDIB;
            bmpI->LoadFromStream(MemoryStream.get());
            ImageSeqLst->Add(bmpI.get(),NULL);
           }
           FreeResource(handle);
          }
     }
     TabSetSeq->TabIndex=-1;

     while(!UniTable1->Eof)
     {
      TMemoryStream* p_MS;
      TIntegerField *hasFew=static_cast<TIntegerField*>(DbManager->UniTable1->FieldByName("hasFew"));
      if(hasFew->AsInteger>1)
       {
         DbManager->UniTable1->Next();
         DbManager->UniTable1->UpdateCursorPos();
         continue;
       }
      TIntegerField *index=static_cast<TIntegerField*>(DbManager->UniTable1->FieldByName("ObjInd"));
      vector<int>::iterator fl_ind=find(FieldIndex.begin(),FieldIndex.end(),index->AsInteger);
      if(fl_ind!=FieldIndex.end())continue;
      FieldIndex.push_back(index->AsInteger);
      TStringField* v_Number=static_cast<TStringField*>(UniTable1->FieldByName("Number"));
      AnsiString Val=v_Number->AsAnsiString;
      int ps=Val.LastDelimiter("“");
      Val=Val.SubString(5,ps-5);//number of group ("SAL°"->4)
      p_MS=reinterpret_cast<TMemoryStream*>(UniTable1->CreateBlobStream(UniTable1->FieldByName("cmpImage2D"),bmRead));
      if(p_MS!= NULL)
      {
       if(p_MS->Size)
        {
           TabSetSeq->Tabs->Add(AnsiString(Val.ToInt()));
           p_MS->Position = 0;
           CvSize imSize;
           int size=sizeof(CvSize);
           p_MS->ReadBuffer(&imSize,size);
           p_MS->Position =size;
           IplImage *Img=cvCreateImage(imSize,IPL_DEPTH_8U,3);
           p_MS->ReadBuffer((char*)Img->imageData,Img->imageSize);
           IplImage* pre=cvCreateImage(cvSize(24,32),Img->depth,Img->nChannels);
           cvResize(Img,pre,CV_INTER_LINEAR);
           pcVision->CleanMemoryIm(Img);
           auto_ptr<Graphics::TBitmap> bmp(new Graphics::TBitmap());
           bmp->Handle=pcVision->IPL_to_BMP(pre,0);
           pcVision->CleanMemoryIm(pre);
           ImageSeqLst->Add(bmp.get(), NULL);
        }
        p_MS->Free();
        p_MS=NULL;
      }
      DbManager->UniTable1->Next();
     }
    UniTable1->First();
    UniTable1->UpdateCursorPos();
    TabSetSeq->TabIndex=0;
    TabSetSeq->Tabs->EndUpdate();
    ScrollBar1->Max=TabSetSeq->Tabs->Count-1;
  }

void __fastcall TDbManager::DBGrid1DrawColumnCell(TObject *Sender, const TRect &Rect,
          int DataCol, TColumn *Column, TGridDrawState State)
{
  if(State.Contains(gdSelected))
    {
      ShowCurrentRecordData();
    }
}
//---------------------------------------------------------------------------

void __fastcall TDbManager::Showonlyfilledgroups1Click(TObject *Sender)
{
  UniTable1->Filter="hasFew >  '0' ORDER BY Number";
}
//---------------------------------------------------------------------------

void __fastcall TDbManager::AllrecordindataBase1Click(TObject *Sender)
{
 UniTable1->Filter="";
 TabSetSeq->TabIndex=0;
}
//---------------------------------------------------------------------------

void __fastcall TDbManager::RemoveallRecords1Click(TObject *Sender)
{
  UniSQL1->SQL->Clear();
  WideString sqlQuery ="DELETE FROM DSO";
  UniSQL1->SQL->Add(sqlQuery);
  UniSQL1->Execute();
  sqlQuery ="DELETE FROM sqlite_sequence WHERE NAME='DSO'";
  UniSQL1->SQL->Clear();
  UniSQL1->SQL->Add(sqlQuery);
  UniSQL1->Execute();
  //UniTable1->Close();
  FieldIndex.clear();
  TabSetSeq->Tabs->Clear();
  SrcImage->Picture->Bitmap->FreeImage();
  SrcImage->Picture->Bitmap->Assign(NULL);
  CmpImg->Picture->Bitmap->FreeImage();
  CmpImg->Picture->Bitmap->Assign(NULL);
  //UniTable1->Open();
}
//---------------------------------------------------------------------------

void __fastcall TDbManager::Removebyrange1Click(TObject *Sender)
{
   PanelSetRange->Visible=true;
   Edit1->SetFocus();
}
//---------------------------------------------------------------------------

void __fastcall TDbManager::SpeedButtonRgSetClick(TObject *Sender)
{
 TSpeedButton* Sb=(TSpeedButton*)Sender;
 int id=StrToIntDef(Edit1->Text,-1);
 int end_t=StrToIntDef(Edit2->Text,-1);
 if(end_t=-1||end_t<id)end_t=UniTable1->RecordCount;
 if  (id!=-1)
  {
     switch(Sb->Tag)
      {
          case 1:
              WideString sqlQuery="";
              for(int i=id;i<end_t;i++)
               {
                WideString sqlQuery ="DELETE FROM DSO where ObjInd="+ AnsiString(i);
                UniSQL1->SQL->Clear();
                UniSQL1->SQL->Add(sqlQuery);
                UniSQL1->Execute();
               }
              sqlQuery ="DELETE FROM sqlite_sequence WHERE NAME='DSO'";
              UniSQL1->SQL->Clear();
              UniSQL1->SQL->Add(sqlQuery);
              UniSQL1->Execute();
              //UniTable1->Close();
              SrcImage->Picture->Bitmap->FreeImage();
              SrcImage->Picture->Bitmap->Assign(NULL);
              //UniTable1->Open();
          break;
      }
  }
  PanelSetRange->Visible=false;
}
//---------------------------------------------------------------------------

void __fastcall TDbManager::DBNavigator1Click(TObject *Sender, TNavigateBtn Button)

{
 switch(Button )
  {
    case nbFirst:
     TabSetSeq->FirstIndex=0;
    break;
    case nbLast:
       TabSetSeq->FirstIndex=TabSetSeq->Tabs->Count-TabSetSeq->VisibleTabs;
    break;
  }
}
//---------------------------------------------------------------------------

void __fastcall TDbManager::TabSetSeqClick(TObject *Sender)
{
 if(FilledTb)return;
 int ind=TabSetSeq->TabIndex;
 ScrollBar1->Position=ind;
 if(ind==0)
   {
    DbManager->UniTable1->Filtered=false;
    UniTable1->FilterSQL="Number LIKE '%SAL°%' ORDER BY ObjInd";
    UniTable1->Filtered=true;
    return;
   }
   AnsiString sqlName="SAL°";
   AnsiString sqlNum=TabSetSeq->Tabs->Strings[ind];
   int numGr=sqlNum.ToInt();
   if(numGr<10)sqlNum="000000";
   else if(numGr<100)sqlNum="00000";
     else if(numGr<1000)sqlNum="0000";
       else if(numGr<10000)sqlNum="000";
         else if(numGr<100000)sqlNum="00";
           else if(numGr<1000000)sqlNum="0";
             sqlNum+=IntToStr(numGr);
    sqlName+=sqlNum;
    DbManager->UniTable1->Filtered=false;
    UniTable1->FilterSQL="Number LIKE  '%"+sqlName+"%' ORDER BY hasFew";
    UniTable1->Filtered=true;
}
//---------------------------------------------------------------------------

void __fastcall TDbManager::Label12MouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y)
{
 long SC_DRAGMOVE = 0xF012;
  if(Button==mbLeft)
   {
    ReleaseCapture();
    SendMessage(PanelSetRange->Handle, WM_SYSCOMMAND, SC_DRAGMOVE, 0);
   }
}
//---------------------------------------------------------------------------




void __fastcall TDbManager::ScrollBar1Scroll(TObject *Sender, TScrollCode ScrollCode,
          int &ScrollPos)
{
  TabSetSeq->TabIndex=ScrollPos;
}
//---------------------------------------------------------------------------
void __fastcall TDbManager::TabSetSeqMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y)
{
   ScrollBar1->Tag=1;
}
//---------------------------------------------------------------------------

void __fastcall TDbManager::UniConnection1AfterConnect(TObject *Sender)
{
  WideString sqlQuery ;
  UniSQL1->SQL->Clear();
  sqlQuery ="PRAGMA auto_vacuum=1";
  UniSQL1->SQL->Add(sqlQuery);
  UniSQL1->Execute();
  sqlQuery ="PRAGMA page_size=1024";
  UniSQL1->SQL->Clear();
  UniSQL1->SQL->Add(sqlQuery);
  UniSQL1->Execute();
  sqlQuery ="PRAGMA synchronous=0";
  UniSQL1->SQL->Clear();
  UniSQL1->SQL->Add(sqlQuery);
  UniSQL1->Execute();
  sqlQuery ="PRAGMA journal_mode=OFF";
  UniSQL1->SQL->Clear();
  UniSQL1->SQL->Add(sqlQuery);
  UniSQL1->Execute();
  sqlQuery ="PRAGMA cache_size=76800";
  UniSQL1->SQL->Clear();
  UniSQL1->SQL->Add(sqlQuery);
  UniSQL1->Execute();
}
//---------------------------------------------------------------------------


void __fastcall TDbManager::UniTable1FilterRecord(TDataSet *DataSet, bool &Accept)

{
  SeaDetobg->IdAntiFreeze1->Process();
}
//---------------------------------------------------------------------------


