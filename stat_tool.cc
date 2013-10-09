#include "CMSPixelDecoder.h"

#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <iostream>

using namespace std;
using namespace CMSPixel;

int main(int argc, char* argv[]) {

  Log::ReportingLevel() = Log::FromString(argv[1] ? argv[1] : "SUMMARY");

  CMSPixelStatistics global_statistics;
  std::vector<pixel> * evt = new std::vector<pixel>;
  timing time;

  for (int i = 2; i < argc; ++i) {

    std::cout << "Trying to decode " << argv[i] << std::endl;
    CMSPixelFileDecoder * decoder = new CMSPixelFileDecoderRAL(argv[i],8,0,ROC_PSI46DIGV2);

    while(1)
      if(decoder->get_event(evt, time) <= DEC_ERROR_NO_MORE_DATA) break;
      
    global_statistics.update(decoder->statistics);
    //    decoder->statistics.print();
    delete decoder;
  }

  global_statistics.print();
  return 0;
}
