#include "CMSPixelDecoder.h"

#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <ctime>

using namespace std;
using namespace CMSPixel;

bool unit_tests();

int main(int argc, char* argv[]) {

  Log::ReportingLevel() = Log::FromString(argv[1] ? argv[1] : "DEBUG1");

  const char * _cmsFile = "mtb.bin.tel.ral";
  const char * _cms2File = "mtb.bin.dig";
  unsigned int _noOfROC = 1;
  unsigned int flaggen = FLAG_ALLOW_CORRUPT_ROC_HEADERS;//0;//FLAG_HAVETBM;//FLAG_ALLOW_CORRUPT_ROC_HEADERS//FLAG_12BITS_PER_WORD;
  std::vector<event> * evt = new std::vector<event>;
  long int timestamp = 0;
  uint8_t ROCTYPE = ROC_PSI46DIG;
  std::cout << "We have " << _noOfROC << " ROC of the type " << static_cast<int>(ROCTYPE) << std::endl;
  std::cout << "The flags are set to " << flaggen << std::endl;
  levelset testlevels;

  CMSPixelFileDecoder * dec = new CMSPixelFileDecoderPSI_ATB(_cms2File,_noOfROC,flaggen,ROCTYPE,"addressParameters.dat.single");
  //CMSPixelFileDecoder * dec = new CMSPixelFileDecoderRAL(_cmsFile,_noOfROC,flaggen,ROCTYPE);

  for (int i = 0; i < 12; i++) {
    if(dec->get_event(evt, timestamp) <= DEC_ERROR_NO_MORE_DATA) break;
    //    std::cout << "Pixels in this event: " << dec->evt->statistics.pixels_valid << std::endl;
    //    std::cout << "Pixels in total:      " << dec->statistics.pixels_valid << std::endl;
    //    std::cout << std::endl;
    }

  dec->statistics.print();

  delete evt;
  delete dec;



  // ###################################################################################

  std::cout << "Now running singe call tests...\n";
  Log::ReportingLevel() = Log::FromString("INFO");

  std::vector<event> * singleevt = new std::vector<event>;
  std::vector<int16_t> singledat;
  CMSPixelEventDecoder * singledec;

  std::cout << "RAL IPBus:\n";
  // Trying IPBus:
  singledat.push_back(0x7f87);
  singledat.push_back(0xf87f);
  singledat.push_back(0x87f8);
  singledat.push_back(0x7f87);
  singledat.push_back(0xfa77);
  singledat.push_back(0xd400);
  singledat.push_back(0x7f87);
  singledat.push_back(0xf84b);

  flaggen = FLAG_16BITS_PER_WORD | FLAG_ALLOW_CORRUPT_ROC_HEADERS;
  singledec = new CMSPixelEventDecoderDigital(8,flaggen,ROC_PSI46DIG);
  std::cout << "Return: " << singledec->get_event(singledat,singleevt) << std::endl;
  delete singledec;
  singledat.clear();

  std::cout << "PSI Digital 4bit:\n";
  singledat.push_back(0x000f);
  singledat.push_back(0x000f);
  singledat.push_back(0x0007);
  singledat.push_back(0x000f);
  singledat.push_back(0x0008);
  singledat.push_back(0x0003);
  singledat.push_back(0x0001);
  singledat.push_back(0x000d);
  singledat.push_back(0x0007);
  singledat.push_back(0x0006);
  singledat.push_back(0x0002);
  singledat.push_back(0x000f);
  singledat.push_back(0x000f);
  singledat.push_back(0x000f);
  singledat.push_back(0x000f);
  singledat.push_back(0x000f);

  flaggen = FLAG_ALLOW_CORRUPT_ROC_HEADERS;
  singledec = new CMSPixelEventDecoderDigital(1,flaggen,ROC_PSI46DIG);
  std::cout << "Return: " << singledec->get_event(singledat,singleevt) << std::endl;
  delete singledec;
  singledat.clear();


  std::cout << "PSI Digital 12bit:\n";
  singledat.push_back(0x0ff7);
  singledat.push_back(0x0f83);
  singledat.push_back(0x01d7);
  singledat.push_back(0x062f);
  singledat.push_back(0x0fff);

  flaggen = FLAG_12BITS_PER_WORD | FLAG_ALLOW_CORRUPT_ROC_HEADERS;
  singledec = new CMSPixelEventDecoderDigital(1,flaggen,ROC_PSI46DIG);
  std::cout << "Return: " << singledec->get_event(singledat,singleevt) << std::endl;
  delete singledec;
  singledat.clear();


  std::cout << "PSI Digital 16bit:\n";
  singledat.push_back(0xff7f);
  singledat.push_back(0x831d);
  singledat.push_back(0x762f);

  flaggen = FLAG_16BITS_PER_WORD | FLAG_ALLOW_CORRUPT_ROC_HEADERS;
  singledec = new CMSPixelEventDecoderDigital(1,flaggen,ROC_PSI46DIG);
  std::cout << "Return: " << singledec->get_event(singledat,singleevt) << std::endl;
  delete singledec;
  singledat.clear();


  delete singleevt;

  //return 0;

  if(!unit_tests()) std::cout << "Unit testing failed!" << std::endl;
  else std::cout << "Unit testing successfully completed." << std::endl;

  return 0;
}

bool compare(CMSPixelStatistics reference, CMSPixelStatistics measurement)
{
  /*  std::cout << "REF" << std::endl;
  reference.print();
  std::cout << "MEAS" << std::endl;
  measurement.print();*/

  if(reference.head_data != measurement.head_data) return false;
  if(reference.head_trigger != measurement.head_trigger) return false;
  if(reference.evt_valid != measurement.evt_valid) return false;
  if(reference.evt_empty != measurement.evt_empty) return false;
  if(reference.evt_invalid != measurement.evt_invalid) return false;
  if(reference.pixels_valid != measurement.pixels_valid) return false;
  if(reference.pixels_invalid != measurement.pixels_invalid) return false;

  return true;
}

bool test_analog_single()
{
  std::cout << "Unit testing: Analog Single ROC" << std::endl;

  // REFERENCE:
  CMSPixelStatistics ref;
  ref.head_data = ref.head_trigger = 109495;
  ref.evt_empty = 84265; //77665;
  ref.evt_valid = 25229; //31829;
  ref.pixels_valid = 29954; //37105;
  ref.evt_invalid = 0;
  ref.pixels_invalid = 8718; //11586;
  double ref_timing = 0.26;

  std::vector<event> * evt = new std::vector<event>;
  long int timestamp;
  CMSPixelFileDecoder * dec = new CMSPixelFileDecoderPSI_ATB("mtb.bin.ana",1,FLAG_ALLOW_CORRUPT_ROC_HEADERS,ROC_PSI46V2,"addressParameters.dat.single");

  clock_t begin = clock();
  while(1)
    if(dec->get_event(evt, timestamp) <= DEC_ERROR_NO_MORE_DATA) break;
  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
  std::cout << "     Timing: " << elapsed_secs << "sec, " << dec->statistics.head_data/elapsed_secs << " events/sec." << std::endl;
  delete evt;
  dec->statistics.print();

  // Timing check:
  if(elapsed_secs <= 0.92*ref_timing || elapsed_secs >= 1.08*ref_timing) {
    std::cout << "     Timing requirements NOT met!" << std::endl;
  }

  // Decoding check:
  if(!compare(ref,dec->statistics)) {
    delete dec;
    return false;
  }
  else {
    delete dec;
    return true;
  }
}

bool test_digital_single()
{
  std::cout << "Unit testing: Digital Single ROC" << std::endl;

  // REFERENCE:
  CMSPixelStatistics ref;
  ref.head_data = ref.head_trigger = 165195;
  ref.evt_empty = 87801;
  ref.evt_valid = 75522;
  ref.pixels_valid = 168980;
  ref.evt_invalid = 1871;
  ref.pixels_invalid = 2368;
  double ref_timing = 1.69;

  std::vector<event> * evt = new std::vector<event>;
  long int timestamp;
  CMSPixelFileDecoder * dec = new CMSPixelFileDecoderPSI_ATB("mtb.bin.dig",1,FLAG_ALLOW_CORRUPT_ROC_HEADERS,ROC_PSI46DIG,"");

  clock_t begin = clock();
  while(1)
    if(dec->get_event(evt, timestamp) <= DEC_ERROR_NO_MORE_DATA) break;
  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
  std::cout << "     Timing: " << elapsed_secs << "sec, " << dec->statistics.head_data/elapsed_secs << " events/sec." << std::endl;
  delete evt;
  dec->statistics.print();

  // Timing check:
  if(elapsed_secs <= 0.92*ref_timing || elapsed_secs >= 1.08*ref_timing) {
    std::cout << "     Timing requirements NOT met!" << std::endl;
  }

  // Decoding check:
  if(!compare(ref,dec->statistics)) {
    delete dec;
    return false;
  }
  else {
    delete dec;
    return true;
  }
}

bool test_analog_module()
{
  std::cout << "Unit testing: Analog Module w/ 16 ROCs" << std::endl;

  // REFERENCE:
  CMSPixelStatistics ref;
  ref.head_data = ref.head_trigger = 263391;
  ref.evt_empty = 52739;
  ref.evt_valid = 210651;
  ref.pixels_valid = 1488121;
  ref.evt_invalid = 0;
  ref.pixels_invalid = 0;
  double ref_timing = 3.76;

  std::vector<event> * evt = new std::vector<event>;
  long int timestamp;
  CMSPixelFileDecoder * dec = new CMSPixelFileDecoderPSI_ATB("mtb.bin.mod.ana",16,FLAG_HAVETBM,ROC_PSI46V2,"addressParameters.dat.mod");

  clock_t begin = clock();
  while(1)
    if(dec->get_event(evt, timestamp) <= DEC_ERROR_NO_MORE_DATA) break;
  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
  std::cout << "     Timing: " << elapsed_secs << "sec, " << dec->statistics.head_data/elapsed_secs << " events/sec." << std::endl;
  delete evt;
  dec->statistics.print();

  // Timing check:
  if(elapsed_secs <= 0.92*ref_timing || elapsed_secs >= 1.08*ref_timing) {
    std::cout << "     Timing requirements NOT met!" << std::endl;
  }

  // Decoding check:
  if(!compare(ref,dec->statistics)) {
    delete dec;
    return false;
  }
  else {
    delete dec;
    return true;
  }
}

bool test_telescope_psi()
{
  std::cout << "Unit testing: Digital ROC Telescope w/ PSI readout" << std::endl;

  // REFERENCE:
  CMSPixelStatistics ref;
  ref.head_trigger = 24954;
  ref.head_data = 24816;
  ref.evt_empty = 1854;
  ref.evt_valid = 19579;
  ref.pixels_valid = 1336046;
  ref.evt_invalid = 3382;
  ref.pixels_invalid = 69444;
  double ref_timing = 7.3;

  std::vector<event> * evt = new std::vector<event>;
  long int timestamp;
  CMSPixelFileDecoder * dec = new CMSPixelFileDecoderPSI_ATB("mtb.bin.tel.psi",8,FLAG_ALLOW_CORRUPT_ROC_HEADERS,ROC_PSI46DIG,"");

  clock_t begin = clock();
  while(1)
    if(dec->get_event(evt, timestamp) <= DEC_ERROR_NO_MORE_DATA) break;
  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
  std::cout << "     Timing: " << elapsed_secs << "sec, " << dec->statistics.head_data/elapsed_secs << " events/sec." << std::endl;
  delete evt;
  dec->statistics.print();

  // Timing check:
  if(elapsed_secs <= 0.92*ref_timing || elapsed_secs >= 1.08*ref_timing) {
    std::cout << "     Timing requirements NOT met: " << ref_timing << "sec." << std::endl;
  }

  // Decoding check:
  if(!compare(ref,dec->statistics)) {
    std::cout << "Statistics comparison failed." << std::endl;
    delete dec;
    return false;
  }
  else {
    delete dec;
    return true;
  }
}

bool test_telescope_ral()
{
  std::cout << "Unit testing: Digital ROC Telescope w/ RAL IPBus readout" << std::endl;

  // REFERENCE:
  CMSPixelStatistics ref;
  ref.head_trigger = 0;
  ref.head_data = 27328;
  ref.evt_empty = 9705;
  ref.evt_valid = 9660;
  ref.pixels_valid = 428704;
  ref.evt_invalid = 7962;
  ref.pixels_invalid = 116826;
  double ref_timing = 1.78;

  std::vector<event> * evt = new std::vector<event>;
  long int timestamp;
  CMSPixelFileDecoder * dec = new CMSPixelFileDecoderRAL("mtb.bin.tel.ral",8,FLAG_ALLOW_CORRUPT_ROC_HEADERS,ROC_PSI46DIG);

  clock_t begin = clock();
  while(1)
    if(dec->get_event(evt, timestamp) <= DEC_ERROR_NO_MORE_DATA) break;
  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
  std::cout << "     Timing: " << elapsed_secs << "sec, " << dec->statistics.head_data/elapsed_secs << " events/sec." << std::endl;
  delete evt;
  dec->statistics.print();

  // Timing check:
  if(elapsed_secs <= 0.95*ref_timing || elapsed_secs >= 1.05*ref_timing) {
    std::cout << "     Timing requirements NOT met!" << std::endl;
  }

  // Decoding check:
  if(!compare(ref,dec->statistics)) {
    std::cout << "Statistics comparison failed." << std::endl;
    delete dec;
    return false;
  }
  else {
    delete dec;
    return true;
  }
}

bool unit_tests() {

  std::cout << "Start data-driven unit testing..." << std::endl;

  Log::ReportingLevel() = Log::FromString("SUMMARY");

  if(!test_analog_single()) return false;
  if(!test_digital_single()) return false;

  if(!test_analog_module()) return false;
  //if(!test_digital_module()) return false;

  if(!test_telescope_psi()) return false;
  if(!test_telescope_ral()) return false;

  return true;
}
