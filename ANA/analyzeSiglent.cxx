
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <sys/stat.h>

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
            }
        }
    }

  return preamble;
}

std::map <std::string, std::vector<Short_t> > read_data(std::ifstream &csv_file, int nFrames, int nPoints){
  std::map <std::string, std::vector<Short_t> > data;
  std::string line;

  for(int f=0;f<nFrames;f++){
    std::vector<Short_t> adc_data(nPoints);
    std::getline(csv_file, line);
    std::string timeStamp;
    std::stringstream ss(line);
    std::getline(ss, timeStamp, ',');
    std::string val;
      for(int p=0;p<nPoints;p++){
        std::getline(ss, val, ',');
        if(p==0)val.erase(0,2);
        std::istringstream(val) >> adc_data[p];
      }
    data[timeStamp] = adc_data;
  }

return data;
}

void csv2root (const std::string &fileName, const std::string &outFileName, int &eventID, double &deadTime){

    std::ifstream csv_file(fileName);
    if (!csv_file.is_open()) {
        std::cerr << "? Error: No se pudo abrir el archivo CSV '" << fileName << "'" << std::endl;
        return;
    }

   auto fout = TFile::Open (outFileName.c_str(), "RECREATE");
   TTree myTree ("tree", "Siglent data");

   Hit hit;
   myTree.Branch("C1",&hit);//TODO make generic for all channels

    std::cout << "Leyendo archivo CSV: '" << fileName << "'" << std::endl;
    do {
      auto preamble = read_preamble(csv_file);
      int nFrames = preamble["sum_frame"];
      int nPoints = preamble["one_frame_pts"];
      hit.TDiv = preamble["tdiv"];
      hit.VDiv = preamble["vdiv"];
      hit.Offset = preamble["offset"];
      hit.Delay = preamble["delay"];
      hit.Interval = preamble["interval"];
      hit.CodePerDiv = preamble["code"];
      //hit.DeadTime += preamble["deadtime_us"]*1E6;

      auto data = read_data(csv_file, nFrames, nPoints);

      for(const auto& [tmpstp, adc] : data){
        hit.id = eventID;
        //hit.TimeStamp = tmpstp; //TBA when python return timestamp
        hit.Pulse = adc;
        hit.analyzeHit();
        myTree.Fill();
        eventID++;
      }
    } while(csv_file.peek() != EOF);

  std::cout<<"Done "<<eventID<<" event processed"<<std::endl;
  
  fout->Write();
  fout->Close();
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
