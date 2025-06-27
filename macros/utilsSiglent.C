#include <iostream>
#include <iomanip>
#include <sys/stat.h>

int i=0;
int NHITS=0;
const std::vector <int> colors {kBlue, kRed, kGreen,kBlack };

TFile * myFile=nullptr;
TChain * tree=nullptr;
std::vector<Hit *> myHits;
std::vector<TH1F *> pulseAll;
std::vector<TH1F *> histos;

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
    if(!tree->GetBranch(brName.c_str()))break;
  }

  myHits.resize(NHITS,nullptr);

  std::cout<<"Number of hits "<<NHITS<<std::endl;

  for(int b=0;b<NHITS;b++){
    std::string brName = "C"+std::to_string(b+1);
    std::cout<<brName<<std::endl;
    tree->SetBranchAddress(brName.c_str(), &myHits[b]);
    tree->GetEntry(0);
    std::cout<<"CH "<<b+1<< " "<<  myHits[b]->TDiv<<" V/Div "<<std::endl;
  }

  std::cout << "Sampling Rate: " << myHits[0]->Interval*1E9 << " ns/pt"<< std::endl;
  std::cout << "Pulse Size: " << myHits[0]->Pulse.size() << " points"<<std::endl;
  std::cout << "Pulse Length: "<< myHits[0]->Interval*1E9*myHits[0]->Pulse.size()<< " ns"<< std::endl;
  std::cout << "Delay: " << myHits[0]->Delay*1E9 << " ns" << std::endl << std::endl;

  std::cout << "N Entries: " << entries << std::endl;
  double runStart = myHits[0]->TimeStamp;

  tree->GetEntry(entries-1);
  double runEnd = myHits[0]->TimeStamp;
  double deadTime = myHits[0]->DeadTime;
  std::cout << "Duration: " << runEnd - runStart <<" seconds"<< std::endl;
  std::cout << "Live Time: " << runEnd - runStart - deadTime <<" seconds" << std::endl;
  std::cout << "Dead Time: " << deadTime <<" seconds" << std::endl;

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
  cout << ent << endl;

  if(ent<0 || ent>= tree->GetEntries()){
    std::cout<<"Entry "<<p <<" out of range 0-"<<tree->GetEntries()-1<<std::endl;
    return;
  }

   for(auto &h : histos){
     delete h;
   }
   histos.clear();

  tree->GetEntry(ent); 
 
  histos.resize(NHITS);
 
  for(int iii=0;iii<NHITS;iii++){
      histos[iii] = (TH1F *)(myHits[iii]->getHisto(iii));
      histos[iii]->SetLineColor(colors[iii]);
	if(iii==0){
  	  histos[iii]->SetStats(0);
	  histos[iii]->GetYaxis()->SetTitle("Amplitude (V)");
	  histos[iii]->GetXaxis()->SetTitle("Time (s)");
	  //histos[iii]->GetYaxis()->SetRangeUser(0,32768);
	  histos[iii]->Draw();
	} else {
	  histos[iii]->Draw("SAME");
	}
  }

}

/////////////////////////////////////////////////////////
// Add together all pulses in 'list' event list
// and plot it
///////////////////////////////////////////////////////
void drawAllPulses(std::string fName ="")
{

 for(auto &h : histos){
    delete h;
 }
 histos.clear();

 for(auto &p : pulseAll){
   delete p;
 }

 pulseAll.clear();

  TEventList * list = (TEventList*)(gDirectory->Get("list"));
  tree->SetEventList(list);

  int ent,cont;
  int max =tree->GetSelectedRows();

  if(max==0) max=tree->GetEntries(); //if no list selected use all pulses.

  pulseAll.resize(NHITS);
    for(int iii=0;iii<NHITS;iii++) {
      std::string pName = "Pulse"+std::to_string(iii+1);
      pulseAll[iii] = new TH1F (pName.c_str(), pName.c_str(),myHits[iii]->Pulse.size(),0,myHits[iii]->Pulse.size()*myHits[iii]->Interval);
      pulseAll[iii]->GetYaxis()->SetTitle("Amplitude (V)");
      pulseAll[iii]->GetXaxis()->SetTitle("Time (s)");
    }

  for (cont = 0; cont<max; cont ++){
    ent = tree->GetEntryNumber(cont);
    tree->GetEntry(ent); 
      for(int iii=0;iii<NHITS;iii++){
        auto h = myHits[iii]->getHisto(iii);
        pulseAll[iii]->Add(h);
        delete h;
      }
  }

  for(int iii=0;iii<NHITS;iii++) {
    pulseAll[iii]->Scale(1./max);
    pulseAll[iii]->SetLineColor(colors[iii]);
    if(iii==0){
      pulseAll[iii]->SetStats(0);
      pulseAll[iii]->GetYaxis()->SetLabelSize(.03);
      pulseAll[iii]->GetXaxis()->SetLabelSize(.03);
      if(NHITS>1)pulseAll[iii]->GetYaxis()->SetRangeUser(0,32768);
      pulseAll[iii]->Draw();
    } else {
      pulseAll[iii]->Draw("SAME");
    }

	if(!fName.empty()){
	  std::string name = "CH"+std::to_string(iii)+"_"+ fName;
	  saveSpc(pulseAll[iii], name);
	}
  }

}

