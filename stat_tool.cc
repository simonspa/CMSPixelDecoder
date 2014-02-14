#include "CMSPixelDecoder.h"

#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <iostream>

using namespace std;
using namespace CMSPixel;

int main(int argc, char* argv[]) {

  Log::ReportingLevel() = Log::FromString("SUMMARY");

  CMSPixelStatistics global_statistics;
  std::vector<pixel> * evt = new std::vector<pixel>;
  timing time;
  unsigned int num_rocs = 8;
  std::vector<std::string> files;

  for (int i = 1; i < argc; i++) {
    // Setting number of expected ROCs:
    if (!strcmp(argv[i],"-n")) { num_rocs = atoi(argv[++i]); }
    // Set the verbosity level:
    else if (!strcmp(argv[i],"-v")) { Log::ReportingLevel() = Log::FromString(argv[++i]); }
    // add to the list of files to be processed:
    else { files.push_back(string(argv[i])); }
  }

  for (std::vector<std::string>::iterator it = files.begin(); it != files.end(); ++it) {
    
    std::cout << "Trying to decode " << (*it) << std::endl;
    CMSPixelFileDecoder * decoder = new CMSPixelFileDecoderRAL((*it).c_str(),num_rocs,0,ROC_PSI46DIGV2);

    while(1)
      if(decoder->get_event(evt, time) <= DEC_ERROR_NO_MORE_DATA) break;
      
    global_statistics.update(decoder->statistics);
    //    decoder->statistics.print();
    delete decoder;
  }

  global_statistics.print();
  return 0;
}
