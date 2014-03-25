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
  bool write_badevents = false, consider_dataphase = false, have_psidtb = false;
  std::string file_badevents;
  uint8_t dataphase = 0;
  std::ofstream badevents;
  size_t max_events = 999999999;

  for (int i = 1; i < argc; i++) {
    // Setting number of expected ROCs:
    if (!strcmp(argv[i],"-n")) { 
       num_rocs = atoi(argv[++i]);
       std::cout << "Decoding data from " << num_rocs << " ROCs." << std::endl;
    } 
    // Maximum events:
    if (!strcmp(argv[i],"-e")) { 
      max_events = atoi(argv[++i]);
      std::cout << "Decoding a maximum number of " << max_events << " events." << std::endl;
    }
    // Set the verbosity level:
    else if (!strcmp(argv[i],"-v")) { Log::ReportingLevel() = Log::FromString(argv[++i]); }
    // Set option for bad event output file
    else if(!strcmp(argv[i],"-o")) { 
      write_badevents = true;
      file_badevents = string(argv[++i]);
      std::cout << "Filtering for bad events, will write bad events to file " 
		<< file_badevents << std::endl;
    }
    // Option to filter one data phase:
    else if(!strcmp(argv[i],"-dp")) {
      consider_dataphase = true;
      dataphase = atoi(argv[++i]);
      std::cout << "Filtering for dataphase " << static_cast<int>(dataphase) 
		<< ", will print additional filtered statistics." << std::endl;
    }
    // Option to filter one data phase:
    else if(!strcmp(argv[i],"-dtb")) {
      have_psidtb = true;
      std::cout << "Data from PSI DTB board." << std::endl;
    }
    // add to the list of files to be processed:
    else { files.push_back(string(argv[i])); }
  }

  CMSPixelStatistics myStatistics(num_rocs);
  if(write_badevents) { badevents.open(file_badevents.c_str(), std::ios::out | std::ios::binary); }

  for (std::vector<std::string>::iterator it = files.begin(); it != files.end(); ++it) {
    
    std::cout << "Trying to decode " << (*it) << std::endl;
    CMSPixelFileDecoder * decoder;
    if(have_psidtb) {
       decoder = new CMSPixelFileDecoderPSI_DTB((*it).c_str(),num_rocs,0,ROC_PSI46DIGV21,"");
    }
    else {
       decoder = new CMSPixelFileDecoderRAL((*it).c_str(),num_rocs,0,ROC_PSI46DIGV2);
    }

    int status;
    size_t events = 0;

    while(1) {
      status = decoder->get_event(evt, time);
      if(status <= DEC_ERROR_NO_MORE_DATA) { break; }
      if(events > max_events) { break; }

      // We want to extract bad events:
      if(write_badevents && status < DEC_ERROR_EMPTY_EVENT) {
	std::vector<uint16_t> raw = decoder->get_rawdata();
	badevents.write(reinterpret_cast<const char*>(&raw.at(0)), sizeof(raw.at(0))*raw.size());
      }

      // Update our statistics depending on dataphase settings:
      if(consider_dataphase && time.data_phase == dataphase) { myStatistics.update(decoder->evt->statistics); }

      events++;
    }

    delete decoder;
  }

  // Close the badevents file:
  if(write_badevents) { badevents.close(); }

  // Print filtered statistics for one dataphase only:
  if(consider_dataphase) {
    std::cout << "Filtered statistics for data phase " 
	      << static_cast<int>(dataphase) << ":" << std::endl;
    myStatistics.print();
  }
  return 0;
}
