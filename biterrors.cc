#include "CMSPixelDecoder.h"

#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <iostream>

using namespace std;
using namespace CMSPixel;

double biterrorrate = 4.7E-4;

bool flip() {
  // Bit error rate:
  return rand()%10000000 < (biterrorrate * 10000000);
}

void simpleBitErrors(std::vector<uint16_t> & raw) {
 
  for(std::vector<uint16_t>::iterator it = raw.begin(); it != raw.end(); it++) {
    for(size_t bit = 0; bit < 16; bit++) {
      // Probability to flip the bit:
      if(flip()) (*it) ^= 1<<bit;
    }
  }
}

void transitionBitErrors(std::vector<uint16_t> & raw) {
  // Bit error rate:
  srand((unsigned)time(0)); 
 
  bool oldbit = false;
  for(std::vector<uint16_t>::iterator it = raw.begin(); it != raw.end(); it++) {
    for(size_t bit = 15; bit > 0; bit--) {
      // Have a signal transition?
      bool newbit = ((*it)>>bit)&1;
      if(oldbit^newbit && flip()) {
	oldbit = newbit;
	(*it) ^= 1<<bit;
      }
    }
  }
}

int main(int argc, char* argv[]) {

  srand((unsigned)time(0));
  Log::ReportingLevel() = Log::FromString("SUMMARY");

  std::vector<pixel> * evt = new std::vector<pixel>;
  timing time;
  unsigned int num_rocs = 8;
  size_t max_events = 9999999;
  std::vector<std::string> files;

  for (int i = 1; i < argc; i++) {
    // Setting number of expected ROCs:
    if (!strcmp(argv[i],"-n")) { num_rocs = atoi(argv[++i]); }
    // Maximum events:
    if (!strcmp(argv[i],"-e")) { max_events = atoi(argv[++i]); }
    // Set the verbosity level:
    else if (!strcmp(argv[i],"-v")) { Log::ReportingLevel() = Log::FromString(argv[++i]); }
    // add to the list of files to be processed:
    else { files.push_back(string(argv[i])); }
  }

  CMSPixelStatistics myStatistics(num_rocs);

  for (std::vector<std::string>::iterator it = files.begin(); it != files.end(); ++it) {
    
    std::cout << "Going to decode " << (*it) << std::endl;
    CMSPixelFileDecoder * decoder = new CMSPixelFileDecoderRAL((*it).c_str(),num_rocs,0,ROC_PSI46DIGV21);
    CMSPixelEventDecoder * redecoder = new CMSPixelEventDecoderDigital(num_rocs,FLAG_16BITS_PER_WORD,ROC_PSI46DIGV21);

    int status;
    size_t event = 0;

    while(1) {
      status = decoder->get_event(evt, time);
      if(status <= DEC_ERROR_NO_MORE_DATA) { break; }

      // Select fine events:
      if(!(status < DEC_ERROR_EMPTY_EVENT)) {

	if(event > max_events) { break; }
	if(event%1000 == 0) std::cout << event << std::endl;
	event++;
	
	// Get the raw data for this event:
	std::vector<uint16_t> raw = decoder->get_eventdata();

	// Introduce bit errors:
	//simpleBitErrors(raw);
	transitionBitErrors(raw);

	// Redecode:
	std::vector<pixel> * redec_evt = new std::vector<pixel>();
	redecoder->get_event(raw, redec_evt);
	myStatistics.update(redecoder->statistics);
	myStatistics.head_data++;
      }
    }

    delete decoder;
  }

  // Print filtered statistics:
  std::cout << "Redecoding statistics, BERR=" << biterrorrate << std::endl;
  myStatistics.print();
  return 0;
}
