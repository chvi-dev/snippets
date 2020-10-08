//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "LoggIn.h"
#include "mSeaDet.h"
#include "neodg.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "cgauges"
#pragma resource "*.dfm"
TLogger *Logger;
//---------------------------------------------------------------------------
__fastcall TLogger::TLogger(TComponent* Owner)
	: TForm(Owner)
{

}
//---------------------------------------------------------------------------
void TLogger::WriteLn(WideString text,TColor color,bool log)
{
  RichEdit1->SelAttributes->Color=color;
  RichEdit1->Lines->Add(text);
  if(log)
   {
    loggerStream->Position=loggerStream->Size;
    AnsiString txt="";
    if(color==clRed)txt+="\r";
    txt+=text;
    txt+="\r\n";
    loggerStream->WriteBuffer(txt.c_str(),text.Length()+2);
   }
}
void __fastcall TLogger::FormHide(TObject *Sender)
{
 SeaDetobg->chShowLog->Checked=false;
}
//---------------------------------------------------------------------------
void TLogger::WindowShow(bool show)
{
 if(show)
  {
   Top=SeaDetobg->Top;
   Left=(SeaDetobg->Left-Width/2)-2;
   SeaDetobg->Left+=((Width/2)+2);
   Show();
   BringToFront();
  }
  else
     {
      if(SpeedButton2->Tag)
       {
        SpeedButton2->Click();
        Application->ProcessMessages();
       }
      SeaDetobg->Left-=((Width/2)+2);
      Hide();
     }
  SeaDetobg->chShowLog->Checked=show;
}
void __fastcall TLogger::SpeedButton1Click(TObject *Sender)
{
 sceneImage->Picture->Bitmap->FreeImage();
 sceneImage->Picture->Bitmap->Assign(NULL);
 RichEdit1->Clear();
 RichEdit1->Lines->EndUpdate();
}
//---------------------------------------------------------------------------


void __fastcall TLogger::SpeedButton2Click(TObject *Sender)
{
  if(!SpeedButton2->Tag)
   {
    Width=Bevel2->Left+Bevel2->Width+15;
    SpeedButton2->Glyph->FreeImage();
    SpeedButton2->Glyph->Assign(NULL);
    SpeedButton2->Glyph->Assign(AImage->Picture->Bitmap);
    SpeedButton2->Hint="Cancel show match";
    SpeedButton2->Tag=1;
   }
    else
        {
         Width=Bevel1->Left+Bevel1->Width+15;
         SpeedButton2->Glyph->FreeImage();
         SpeedButton2->Glyph->Assign(NULL);
         SpeedButton2->Glyph->Assign(SImage->Picture->Bitmap);
         SpeedButton2->Tag=0;
         SpeedButton2->Hint="Show match images";
        }

}
//---------------------------------------------------------------------------
void __fastcall TLogger::SpeedButton3Click(TObject *Sender)
{
  if(sdLogResult->Execute())
   {
       RichEdit1->Lines->SaveToFile(sdLogResult->FileName);
   }
}
//---------------------------------------------------------------------------


void __fastcall TLogger::FormCreate(TObject *Sender)
{
   DecimalSeparator='.';
   TDateTime dateCreateFile;
   TimeBuild=dateCreateFile.CurrentDateTime();
   WideString FileData=TimeBuild.DateString();
   for(int i=1;i<FileData.Length()+1;i++)
     {
      if(FileData.IsDelimiter(".",i)||FileData.IsDelimiter("\/",i)||FileData.IsDelimiter("-",i))
       {
         FileData.Delete(i,1);
         FileData.Insert("•",i);
       }
     }
   WideString FileName="Salamander_Log_"+FileData+".txt";
   WideString LogDir=ParamStr(0);
   LogDir=ExtractFilePath(LogDir);
   LogDir+="Log\\";
   if(!DirectoryExists(LogDir))ForceDirectories(LogDir);
   WideString FileName_log=LogDir;
   FileName_log+=FileName;
   WideString time_Add=TimeBuild.DateTimeString();
   WideString New_time_data="Begin session -";
   for(int i=1;i<time_Add.Length()+1;i++)
     {
       if(time_Add.IsDelimiter(".",i)||time_Add.IsDelimiter("\/",i)||time_Add.IsDelimiter("-",i))
       {
         time_Add.Delete(i,1);
         time_Add.Insert("•",i);
       }
      if(time_Add.IsDelimiter(" ",i))
       {
         time_Add.Delete(i,1);
         time_Add.Insert("_",i);
       }
      if(time_Add.IsDelimiter(":",i))
       {
         time_Add.Delete(i,1);
         time_Add.Insert("°",i);
       }
     }
   New_time_data+=time_Add;
   RichEdit1->Clear();
   try
     {
         if (FileExists(FileName_log))
          {
           loggerStream=new TFileStream(FileName_log,fmOpenWrite|fmShareDenyWrite);
           loggerStream->Position=loggerStream->Size;
          }
           else
               {
                loggerStream=new TFileStream(FileName_log,fmCreate|fmShareDenyWrite);
               }
     }
     catch(...)
      {
       FileName="Salamander_Log.txt";
       FileName_log=LogDir;
       FileName_log+=FileName;
       loggerStream=new TFileStream(FileName_log,fmCreate|fmShareDenyWrite);
      }
   WriteLn(New_time_data,clBlack,IS_TEXT_LOG); // ? true
}
//---------------------------------------------------------------------------


