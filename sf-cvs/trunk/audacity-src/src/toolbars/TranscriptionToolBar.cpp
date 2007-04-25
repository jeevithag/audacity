/**********************************************************************

  Audacity: A Digital Audio Editor

  TranscriptionToolBar.cpp

  Shane T. Mueller
  Leland Lucius

*******************************************************************//**

\class TranscriptionToolBar
\brief A kind of ToolBar used to help with analysing voice recordings.

*//*******************************************************************/

#include "../Audacity.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/choice.h>
#include <wx/defs.h>
#include <wx/brush.h>
#include <wx/image.h>
#include <wx/intl.h>
#endif // WX_PRECOMP

#include "TranscriptionToolBar.h"

#include "ControlToolBar.h"
#include "../AllThemeResources.h"
#include "../AudioIO.h"
#include "../Experimental.h"
#include "../ImageManipulation.h"
#include "../Project.h"
#include "../TimeTrack.h"
#include "../VoiceKey.h"
#include "../WaveTrack.h"
#include "../widgets/AButton.h"
#include "../widgets/ASlider.h"

IMPLEMENT_CLASS(TranscriptionToolBar, ToolBar);

///////////////////////////////////////////
///  Methods for TranscriptionToolBar
///////////////////////////////////////////
 

BEGIN_EVENT_TABLE(TranscriptionToolBar, ToolBar)
   EVT_CHAR(TranscriptionToolBar::OnKeyEvent)
   EVT_COMMAND_RANGE(TTB_PlaySpeed, TTB_PlaySpeed,
                     wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnPlaySpeed)
   EVT_SLIDER(TTB_PlaySpeedSlider, TranscriptionToolBar::OnSpeedSlider)
   EVT_COMMAND_RANGE(TTB_StartOn, TTB_StartOn,
                     wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnStartOn)
   EVT_COMMAND_RANGE(TTB_StartOff, TTB_StartOff,
                     wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnStartOff)
   EVT_COMMAND_RANGE(TTB_EndOn, TTB_EndOn,
                     wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnEndOn)
   EVT_COMMAND_RANGE(TTB_EndOff, TTB_EndOff,
                     wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnEndOff)
   EVT_COMMAND_RANGE(TTB_SelectSound, TTB_SelectSound,
                     wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnSelectSound)
   EVT_COMMAND_RANGE(TTB_SelectSilence, TTB_SelectSilence,
                     wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnSelectSilence)
   EVT_COMMAND_RANGE(TTB_AutomateSelection, TTB_AutomateSelection,
                     wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnAutomateSelection)
   EVT_COMMAND_RANGE(TTB_MakeLabel, TTB_MakeLabel,
                     wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnMakeLabel)
   EVT_COMMAND_RANGE(TTB_Calibrate, TTB_Calibrate,
                     wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnCalibrate)
   EVT_SLIDER(TTB_SensitivitySlider, TranscriptionToolBar::OnSensitivitySlider)

   EVT_CHOICE(TTB_KeyType, TranscriptionToolBar::SetKeyType)
END_EVENT_TABLE()
   ;   //semicolon enforces  proper automatic indenting in emacs.

////Standard Constructor
TranscriptionToolBar::TranscriptionToolBar()
: ToolBar(TranscriptionBarID, _("Transcription"), wxT("Transcription"))
{
   mPlaySpeed = 1.0;
#ifdef EXPERIMENTAL_VOICE_DETECTION
   mVk = new VoiceKey();
#endif
}

TranscriptionToolBar::~TranscriptionToolBar()
{
#ifdef EXPERIMENTAL_VOICE_DETECTION
   delete mVk;
#endif
}

void TranscriptionToolBar::Create(wxWindow * parent)
{
   ToolBar::Create(parent);

   mBackgroundBrush.SetColour(wxColour(204, 204, 204));
   mBackgroundPen.SetColour(wxColour(204, 204, 204));

   mBackgroundHeight = 0;
   mBackgroundWidth = 0;

#ifdef EXPERIMENTAL_VOICE_DETECTION
   mButtons[TTB_StartOn]->Disable();
   mButtons[TTB_StartOff]->Disable();
   mButtons[TTB_EndOn]->Disable();
   mButtons[TTB_EndOff]->Disable();
   mButtons[TTB_SelectSound]->Disable();
   mButtons[TTB_SelectSilence]->Disable();
   mButtons[TTB_Calibrate]->Enable();
   mButtons[TTB_AutomateSelection]->Disable();
   mButtons[TTB_MakeLabel]->Enable();
#endif

   //Process a dummy event to set up the slider
   wxCommandEvent dummy;
   OnSpeedSlider(dummy);
}

/// This is a convenience function that allows for button creation in
/// MakeButtons() with fewer arguments
/// Very similar to code in ControlToolBar...
AButton *TranscriptionToolBar::AddButton(
   teBmps eFore, teBmps eDisabled,
   int id,
   const wxChar *label,
   const wxChar *tip)
{
   AButton *&r = mButtons[id];

   r = ToolBar::MakeButton(
      bmpRecoloredUpSmall, bmpRecoloredDownSmall, bmpRecoloredHiliteSmall,
      eFore, eDisabled,
      wxWindowID(id),
      wxDefaultPosition, 
      false,
      theTheme.ImageSize( bmpRecoloredUpSmall ));

   r->SetLabel(label);
// JKC: Unlike ControlToolBar, does not have a focus rect.  Shouldn't it?
// r->SetFocusRect( r->GetRect().Deflate( 4, 4 ) );

#if wxUSE_TOOLTIPS
   r->SetToolTip(tip);
#endif

   Add( r, 0, wxALIGN_CENTER );

   return r;
}

void TranscriptionToolBar::Populate()
{
// Very similar to code in ControlToolBar...
// Very similar to code in EditToolBar
   MakeButtonBackgroundsSmall();

   AddButton(bmpPlay,     bmpPlayDisabled,   TTB_PlaySpeed,
      _("Play at selected speed"),
      _("Play-at-speed"));
   
   //Add a slider that controls the speed of playback.
   const int SliderWidth=100;
   mPlaySpeedSlider = new ASlider(this,
                                  TTB_PlaySpeedSlider,
                                  _("Playback Speed"), 
                                  wxDefaultPosition,
                                  wxSize(SliderWidth,25),
                                  SPEED_SLIDER);
   mPlaySpeedSlider->Set(1.0);
   mPlaySpeedSlider->SetLabel(_("Playback Speed"));
   Add( mPlaySpeedSlider, 0, wxALIGN_CENTER );

#ifdef EXPERIMENTAL_VOICE_DETECTION
   AddButton(bmpTnStartOn,     bmpTnStartOnDisabled,  TTB_StartOn,
      _("Adjust left selection to next onset"),
      _("Left-to-On"));
   AddButton(bmpTnEndOn,       bmpTnEndOnDisabled,   TTB_EndOn,
      _("Adjust right selection to previous offset"),
      _("Right-to-Off"));
   AddButton(bmpTnStartOff,    bmpTnStartOffDisabled,  TTB_StartOff,
      _("Adjust left selection to next offset"),
      _("Left-to-Off"));
   AddButton(bmpTnEndOff,      bmpTnEndOffDisabled,    TTB_EndOff,
      _("Adjust right selection to previous onset"),
      _("Right-to-On"));
   AddButton(bmpTnSelectSound, bmpTnSelectSoundDisabled, TTB_SelectSound,
      _("Select region of sound around cursor"),
      _("Select-Sound"));
   AddButton(bmpTnSelectSilence, bmpTnSelectSilenceDisabled, TTB_SelectSilence,
      _("Select region of silence around cursor"),
      _("Select-Silence"));
   AddButton(bmpTnAutomateSelection,   bmpTnAutomateSelectionDisabled,  TTB_AutomateSelection,
      _("Automatically make labels from words"),
      _("Make Labels"));
   AddButton(bmpTnMakeTag, bmpTnMakeTagDisabled,  TTB_MakeLabel,  
      _("Add label at selection"),
      _("Add Label"));
   AddButton(bmpTnCalibrate, bmpTnCalibrateDisabled, TTB_Calibrate,
      _("Calibrate voicekey"),
      _("Calibrate"));
 
   mSensitivitySlider = new ASlider(this,
                                    TTB_SensitivitySlider,
                                    _("Adjust Sensitivity"),
                                    wxDefaultPosition,
                                    wxSize(SliderWidth,25),
                                    SPEED_SLIDER);
   mSensitivitySlider->Set(.5);
   mSensitivitySlider->SetLabel(_("Sensitivity"));
   Add( mSensitivitySlider, 0, wxALIGN_CENTER );

   wxString choices[] =
   {
      _("Energy"),
      _("Sign Changes (Low Threshold)"),
      _("Sign Changes (High Threshold)"),
      _("Direction Changes (Low Threshold)"),
      _("Direction Changes (High Threshold)")
   };
   
   mKeyTypeChoice = new wxChoice(this, TTB_KeyType,
                                 wxDefaultPosition,
                                 wxDefaultSize,
                                 5,
                                 choices );
   mKeyTypeChoice->SetName(_("Key type"));
   mKeyTypeChoice->SetSelection(0);
   Add( mKeyTypeChoice, 0, wxALIGN_CENTER );
#endif

   // Add a little space
   Add(2, -1);
}

//This handles key-stroke events????
void TranscriptionToolBar::OnKeyEvent(wxKeyEvent & event)
{
   if (event.ControlDown()) {
      event.Skip();
      return;
   }
  
   if (event.GetKeyCode() == WXK_SPACE) {
      if (gAudioIO->IsBusy()) {
         /*Do Stuff Here*/
      }
      else {
         /*Do other stuff Here*/
      }
   }
}



//This changes the state of the various buttons
void TranscriptionToolBar::SetButton(bool down, AButton* button)
{
   if (down) {
      button->PushDown();
   }
   else {
      button->PopUp();
   }
}

void TranscriptionToolBar::GetSamples(WaveTrack *t, sampleCount *s0, sampleCount *slen)
{
   // GetSamples attempts to translate the start and end selection markers into sample indices 
   // These selection numbers are doubles.

   AudacityProject *p = GetActiveProject();
   if (!p) {
      return;
   }

   //First, get the current selection. It is part of the mViewInfo, which is
   //part of the project
   
   double start = p->GetSel0();
   double end = p->GetSel1();
      
   sampleCount ss0 = sampleCount( (start - t->GetOffset()) * t->GetRate() );
   sampleCount ss1 = sampleCount( (end - t->GetOffset()) * t->GetRate() );

   if (start < t->GetOffset()) {
      ss0 = 0;
   }

#if 0
   //This adjusts the right samplecount to the maximum sample.
   if (ss1 >= t->GetNumSamples()) {
      ss1 = t->GetNumSamples();
   }
#endif

   if (ss1 < ss0) {
      ss1 = ss0;
   }

   *s0 = ss0;
   *slen = ss1 - ss0;
}

void TranscriptionToolBar::OnPlaySpeed(wxCommandEvent & event)
{
   // Pop up the button
   SetButton(false, mButtons[TTB_PlaySpeed]); 

   // If IO is busy, abort immediately
   if (gAudioIO->IsBusy()) {
      SetButton(false, mButtons[TTB_PlaySpeed]);
      return;
   }

   // Can't do anything without an active project
   AudacityProject * p = GetActiveProject();
   if (!p) {
      return;
   }

   // Can't do anything without a time track
   TimeTrack *tt = new TimeTrack(p->GetDirManager());

   // Set the speed range
   tt->SetRangeUpper((long int)mPlaySpeed);
   tt->SetRangeLower((long int)mPlaySpeed);

   // Get the current play region
   double playRegionStart, playRegionEnd;
   p->GetPlayRegion(&playRegionStart, &playRegionEnd);

   // Start playing
   if (playRegionStart >= 0) {
//      playRegionEnd = playRegionStart + (playRegionEnd-playRegionStart)* 100.0/mPlaySpeed;

      p->GetControlToolBar()->PlayPlayRegion(playRegionStart,
                                             playRegionEnd,
                                             false,
                                             false,
                                             tt);
   }
}

void TranscriptionToolBar::OnSpeedSlider(wxCommandEvent& event)
{
   mPlaySpeed = (mPlaySpeedSlider->Get()) * 100;
}

void TranscriptionToolBar::OnStartOn(wxCommandEvent &event)
{
   //If IO is busy, abort immediately
   if (gAudioIO->IsBusy()){
      SetButton(false,mButtons[TTB_StartOn]);
      return;
   }
	
   mVk->AdjustThreshold(GetSensitivity());
   AudacityProject *p = GetActiveProject();
	
 
   TrackList *tl = p->GetTracks();
   TrackListIterator iter(tl);

   Track *t = iter.First();   //Make a track
   if(t) 
      {		
         sampleCount start,len;
         GetSamples((WaveTrack*)t, &start,&len);
        
         //Adjust length to end if selection is null
         //if(len == 0)
         //len = (WaveTrack*)t->GetSequence()->GetNumSamples()-start;
        
         sampleCount newstart = mVk->OnForward(*(WaveTrack*)t,start,len);
         double newpos = newstart / ((WaveTrack*)t)->GetRate();
        
         p->SetSel0(newpos);
         p->RedrawProject();

         SetButton(false, mButtons[TTB_StartOn]); 
      }
}

void TranscriptionToolBar::OnStartOff(wxCommandEvent &event)
{
   //If IO is busy, abort immediately
   if (gAudioIO->IsBusy()){
      SetButton(false,mButtons[TTB_StartOff]);
      return;
   }
   mVk->AdjustThreshold(GetSensitivity());
   AudacityProject *p = GetActiveProject();
   
   TrackList *tl = p->GetTracks();
   TrackListIterator iter(tl);
   
   SetButton(false, mButtons[TTB_StartOff]);
   Track *t = iter.First();   //Make a track
   if(t) 
      {		
         sampleCount start,len;
         GetSamples((WaveTrack*)t, &start,&len);
         
         //Adjust length to end if selection is null
         //if(len == 0)
         //len = (WaveTrack*)t->GetSequence()->GetNumSamples()-start;
         
         sampleCount newstart = mVk->OffForward(*(WaveTrack*)t,start,len);
         double newpos = newstart / ((WaveTrack*)t)->GetRate();
         
         p->SetSel0(newpos);
         p->RedrawProject();
         
         SetButton(false, mButtons[TTB_StartOn]); 
      }
   
	
}

void TranscriptionToolBar::OnEndOn(wxCommandEvent &event)
{
	
   //If IO is busy, abort immediately
   if (gAudioIO->IsBusy()){
      SetButton(false,mButtons[TTB_EndOn]);
      return;
   }
  
   mVk->AdjustThreshold(GetSensitivity());
   AudacityProject *p = GetActiveProject();
   TrackList *tl = p->GetTracks();
   TrackListIterator iter(tl);



   Track *t = iter.First();   //Make a track
   if(t) 
      {		
         sampleCount start,len;
         GetSamples((WaveTrack*)t, &start,&len);
        
         //Adjust length to end if selection is null
         if(len == 0)
            {
               len = start;
               start = 0;
            }
         sampleCount newEnd = mVk->OnBackward(*(WaveTrack*)t,start+ len,len);
         double newpos = newEnd / ((WaveTrack*)t)->GetRate();
        
         p->SetSel1(newpos);
         p->RedrawProject();
        
         SetButton(false, mButtons[TTB_EndOn]);		

      }
}



void TranscriptionToolBar::OnEndOff(wxCommandEvent &event)
{
	
   //If IO is busy, abort immediately
   if (gAudioIO->IsBusy()){
      SetButton(false,mButtons[TTB_EndOff]);
      return;
   }
   mVk->AdjustThreshold(GetSensitivity());
   AudacityProject *p = GetActiveProject();
   TrackList *tl = p->GetTracks();
   TrackListIterator iter(tl);

   Track *t = iter.First();   //Make a track
   if(t) 
      {		
         sampleCount start,len;
         GetSamples((WaveTrack*)t, &start,&len);
        
         //Adjust length to end if selection is null
         if(len == 0)
            {
               len = start;
               start = 0;
            }
         sampleCount newEnd = mVk->OffBackward(*(WaveTrack*)t,start+ len,len);
         double newpos = newEnd / ((WaveTrack*)t)->GetRate();
        
         p->SetSel1(newpos);
         p->RedrawProject();
        
         SetButton(false, mButtons[TTB_EndOff]);
      }

}



void TranscriptionToolBar::OnSelectSound(wxCommandEvent &event)
{

   //If IO is busy, abort immediately
   if (gAudioIO->IsBusy()){
      SetButton(false,mButtons[TTB_SelectSound]);
      return;
   }
	

   mVk->AdjustThreshold(GetSensitivity());
   AudacityProject *p = GetActiveProject();
   
   
   TrackList *tl = p->GetTracks();
   TrackListIterator iter(tl);

   Track *t = iter.First();   //Make a track
   if(t) 
      {		
         sampleCount start,len;
         GetSamples((WaveTrack*)t, &start,&len);
        
         //Adjust length to end if selection is null
         //if(len == 0)
         //len = (WaveTrack*)t->GetSequence()->GetNumSamples()-start;
        
         double rate =  ((WaveTrack*)t)->GetRate();
         sampleCount newstart = mVk->OffBackward(*(WaveTrack*)t,start,start);
         sampleCount newend   = mVk->OffForward(*(WaveTrack*)t,start+len,(int)(tl->GetEndTime()*rate));

         //reset the selection bounds.
         p->SetSel0(newstart / rate);
         p->SetSel1(newend /  rate);
         p->RedrawProject();

      }
   
   SetButton(false,mButtons[TTB_SelectSound]);
}

void TranscriptionToolBar::OnSelectSilence(wxCommandEvent &event)
{

   //If IO is busy, abort immediately
   if (gAudioIO->IsBusy()){
      SetButton(false,mButtons[TTB_SelectSilence]);
      return;
   }

   mVk->AdjustThreshold(GetSensitivity());
   AudacityProject *p = GetActiveProject();
   
   
   TrackList *tl = p->GetTracks();
   TrackListIterator iter(tl);

   Track *t = iter.First();   //Make a track
   if(t) 
      {		
         sampleCount start,len;
         GetSamples((WaveTrack*)t, &start,&len);
        
         //Adjust length to end if selection is null
         //if(len == 0)
         //len = (WaveTrack*)t->GetSequence()->GetNumSamples()-start;
         double rate =  ((WaveTrack*)t)->GetRate();
         sampleCount newstart = mVk->OnBackward(*(WaveTrack*)t,start,start);
         sampleCount newend   = mVk->OnForward(*(WaveTrack*)t,start+len,(int)(tl->GetEndTime()*rate));

         //reset the selection bounds.
         p->SetSel0(newstart /  rate);
         p->SetSel1(newend / rate);
         p->RedrawProject();

      }
   
   SetButton(false,mButtons[TTB_SelectSilence]);

}



void TranscriptionToolBar::OnCalibrate(wxCommandEvent &event)
{
   //If IO is busy, abort immediately
   if (gAudioIO->IsBusy()){
      SetButton(false,mButtons[TTB_Calibrate]);
      return;
   }


   AudacityProject *p = GetActiveProject();

   TrackList *tl = p->GetTracks();
   TrackListIterator iter(tl);
   Track *t = iter.First();   //Get a track
  
   if(t) 
      {		
         sampleCount start,len;
         GetSamples((WaveTrack*)t, &start,&len);
        
         mVk->CalibrateNoise(*((WaveTrack*)t),start,len);
         mVk->AdjustThreshold(3);
		
         mButtons[TTB_StartOn]->Enable();
         mButtons[TTB_StartOff]->Enable();
         mButtons[TTB_EndOn]->Enable();
         mButtons[TTB_EndOff]->Enable();
         //mThresholdSensitivity->Set(3);

         SetButton(false,mButtons[TTB_Calibrate]);    
      }
  
   mButtons[TTB_StartOn]->Enable();
   mButtons[TTB_StartOff]->Enable();
   mButtons[TTB_EndOn]->Enable();
   mButtons[TTB_EndOff]->Enable();
   mButtons[TTB_SelectSound]->Enable();  
   mButtons[TTB_SelectSilence]->Enable();  
   mButtons[TTB_AutomateSelection]->Enable();

   //Make the sensititivy slider set the sensitivity by processing an event.
   wxCommandEvent dummy;
   OnSensitivitySlider(dummy);

}

//This automates selection through a selected region,
//selecting its best guess for words and creating labels at those points.

void TranscriptionToolBar::OnAutomateSelection(wxCommandEvent &event)
{

	
   //If IO is busy, abort immediately
   if (gAudioIO->IsBusy())
      {
         SetButton(false,mButtons[TTB_EndOff]);
         return;
      }

   wxBusyCursor busy;

   mVk->AdjustThreshold(GetSensitivity());
   AudacityProject *p = GetActiveProject();
   TrackList *tl = p->GetTracks();
   TrackListIterator iter(tl);
   
   Track *t = iter.First();   //Make a track
   if(t) 
      {		
         sampleCount start,len;
         GetSamples((WaveTrack*)t, &start,&len);
         
         //Adjust length to end if selection is null
         if(len == 0)
            {
               len = start;
               start = 0;
            }
         int lastlen = 0;
         sampleCount newStart, newEnd;
         double newStartPos, newEndPos;
         

         //This is the minumum word size in samples (.05 is 50 ms)
         int minWordSize = (int)(((WaveTrack*)t)->GetRate() * .05);
         
         //Continue until we have processed the entire
         //region, or we are making no progress.
         while(len > 0 && lastlen != len)
            {
               
               lastlen = len;
               
               newStart = mVk->OnForward(*(WaveTrack*)t,start,len);

               //JKC: If no start found then don't add any labels.
               if( newStart==start)
                  break;
               
               //Adjust len by the new start position
               len -= (newStart - start);
               
               //Adjust len by the minimum word size
               len -= minWordSize;
               
               

               //OK, now we have found a new starting point.  A 'word' should be at least 
               //50 ms long, so jump ahead minWordSize
               
               newEnd   = mVk->OffForward(*(WaveTrack*)t,newStart+minWordSize, len);
               
               //If newEnd didn't move, we should give up, because
               // there isn't another end before the end of the selection.
               if(newEnd == (newStart + minWordSize))
                  break;


               //Adjust len by the new word end
               len -= (newEnd - newStart);
               
               //Calculate the start and end of the words, in seconds
               newStartPos = newStart / ((WaveTrack*)t)->GetRate();
               newEndPos = newEnd / ((WaveTrack*)t)->GetRate();
               
               
               //Increment
               start = newEnd;
               
               p->DoAddLabel(newStartPos, newEndPos);
               p->RedrawProject();
            }
         SetButton(false, mButtons[TTB_AutomateSelection]);
      }
}

void TranscriptionToolBar::OnMakeLabel(wxCommandEvent &event)
{
   AudacityProject *p = GetActiveProject();
   SetButton(false, mButtons[TTB_MakeLabel]);  
   p->DoAddLabel(p->GetSel0(),  p->GetSel1());
}

//This returns a double z-score between 0 and 10.
double TranscriptionToolBar::GetSensitivity()
{
   return (double)mSensitivity;
}

void TranscriptionToolBar::OnSensitivitySlider(wxCommandEvent & event)
{
   mSensitivity = (mSensitivitySlider->Get());
}

void TranscriptionToolBar::EnableDisableButtons()
{

#ifdef EXPERIMENTAL_VOICE_DETECTION
   AudacityProject *p = GetActiveProject();
   if (!p) return;
   // Is anything selected?
   bool selection = false;
   TrackListIterator iter(p->GetTracks());
   for (Track *t = iter.First(); t; t = iter.Next())
      if (t->GetSelected()) {
         selection = true;
         break;
      }
   selection &= (p->GetSel0() < p->GetSel1());

   mButtons[TTB_Calibrate]->SetEnabled(selection);
#endif
}

void TranscriptionToolBar::SetKeyType(wxCommandEvent & event)
{
   int value = mKeyTypeChoice->GetSelection();

   //Only use one key type at a time.
   switch(value)
      {
      case 0:
         mVk->SetKeyType(true,0,0,0,0);
         break;
      case 1:
         mVk->SetKeyType(0,true,0,0,0);
         break;
      case 2:
         mVk->SetKeyType(0,0,true,0,0);
         break;
      case 3:
         mVk->SetKeyType(0,0,0,true,0);
         break;
      case 4:
         mVk->SetKeyType(0,0,0,0,true);
         break;
      }

}

// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: ToDo

