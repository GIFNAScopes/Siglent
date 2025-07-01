#ifndef __HIT_H__
#define __HIT_H__

//**************************
// Class for store the hits
//**************************

#include <Rtypes.h>
#include <TH1F.h>
#include <TObject.h>

class Hit {

public:
  Double_t TimeStamp=0; // seconds
  Double_t High=0; // Volts
  Short_t Max=0; // In points
  Double_t Area=0; // V us
  Float_t T0=0; // In points
  Float_t TEnd=0; // In points
  Float_t RT=0;  // In points
  Float_t Width=0;  // In points
  Int_t DC=0; // ADCs
  Float_t Baseline=0; // ADCs
  Float_t BaselineSigma=0; // ADCs
  Bool_t saturated = false;
  Int_t id=0; // Event ID
  Double_t TDiv=0; // seconds
  Double_t VDiv=0; // Volts
  Double_t Offset=0; // Volts
  Double_t Interval=0; // seconds
  Double_t CodePerDiv=0; // ADCs per div
  Double_t Delay=0; // seconds
  Double_t DeadTime=0; // Cumulative deadtime per event (seconds)
  std::vector<Short_t> Pulse; // ADC

  Hit();
  ~Hit();
  TH1F* getHisto( const std::string & ch );
  std::vector<double> GetSignalSmoothed(int neighbours=5);
  void analyzeHit( );

  ClassDef(Hit, 1)

};

#endif
