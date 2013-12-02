/*==========================================================================*/
/*          CMSPixel Decoder                                                */
/*          Questions: s.spannagel@cern.ch                                  */
/*          Source: https://github.com/simonspa/CMSPixelDecoder             */
/*                                                                          */
/* CMSPixel Decoder is free software: you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by     */
/* the Free Software Foundation, either version 3 of the License, or        */
/* (at your option) any later version.                                      */
/*==========================================================================*/

#include "CMSPixelDecoder.h"

#include <cstring>
#include <string>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <map>
#include <sys/types.h>

using namespace CMSPixel;

void CMSPixelStatistics::init() {
  head_data = head_trigger = 0;
  evt_valid = evt_empty = evt_invalid = 0;
  pixels_valid = pixels_invalid = 0;
}

void CMSPixelStatistics::update(CMSPixelStatistics stats) {
  data_blocks += stats.data_blocks;
  head_data += stats.head_data;
  head_trigger += stats.head_trigger;
  evt_valid += stats.evt_valid;
  evt_empty += stats.evt_empty;
  evt_invalid += stats.evt_invalid;
  pixels_valid += stats.pixels_valid;
  pixels_invalid += stats.pixels_invalid;
}

std::string CMSPixelStatistics::get() {
  std::stringstream os;
  os << std::endl;
  os << "    Data blocks read : " << std::setw(8) << data_blocks << std::endl;

  os << "    TB Trigger Marker: " << std::setw(8) << head_trigger << std::endl;
  os << "    TB Data Marker:    " << std::setw(8) << head_data << std::endl;

  os << "    Events empty:      " << std::setw(8) << evt_empty << std::endl;
  os << "    Events valid:      " << std::setw(8) << evt_valid << std::endl;
  os << "    Events invalid:    " << std::setw(8) << evt_invalid << std::endl;

  os << "    Pixels valid:      " << std::setw(8) << pixels_valid << std::endl;
  os << "    Pixels invalid:    " << std::setw(8) << pixels_invalid;
  return os.str();
}

void CMSPixelStatistics::print() {
  LOG(logSUMMARY) << get();
}

bool CMSPixelFileDecoderPSI_ATB::process_rawdata(std::vector< uint16_t > * /*data*/)
{
  // Currently nothing to do here...
  LOG(logDEBUG4) << "No need for raw data processing with PSI ATB.";
  return true;
}

bool CMSPixelFileDecoderPSI_DTB::process_rawdata(std::vector< uint16_t > * /*data*/) 
{
  // Currently nothing to do here...
  LOG(logDEBUG4) << "No need for raw data processing with PSI DTB.";
  return true;
}

bool CMSPixelFileDecoderRAL::process_rawdata(std::vector< uint16_t > * rawdata) {

  // IPBus data format: we need to delete some additional headers from the test board.
  // remove padding to 32bit words at the end of the event by reading the data length.

  // IPBus Data Format:
  // 0: 32 bit Header (programmable, agreed to be 0xFFFFFFFF)
  // 1: 32 bit Event number from readout block (should always be contiguous)
  // 2: 32 bit Event body length in bytes ('n'; need not be aligned to whole words)
  // 3: 32 bit Event type / run number (programmable, 0xDEADBEEF in the example)
  // 4 -: Pixel data, followed by 14/15 byte trailer
  //
  // OLD Trailer Format with 14bytes:
  // (use FLAG_OLD_RAL_FORMAT to decode)
  // Bytes 3-0: Trigger number
  // Bytes 7-4: Token number
  // Bytes b-8: Timestamp (most significant 32b of 40b word, 1us ticks)
  // Byte c: Timestamp (lower 8b)
  // Byte d: Bits 1-0 are phase of the trigger pulse; bits 3-2 are phase of the received data
  //
  // NEW Trailer format with 15bytes:
  // 15 byte trailer:
  // First 13 bytes unchanged.
  // Byte 14 (d): lower four bits: data phase; upper four bits: trigger phase
  // Byte 15 (e): lower four bits: event status (good event == 7); upper four bits: reserved

  // FIXME apparently trigger phase is stored in upper four bits of byte 15! (just right before the "good event" marker):
  // Byte 15: TTT00111 (only using 3bit)

  // Catch strange events with corrupted length or so:
  try {

    // We need to swap the endianness since the data block comes in scrumbled 8bit-blocks:
    // D | C | B | A  ->  A | B | C | D
    uint32_t event_length = (((rawdata->at(1)<<24)&0xff000000) | 
			     ((rawdata->at(1)<<8)&0xff0000) | 
			     ((rawdata->at(0)<<8)&0xff00) | 
			     ((rawdata->at(0)>>8)&0xff));

    // Old dataformat has only 14bytes trailer, new format has 15bytes:
    if(ral_flags & FLAG_OLD_RAL_FORMAT) event_length -= 14;
    else event_length -= 15;
  
    LOG(logDEBUG4) << "IPBus event length: " << event_length << " bytes.";

    // Read the timestamp from the trailer:
    uint16_t stamp_pos = (event_length/2) + 8;

    uint64_t time2 = rawdata->at(stamp_pos+2);
    uint64_t time1 = rawdata->at(stamp_pos+1);
    uint64_t time0 = rawdata->at(stamp_pos);

    uint16_t phase;
    if(ral_flags & FLAG_OLD_RAL_FORMAT) phase = 0;
    else phase = rawdata->at(stamp_pos+3);

    uint64_t tnumber2 = rawdata->at(stamp_pos-2);
    uint32_t tnumber1 = rawdata->at(stamp_pos-3);
    uint32_t tnumber0 = rawdata->at(stamp_pos-4);

    if(event_length%2 == 0) {
      LOG(logDEBUG4) << "IPBUS even event length.";

      cms_t.timestamp = ((time1<<32)&0xff00000000LLU) | 
	((time1<<16)&0xff000000) | 
	((time0<<16)&0xff0000) | 
	((time0)&0xff00) | 
	((time2>>8)&0xff);

      cms_t.trigger_number = (((tnumber1<<24)&0xff000000) | 
			     ((tnumber1<<8)&0xff0000) | 
			     ((tnumber0<<8)&0xff00) | 
			     ((tnumber0>>8)&0xff));

      cms_t.trigger_phase = (phase>>13)&0xf;

    }
    else {
      cms_t.timestamp = ((time2<<24)&0xff00000000LLU) | 
	((time1<<24)&0xff000000) | 
	((time1<<8)&0xff0000) | 
	((time0<<8)&0xff00) | 
	((time2)&0xff);

      cms_t.trigger_number = (((tnumber2<<32)&0xff000000) | 
			      ((tnumber1<<16)&0xff0000) | 
			     ((tnumber1<<8)&0xff00) | 
			     ((tnumber0)&0xff));

      cms_t.trigger_phase = (phase>>5)&0xf;

    }

    cms_t.trigger_phase &= 0x07;
    //  0000:1111 = 0F
    //  0000:0111 = 07

    LOG(logDEBUG4) << "IPBus timestamp: " << std::hex << cms_t.timestamp << std::dec << " = " << cms_t.timestamp << "us.";
    LOG(logDEBUG4) << "IPBus trigger number: " << cms_t.trigger_number;

    // cut first 8 bytes from header:
    rawdata->erase(rawdata->begin(),rawdata->begin()+4);
    //  and last 14/15 bytes plus the padding from trailer:
    rawdata->erase(rawdata->begin() + (event_length/2 + event_length%2),rawdata->end());

  }
  catch(...) {
    LOG(logERROR) << "Invalid IPBus event detected.";
    statistics.evt_invalid++;
    return false;
  }
  return true;
}

CMSPixelFileDecoder::CMSPixelFileDecoder(const char *FileName, unsigned int rocs, int flags, uint8_t ROCTYPE, const char *addressFile)
  : statistics(), evt(), theROC(0), mtbStream(), cms_t(), addressLevels()
{

  LOG(logDEBUG) << "Preparing a file decoder instance...";

  // Make the roc type available:
  theROC = ROCTYPE;
  
  // Prepare a new event decoder instance:
  if(ROCTYPE & ROC_PSI46V2 || ROCTYPE & ROC_PSI46XDB) {
    if(!read_address_levels(addressFile,rocs,addressLevels)) {
      LOG(logERROR) << "Could not read address parameters correctly!";
    }
    else {
      LOG(logDEBUG) << print_addresslevels(addressLevels);
    }

    evt = new CMSPixelEventDecoderAnalog(rocs, flags, ROCTYPE, addressLevels);
  }
  else {
    evt = new CMSPixelEventDecoderDigital(rocs, flags, ROCTYPE);
  }

  // Initialize the statistics counter:
  statistics.init();

  // Open the requested file stream:
  LOG(logDEBUG1) << "Open data file...";
  if((mtbStream = fopen(FileName,"r")) == NULL)
    LOG(logERROR) << " ...could not open file!";
  else
    LOG(logDEBUG1) << " ...successfully.";
}

CMSPixelFileDecoder::~CMSPixelFileDecoder() {
  LOG(logSUMMARY) << statistics.get();
}

int CMSPixelFileDecoder::get_event(std::vector<pixel> * decevt, timing & evt_timing) {
  // Check if stream is open:
  if(!mtbStream) return DEC_ERROR_INVALID_FILE;

  std::vector < uint16_t > data;
  LOG(logDEBUG) << "STATUS Start chopping file.";

  // Cut off the next event from the filestream:
  if(!chop_datastream(&data)) return DEC_ERROR_NO_MORE_DATA;

  // Take into account that different testboards write the data differently:
  if(!process_rawdata(&data)) return DEC_ERROR_INVALID_EVENT;

  // Deliver the timing information:
  evt_timing = cms_t;

  // And finally call the decoder to do the rest of the work:
  int status = evt->get_event(data, decevt);
  statistics.update(evt->statistics);

  return status;
}

bool CMSPixelFileDecoder::readWord(uint16_t &word) {
  unsigned char a, b;
  if(feof(mtbStream) || !fread(&a,sizeof(a),1,mtbStream) || !fread(&b,sizeof(b),1,mtbStream)) 
    return false;
  word = (b << 8) | a;
  return true;
}

bool CMSPixelFileDecoderRAL::readWord(uint16_t &word) {
  unsigned char a, b;
  if(feof(mtbStream) || !fread(&a,sizeof(a),1,mtbStream) || !fread(&b,sizeof(b),1,mtbStream)) 
    return false;
  // Do not swap endianness in the IPBus case:
  word = (a << 8) | b;
  return true;
}

bool CMSPixelFileDecoder::chop_datastream(std::vector< uint16_t > * rawdata) {

  uint16_t word;
  rawdata->clear();
    
  LOG(logDEBUG1) << "Chopping datastream at MTB headers...";
  if(!readWord(word)) return false;

  while (!word_is_data(word)) {
    // If header is detected read more words:
    if(word_is_trigger(word)) {
      LOG(logDEBUG1) << "STATUS trigger detected: " << std::hex << word << std::dec;
      statistics.head_trigger++;

      // read 3 more words after header:
      uint16_t head[4];
      for(int h = 1; h < 4; h++ ) {
	if(!readWord(word)) return false;
	head[h] = word;
      }
      LOG(logDEBUG4) << "Headers: " << head[1] << " " << head[2] << " " << head[3];
      // Calculating the tigger time of the event:
      cms_t.timestamp = ((static_cast<uint64_t>(head[1])<<32)&0xffff00000000LLU) | ((static_cast<uint64_t>(head[2])<<16)&0xffff0000) | (head[3]&0xffff);
      LOG(logDEBUG1) << "Timestamp: " << cms_t.timestamp;
    }
    else if(word_is_header(word) ) {
      LOG(logDEBUG1) << "STATUS header detected : " << std::hex << word << std::dec;
      // read 3 more words after header:
      for(int h = 1; h < 4; h++ ) if(!readWord(word)) return false;
    }
    else {
      LOG(logDEBUG1) << "STATUS drop: " << std::hex << word << std::dec;
    }
    if(!readWord(word)) return false;
  }
    
  LOG(logDEBUG) << "STATUS data header     : " << std::hex << word << std::dec;
  statistics.head_data++;

  // read 3 more words after header:
  for(int i = 1; i < 4; i++) if(!readWord(word)) return false;
            
  // read the data until the next MTB header arises:
  // morewords:
  if(!readWord(word)) return false;
  while( !word_is_header(word) && !feof(mtbStream)){
    rawdata->push_back(word);
    if(!readWord(word)) return false;
  }

  //FIXME For IPBus readout check the next header words, too - they should be header again:
  if(!readWord(word)) return false;
  //  if(!word_is_2nd_header(word)) {
  //  rawdata->push_back(word);
  //  goto morewords;
  // }
  //else LOG(logDEBUG4) << "Double-checked header, was " << std::hex << word << std::dec;
  
  LOG(logDEBUG1) << "Raw data array size: " << rawdata->size() << ", so " << 16*rawdata->size() << " bits.";
    
  // Rewind one word to detect the header correctly later on:
  fseek(mtbStream , -4 , SEEK_CUR);
  
  return true;
}

bool CMSPixelFileDecoder::read_address_levels(const char* levelsFile, unsigned int rocs, levelset & addressLevels) {
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

std::string CMSPixelFileDecoder::print_addresslevels(levelset addLevels) {
  std::stringstream os;
  os << std::endl << "STATUS TBM    header: ";
  std::vector<int>::const_iterator it;
  for(it = addLevels.TBM.level.begin(); it < addLevels.TBM.level.end(); ++it)
    os << std::setw(5) << static_cast<int>(*it) << " ";

  for (unsigned int iroc = 0; iroc < addLevels.address.size(); iroc++ ) {
    os << std::endl << "STATUS ROC" << std::setw(2) << iroc << "  header: ";
    for(it = addLevels.ROC[iroc].level.begin(); it < addLevels.ROC[iroc].level.end(); ++it)
      os << std::setw(5) << *it << " ";
    os << std::endl;

    os << "STATUS ROC" << std::setw(2) << iroc << " address: ";
    for(it = addLevels.address[iroc].level.begin(); it < addLevels.address[iroc].level.end(); ++it)
      os << std::setw(5) << *it << " ";
  }
  return os.str();
}

/*========================================================================*/
/*          CMSPixel Event Decoder                                        */
/*          parent class CMSPixelDecoder                                  */
/*========================================================================*/


CMSPixelEventDecoder::CMSPixelEventDecoder(unsigned int rocs, int flags, uint8_t ROCTYPE)
  : statistics(), L_HEADER(0), L_TRAILER(0), L_EMPTYEVT(0), L_GRANULARITY(0), L_HIT(0), L_ROC_HEADER(0), L_HUGE_EVENT(0), flag(flags), noOfROC(rocs), theROC(ROCTYPE)
{
}

CMSPixelEventDecoder::~CMSPixelEventDecoder() {
  // Nothing to do.
}

int CMSPixelEventDecoder::get_event(std::vector< uint16_t > & data, std::vector<pixel> * evt) {

  LOG(logDEBUG) << "Start decoding.";
  LOG(logDEBUG1) << "Received " << 16*data.size() << " bits of event data.";
  statistics.data_blocks = data.size();

  evt->clear();
  statistics.init();

  unsigned int pos = 0;
  
  // It might be interesting to print the raw data stream once:
  LOG(logDEBUG4) << print_data(&data);

  // Check sanity
  int status = pre_check_sanity(&data,&pos);
  if(status != 0) return status;

  // Init ROC id:
  unsigned int roc = 0;
  pixel tmp;
    
  LOG(logDEBUG3) << "Looping over event data with granularity " << L_GRANULARITY << ", " << L_GRANULARITY*data.size() << " iterations.";
        
  while(pos < L_GRANULARITY*data.size()) {
    // Try to find a new ROC header:
    if(find_roc_header(data,&pos,roc+1)) roc++;
    else if(decode_hit(data,&pos,roc,&tmp) == 0) 
      evt->push_back(tmp);
  }

  // Check sanity of output
  status = post_check_sanity(evt,roc);
  if(status != 0) return status;

  LOG(logDEBUG) << "STATUS end of event processing.";
  return 0;
}


int CMSPixelEventDecoder::pre_check_sanity(std::vector< uint16_t > * data, unsigned int * pos) {

  LOG(logDEBUG2) << "Checking pre decoding event sanity...";
  unsigned int length = L_GRANULARITY*data->size();
    
  // Check presence of TBM tokens if necessary:
  if(flag & FLAG_HAVETBM) {
    if(!find_tbm_header(*data,0)) {
      LOG(logWARNING) << "Event contained no valid TBM_HEADER. Skipped.";
      return DEC_ERROR_NO_TBM_HEADER;
    }
    else LOG(logDEBUG) << "STATUS TBM_HEADER correctly detected.";

    if(!find_tbm_trailer(*data,length - L_TRAILER)) {
      LOG(logWARNING) << "Event contained no valid TBM_TRAILER. Skipped.";
      return DEC_ERROR_NO_TBM_HEADER;
    }
    else LOG(logDEBUG) << "STATUS TBM_TRAILER correctly detected.";

    // If we have TBM headers check for them and remove them from data:
    if(L_TRAILER > 0) {
      // Delete the trailer from valid hit data:
      data->erase(data->end() - L_TRAILER/L_GRANULARITY,data->end());
      LOG(logDEBUG3) << "Deleted trailer: " << L_TRAILER << " words.";
    }
    
    if(L_HEADER > 0) {
      // Delete the header from valid hit data:
      data->erase(data->begin(),data->begin()+L_HEADER/L_GRANULARITY);
      LOG(logDEBUG3) << "Deleted header: " << L_HEADER << " words.";
    }
  }
  else LOG(logDEBUG2) << "Not checking for TBM presence.";

  // Checking for empty events and skip them if necessary:
  // FIXME maybe skip this check to make it more flexible and thorough? Emptiness will be checked after decoding...
  if( length <= noOfROC*L_ROC_HEADER ){
    LOG(logINFO) << "Event is empty, " << length << " words <= " <<  noOfROC*L_ROC_HEADER << ".";
    statistics.evt_empty++;
    return DEC_ERROR_EMPTY_EVENT;
  }
  // There might even be a huge event:
  else if( length > 2222*L_GRANULARITY ) {
    LOG(logERROR) << "Detected huge event (" << length << " words). Skipped.";
    statistics.evt_invalid++;
    return DEC_ERROR_HUGE_EVENT;
  }


  // Maybe we deleted something, recompute length:
  length = L_GRANULARITY*data->size();

  // Ugly hack for single analog/xdb chip readout without TBM or emulator: we have some testboard trailer...
  if((theROC & ROC_PSI46V2 || theROC & ROC_PSI46XDB) && !(flag & FLAG_HAVETBM) && noOfROC == 1) {
    data->erase(data->end() - 4,data->end());
    LOG(logDEBUG3) << "FIXME(singleAnalogROC): Removed some trailers.";
  }

  // Find the start position with the first ROC header (maybe there are idle patterns before...)
  bool found = false;
  for(unsigned int i = 0; i < length; i++) {
    if(find_roc_header(*data, &i, 0)) {
      *pos = i;
      found = true;
      break;
    }
  }

  if(!found) {
    LOG(logERROR) << "Event contains no valid ROC header: " << print_data(data);
    statistics.evt_invalid++;
    return DEC_ERROR_NO_ROC_HEADER;
  }
  else
    LOG(logDEBUG3) << "Starting position after first ROC header: " << *pos;
  
  LOG(logDEBUG2) << "Pre-decoding event sane.";
  LOG(logDEBUG) << "STATUS event: " << length << " data words.";
  return 0;
}
  


int CMSPixelEventDecoder::post_check_sanity(std::vector< pixel > * evt, unsigned int rocs) {    

  LOG(logDEBUG2) << "Checking post-decoding event sanity...";

  if(noOfROC != rocs+1) {
    LOG(logERROR) << "STATUS ROCs: preset " << noOfROC << " != data " << rocs+1 << ". Skipped.";
    statistics.evt_invalid++;
    evt->clear();
    return DEC_ERROR_NO_OF_ROCS;
  }
  else LOG(logDEBUG) << "STATUS correctly detected " << rocs+1 << " ROC(s).";

  if(evt->size() == 0) {
    LOG(logINFO) << "Event is empty, no hits decoded.";
    statistics.evt_empty++;
    return DEC_ERROR_EMPTY_EVENT;
  }

  LOG(logDEBUG2) << "Post-decoding event sane.";
  statistics.evt_valid++;
  return 0;
}


bool CMSPixelEventDecoder::convertDcolToCol(int dcol, int pix, int & col, int & row)
{
  if( dcol < 0 || dcol >= ROCNUMDCOLS || pix < 2 || pix > 161) 
    return false;

  // pix%2: 0 = 1st col, 1 = 2nd col of a double column
  col = dcol * 2 + pix%2; // col address, starts from 0
  row = abs( static_cast<int>(pix/2) - ROCNUMROWS);  // row address, starts from 0

  if( col < 0 || col > ROCNUMCOLS || row < 0 || row > ROCNUMROWS ) 
    return false;

  LOG(logDEBUG3) << "Converted dcol " << dcol << " pix " << pix << " to col " << col << " row " << row;
  return true;
}

/*========================================================================*/
/*          CMSPixel Event Decoder                                        */
/*          child class CMSPixelEventDecoderDigital                       */
/*          decoding DIGITAL chip data                                    */
/*========================================================================*/

CMSPixelEventDecoderDigital::CMSPixelEventDecoderDigital(unsigned int rocs, int flags, uint8_t ROCTYPE) : CMSPixelEventDecoder(rocs, flags, ROCTYPE)
{
  LOG(logDEBUG) << "Preparing a digital event decoder instance, ROC type " << static_cast<int>(ROCTYPE) << "...";
  // Loading constants and flags:
  LOG(logDEBUG2) << "Loading constants...";
  load_constants(flags);
}

std::string CMSPixelEventDecoderDigital::print_data(std::vector< uint16_t> * data) {
  // This simply prints the raw data, useful only for debugging.
  std::stringstream os;
  for(unsigned int i = 0; i < data->size();i++) 
    os << std::hex << get_bits(*data,i*L_GRANULARITY,L_GRANULARITY);
  os << std::dec;
  return os.str();
}

int CMSPixelEventDecoderDigital::get_bit(std::vector< uint16_t > data, int bit_offset) {
  unsigned int ibyte=bit_offset/L_GRANULARITY;
  int byteval;
  if (ibyte < data.size()) {
    byteval = data[ibyte];
  }
  else return 0;

  // get bit, counting from most significant bit to least significant bit
  int ibit = (L_GRANULARITY-1) - (bit_offset-ibyte*L_GRANULARITY);
  int bitval = (byteval >> ibit)&1;
  //LOG(logDEBUG4) << "returning value " << bitval << " at position " << ibit << " in short " << std::hex << byteval << std::dec;
  return bitval;
}

int CMSPixelEventDecoderDigital::get_bits(std::vector< uint16_t > data, int bit_offset,int number_of_bits) {
  int value = 0;
  for (int ibit = 0; ibit < number_of_bits; ibit++) {
    value = 2*value+get_bit(data,bit_offset+ibit);
  }
  //LOG(logDEBUG4) << "delivering " << std::hex << value << std::dec << " requested at offset " << bit_offset << " with length " << number_of_bits;
  return value;
}

bool CMSPixelEventDecoderDigital::find_roc_header(std::vector< uint16_t > data, unsigned int * pos, unsigned int)
{
  // ROC header: 0111 1111 10SD, S & D are reserved status bits.
  // Possible ROC headers: 0x7f8 0x7f9 0x7fa 0x7fb
  int search_head = get_bits(data, *pos, L_ROC_HEADER);

  // Ugly hack to compensate for the chip sending correct ROC headers: 0x7fX might be 0x3fX too...
  if((flag & FLAG_ALLOW_CORRUPT_ROC_HEADERS) && (search_head == 0x7f0 || search_head == 0x7fc || search_head == 0x3f8 || search_head == 0x3f9 || search_head == 0x3fa || search_head == 0x3fb)) {
    LOG(logDEBUG2) << "Found ROC header with bit error (" 
		   << std::hex << std::setw(3) << search_head << ") after " << std::dec << *pos << " bit.";
    *pos += L_ROC_HEADER; // jump to next position.    
    return true;
  }
  else if(search_head == 0x7f8 || search_head == 0x7f9 || search_head == 0x7fa || search_head == 0x7fb) {
    LOG(logDEBUG2) << "Found ROC header (" << std::hex << std::setw(3) << search_head << ") after " << std::dec << *pos << " bit.";
    *pos += L_ROC_HEADER; // jump to next position.    
    return true;
  }
  else return false;
}

bool CMSPixelEventDecoderDigital::find_tbm_trailer(std::vector< uint16_t > data, unsigned int pos)
{
  if (get_bits(data, pos, L_TRAILER)==0x7fa) {
    LOG(logDEBUG1) << "Found TBM trailer at " << pos << ".";
    return true;
  }
  return false;
}

bool CMSPixelEventDecoderDigital::find_tbm_header(std::vector< uint16_t > data, unsigned int pos)
{
  if (get_bits(data, pos, L_HEADER)==0x7ff) {
    LOG(logDEBUG1) << "Found TBM header at " << pos << ".";
    return true;
  }
  return false;
}

std::string CMSPixelEventDecoderDigital::print_hit(int hit) {
  // Just print the hit data we have so far:
  std::ostringstream os;
  os << "hit: " << std::hex << hit << std::dec << ": ";
  for(int i = 23; i >= 0; i--) {
    os << ((hit>>i)&1);
    if(i==4 || i==5|| i==9|| i==12|| i==15|| i==18|| i==21) os << ".";
  }
  return os.str();
}

int CMSPixelEventDecoderDigital::decode_hit(std::vector< uint16_t > data, unsigned int * pos, unsigned int roc, pixel * pixhit)
{
  if(L_GRANULARITY*data.size() - *pos < L_HIT) {
    LOG(logDEBUG1) << "Dropping " << L_GRANULARITY*data.size() - *pos << " bit at the end of the event.";
    *pos = L_GRANULARITY*data.size();   // Set pointer to the end of data.
    return DEC_ERROR_NO_MORE_DATA;
  }
  int pixel_hit = get_bits(data,*pos,L_HIT);
  *pos += L_HIT; //jump to next hit.
    
  LOG(logDEBUG3) << print_hit(pixel_hit);

  // Double Column magic:
  //  dcol =  dcol msb        *6 + dcol lsb
  int dcol =  (pixel_hit>>21)*6 + ((pixel_hit>>18)&7);

  // Pixel ID magic:
  //  drow =  pixel msb           *36 + pixel nmsb *6 + pixel lsb
  // Bug in the PSI46DIG ROC: row has to be inverted (~).
  int drow;
  if(theROC & ROC_PSI46DIG)
    drow =  (~(pixel_hit>>15)&7)*36 + (~(pixel_hit>>12)&7)*6 + (~(pixel_hit>>9)&7);
  else
    drow =  ((pixel_hit>>15)&7)*36 + ((pixel_hit>>12)&7)*6 + ((pixel_hit>>9)&7);

  // pulseheight is 8 bit binary with a zero in the middle
  // to make sure we have less than eight consecutive ones.
  // pattern: XXXX 0 YYYY
  int pulseheight = ((pixel_hit>>5)&15)*16 + (pixel_hit&15);
    
  LOG(logDEBUG1) << "pixel dcol = " << std::setw(2) << dcol << " pixnum = " << std::setw(3) << drow << " pulseheight = " << ((pixel_hit>>5)&15) << "*16+" << (pixel_hit&15) << "= " << pulseheight << "(" << std::hex << pulseheight << ")" << std::dec;

  int col = -1;
  int row = -1;            

  // Convert and check pixel address and zero bit. Returns TRUE if address is okay:
  if( convertDcolToCol( dcol, drow, col, row ) && ((pixel_hit>>4)&1) == 0 ) {
    pixhit->raw = pulseheight;
    pixhit->col = col;
    pixhit->row = row;
    pixhit->roc = roc;

    LOG(logINFO) << "HIT ROC" << std::setw(2) << pixhit->roc << " | pix " << pixhit->col << " " << pixhit->row << ", adc " << pixhit->raw;
    CMSPixelEventDecoder::statistics.pixels_valid++;
    return 0;
  }
  else {
    // Something went wrong with the decoding:
    LOG(logWARNING) << "Failed to convert address of [" << std::hex << pixel_hit << std::dec << "]: dcol " << dcol << " drow " << drow << " (ROC" << roc << ", adc: " << pulseheight << ")";
    CMSPixelEventDecoder::statistics.pixels_invalid++;
    return DEC_ERROR_INVALID_ADDRESS;
  }
}




/*========================================================================*/
/*          CMSPixel Event Decoder                                        */
/*          child class CMSPixelEventDecoderAnalog                        */
/*          decoding ANALOG chip data                                     */
/*========================================================================*/

CMSPixelEventDecoderAnalog::CMSPixelEventDecoderAnalog(unsigned int rocs, int flags, uint8_t ROCTYPE, levelset addLevels) : CMSPixelEventDecoder(rocs, flags, ROCTYPE), addressLevels()
{
  LOG(logDEBUG) << "Preparing an analog event decoder instance, ROC type " << static_cast<int>(ROCTYPE) << "...";

  // Loading constants:
  LOG(logDEBUG2) << "Loading constants...";
  load_constants(flags);
    
  // Try to read the address levels file
  addressLevels = addLevels;
}

std::string CMSPixelEventDecoderAnalog::print_data(std::vector< uint16_t > * data) {
  std::stringstream os;
  for(unsigned int i = 0; i < data->size();i++) {
    os << std::dec << sign(data->at(i)) << " ";
  }
  return os.str();
}

bool CMSPixelEventDecoderAnalog::find_roc_header(std::vector< uint16_t > data, unsigned int * pos, unsigned int roc) {
  // Did we reach max number of ROCs read in from address levels file?
  if(roc >= addressLevels.ROC.size()) return false;
    
  // Some analog single ROCs seem to produce no ROC header at all. Hard setting the position with the flag below:
  if((theROC & ROC_PSI46V2 || theROC & ROC_PSI46XDB) && !(flag & FLAG_HAVETBM) && (flag & FLAG_OVERWRITE_ROC_HEADER_POS)) {
    if(*pos == 1) {
      LOG(logDEBUG1) << "Assumed corrupt ROC header ("<< std::setw(5) << sign(data[*pos]) << " " << std::setw(5) << sign(data[*pos+1]) << ") after " << std::dec << *pos << " words.";
      *pos += L_ROC_HEADER;
      return true;
    }
    else return false;
  }

  // Corrupt ROC header for single analog ROCs w/o TBM: somehow this looks like Black, UltraBlack:
  if ((flag & FLAG_ALLOW_CORRUPT_ROC_HEADERS) && findBin(sign(data[*pos]),2,addressLevels.ROC[roc].level) == 1 
      && findBin(sign(data[*pos+1]),2,addressLevels.ROC[roc].level) == 0 ) {
    LOG(logDEBUG1) << "Found corrupt ROC header ("<< std::setw(5) << sign(data[*pos]) << " " << std::setw(5) << sign(data[*pos+1]) << ") after " << std::dec << *pos << " bits.";
    *pos += L_ROC_HEADER;
    return true;
  }
  // ROC header signature: UltraBlack, Black, lastDAC
  else if(findBin(sign(data[*pos]),2,addressLevels.ROC[roc].level) == 0 
	  && findBin(sign(data[*pos+1]),2,addressLevels.ROC[roc].level) == 1 ) {
    LOG(logDEBUG1) << "Found ROC header ("<< std::setw(5) << sign(data[*pos]) << " " << std::setw(5) << sign(data[*pos+1]) << ") after " << std::dec << *pos << " bits.";
    *pos += L_ROC_HEADER;
    return true;
  }
  else return false;
}

bool CMSPixelEventDecoderAnalog::find_tbm_trailer(std::vector< uint16_t > data, unsigned int pos) {
  // TBM trailer signature: UltraBlack, UltraBlack, Black, Black (+ 4 status)
  if(     findBin(sign(data[pos]),3,addressLevels.TBM.level) != 0 
	  || findBin(sign(data[pos+1]),3,addressLevels.TBM.level) != 0 
	  || findBin(sign(data[pos+2]),3,addressLevels.TBM.level) != 1 
	  || findBin(sign(data[pos+3]),3,addressLevels.TBM.level) != 1) {
    LOG(logDEBUG1) << "Found TBM trailer at " << pos << ".";
    return false;
  }
  else return true;
}

bool CMSPixelEventDecoderAnalog::find_tbm_header(std::vector< uint16_t > data, unsigned int pos) {
  // TBM trailer signature: UltraBlack, UltraBlack, UltraBlack, Black (+ 4 trigger num)
  if(     findBin(sign(data[pos]),3,addressLevels.TBM.level) != 0 
	  || findBin(sign(data[pos+1]),3,addressLevels.TBM.level) != 0 
	  || findBin(sign(data[pos+2]),3,addressLevels.TBM.level) != 0 
	  || findBin(sign(data[pos+3]),3,addressLevels.TBM.level) != 1) {
    LOG(logDEBUG1) << "Found TBM header at " << pos << ".";
    return false;
  }
  else return true;
}

int CMSPixelEventDecoderAnalog::decode_hit(std::vector< uint16_t > data, unsigned int * pos, unsigned int roc, pixel * pixhit) {
  if(L_GRANULARITY*data.size() - *pos < L_HIT) {
    LOG(logDEBUG4) << "Not enough data for a pixel hit.";
    *pos = L_GRANULARITY*data.size();   // Set pointer to the end of data.
    return DEC_ERROR_NO_MORE_DATA;
  }
    
  // Find levels and translate them into DCOL and pixel addresses:
  int c1 = findBin( sign(data[*pos+0]), 5, addressLevels.address[roc].level );
  int c0 = findBin( sign(data[*pos+1]), 5, addressLevels.address[roc].level );
  int a2 = findBin( sign(data[*pos+2]), 5, addressLevels.address[roc].level );
  int a1 = findBin( sign(data[*pos+3]), 5, addressLevels.address[roc].level );
  int a0 = findBin( sign(data[*pos+4]), 5, addressLevels.address[roc].level );
  int aa = sign(data[*pos+5]);

  *pos += L_HIT; //jump to next hit.

  int dcol =  c1*6 + c0;
  int drow =  a2*36 + a1*6 + a0;

  int col = -1;
  int row = -1;

  // Convert and check pixel address from double column address. Returns TRUE if address is okay:
  if( convertDcolToCol( dcol, drow, col, row ) ) {
    pixhit->raw = aa;
    pixhit->col = col;
    pixhit->row = row;
    pixhit->roc = roc;

    LOG(logINFO) << "HIT ROC" << std::setw(2) << pixhit->roc << " | pix " << pixhit->col << " " << pixhit->row << ", adc " << pixhit->raw;
    CMSPixelEventDecoder::statistics.pixels_valid++;
    return 0;
  }
  else {
    LOG(logWARNING) << "Failed to convert address: dcol " << dcol << " drow " << drow << " (ROC" << roc << ", adc: " << aa << ")";
    CMSPixelEventDecoder::statistics.pixels_invalid++;
    return DEC_ERROR_INVALID_ADDRESS;
  }
}

int CMSPixelEventDecoderAnalog::findBin(int adc, int nlevel, std::vector< int > level ) {

  if( adc < level[0] ) return 0;
  for( int i = 0; i < nlevel; i++ ) if( adc >= level[i] && adc < level[i+1] ) return i;
  return nlevel;
}

int16_t CMSPixelEventDecoderAnalog::sign(uint16_t val) {
  // Convert unsigned int into positive and negative levels:
  int16_t ret = val & 0x0fff;
  if(ret & 0x0800) return ret -= 4096;
  else return ret;
}
