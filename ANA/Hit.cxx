#include "Hit.h"

#include<iostream>

ClassImp(Hit);


Hit::Hit(){
}

Hit::~Hit(){
}

void Hit::analyzeHit( ){

  auto smoothed = GetSignalSmoothed();
  int nPointsB = Delay/Interval/2; //Baseline

  //std::cout<<"Pulse size "<<smoothed.size()<<" Points baseline "<<nPointsB<<" Pretrigger "<<Pretrigger<<std::endl;

  double baseLine = 0, baseLineSigma = 0;
  for (int p = 0; p < nPointsB; p++) {
    int val = smoothed[p];
    baseLine += val;
    baseLineSigma += val * val;
  }

  if (nPointsB > 0) {
    baseLine /= nPointsB;
    baseLineSigma = sqrt(baseLineSigma / nPointsB - baseLine * baseLine);
  }

  //std::cout<<" Baseline "<<baseLine<<" "<<baseLineSigma<<std::endl;

  double max=-1;
  int maxBin=-1;
  double integral =0;
  bool negPolarity = true;
  if(Offset<0)negPolarity = false;

  for(int p=0; p < smoothed.size(); p++){
     double val =0;
     if(negPolarity) val = baseLine - smoothed[p];
     else val = smoothed[p] - baseLine;
       if(val > max){
         max=val;
         maxBin=p;
       }
     integral += val;
     if(smoothed[p]<=0 || smoothed[p]>=32768)saturated=true;
  }

   int rsStart=-1, rsEnd=-1,wdStart=-1, wdEnd=-1,pEnd=-1;
   int integ2=0;
    for(int p=0;p<smoothed.size();p++){
      double val =0;
      //Subtract baseline
      if(negPolarity) val = baseLine - smoothed[p];
      else val = smoothed[p] - baseLine;
      integ2 += val;
      if(rsStart == -1 && val >= max*0.15)rsStart = p;
      if(wdStart == -1 && rsStart !=-1 && val >= max*0.5) wdStart = p;
      if(rsEnd == -1 && wdStart != -1 && val >= max*0.85)rsEnd = p;
      if(wdEnd == -1 && rsEnd !=-1 && val <= max*0.5) wdEnd = p;
      if(pEnd == -1 && integ2 >= integral*0.95) pEnd = p;
    }

  Area = integral*Interval*1E6*VDiv/CodePerDiv;//V*us
  Max = maxBin;
  High = max*VDiv/CodePerDiv;//V
  DC = std::round(max);
  T0 = rsStart; // In points
  TEnd = pEnd; // In points
  RT = rsEnd - rsStart;  // In points
  Width = wdEnd - wdStart;  // In points
  Baseline = baseLine;
  BaselineSigma = baseLineSigma;

  //std::cout<<"Max "<<max<<" Integral "<<integral<<" Width "<<Width<<" Risetime "<<RT<<std::endl;
}

std::vector<double> Hit::GetSignalSmoothed(int neighbours) {

    int pulse_depth = Pulse.size();
    std::vector<double> smoothed(pulse_depth,0);
    for (int p = 0; p < pulse_depth; p++){
      int nPoints=0;
      for (int j = - neighbours; j<=neighbours;j++){
        int index = p + j;
        if(index<0||index>= pulse_depth)continue;
        int val = Pulse[p];
        smoothed[p] += val;
        nPoints++;
      }
      if(nPoints)smoothed[p] /= nPoints ;
    }
  return smoothed;
}
 
TH1F *Hit::getHisto(const std::string & ch ){
    std::string hName = ch+"_EV"+std::to_string(id);
    auto h = new TH1F (hName.c_str(),hName.c_str(),Pulse.size(),0.,Pulse.size()*Interval);
        for(int p=0;p<Pulse.size();p++){
          double val = Pulse[p]*VDiv/CodePerDiv-Offset;
          h->SetBinContent(p+1,val);
        }
    return h;
}

