//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include "mSeaDet.h"
#include "ThreadSynchro.h"
#include "DbManager.h"
#include "neodg.h"
#include "pc_v.h"
#include "LoggIn.h"
#pragma package(smart_init)
__fastcall SynchroVCL::SynchroVCL(bool CreateSuspended)
	: TThread(CreateSuspended)
{
 OnTerminate=On_Terminate;
}
//---------------------------------------------------------------------------
void __fastcall SynchroVCL::Execute()
{
 while (!Terminated)
   {
       code_exit=WaitForMultipleObjects(2,hEvent,FALSE,100);
       switch(code_exit)
        {
         case WAIT_OBJECT_0:
         Synchronize(&ShowVCLobj);
         break;
         case WAIT_OBJECT_0+1:
         Synchronize(&Post);
         break;
        }
   }
}
//---------------------------------------------------------------------------

void __fastcall SynchroVCL::ShowVCLobj()
 {
  SeaDetobg->MatchDbImg();
 }
void __fastcall SynchroVCL::Post()
 {
  Terminate();
 }
 void __fastcall SynchroVCL::On_Terminate(TObject * Sender)
 {
  /*if(DbManager->UniConnection1->Connected)
    {
     if(DbManager->UniTable1->Active)DbManager->UniTable1->Close();
     DbManager->UniConnection1->Close();
    } */
  /*delete odDataBase;
  delete odImageObj;
  delete sdDataBase;
  delete sdLogResult;
  CloseHandle(hEvent[0]);
  CloseHandle(hEvent[1]);
  SeaDetobg->pMS->Clear();
  delete SeaDetobg->pMS;
  SeaDetobg->FilesX->Clear();
  delete SeaDetobg->FilesX;
  delete SeaDetobg->bmpX;
  delete pcVision;
  /*aDetobg->BR_detector.release();
  SeaDetobg->BR_extractor.release();
  SeaDetobg->OR_detector.release();
  SeaDetobg->OR_extractor.release();
  DbManager->Close();
  Logger->Close(); */
  PostMessage(SeaDetobg->Handle,WM_CLOSE,0,0);
 }

