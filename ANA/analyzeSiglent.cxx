
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

std::deque< std::pair<double, std::vector<Short_t> > > read_data(std::ifstream &csv_file, int nFrames, int nPoints){
  std::deque< std::pair<double, std::vector<Short_t> > > data(nFrames);
  std::string line;

  for(int f=0;f<nFrames;f++){
    std::vector<Short_t> adc_data(nPoints);
    std::getline(csv_file, line);
    std::string val;
    std::stringstream ss(line);
    std::getline(ss, val, ',');
    double timeStamp;
    std::istringstream(val) >> timeStamp;
      for(int p=0;p<nPoints;p++){
        std::getline(ss, val, ',');
        if(p==0)val.erase(0,2);
        std::istringstream(val) >> adc_data[p];
      }
    data[f] = std::make_pair(timeStamp, adc_data);
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

    do {
      int nFrames=0;
      int nPoints=0;
      std::vector< std::deque< std::pair<double, std::vector<Short_t> > > > dataMap(channels.size());
      for(int i=0;i<channels.size();i++){
          auto preamble = read_preamble(csv_file);
          nFrames = preamble["sum_frame"];
          nPoints = preamble["one_frame_pts"];
          myHits[i].TDiv = preamble["tdiv"];
          myHits[i].VDiv = preamble["vdiv"];
          myHits[i].Offset = preamble["offset"];
          myHits[i].Delay = preamble["delay"];
          myHits[i].Interval = preamble["interval"];
          myHits[i].CodePerDiv = preamble["code"];
          deadTime += preamble["deadtime_us"];
          myHits[i].DeadTime = deadTime;
          dataMap[i] = read_data(csv_file, nFrames, nPoints);
       }

         for(int n=0;n<nFrames;n++){
           for(int i=0;i<channels.size();i++){
             const auto& [tmstp, adc] = dataMap[i].front();
             myHits[i].id = eventID;
             myHits[i].TimeStamp = tmstp;
             myHits[i].Pulse = adc;
             myHits[i].analyzeHit();
             dataMap[i].pop_front();
            }
           tree.Fill();
           eventID++;
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
