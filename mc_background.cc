#include "CMSPixelDecoder.h"

#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <time.h>
#include <limits>
#include <inttypes.h>

using namespace std;
using namespace CMSPixel;

int main(int argc, char* argv[]) {

  (void)argc;
  Log::ReportingLevel() = Log::FromString(argv[1] ? argv[1] : "SUMMARY");
  uint32_t ihit;
  uint32_t failed = 0;
  uint32_t valid = 0;
  uint32_t samples;

  std::vector<uint16_t> data;
  std::vector<pixel> * evt = new std::vector<pixel>;

  CMSPixelEventDecoder * evtdec = new CMSPixelEventDecoderDigital(1,FLAG_16BITS_PER_WORD | FLAG_ALLOW_CORRUPT_ROC_HEADERS,ROC_PSI46DIG);

  /* initialize random seed: */
  srand (time(NULL));

  std::cout << "Generating and analysing " << (std::numeric_limits<uint32_t>::max() - std::numeric_limits<uint32_t>::min())/10 << " samples.\n";
  for(samples = std::numeric_limits<uint32_t>::min(); samples < (10); samples++) {
    data.clear();
    // generate random hit patterns (24bit = max. 16,777,215):
    ihit = rand() | rand()<<16;

    // Add fake ROC header:
    data.push_back(0xf7f8);

    // Adding the hit to the data vector:
    data.push_back(ihit&0xffff);
    data.push_back(ihit&0xff0000);

    // Decode the event and check the result
    evtdec->get_event(data, evt);
    failed += evtdec->statistics.pixels_invalid;
    valid += evtdec->statistics.pixels_valid;

    if(samples%100000 == 0) std::cout << "At " << samples << " samples.\n";
    if(samples%1000000 == 0) std::cout << "Currently " << (double)valid/samples << " samples.\n";
  }

  std::cout << "Samples generated:  " << samples - std::numeric_limits<uint32_t>::min() << std::endl;
  std::cout << "Valid pixel hits:   " << valid << std::endl;
  std::cout << "Invalid pixel hits: " << failed << std::endl;

  std::cout << "Fraction of false positives: " << (double)valid/samples << std::endl;

  delete evtdec;
  return 0;
}
