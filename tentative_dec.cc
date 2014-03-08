#include "CMSPixelDecoder.h"

#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <iostream>

using namespace std;
using namespace CMSPixel;

void asciiHitmap(std::vector<pixel> * evt) {

    for(size_t col = 0; col < ROCNUMCOLS; col++) {
    std::cout << "-";
    for(size_t row = 0; row < ROCNUMROWS; row++) {
      bool found = false;
      for(std::vector<pixel>::iterator it = evt->begin(); it != evt->end(); ++it) {
	if(it->col == col && it->row == row && it->roc == 0) {
	  found = true;
	  std::cout << "X";
	}
      }
      if(!found) std::cout << " ";
    }
    std::cout << std::endl;
  }
}

int main(int argc, char* argv[]) {

  Log::ReportingLevel() = Log::FromString(argv[1] ? argv[1] : "SUMMARY");

  unsigned int nrocs = 1;
  CMSPixelStatistics global_statistics(nrocs);
  std::vector<pixel> * evt = new std::vector<pixel>;
  std::vector<std::pair<uint8_t,uint8_t> > * rbdat = new std::vector<std::pair<uint8_t,uint8_t> >();
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
      int status = decoder->get_event(evt, rbdat, time);
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

      std::cout << "Readback:" << std::endl;
      for(std::vector<std::pair<uint8_t,uint8_t> >::iterator it = rbdat->begin(); it != rbdat->end(); ++it) {
	std::cout << static_cast<int>(it->second) << " ";
      }
      std::cout << std::endl;

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
