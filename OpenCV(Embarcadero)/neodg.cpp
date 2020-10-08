//---------------------------------------------------------------------------

#pragma hdrstop
#include "neodg.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
__fastcall TOpenDialogEx::TOpenDialogEx(TComponent * Owner) : TOpenDialog(Owner)
{
 panel=new TPanel(this);
 panel->Name="TextPanel";
 panel->Caption="";
 panel->BevelOuter= bvNone;
 panel->Color=clBtnFace;
 panel->TabOrder= 1;
 panel->Height=40;
 panel->Width=220;
 chBox=new TCheckBox(panel);
 chBox->Parent=panel;
 chBox->Anchors=TAnchors()<<akTop;
 chBox->Anchors=TAnchors()<<akLeft;
 chBox->Color=clBtnFace;
 chBox->Name="ChBxSaveSl";
 //chBox->Ctl3D=true;
 chBox->DoubleBuffered=true;
 chBox->Font->Name="Courier New"; //Ms Serif
 chBox->Font->Style=chBox->Font->Style<<fsBold;
 chBox->Caption="Remember selection";
 chBox->Font->Size=8;
}
//---------------------------------------------------------------------------
__fastcall TOpenDialogEx::~TOpenDialogEx()
{
  delete chBox;
  delete panel;
}
//---------------------------------------------------------------------------
void __fastcall TOpenDialogEx::ChBxClick(TObject *Sender)
{


}
void __fastcall TOpenDialogEx::NewCheckBoxWndProc(TMessage &Msg)
{
 if(Msg.Msg!=WM_ERASEBKGND)
  {
    Msg.Result=false;
  }
  Dispatch(&Msg.Msg);
}
void __fastcall TOpenDialogEx::DoShow()
{
     TOpenDialog::DoShow();
     TRect PreviewRect;
     GetClientRect(Handle, &PreviewRect);
     PreviewRect.Top=PreviewRect.Bottom-35;
     panel->BoundsRect=PreviewRect;
     panel->BringToFront();
     panel->ParentWindow= Handle;
     panel->Visible=true;
     chBox->Left=5;
     chBox->Top=5;
     chBox->Height=15;
     chBox->Width=210;
     chBox->Enabled=true;
     chBox->Visible=true;

}
//---------------------------------------------------------------------------
bool __fastcall TOpenDialogEx::Execute()
{
if( NewStyleControls && !Options.Contains(ofOldStyleDialog))
Template=L"DLGTEMPLATE";
else
   Template=NULL;
return TOpenDialog::Execute();
}
//---------------------------------------------------------------------------
void __fastcall TOpenDialogEx::DoSelectionChange()
{
TOpenDialog::DoSelectionChange();
}

//---------------------------------------------------------------------------
__fastcall TSaveDialogEx::TSaveDialogEx(TComponent * Owner) : TSaveDialog(Owner)
{
 panel=new TPanel(this);
 panel->Name="TextPanel";
 panel->Caption="";
 panel->BevelOuter= bvNone;
 panel->Color=clBtnFace;
 panel->TabOrder= 1;
 chBox=new TCheckBox(panel);
 chBox->Parent=panel;
 chBox->Anchors=TAnchors()<<akTop;
 chBox->Anchors=TAnchors()<<akLeft;
 chBox->Color=clBtnFace;
 chBox->Name="ChBxSaveSl";
 chBox->Ctl3D=true;
 chBox->DoubleBuffered=true;
 chBox->Font->Name="Courier New"; //Ms Serif
 chBox->Font->Style=chBox->Font->Style<<fsBold;
 chBox->Caption="Remember selection";
 chBox->Font->Size=8;
}
//---------------------------------------------------------------------------
__fastcall TSaveDialogEx::~TSaveDialogEx()
{
  delete chBox;
  delete panel;
}
//---------------------------------------------------------------------------
void __fastcall TSaveDialogEx::ChBxClick(TObject *Sender)
{


}
void __fastcall TSaveDialogEx::NewCheckBoxWndProc(TMessage &Msg)
{
 if(Msg.Msg!=WM_ERASEBKGND)
  {
    Msg.Result=false;
  }
  Dispatch(&Msg.Msg);
}
void __fastcall TSaveDialogEx::DoShow()
{
     TRect PreviewRect;
     GetClientRect(Handle, &PreviewRect);
     PreviewRect.Top=PreviewRect.Bottom-35;
     panel->BoundsRect=PreviewRect;
     panel->ParentWindow= Handle;
     chBox->Left=5;
     chBox->Top=5;
     chBox->Height=15;
     chBox->Width=280;
     chBox->Enabled=true;
     chBox->Visible=true;
 TSaveDialog::DoShow();
}
//---------------------------------------------------------------------------
bool __fastcall TSaveDialogEx::Execute()
{
if( NewStyleControls && !Options.Contains(ofOldStyleDialog))
Template=L"DLGTEMPLATE";
else
Template=NULL;
return TSaveDialog::Execute();
}
//---------------------------------------------------------------------------
void __fastcall TSaveDialogEx::DoSelectionChange()
{
 TSaveDialog::DoSelectionChange();
}//---------------------------------------------------------------------------


