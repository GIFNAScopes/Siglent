
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <sys/stat.h>
#include <deque>

#include <TFile.h>
#include <TTree.h>


#include "Hit.h"

void help() {
    std::cout << " analyzeSiglent options:" << std::endl;
    std::cout << "    --f       : Filename" << std::endl;
    std::cout << "    --h       : Print this help" << std::endl;
}

std::map<std::string, double> read_preamble(std::ifstream &csv_file){

  std::map<std::string, double> preamble;
  std::string line;
    while (std::getline(csv_file, line)) {
        if (!line.empty() && line[0] != '#') {
            size_t comma_pos = line.find(',');
            if (comma_pos != std::string::npos) {
                std::string key = line.substr(0, comma_pos);
                std::string value = line.substr(comma_pos + 1);
                if(key=="timestamp" || value=="adc_data")break;
                double val;
                std::istringstream(value) >> val;
                preamble[key] = val;
                //std::cout<<key<<" "<<val<<std::endl;
            }
        }
    }

  return preamble;
}

std::vector< std::pair<double, std::vector<Short_t> > > read_data(std::ifstream &csv_file, int nFrames, int nPoints){
  std::vector< std::pair<double, std::vector<Short_t> > > data(nFrames);
  std::string line;
  line.reserve(nPoints * 10);

     for (int f = 0; f < nFrames; f++) {
        if (!std::getline(csv_file, line)) break;
        const char* ptr = line.c_str();
        char* next_ptr;

        double timeStamp = std::strtod(ptr, &next_ptr);
        if (ptr == next_ptr) continue; // Error de lectura
        ptr = next_ptr;
        if (*ptr == ',') ptr++;
        std::vector<Short_t> adc_data(nPoints);

        for (int p = 0; p < nPoints; p++) {
            if (p == 0) {
                while (*ptr && (*ptr == ' ' || *ptr == '[' || *ptr == '(' || *ptr == '"')) {
                    ptr++;
                }
            }
            adc_data[p] = static_cast<Short_t>(std::strtol(ptr, &next_ptr, 10));
            ptr = next_ptr;
            if (*ptr == ',') ptr++;
        }
        data[f] = std::make_pair(timeStamp, std::move(adc_data));
    }

return data;
}

std::vector<std::string> getChannels(std::ifstream &csv_file){

  std::vector<std::string> channels;
  
  std::string line;
  std::getline(csv_file, line);
  std::stringstream ss(line);
  std::string str;
  std::getline(ss, str, ':');
    if(str =="#CHANNELS"){
      while (getline(ss, str, ',')){
        str.erase(std::remove_if(str.begin(), str.end(), isspace), str.end());
        channels.push_back(str);
        }
    }
  
  return channels;
}

void csv2root (const std::string &fileName, const std::string &outFileName, int &eventID, double &deadTime){

    std::ifstream csv_file(fileName);
    if (!csv_file.is_open()) {
        std::cerr << "? Error: Cannot open file " << fileName << std::endl;
        return;
    }

   std::cout << "Reading CSV file: '" << fileName << "'" << std::endl;

   auto channels = getChannels(csv_file);
   if(channels.empty()){
        std::cerr << "? No channel found on file '" << fileName << std::endl;
        return;
    }

   auto fout = TFile::Open (outFileName.c_str(), "RECREATE");
   TTree tree ("tree", "Siglent data");
   std::vector<Hit> myHits(channels.size());

     for(int i=0;i<channels.size();i++)
       tree.Branch(channels[i].c_str(), &myHits[i]);

int c=0;

    do {
      int nFrames=0;
      int nPoints=0;
      std::vector< std::vector< std::pair<double, std::vector<Short_t> > > > dataMap(channels.size());
      for(int i=0;i<channels.size();i++){
          auto preamble = read_preamble(csv_file);
          nFrames = preamble["sum_frame"];
          nPoints = preamble["one_frame_pts"];
          auto& hit = myHits[i];
          hit.TDiv     = preamble["tdiv"];
          hit.VDiv     = preamble["vdiv"];
          hit.Offset   = preamble["offset"];
          hit.Delay    = preamble["delay"];
          hit.Interval = preamble["interval"];
          hit.CodePerDiv = preamble["code"];
        
          if (i == 0) deadTime += preamble["deadtime_us"];
          hit.DeadTime = deadTime;
          dataMap[i].reserve(nFrames); 
          dataMap[i] = read_data(csv_file, nFrames, nPoints);
       }

         for(int n=0;n<nFrames;n++){
           for(int i=0;i<channels.size();i++){
             auto& hit = myHits[i];
             auto& dataPair = dataMap[i][n];
             hit.id = eventID;
             hit.TimeStamp = dataPair.first;
             hit.Pulse = std::move(dataPair.second); 
             hit.analyzeHit(); 
            }
           tree.Fill();
           eventID++;
           if(eventID%1000==0)std::cout<<"Processed "<<eventID<<" events "<<std::endl;
         }
    } while(csv_file.peek() != EOF);

  std::cout<<"Done "<<eventID<<" event processed"<<std::endl;
  std::cout << "\nTotal entries filled into tree (before Write): " << tree.GetEntries() << std::endl;

  fout->Write();
  fout->Close();
  
  delete fout;
  csv_file.close();
}


int main(int argc, char ** argv){
 
  std::string fileName="";

    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
        if (arg == "--f") {
            i++;
            fileName = argv[i];
        } else if (arg == "--h") {
            help();
            return 0;
        } else {  // unmatched options
            std::cerr << "Warning invalid argument " << arg << std::endl;
            help();
            return -1;
        }
    }

    if(fileName.empty()){
      std::cerr<<"Please, provide a file name"<<std::endl;
      help();
      return -1;
    }

  struct stat fb;
  int eventID=0;
  double deadTime=0;

    if(stat (fileName.c_str(), &fb) == 0){
      std::string fileRoot = fileName +".root";
      csv2root(fileName, fileRoot, eventID, deadTime);
    } else {
      int nFiles=1;
      char outFileName[1024];
      sprintf(outFileName,"%s.%02d", fileName.c_str(), nFiles);
      std::cout<<outFileName<<std::endl;
        if(stat (outFileName, &fb) !=0){
          std::cerr<<"Filename "<<fileName<<" not found"<<std::endl;
          return -1;
        }
        while(stat (outFileName, &fb) == 0 ){
          std::string fName = outFileName;
          std::string fileRoot = fName +".root";
          csv2root(fName, fileRoot,eventID, deadTime);
          nFiles++;
          sprintf(outFileName,"%s.%02d", fileName.c_str(), nFiles);
        }
    }

}
