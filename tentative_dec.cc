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

  unsigned int nrocs = 1;
  CMSPixelStatistics global_statistics(nrocs);
  std::vector<pixel> * evt = new std::vector<pixel>;
  timing time;
  int64_t old_timestamp = 0;
  int flags = 0; //FLAG_OVERWRITE_ROC_HEADER_POS; //FLAG_OLD_RAL_FORMAT;
  
  int events = atoi(argv[2]);
  std::cout << "Decoding " << events << " events." << std::endl;

  for (int i = 3; i < argc; ++i) {
    std::cout << "Trying to decode " << argv[i] << std::endl;
    CMSPixelFileDecoder * decoder = new CMSPixelFileDecoderPSI_DTB(argv[i],nrocs,flags,ROC_PSI46DIGV2,"");
    //CMSPixelFileDecoder * decoder = new CMSPixelFileDecoderRAL(argv[i],nrocs,flags,ROC_PSI46DIGV2);

    FILE * pFile;
    pFile = fopen("faileddecoding.dat","w");

    for(int j = 0; j < events; ++j) {
      int status = decoder->get_event(evt, time);
      //std::cout <<"Return: " << status << std::endl;
      if(status  <= DEC_ERROR_NO_MORE_DATA) break;

      // Get raw data of everything which has errors in it:
      if(status < DEC_ERROR_EMPTY_EVENT) {
	std::vector<uint16_t> raw = decoder->get_rawdata();
	std::cout << "Got raw data with length of " << raw.size() << std::endl;
	for(std::vector<uint16_t>::iterator it = raw.begin(); it != raw.end(); ++it) {
	  std::cout << std::hex << std::setw(4) << std::setfill('0') << (int)(*it) << " ";
	}
	std::cout << std::endl;

	// Write to file:
	fwrite (&raw[0], sizeof(uint16_t), raw.size(), pFile);
      }

      if(time.timestamp < old_timestamp) {
	LOG(logWARNING) << "Timestamps not monotonically increasing!";
      }
      old_timestamp = time.timestamp;

    }

    fclose(pFile);
    global_statistics.update(decoder->statistics);
    delete decoder;
  }

  //  global_statistics.print();
  return 0;
}
