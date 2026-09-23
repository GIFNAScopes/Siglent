#include <iostream>
#include <iomanip>
#include <sys/stat.h>

int i=0;
int NHITS=0;
double liveTime=0;
const std::vector <int> colors {kBlue, kRed, kGreen,kBlack };

// Variables globales/estáticas para mantener el histórico acumulado
double totalDuration = 0.0;
double totalLiveTime = 0.0;
double totalDeadTime = 0.0;
int totalEntriesAccumulated = 0;

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

/// CARGA O AÑADE DATOS AL TCHAIN Y LANZA EL VIEWER
// Usage inicial: readData("filename")
// Usage para añadir: readData("filename", true)
void readData(const std::string &fileName, bool appendData = false)
{
  // 1. GESTIÓN DE MEMORIA Y RESET DE ACUMULADORES
  if (!appendData) {
    if (tree != nullptr) {
      delete tree; 
    }
    tree = new TChain("tree");
    
    // Reseteamos los acumuladores si es una carga limpia
    totalDuration = 0.0;
    totalLiveTime = 0.0;
    totalDeadTime = 0.0;
    totalEntriesAccumulated = 0;
  } else {
    if (!tree) {
      tree = new TChain("tree");
    }
  }

  // Guardamos cuántos eventos había ANTES de añadir los nuevos archivos
  int previousEntries = tree->GetEntries();

  // 2. BÚSQUEDA Y CARGA DE ARCHIVOS (Mismo comportamiento)
  struct stat fb;
  char outFileName[1024];
  sprintf(outFileName, "%s.root", fileName.c_str());
  if (fileName.find(".root") != std::string::npos && stat(fileName.c_str(), &fb) == 0) {
    std::cout << fileName << std::endl;
    tree->Add(fileName.c_str(), -1);
  } else if (stat(outFileName, &fb) == 0) {
    std::cout << outFileName << std::endl;
    tree->Add(outFileName, -1);
  } else {
    int nFiles = 1;
    sprintf(outFileName, "%s.%02d.root", fileName.c_str(), nFiles);
    if (stat(outFileName, &fb) != 0) {
      std::cerr << "Filename " << outFileName << " not found" << std::endl;
      return;
    }
    while (stat(outFileName, &fb) == 0) {
      std::cout << outFileName << std::endl;
      tree->Add(outFileName, -1);
      nFiles++;
      sprintf(outFileName, "%s.%02d.root", fileName.c_str(), nFiles);
    }
  }

  int entries = tree->GetEntries();
  tree->SetEstimate(entries);

  // 3. MAPEO DE RAMAS
  for (NHITS = 0; NHITS < 4; NHITS++) {
    std::string brName = "C" + std::to_string(NHITS + 1);
    if (!tree->GetBranch(brName.c_str())) continue;
    
    if (!appendData || myHits.find(brName) == myHits.end()) {
      Hit *hit = nullptr;
      myHits[brName] = hit;
    }
  }

  std::cout << "Number of channels " << myHits.size() << std::endl;

  // 4. PROCESAMIENTO DE METADATOS DEL NUEVO BLOQUE DE DATOS
  int c = 0;
  double DTfirstEvent = 0.;
  for (auto & [brName, hit] : myHits) {
    tree->SetBranchAddress(brName.c_str(), &hit);
    
    // Obtenemos el PRIMER evento del NUEVO bloque añadido
    tree->GetEntry(previousEntries); 
    
    std::cout << brName << " " << hit->VDiv << " V/Div " << std::endl;
    if (c == 0) {
      std::cout << "Sampling Rate: " << hit->Interval * 1E9 << " ns/pt" << std::endl;
      std::cout << "Pulse Size: " << hit->Pulse.size() << " points" << std::endl;
      std::cout << "Pulse Length: " << hit->Interval * 1E9 * hit->Pulse.size() << " ns" << std::endl;
      std::cout << "Delay: " << hit->Delay * 1E9 << " ns" << std::endl << std::endl;

      // Datos de tiempo de la nueva tanda
      double runStart = hit->TimeStamp;
      if (hit->id != 0) DTfirstEvent = hit->DeadTime * 1E-6;

      // Obtenemos el ÚLTIMO evento de la NUEVA tanda añadida
      tree->GetEntry(entries - 1);
      double runEnd = hit->TimeStamp;
      double deadTime = hit->DeadTime * 1E-6 - DTfirstEvent;
      double liveTimeFile = runEnd - runStart - deadTime;

      // Mostramos las fechas del bloque actual que se acaba de leer
      std::time_t unix_time = runStart;
      std::tm* time_start = std::localtime(&unix_time);
      if (time_start) {
        std::cout << "Current Batch Start: " << std::put_time(time_start, "%Y-%m-%d %H:%M:%S") << std::endl;
      }
      unix_time = runEnd;
      std::tm* time_end = std::localtime(&unix_time);
      if (time_end) {
        std::cout << "Current Batch End: " << std::put_time(time_end, "%Y-%m-%d %H:%M:%S") << std::endl;
      }
      
      // ACUMULACIÓN RECTIFICADA: Sumamos los tiempos reales de esta corrida a los acumulados
      totalDuration += (runEnd - runStart);
      totalDeadTime += deadTime;
      totalLiveTime += liveTimeFile;
      totalEntriesAccumulated = entries; // Actualizamos el total de entradas del TChain

      // Imprimimos las estadísticas globales unificadas (libres de huecos temporales)
      std::cout << "\n=== ESTADÍSTICAS ACUMULADAS ===" << std::endl;
      std::cout << "N Total Entries: " << totalEntriesAccumulated << std::endl;
      std::cout << "Total Active Duration: " << totalDuration << " seconds" << std::endl;
      std::cout << "Total Live Time: " << totalLiveTime << " seconds" << std::endl;
      std::cout << "Total Dead Time: " << totalDeadTime << " seconds" << std::endl;
      
      if (totalLiveTime > 0) {
        std::cout << "Avg Rate (on Live Time): " << totalEntriesAccumulated / totalLiveTime << " Hz" << std::endl;
      } else {
        std::cout << "Avg Rate: 0 Hz" << std::endl;
      }
      std::cout << "===============================\n" << std::endl;

      // Devolvemos el puntero al evento 0 por consistencia con tu código original
      tree->GetEntry(0);
      c++;
    }
  }
}

// AÑADE DATOS AL TCHAIN EXISTENTE LLAMANDO A READDATA EN MODO APPEND
// Usage: addData filename
void addData(const std::string &fileName)
{
  readData(fileName, true);
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

