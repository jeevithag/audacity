/**********************************************************************

  Audacity: A Digital Audio Editor
  
  MeterToolbar.h
 
  Dominic Mazzoni
  Leland Lucius
 
  ToolBar to hold the VU Meter

**********************************************************************/

#ifndef __AUDACITY_METER_TOOLBAR__
#define __AUDACITY_METER_TOOLBAR__

#include "ToolBar.h"

class wxDC;
class wxGridBagSizer;
class wxSizeEvent;
class wxWindow;

class Mete
// Constants used as bit pattern
const int kWithRecordMeter = 1;
const int kWithPlayMeter = 2;

class MeterToolBar:public ToolBar {

 public:

   MeterToolBar(int WhichMetersBar();
   virtual ~MeterToolBar();

   void Create(wxWindow *parent);
   bool DestroyChildren();

   virtual void Populate();
   virtual void Repaint(wxDC * WXUNUSED(dc)) {};
   virtual void EnableDisableButtons() {};
   virtual void UpdatePrefs();

irtual void OnSize(wxSizeEvent & event);
   virtual bool Expose( bool show );

   int GetInitialWidth() {return 338;}
   int GetMinToolbarWidth() { return 100; }

 private:
   void OnMeterPrefsUpdated(wxCommandEvent & evt);
   void RegenerateTooltips();

   int mWhichMeters;();

   wxGridBagSizer *mSizer;
   Meter *mPlayMeter;
   Meter *mRecordMeter;

 public:

   DECLARE_CLASS(MeterToolBar);
   DECLARE_EVENT_TABLE();
 namespace MeterToolBars {
   void AddMeters(Meter *playMeter, Meter *recordMeter);
   void RemoveMeters(Meter *playMeter, Meter *recordMeter);();

   void GetMeters(Meter **playMeter, Meter **recordMeter);
   void StartMonitoring();
   void Clear   //Meter *mPlayMeter;
   //Meter *mRecordMeter;
};

#endif

