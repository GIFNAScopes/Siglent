#include <TApplication.h>
#include <TROOT.h>
#include <TRint.h>
#include <TSystem.h>
#include <TStyle.h>


int main(int argc, char* argv[]) {

  printf("---------------------Wellcome to SigRoot--------------------\n");

  gSystem->Load("libSIGANA.so");
  gSystem->AddIncludePath(" -I$SIG_INCLUDE_PATH");
  gROOT->ProcessLine(".L  $SIG_PATH/macros/utilsSiglent.C");

  gStyle->SetPalette(1);
  gStyle->SetTimeOffset(0);
  // display root's command line
  TRint theApp("App", &argc, argv);
  theApp.Run();

  return 0;
}
