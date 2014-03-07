#include "CMSPixelDecoder.h"

#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

using namespace std;
using namespace CMSPixel;

int main(int argc, char* argv[]) {

  Log::ReportingLevel() = Log::FromString("DEBUG4");

  std::vector<pixel> * evt = new std::vector<pixel>;
  std::vector<uint16_t> singledat;
  CMSPixelEventDecoder * singledec;

  unsigned int num_rocs = 8;
  std::string filename;

  for (int i = 1; i < argc; i++) {
    // Setting number of expected ROCs:
    if (!strcmp(argv[i],"-n")) { num_rocs = atoi(argv[++i]); }
    // Set the verbosity level:
    else if (!strcmp(argv[i],"-v")) { Log::ReportingLevel() = Log::FromString(argv[++i]); }
    // add to the list of files to be processed:
    else { filename = string(argv[i]); }
  }

  // Read in all strings from file:
  ifstream file(filename.c_str());
  do {
    std::string buffer;
    getline(file,buffer);
    
    // found last line:
    if(buffer.compare("mpdud Enenevg ints") == 0) break;

    uint32_t x;
    std::stringstream ss;
    ss << std::hex << buffer;
    ss >> x;
    //std::cout << buffer << " is " << std::hex << x << std::dec << std::endl;
    singledat.push_back((x>>16)&0xffff);
    singledat.push_back(x&0xffff);
  } while(!file.eof());

  // Check if the data is demangled:
  if(singledat.at(0) == 0xdead) {
    std::cout << "Mangled data..." << std::endl;
    return -1;
  }

  // Delete the header part:
  singledat.erase(singledat.begin(),singledat.begin()+2);
  // Delete last 14 bytes:
  singledat.erase(singledat.end()-10,singledat.end());

  std::cout << "Data length: " << singledat.size() << std::endl;

  singledec = new CMSPixelEventDecoderDigital(num_rocs,FLAG_16BITS_PER_WORD,ROC_PSI46DIGV21);

  std::cout << "Return: " << singledec->get_event(singledat,evt) << std::endl;
  singledec->statistics.print();
  delete singledec;
  singledat.clear();

  delete evt;
  return 0;
}
