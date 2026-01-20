#include <iostream>
#include <iomanip>
#include <sys/stat.h>

int i=0;
int NHITS=0;
const std::vector <int> colors {kBlue, kRed, kGreen,kBlack };

TFile * myFile=nullptr;
TChain * tree=nullptr;
std::map<std::string, Hit *> myHits;
std::map<std::string, TH1F *> pulseAll;
std::map<std::string, TH1F *> histos;
THStack *hs = nullptr;

///////////////////////////////////////////////////
// Read spectrum from ascii file and store it in h
///////////////////////////////////////////////////
void readSpc(TH1 * h, const std::string &filename)
{
  ifstream fq (filename, ios::in);
  double aux;

  for (int b = 1; b<=h->GetNbinsX(); b++) {
    fq >> aux;
    h->SetBinContent(b,aux);
  }
  fq.close();
}

///////////////////////////////////////////////////
// Read spectrum of nChannels from ascii file and returns
// an hitogram with the spectrum
///////////////////////////////////////////////////
TH1F * readSpc(const std::string &filename, int nChannels){

  TH1F * h=new TH1F (filename.c_str(), filename.c_str(), nChannels, 0, nChannels);
  ifstream fq (filename, ios::in);
  double aux;

  for (int b = 1; b<=h->GetNbinsX(); b++)
  {
    fq >> aux;
    h->SetBinContent(b,aux);
  }
  fq.close();
 
  return h;
}

///////////////////////////////////////////////////
// Save histogram h in ascii file filename
///////////////////////////////////////////////////
void saveSpc(TH1 * h, const std::string &filename)
{
  ofstream fq (filename, ios::out);
    for (int b = 1; b<=h->GetNbinsX(); b++){
      fq << h->GetBinContent(b) << "\n";
    }
  fq.close();
}

// SOLO CARGA  LOS DATOS Y LANZA EL VIEWER, SIN REPRESENTAR NADA
// Usage: readData filename 
// Filename : FILENAMExxxx.csv
void readData(const std::string &fileName)
{

  tree = new TChain("tree");

  struct stat fb;
   char outFileName[1024];
   sprintf(outFileName,"%s.root", fileName.c_str());
   if(fileName.find(".root") !=std::string::npos && stat (fileName.c_str(), &fb) == 0){
      std::cout<<fileName<<std::endl;
      tree->Add(fileName.c_str(), -1);
    } else if(stat (outFileName, &fb) == 0){
      std::cout<<outFileName<<std::endl;
      tree->Add(outFileName, -1);
    } else {
      int nFiles=1;
      sprintf(outFileName,"%s.%02d.root", fileName.c_str(), nFiles);
        if(stat (outFileName, &fb) !=0){
          std::cerr<<"Filename "<<outFileName<<" not found"<<std::endl;
          return;
        }
        while(stat (outFileName, &fb) == 0 ){
          std::cout<<outFileName<<std::endl;
          tree->Add(outFileName, -1);
          nFiles++;
          sprintf(outFileName,"%s.%02d.root", fileName.c_str(), nFiles);
        }
    }

  // Set stimation of the tree size 
  int entries = tree->GetEntries();
  tree->SetEstimate(entries);

  //tree->Print();

   for(NHITS=0;NHITS<4;NHITS++) {
    std::string brName = "C"+std::to_string(NHITS+1);
    if(!tree->GetBranch(brName.c_str()))continue;
    Hit *hit = nullptr;
    myHits[brName] = hit;
  }

  std::cout<<"Number of channels "<<myHits.size()<<std::endl;

  int c=0;
  for(auto & [brName, hit] : myHits ){
    tree->SetBranchAddress(brName.c_str(), &hit);
    tree->GetEntry(0);
    std::cout<<brName<< " "<<  hit->VDiv<<" V/Div "<<std::endl;
      if(c==0){
        std::cout << "Sampling Rate: " << hit->Interval*1E9 << " ns/pt"<< std::endl;
        std::cout << "Pulse Size: " << hit->Pulse.size() << " points"<<std::endl;
        std::cout << "Pulse Length: "<< hit->Interval*1E9*hit->Pulse.size()<< " ns"<< std::endl;
        std::cout << "Delay: " << hit->Delay*1E9 << " ns" << std::endl << std::endl;

        std::cout << "N Entries: " << entries << std::endl;
        double runStart = hit->TimeStamp;

        tree->GetEntry(entries-1);
        double runEnd = hit->TimeStamp;
        double deadTime = hit->DeadTime*1E-6;

        std::cout << "Duration: " << runEnd - runStart <<" seconds"<< std::endl;
        std::cout << "Live Time: " << runEnd - runStart - deadTime <<" seconds" << std::endl;
        std::cout << "Dead Time: " << deadTime <<" seconds" << std::endl;
        tree->GetEntry(0);
        c++;
      }
  }

}


///////////////////////////////////////////
// Draw pulses corresponding to p event
// p number corresponds to index in 'list' Event list
// if it exists
/////////////////////////////////////////
void drawPulse(int p){

  TEventList * list = (TEventList*)(gDirectory->Get("list"));
  tree->SetEventList(list);

  int ent = tree->GetEntryNumber(p);
  cout<<"Drawing entry " << ent << endl;

  if(ent<0 || ent>= tree->GetEntries()){
    std::cout<<"Entry "<<p <<" out of range 0-"<<tree->GetEntries()-1<<std::endl;
    return;
  }

  for(auto &[chName, h] : histos)
    delete h;

  histos.clear();

  tree->GetEntry(ent); 

  if(hs)delete hs;
  hs = new THStack("Pulses","");

  int c=0;
  for(auto & [chName, hit] : myHits ){
      histos[chName] = (TH1F *)(hit->getHisto(chName));
      histos[chName]->SetLineColor(colors[c%4]);
      histos[chName]->SetMarkerColor(colors[c%4]);
      hs->Add(histos[chName],chName.c_str());
      c++;
  }

  hs->Draw("nostack,lp");
  hs->GetYaxis()->SetTitle("Amplitude (V)");
  hs->GetXaxis()->SetTitle("Time (s)");

  gPad->BuildLegend(0.75,0.75,0.95,0.95,"");
}

/////////////////////////////////////////////////////////
// Add together all pulses in 'list' event list
// and plot it
///////////////////////////////////////////////////////
void drawAllPulses(std::string fName ="")
{

  for(auto &[chName, h] : histos)
    delete h;

  histos.clear();

  for(auto &[pName, p] : pulseAll)
    delete p;

  pulseAll.clear();

  TEventList * list = (TEventList*)(gDirectory->Get("list"));
  tree->SetEventList(list);

  int ent,cont;
  int max =tree->GetSelectedRows();

  if(max==0) max=tree->GetEntries(); //if no list selected use all pulses.

    for(const auto & [chName, hit] : myHits ){
      pulseAll[chName] = new TH1F (chName.c_str(), chName.c_str(),hit->Pulse.size(),0,hit->Pulse.size()*hit->Interval);
      pulseAll[chName]->GetYaxis()->SetTitle("Amplitude (V)");
      pulseAll[chName]->GetXaxis()->SetTitle("Time (s)");
    }

  for (cont = 0; cont<max; cont ++){
    ent = tree->GetEntryNumber(cont);
    tree->GetEntry(ent); 
      
      for(auto & [chName, hit] : myHits ){
        auto h = hit->getHisto(chName);
        pulseAll[chName]->Add(h);
        delete h;
      }
  }

  if(hs)delete hs;
  hs = new THStack("All pulses","");

  int c=0;
  for(auto & [chName, pulse] : pulseAll ) {
    pulse->Scale(1./max);
    pulse->SetLineColor(colors[c%4]);
    pulse->SetMarkerColor(colors[c%4]);
    hs->Add(pulse,chName.c_str());
	if(!fName.empty()){
	  std::string name = chName+"_"+ fName;
	  saveSpc(pulse, name);
	}
    c++;
  }

  hs->Draw("nostack,lp");
  hs->GetYaxis()->SetTitle("Amplitude (V)");
  hs->GetXaxis()->SetTitle("Time (s)");

  gPad->BuildLegend(0.75,0.75,0.95,0.95,"");
}

