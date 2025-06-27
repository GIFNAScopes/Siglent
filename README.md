# SiglentDAQ

Data AcQuisition software for Siglent SDS7000A series oscilloscopes. Based on sequence mode acquisition for Siglent scopes https://www.siglent.eu/product/12325419/siglent-sds7304a-h12-4-channel-3ghz-12-bit-20-gsa-s-oscilloscope. The computer communicate with the oscilloscope via ethernet, although other communication protocols may work (USB, GPIB, serial,...) if the proper oscilloscope address is provided.

## Pre-requisites

This project requires ROOT installation (version > 6.24) and some python modules. The requisites for the python modules are under `pythonRequirements.txt` file.

## Installation

Checkout the repository:
```
cd $HOME
git clone  siglent
cd siglent
```
Compile and install:
```
mkdir build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=../install/ 
make -j4 install
```
This will create an install directory from where you can source the repository:

```
source ../install/thisSiglent.sh
```

## Getting started:

Make sure that the oscilloscope is reachable and in the same network Make sure that the config file (aka `siglent.cfg`) contains the proper oscilloscope address `TCPIP0::192.168.0.102::INSTR` in the template config.

Usefull commands:
* Start DAQ with the oscillosope `Siglent_DAQ.py` note that you have to provide a config file, please copy it in your working folder, a template is available under install/config.
* Instead of `root` use `SigRoot` which is the same but with the TDS libraries and macros loaded.
* Analyze data from binary file `analyzeSiglent --f TFIIIxxx.csv`
* Decode binary file and load it in SigRoot `processdata TFIII0000.csv`

TDSRoot (load root, TDS libraries and macros ):
* Load root files (analyzed files) `readData("TFIIIxxx.raw")`
* Draw amplitude (int) vs area `tree->Draw("C1.DC:C1.Area")`
* Histogram with a given bin size and limits `tree->Draw("C1.Area>>h(1000,0,100000)")`
* Save spectrum (histogram) `saveSpc(h,"Test.txt")`
* Create a list after grafical cut `tree->Draw(">>list","CUTG")`
* Draw pulses from the list `drawPulse(i++)`
* Draw all pulses `drawAllPulses("OutName")`
* Reset Event list `tree->SetEventList(0)`

Tree variables:
```
Double_t TimeStamp; // Unix time in seconds seconds
std::vector<Short_t> Pulse; // Vector containing the ADC values of the puls
Double_t High; // Amplitude of the pulse (substracted baseline) in Volts
Short_t Max; // Max bin in points
Double_t Area; // Integral in V*us
Float_t T0; // Pulse start in points (when 15% of the Amplitude is reached)
Float_t TEnd; // Pulse end in points (when 95% of the Integral is reached)
Float_t RT;  // Risetime in points ( 85% -15% of the amplitude)
Float_t Width;  // Pulse width in points at %50% of the amplitude
Int_t DC; // Amplitude but ADC
Float_t Baseline; // Baseline using 50% of Delay in ADCs
Float_t BaselineSigma; // Baseline sigma in ADCs
Bool_t saturated = false; // Saturated pulse?
Int_t id; // Event ID
Double_t TDiv; // Horizontal scale seconds/div
Double_t VDiv; // Vertical scale Volts/div
Double_t Offset; // Pulse offset in Volts
Double_t Interval; // Time interval between 2 points in seconds
Double_t CodePerDiv; // Number of ADCs per div
Double_t Delay; // Time delay in seconds
Double_t DeadTime; //Cumulative deadtime per event in seconds
```


