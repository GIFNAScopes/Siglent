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
  Double_t TimeStamp; //seconds
  std::vector<Short_t> Pulse; //ADC
  Double_t High; // Volts
  Short_t Max; // In points
  Double_t Area; // V*us
  Float_t T0; // In points
  Float_t TEnd; // In points
  Float_t RT;  // In points
  Float_t Width;  // In points
  Int_t DC; // ADCs
  Float_t Baseline; //ADCs
  Float_t BaselineSigma; //ADCs
  Bool_t saturated = false;
  Int_t id; // Event ID
  Double_t TDiv; // seconds
  Double_t VDiv; // Volts
  Double_t Offset; // Volts
  Double_t Interval; // seconds
  Double_t CodePerDiv; // ADCs per div
  Double_t Delay; // seconds
  Double_t DeadTime; //Cumulative deadtime per event (seconds)

  Hit();
  ~Hit();
  TH1F* getHisto( const int &ch );
  std::vector<double> GetSignalSmoothed(int neighbours=5);
  void analyzeHit( );

  ClassDef(Hit, 1)

};

#endif
