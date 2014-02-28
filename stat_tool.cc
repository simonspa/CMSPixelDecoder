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

  std::vector<pixel> * evt = new std::vector<pixel>;
  timing time;
  unsigned int num_rocs = 8;
  std::vector<std::string> files;
  bool write_badevents = false;
  std::string file_badevents;
  FILE * badevents;

  for (int i = 1; i < argc; i++) {
    // Setting number of expected ROCs:
    if (!strcmp(argv[i],"-n")) { num_rocs = atoi(argv[++i]); }
    // Set the verbosity level:
    else if (!strcmp(argv[i],"-v")) { Log::ReportingLevel() = Log::FromString(argv[++i]); }
    else if(!strcmp(argv[i],"-o")) { 
      write_badevents = true;
      file_badevents = string(argv[++i]);
    }
    // add to the list of files to be processed:
    else { files.push_back(string(argv[i])); }
  }

  CMSPixelStatistics global_statistics(num_rocs);
  if(write_badevents) { badevents = fopen(file_badevents.c_str(),"w"); }

  for (std::vector<std::string>::iterator it = files.begin(); it != files.end(); ++it) {
    
    std::cout << "Trying to decode " << (*it) << std::endl;
    CMSPixelFileDecoder * decoder = new CMSPixelFileDecoderRAL((*it).c_str(),num_rocs,0,ROC_PSI46DIGV2);

    int status;

    while(1) {
      status = decoder->get_event(evt, time);
      if(status <= DEC_ERROR_NO_MORE_DATA) { break; }
      // We want to extract bad events:
      else if(write_badevents && status < DEC_ERROR_EMPTY_EVENT) {
	std::vector<uint16_t> raw = decoder->get_rawdata();
	fwrite (&raw[0], sizeof(uint16_t), raw.size(), badevents);
      }
    }

    if(write_badevents) { fclose(badevents); }
    global_statistics.update(decoder->statistics);
    //    decoder->statistics.print();
    delete decoder;
  }

  global_statistics.print();
  return 0;
}
