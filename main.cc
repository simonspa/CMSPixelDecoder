#include "CMSPixelDecoder.h"

#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <iostream>


using namespace std;
using namespace CMSPixel;

int main(int argc, char* argv[]) {

  Log::ReportingLevel() = Log::FromString(argv[1] ? argv[1] : "DEBUG1");

  const char * _cmsFile = "mtb.bin.dig";
  unsigned int _noOfROC = 1;
  unsigned int flaggen = 0;//FLAG_HAVETBM;//FLAG_ALLOW_CORRUPT_ROC_HEADERS;
  std::vector<event> * evt = new std::vector<event>;
  long int timestamp = 0;
  uint8_t ROCTYPE = ROC_PSI46DIG;
  std::cout << "We have " << _noOfROC << " ROC of type " << static_cast<int>(ROCTYPE) << std::endl;
  std::cout << "The flags are set to " << flaggen << std::endl;
  levelset testlevels;

  CMSPixelFileDecoder * dec = new CMSPixelFileDecoderPSI_ATB(_cmsFile,_noOfROC,flaggen,ROCTYPE,"addressParameters.dat.single");

  for (int i = 0; i < 12; i++) {
    if(dec->get_event(evt, timestamp) == DEC_ERROR_INVALID_FILE) break;
    //    std::cout << "Pixels in this event: " << dec->evt->statistics.pixels_valid << std::endl;
    //    std::cout << "Pixels in total:      " << dec->statistics.pixels_valid << std::endl;
    //    std::cout << std::endl;
    }

  while(0) {
    if(dec->get_event(evt, timestamp) < DEC_ERROR_NO_OF_ROCS) break;
    //    std::cout << std::endl;
  }

  dec->statistics.print();

  delete evt;
  delete dec;
  return 0;

  std::cout << "Now running singe call tests...";

  CMSPixelEventDecoder * singledec;
  if(ROCTYPE & ROC_PSI46V2 || ROCTYPE & ROC_PSI46XDB)
    singledec = new CMSPixelEventDecoderAnalog(_noOfROC,flaggen,ROCTYPE,testlevels);
  else
    singledec = new CMSPixelEventDecoderDigital(_noOfROC,flaggen,ROCTYPE);

  delete singledec;
}
