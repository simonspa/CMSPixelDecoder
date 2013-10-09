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
bool read_address_levels(const char* levelsFile, unsigned int rocs, levelset & addressLevels);

int main(int argc, char* argv[]) {

  (void)argc;
  Log::ReportingLevel() = Log::FromString(argv[1] ? argv[1] : "DEBUG1");

  // ###################################################################################

  std::cout << "Now running singe call tests...\n";
  Log::ReportingLevel() = Log::FromString("DEBUG4");

  std::vector<pixel> * singleevt = new std::vector<pixel>;
  std::vector<uint16_t> singledat;
  CMSPixelEventDecoder * singledec;
  int return_value = 0;


  std::cout << "PSI46 Analog:\n";

  levelset addressLevels;
  read_address_levels("addressParameters.dat",1,addressLevels);

  singledat.push_back(0x0d12);
  singledat.push_back(0x0000);
  singledat.push_back(0x1234);

  singledat.push_back(0x40ba);
  singledat.push_back(0x0f44);
  singledat.push_back(0x0c68);
  singledat.push_back(0x002c);
  singledat.push_back(0x0250);
  singledat.push_back(0x0014);

  singledat.push_back(0x40ba);
  singledat.push_back(0x0d12);
  singledat.push_back(0x0d12);
  singledat.push_back(0x1234);
  singledat.push_back(0x0345);
  singledat.push_back(0x0014);

  int flaggen = FLAG_ALLOW_CORRUPT_ROC_HEADERS;
  singledec = new CMSPixelEventDecoderAnalog(1,flaggen,ROC_PSI46V2,addressLevels);
  std::cout << "Return: " << (return_value += singledec->get_event(singledat,singleevt)) << std::endl;
  delete singledec;
  singledat.clear();


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
  std::cout << "Return: " << (return_value += singledec->get_event(singledat,singleevt)) << std::endl;
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
  std::cout << "Return: " << (return_value += singledec->get_event(singledat,singleevt)) << std::endl;
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
  std::cout << "Return: " << (return_value += singledec->get_event(singledat,singleevt)) << std::endl;
  delete singledec;
  singledat.clear();


  std::cout << "PSI Digital 16bit:\n";
  singledat.push_back(0xff7f);
  singledat.push_back(0x831d);
  singledat.push_back(0x762f);

  flaggen = FLAG_16BITS_PER_WORD | FLAG_ALLOW_CORRUPT_ROC_HEADERS;
  singledec = new CMSPixelEventDecoderDigital(1,flaggen,ROC_PSI46DIG);
  std::cout << "Return: " << (return_value += singledec->get_event(singledat,singleevt)) << std::endl;
  delete singledec;
  singledat.clear();

  delete singleevt;

  if(return_value != 0) {
    std::cout << "Unit testing of single event decoding failed!" << std::endl;
    return 1;
  }
  else std::cout << "Unit testing of single event decoding SUCCEEDED." << std::endl;

  // ###################################################################################

  if(!unit_tests()) {
    std::cout << "Unit testing failed!" << std::endl;
    return 1;
  }
  else {
    std::cout << "Unit testing successfully completed." << std::endl;
    return 0;
  }
}

bool compare(CMSPixelStatistics reference, CMSPixelStatistics measurement)
{
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
  ref.evt_empty = 75595; //77665;
  ref.evt_valid = 33899; //31829;
  ref.pixels_valid = 39973; //37105;
  ref.evt_invalid = 0;
  ref.pixels_invalid = 8718; //11586;
  double ref_timing = 0.26;

  std::vector<pixel> * evt = new std::vector<pixel>;
  timing time;
  CMSPixelFileDecoder * dec = new CMSPixelFileDecoderPSI_ATB("data/mtb.bin.ana",1,FLAG_ALLOW_CORRUPT_ROC_HEADERS,ROC_PSI46V2,"data/addressParameters.dat.single");

  clock_t begin = clock();
  while(1)
    if(dec->get_event(evt, time) <= DEC_ERROR_NO_MORE_DATA) break;
  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
  std::cout << "     Timing: " << elapsed_secs << " sec, " << dec->statistics.head_data/elapsed_secs << " events/sec." << std::endl;
  delete evt;

  // Timing check:
  if(elapsed_secs <= 0.92*ref_timing || elapsed_secs >= 1.08*ref_timing) {
    std::cout << "     Timing requirements NOT met! (should be " << ref_timing << " sec)" << std::endl;
  }

  // Decoding check:
  if(!compare(ref,dec->statistics)) {
    delete dec;
    return false;
  }
  else {
    delete dec;
    std::cout << "SUCCEEDED." << std::endl;
    return true;
  }
}

bool test_analog_single_tb()
{
  std::cout << "Unit testing: Analog Single ROC, DESY Testbeam settings" << std::endl;

  // REFERENCE:
  CMSPixelStatistics ref;
  ref.head_data = ref.head_trigger = 165243;
  ref.evt_empty = 91815;
  ref.evt_valid = 73427;
  ref.pixels_valid = 103705;
  ref.evt_invalid = 0;
  ref.pixels_invalid = 0;
  double ref_timing = 0.26;

  std::vector<pixel> * evt = new std::vector<pixel>;
  timing time;
  CMSPixelFileDecoder * dec = new CMSPixelFileDecoderPSI_ATB("data/mtb.bin.ana.tb",1,FLAG_OVERWRITE_ROC_HEADER_POS,ROC_PSI46V2,"data/addressParameters.dat.single.tb");

  clock_t begin = clock();
  while(1)
    if(dec->get_event(evt, time) <= DEC_ERROR_NO_MORE_DATA) break;
  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
  std::cout << "     Timing: " << elapsed_secs << "sec, " << dec->statistics.head_data/elapsed_secs << " events/sec." << std::endl;
  delete evt;

  // Timing check:
  if(elapsed_secs <= 0.92*ref_timing || elapsed_secs >= 1.08*ref_timing) {
    std::cout << "     Timing requirements NOT met! (should be " << ref_timing << " sec)" << std::endl;
  }

  // Decoding check:
  if(!compare(ref,dec->statistics)) {
    delete dec;
    return false;
  }
  else {
    delete dec;
    std::cout << "SUCCEEDED." << std::endl;
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

  std::vector<pixel> * evt = new std::vector<pixel>;
  timing time;
  CMSPixelFileDecoder * dec = new CMSPixelFileDecoderPSI_ATB("data/mtb.bin.dig",1,FLAG_ALLOW_CORRUPT_ROC_HEADERS,ROC_PSI46DIG,"");

  clock_t begin = clock();
  while(1)
    if(dec->get_event(evt, time) <= DEC_ERROR_NO_MORE_DATA) break;
  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
  std::cout << "     Timing: " << elapsed_secs << " sec, " << dec->statistics.head_data/elapsed_secs << " events/sec." << std::endl;
  delete evt;

  // Timing check:
  if(elapsed_secs <= 0.92*ref_timing || elapsed_secs >= 1.08*ref_timing) {
    std::cout << "     Timing requirements NOT met! (should be " << ref_timing << " sec)" << std::endl;
  }

  // Decoding check:
  if(!compare(ref,dec->statistics)) {
    delete dec;
    return false;
  }
  else {
    delete dec;
    std::cout << "SUCCEEDED." << std::endl;
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

  std::vector<pixel> * evt = new std::vector<pixel>;
  timing time;
  CMSPixelFileDecoder * dec = new CMSPixelFileDecoderPSI_ATB("data/mtb.bin.mod.ana",16,FLAG_HAVETBM,ROC_PSI46V2,"data/addressParameters.dat.mod");

  clock_t begin = clock();
  while(1)
    if(dec->get_event(evt, time) <= DEC_ERROR_NO_MORE_DATA) break;
  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
  std::cout << "     Timing: " << elapsed_secs << " sec, " << dec->statistics.head_data/elapsed_secs << " events/sec." << std::endl;
  delete evt;

  // Timing check:
  if(elapsed_secs <= 0.92*ref_timing || elapsed_secs >= 1.08*ref_timing) {
    std::cout << "     Timing requirements NOT met! (should be " << ref_timing << " sec)" << std::endl;
  }

  // Decoding check:
  if(!compare(ref,dec->statistics)) {
    delete dec;
    return false;
  }
  else {
    delete dec;
    std::cout << "SUCCEEDED." << std::endl;
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

  std::vector<pixel> * evt = new std::vector<pixel>;
  timing time;
  CMSPixelFileDecoder * dec = new CMSPixelFileDecoderPSI_ATB("data/mtb.bin.tel.psi",8,FLAG_ALLOW_CORRUPT_ROC_HEADERS,ROC_PSI46DIG,"");

  clock_t begin = clock();
  while(1)
    if(dec->get_event(evt, time) <= DEC_ERROR_NO_MORE_DATA) break;
  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
  std::cout << "     Timing: " << elapsed_secs << " sec, " << dec->statistics.head_data/elapsed_secs << " events/sec." << std::endl;
  delete evt;

  // Timing check:
  if(elapsed_secs <= 0.92*ref_timing || elapsed_secs >= 1.08*ref_timing) {
    std::cout << "     Timing requirements NOT met! (should be " << ref_timing << " sec)" << std::endl;
  }

  // Decoding check:
  if(!compare(ref,dec->statistics)) {
    std::cout << "Statistics comparison failed." << std::endl;
    delete dec;
    return false;
  }
  else {
    delete dec;
    std::cout << "SUCCEEDED." << std::endl;
    return true;
  }
}

bool test_telescope_ral()
{
  std::cout << "Unit testing: Digital ROC Telescope w/ RAL IPBus readout" << std::endl;

  // REFERENCE:
  CMSPixelStatistics ref;
  ref.head_trigger = 0;
  ref.head_data = 354253;
  ref.evt_empty = 51256;
  ref.evt_valid = 202117;
  ref.pixels_valid = 8328585;
  ref.evt_invalid = 100879;
  ref.pixels_invalid = 1397125;
  double ref_timing = 31.7;

  std::vector<pixel> * evt = new std::vector<pixel>;
  timing time;
  int flags = FLAG_ALLOW_CORRUPT_ROC_HEADERS | FLAG_OLD_RAL_FORMAT;
  CMSPixelFileDecoder * dec = new CMSPixelFileDecoderRAL("data/mtb.bin.tel.ral",8,flags,ROC_PSI46DIG);

  clock_t begin = clock();
  while(1)
    if(dec->get_event(evt, time) <= DEC_ERROR_NO_MORE_DATA) break;
  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
  std::cout << "     Timing: " << elapsed_secs << " sec, " << dec->statistics.head_data/elapsed_secs << " events/sec." << std::endl;
  delete evt;

  // Timing check:
  if(elapsed_secs <= 0.95*ref_timing || elapsed_secs >= 1.05*ref_timing) {
    std::cout << "     Timing requirements NOT met! (should be " << ref_timing << " sec)" << std::endl;
  }

  // Decoding check:
  if(!compare(ref,dec->statistics)) {
    std::cout << "Statistics comparison failed." << std::endl;
    delete dec;
    return false;
  }
  else {
    delete dec;
    std::cout << "SUCCEEDED." << std::endl;
    return true;
  }
}

bool test_telescope_ral2()
{
  std::cout << "Unit testing: Digital ROC Telescope w/ RAL IPBus readout, Bridge Board Rev.3" << std::endl;

  // REFERENCE:
  CMSPixelStatistics ref;
  ref.head_trigger = 0;
  ref.head_data = 156123;
  ref.evt_empty = 95080;
  ref.evt_valid = 61037;
  ref.pixels_valid = 440936;
  ref.evt_invalid = 5;
  ref.pixels_invalid = 3;

  double ref_timing = 10.7;

  std::vector<pixel> * evt = new std::vector<pixel>;
  timing time;
  CMSPixelFileDecoder * dec = new CMSPixelFileDecoderRAL("data/mtb.bin.tel.ral2",8,0,ROC_PSI46DIGV2);

  clock_t begin = clock();
  while(1)
    if(dec->get_event(evt, time) <= DEC_ERROR_NO_MORE_DATA) break;
  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
  std::cout << "     Timing: " << elapsed_secs << " sec, " << dec->statistics.head_data/elapsed_secs << " events/sec." << std::endl;
  delete evt;

  // Timing check:
  if(elapsed_secs <= 0.95*ref_timing || elapsed_secs >= 1.05*ref_timing) {
    std::cout << "     Timing requirements NOT met! (should be " << ref_timing << " sec)" << std::endl;
  }

  // Decoding check:
  if(!compare(ref,dec->statistics)) {
    std::cout << "Statistics comparison failed." << std::endl;
    delete dec;
    return false;
  }
  else {
    delete dec;
    std::cout << "SUCCEEDED." << std::endl;
    return true;
  }
}

bool unit_tests() {

  std::cout << "Start data-driven unit testing..." << std::endl;

  Log::ReportingLevel() = Log::FromString("SUMMARY");

  if(!test_analog_single()) return false;
  if(!test_analog_single_tb()) return false;
  if(!test_digital_single()) return false;

  if(!test_analog_module()) return false;
  //if(!test_digital_module()) return false;

  if(!test_telescope_psi()) return false;
  if(!test_telescope_ral()) return false;
  if(!test_telescope_ral2()) return false;

  return true;
}


bool read_address_levels(const char* levelsFile, unsigned int rocs, levelset & addressLevels) {
  LOG(logDEBUG3) << "READ_ADDRESS_LEVELS starting...";
  // Reading files with format defined by psi46expert::DecodeRawPacket::Print
  short level;
  char dummyString[100];
  char separation[100];
  levels *tempLevels = new levels();
  std::ifstream* file = new std::ifstream(levelsFile);

  if ( *file == 0 ){
    LOG(logERROR) << "READ_ADDRESS_LEVELS::ERROR cannot open the address levels file!";
    return false;
  }

  // Skip reading first labels:
  for (unsigned int iskip = 0; iskip < 4; iskip++ ) *file >> dummyString;

  // Read separation line:
  *file >> separation;

  // Skip reading other labels:
  for (unsigned int iskip = 0; iskip < 7; iskip++ ) *file >> dummyString;

  // Read TBM UltraBlack, black and address levels
  for (unsigned int ilevel = 0; ilevel < 8; ilevel++ ){
    *file >> level;
    addressLevels.TBM.level.push_back(level);

    if ( file->eof() || file->bad() ){
      LOG(logERROR) << "READ_ADDRESS_LEVELS::ERROR invalid format of address levels file!";
      return false;
    }
  }

  // Skip reading labels and separation lines:
  for (unsigned int iskip = 0; iskip < 9; iskip++ ) *file >> dummyString;

  // Read UltraBlack, black and address levels for each ROC
  // Skip reading ROC labels:
  *file >> dummyString;

  // Read file until we reach the second separation line (so EOF):    
  while((strcmp(dummyString,separation) != 0) && !file->eof() && !file->bad()) {

    // Read ROC Ultrablack and black levels:
    for (unsigned int ilevel = 0; ilevel < 3; ilevel++ ){
      if(!file->eof()) *file >> level; else goto finish;
      tempLevels->level.push_back(level);
    }
    addressLevels.ROC.push_back(*tempLevels);
    tempLevels->level.clear();
        
    // Read ROC address level encoding:
    for (unsigned int ilevel = 0; ilevel < 7; ilevel++ ){
      if(!file->eof()) *file >> level; else goto finish;
      tempLevels->level.push_back(level);
    }
    addressLevels.address.push_back(*tempLevels);
    tempLevels->level.clear();
        
    // Skip reading ROC labels:
    if(!file->eof()) *file >> dummyString; else goto finish;
         
  }

 finish:
  delete file;
  delete tempLevels;
    
  if (addressLevels.address.size() != rocs) {
    LOG(logERROR) << "READ_ADDRESS_LEVELS::ERROR No of ROCs in address levels file (" << addressLevels.address.size() << ") does not agree with given parameter (" << rocs << ")!";
    return false;
  }
  LOG(logDEBUG3) << "READ_ADDRESS_LEVELS finished, read " << addressLevels.address.size() << " rocs:";
  return true;
}
