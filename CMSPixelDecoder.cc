// Version: $Id: CMSPixelDecoder.cc 2371 2013-02-14 08:49:35Z spanns $
/*========================================================================*/
/*          CMSPixel Decoder v3.0                                         */
/*          Author: Simon Spannagel (s.spannagel@cern.ch)                 */
/*          Created       23 feb 2012                                     */
/*          Last modified 29 apr 2013                                     */
/*========================================================================*/

#include "CMSPixelDecoder.h"

#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <math.h>
#include <map>

using namespace CMSPixel;

void CMSPixelStatistics::init() {
  head_data = head_trigger = 0;
  evt_valid = evt_empty = evt_invalid = 0;
  pixels_valid = pixels_invalid = 0;
};
void CMSPixelStatistics::update(CMSPixelStatistics stats) {
  head_data += stats.head_data;
  head_trigger += stats.head_trigger;
  evt_valid += stats.evt_valid;
  evt_empty += stats.evt_empty;
  evt_invalid += stats.evt_invalid;
  pixels_valid += stats.pixels_valid;
  pixels_invalid += stats.pixels_invalid;
};
void CMSPixelStatistics::print() {
  LOG(logSUMMARY) << "TB Trigger Marker: " << std::setw(8) <<  head_trigger;
  LOG(logSUMMARY) << "TB Data Marker:    " << std::setw(8) <<  head_data;
  LOG(logSUMMARY) << "  Events empty:    " << std::setw(8) <<  evt_empty;
  LOG(logSUMMARY) << "  Events valid:    " << std::setw(8) <<  evt_valid;
  LOG(logSUMMARY) << "    Pixels valid:  " << std::setw(8) <<  pixels_valid;
  LOG(logSUMMARY) << "  Events invalid:  " << std::setw(8) <<  evt_invalid;
  LOG(logSUMMARY) << "    Pixels invalid:" << std::setw(8) <<  pixels_invalid;
};

bool CMSPixelFileDecoderPSI_ATB::process_rawdata(std::vector< int16_t > * data)
{
  if(!(theROC & ROC_PSI46V2 || theROC & ROC_PSI46XDB)) {
    LOG(logDEBUG4) << "Compressing data from chip type " << static_cast<int>(theROC);
    // Currently we need to dump some zero bits with the hybrid testboard setup...
    // 0000 0000 0000 0111 -> 0111
    std::vector< int16_t > rawdata = *data;
    data->clear();

    for(unsigned int i = 0; i < rawdata.size(); i+=4) {
      unsigned short tempword = 0;
      // Take four words and combine them, each word contains 4 bits in data:
      int tail = rawdata.size() - i;
      if(tail >= 4) tail = 4;

      for(int j = 0; j < tail; j++) tempword = tempword | ((rawdata[i+j]&15) << (4*(3-j)));
      data->push_back(tempword);
    }
  }
  return true;
}

bool CMSPixelFileDecoderPSI_DTB::process_rawdata(std::vector< int16_t > * data) 
{
  //FIXME will probably have 12-of-16 readout!
  if(!(theROC & ROC_PSI46V2 || theROC & ROC_PSI46XDB)) {
    LOG(logDEBUG4) << "Compressing data from chip type " << static_cast<int>(theROC);
    // Currently we need to dump some zero bits with the hybrid testboard setup...
    // 0000 0000 0000 0111 -> 0111
    std::vector< int16_t > rawdata = *data;
    data->clear();

    for(unsigned int i = 0; i < rawdata.size(); i+=4) {
      unsigned short tempword = 0;
      // Take four words and combine them, each word contains 4 bits in data:
      int tail = rawdata.size() - i;
      if(tail >= 4) tail = 4;

      for(int j = 0; j < tail; j++) tempword = tempword | ((rawdata[i+j]&15) << (4*(3-j)));
      data->push_back(tempword);
    }
  }
  return true;
}

bool CMSPixelFileDecoderRAL::process_rawdata(std::vector< int16_t > * rawdata) {
  // IPBus data format: we need to delete some additional headers from the test board.
  // remove padding to 32bit words at the end of the event by reading the data length:
  unsigned int event_length = ((rawdata->at(1)&0xffff0000) | (rawdata->at(0)&0x0000ffff)) - 14;
            
  // Check for stupid event sizes:
  if(event_length/2 > rawdata->size()) return false;
            
  // cut first 8 bytes from header:
  rawdata->erase(rawdata->begin(),rawdata->begin()+4);
  //  and last 14 bytes plus the padding from trailer:
  rawdata->erase(rawdata->begin() + (event_length/2 + event_length%2),rawdata->end());
            
  // Swap endianness of the data:
  for(unsigned int i = 0; i < rawdata->size(); i++) {
    unsigned int swapped = ((rawdata->at(i)<<8)&0xff00) | ((rawdata->at(i)>>8)&0xff);
    rawdata->at(i) = swapped;
  }

  return true;
}

CMSPixelFileDecoder::CMSPixelFileDecoder(const char *FileName, unsigned int rocs, int flags, uint8_t ROCTYPE, const char *addressFile)
{
  // Make the roc type available:
  theROC = ROCTYPE;
  
  // Prepare a new even decoder instance:
  if(ROCTYPE & ROC_PSI46V2 || ROCTYPE & ROC_PSI46XDB) {
    if(!read_address_levels(addressFile,rocs,addressLevels))
      LOG(logERROR) << "Could not read address parameters correctly!";
    else
      print_addresslevels(addressLevels);
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


int CMSPixelFileDecoder::get_event(std::vector<event> * decevt, long int & timestamp) {
  // Check if stream is open:
  if(!mtbStream) return DEC_ERROR_INVALID_FILE;

  std::vector < int16_t > data;
  LOG(logDEBUG) << "GET_EVENT: Start chopping file.";

  // Cut off the next event from the filestream:
  if(!chop_datastream(&data)) return DEC_ERROR_NO_MORE_DATA;

  // Take into account that different testboards write the data differently:
  if(!process_rawdata(&data)) return DEC_ERROR_NO_MORE_DATA;

  // Deliver the timestamp:
  timestamp = cmstime;

  // And finally call the decoder to do the rest of the work:
  int status = evt->get_event(data, decevt);
  statistics.update(evt->statistics);

  return status;
}


bool CMSPixelFileDecoder::readWord(int16_t &word) {
  unsigned char a, b;
  if(feof(mtbStream) || !fread(&a,sizeof(a),1,mtbStream) || !fread(&b,sizeof(b),1,mtbStream)) 
    return false;
  word = (b << 8) | a;
  return true;
}

bool CMSPixelFileDecoder::chop_datastream(std::vector< int16_t > * rawdata) {

  int16_t word;
  rawdata->clear();
    
  LOG(logDEBUG1) << "Chopping datastream at MTB headers...";
  if(!readWord(word)) return false;

  while (!word_is_data(word)) {
    // If header is detected read more words:
    if(word_is_trigger(word)) {
      LOG(logDEBUG1) << "GET_EVENT::STATUS trigger detected: " << std::hex << word << std::dec;
      statistics.head_trigger++;

      // read 3 more words after header:
      int16_t head[4];
      for(int h = 1; h < 4; h++ ) {
	if(!readWord(word)) return false;
	head[h] = word;
      }
      // Calculating the tigger time of the event:
      cmstime = ( static_cast< long >(head[1]) << 32 ) + static_cast< long >((head[2] << 16) | head[3]);
      LOG(logDEBUG1) << "Timestamp: " << cmstime;
    }
    else if(word_is_header(word) ) {
      LOG(logDEBUG1) << "GET_EVENT::STATUS header detected : " << std::hex << word << std::dec;
      // read 3 more words after header:
      for(int h = 1; h < 4; h++ ) if(!readWord(word)) return false;
    }
    else {
      LOG(logDEBUG1) << "GET_EVENT::STATUS drop: " << std::hex << word << std::dec;
    }
    if(!readWord(word)) return false;
  }
    
  LOG(logDEBUG) << "GET_EVENT::STATUS data header     : " << std::hex << word << std::dec;
  statistics.head_data++;

  // read 3 more words after header:
  for(int i = 1; i < 4; i++) if(!readWord(word)) return false;
            
  // read the data until the next MTB header arises:
 morewords:
  if(!readWord(word)) return false;
  while( !word_is_header(word) && !feof(mtbStream)){
    rawdata->push_back(word);
    if(!readWord(word)) return false;
  }

  //FIXME For IPBus readout check the next header words, too - they should be header again:
  if(!readWord(word)) return false;
  /*  if(ipbus && !tb->word_is_header(word)) {
      rawdata->push_back(word);
      goto morewords;
      }
      else LOG(logDEBUG1) << "Double-checked header.";
  */
  LOG(logDEBUG1) << "Raw data array size: " << rawdata->size() << ", so " << 16*rawdata->size() << " bits.";
    
  // Rewind one word to detect the header correctly later on:
  fseek(mtbStream , -4 , SEEK_CUR);
  
  // Might be empty, let's catch that here:
  if(rawdata->empty()) return false;

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

void CMSPixelFileDecoder::print_addresslevels(levelset addLevels) {
  std::stringstream os;
  os << "STATUS TBM    header: ";
  std::vector<int>::iterator it;
  for(it = addLevels.TBM.level.begin(); it < addLevels.TBM.level.end(); ++it)
    os << std::setw(5) << static_cast<int>(*it) << " ";
  LOG(logDEBUG3) << os.str();
  os.str("");

  for (unsigned int iroc = 0; iroc < addLevels.address.size(); iroc++ ){
    os << "STATUS ROC" << std::setw(2) << iroc << "  header: ";
    for(it = addLevels.ROC[iroc].level.begin(); it < addLevels.ROC[iroc].level.end(); ++it)
      os << std::setw(5) << *it << " ";
    LOG(logDEBUG3) << os.str();
    os.str("");
    os << "STATUS ROC" << std::setw(2) << iroc << " address: ";
    for(it = addLevels.address[iroc].level.begin(); it < addLevels.address[iroc].level.end(); ++it)
      os << std::setw(5) << *it << " ";
    LOG(logDEBUG3) << os.str();
  }
}

/*========================================================================*/
/*          CMSPixel Event Decoder                                        */
/*          parent class CMSPixelDecoder                                  */
/*========================================================================*/


CMSPixelEventDecoder::CMSPixelEventDecoder(unsigned int rocs, int flags, uint8_t ROCTYPE) {

  flag = flags;
  noOfROC = rocs;
  theROC = ROCTYPE;

}

CMSPixelEventDecoder::~CMSPixelEventDecoder() {
  // Nothing to do.
}

int CMSPixelEventDecoder::get_event(std::vector< int16_t > data, std::vector<event> * evt) {

  LOG(logDEBUG) << "GET_EVENT: Start decoding.";
  LOG(logDEBUG1) << "Received " << 16*data.size() << " bits of event data.";
  evt->clear();
  statistics.init();

  unsigned int pos = 0;
  
  // Some data needs preprocessing, map uint to the integer level values (analog chips)
  if(!preprocessing(&data)) return DEC_ERROR_INVALID_DATA;

  // Check sanity
  int status = pre_check_sanity(&data,&pos);
  if(status != 0) return status;

  // Init ROC id:
  unsigned int roc = 0;
  event tmp;
    
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

  LOG(logDEBUG) << "GET_EVENT::STATUS end of event processing.";
  return 0;
}


int CMSPixelEventDecoder::pre_check_sanity(std::vector< int16_t > * data, unsigned int * pos) {

  LOG(logDEBUG2) << "Checking pre decoding event sanity...";
  unsigned int length = L_GRANULARITY*data->size();
    
  // Check presence of TBM tokens if necessary:
  if(flag & FLAG_HAVETBM) {
    if(!find_tbm_header(*data,0)) {
      LOG(logWARNING) << "GET_EVENT: event contained no valid TBM_HEADER. Skipped.";
      return DEC_ERROR_NO_TBM_HEADER;
    }
    else LOG(logDEBUG) << "GET_EVENT::STATUS TBM_HEADER correctly detected.";

    if(!find_tbm_trailer(*data,length - L_TRAILER)) {
      LOG(logWARNING) << "GET_EVENT: event contained no valid TBM_TRAILER. Skipped.";
      return DEC_ERROR_NO_TBM_HEADER;
    }
    else LOG(logDEBUG) << "GET_EVENT::STATUS TBM_TRAILER correctly detected.";

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
  if( length <= noOfROC*L_ROC_HEADER ){
    LOG(logINFO) << "GET_EVENT: event is empty, " << length << " words <= " <<  noOfROC*L_ROC_HEADER << ".";
    statistics.evt_empty++;
    return DEC_ERROR_EMPTY_EVENT;
  }
  // There might even be a huge event:
  else if( length > 2222*L_GRANULARITY ) {
    LOG(logWARNING) << "GET_EVENT: detected huge event (" << length << " words). Skipped.";
    statistics.evt_invalid++;
    return DEC_ERROR_INVALID_DATA;
  }


  // Maybe we deleted something, recompute length:
  length = L_GRANULARITY*data->size();

  // Ugly hack for single analog/xdb chip readout without TBM or emulator: ROC UB level seems broken there...
  if((theROC & ROC_PSI46V2 || theROC & ROC_PSI46XDB) && !(flag & FLAG_HAVETBM) && noOfROC == 1) {
    // Just set the starting position after the first ROC header, assuming no idle pattern:
    *pos = L_ROC_HEADER+1;
    LOG(logDEBUG3) << "FIXME(singleAnaROC): Set starting position to: " << *pos;
    data->erase(data->end() - 6,data->end());
    LOG(logDEBUG3) << "FIXME(singleAnaROC): Removed some trailers.";
  }
  // Find the start position with the first ROC header (maybe there are idle patterns before...)
  else {
    bool found = false;
    for(unsigned int i = 0; i < length; i++) {
      if(find_roc_header(*data, &i, 0)) {
	*pos = i;
	found = true;
	break;
      }
    }
    
    if(!found) {
      LOG(logERROR) << "GET_EVENT event contains no ROC header.";
      statistics.evt_invalid++;
      return DEC_ERROR_NO_ROC_HEADER;
    }
    else
      LOG(logDEBUG3) << "Starting position after first ROC header: " << *pos;
  }
  
  LOG(logDEBUG2) << "Pre-decoding event sane.";
  LOG(logDEBUG) << "GET_EVENT::STATUS event: " << length << " data words.";
  return 0;
}
  


int CMSPixelEventDecoder::post_check_sanity(std::vector< event > * evt, unsigned int rocs) {    

  LOG(logDEBUG2) << "Checking post-decoding event sanity...";

  if(noOfROC != rocs+1) {
    LOG(logWARNING) << "GET_EVENT ROCS: PRESET " << noOfROC << " != DATA " << rocs+1 << ". Skipped.";
    statistics.evt_invalid++;
    return DEC_ERROR_NO_OF_ROCS;
  }
  else LOG(logDEBUG) << "GET_EVENT::STATUS correctly detected " << rocs+1 << " ROC(s).";

  if(evt->size() == 0) {
    LOG(logINFO) << "GET_EVENT: event is empty, no hits decoded.";
    statistics.evt_empty++;
    return DEC_ERROR_EMPTY_EVENT;
  }

  LOG(logDEBUG2) << "Post-decoding event sane.";
  statistics.evt_valid++;
  return 0;
}


bool CMSPixelEventDecoder::convertDcolToCol(int dcol, int pix, int & col, int & row)
{
  LOG(logDEBUG3) << "converting dcol " << dcol << " pix " << pix;
  if( dcol < 0 || dcol >= ROCNUMDCOLS || pix < 2 || pix > 161) 
    return false;

  // pix%2: 0 = 1st col, 1 = 2nd col of a double column
  col = dcol * 2 + pix%2; // col address, starts from 0
  row = abs( static_cast<int>(pix/2) - ROCNUMROWS);  // row address, starts from 0

  if( col < 0 || col > ROCNUMCOLS || row < 0 || row > ROCNUMROWS ) 
    return false;

  LOG(logDEBUG3) << " to col " << col << " row " << row;
  return true;
}



/*========================================================================*/
/*          CMSPixel Event Decoder                                        */
/*          child class CMSPixelEventDecoderDigital                       */
/*          decoding DIGITAL chip data                                    */
/*========================================================================*/

CMSPixelEventDecoderDigital::CMSPixelEventDecoderDigital(unsigned int rocs, int flags, uint8_t ROCTYPE) : CMSPixelEventDecoder(rocs, flags, ROCTYPE)
{
  LOG(logDEBUG) << "Preparing a digital decoder instance...";
  // Loading constants:
  LOG(logDEBUG2) << "Loading constants...";
  load_constants();
}

bool CMSPixelEventDecoderDigital::preprocessing(std::vector< int16_t > * data) {
  std::stringstream os;
  for(unsigned int i = 0; i < data->size();i++) 
    os << std::hex << data->at(i) << " ";
  LOG(logDEBUG4) << os.str() << std::dec;
  return true;
}

int CMSPixelEventDecoderDigital::get_bit(std::vector< int16_t > data, int bit_offset) {
  unsigned int ibyte=bit_offset/L_GRANULARITY;
  int byteval;
  if (ibyte < data.size()) {
    byteval = data[ibyte];
  }
  else return 0;

  // get bit, counting from most significant bit to least significant bit
  int ibit = (L_GRANULARITY-1) - (bit_offset-ibyte*L_GRANULARITY);
  int bitval = (byteval >> ibit)&1;
  return bitval;
}

int CMSPixelEventDecoderDigital::get_bits(std::vector< int16_t > data, int bit_offset,int number_of_bits) {
  int value = 0;
  for (int ibit = 0; ibit < number_of_bits; ibit++) {
    value = 2*value+get_bit(data,bit_offset+ibit);
  }
  return value;
}

bool CMSPixelEventDecoderDigital::find_roc_header(std::vector< int16_t > data, unsigned int * pos, unsigned int)
{
  // ROC header: 0111 1111 10SD, S & D are reserved status bits.
  // Possible ROC headers: 0x7f8 0x7f9 0x7fa 0x7fb
  int search_head = get_bits(data, *pos, L_ROC_HEADER);

  // Ugly hack to compensate for the chip sending correct ROC headers: 0x7fX might be 0x3fX too...
  if((flag & FLAG_ALLOW_CORRUPT_ROC_HEADERS) && (search_head == 0x7f0 || search_head == 0x3f8 || search_head == 0x3f9 || search_head == 0x3fa || search_head == 0x3fb)) {
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

bool CMSPixelEventDecoderDigital::find_tbm_trailer(std::vector< int16_t > data, unsigned int pos)
{
  if (get_bits(data, pos, L_TRAILER)==0x7fa) {
    LOG(logDEBUG1) << "Found TBM trailer at " << pos << ".";
    return true;
  }
  return false;
}

bool CMSPixelEventDecoderDigital::find_tbm_header(std::vector< int16_t > data, unsigned int pos)
{
  if (get_bits(data, pos, L_HEADER)==0x7ff) {
    LOG(logDEBUG1) << "Found TBM header at " << pos << ".";
    return true;
  }
  return false;
}

int CMSPixelEventDecoderDigital::decode_hit(std::vector< int16_t > data, unsigned int * pos, unsigned int roc, event * pixhit)
{
  if(L_GRANULARITY*data.size() - *pos < L_HIT) {
    LOG(logDEBUG1) << "Dropping " << L_GRANULARITY*data.size() - *pos << " bit at the end of the event.";
    *pos = L_GRANULARITY*data.size();   // Set pointer to the end of data.
    return DEC_ERROR_NO_MORE_DATA;
  }
  int pixel_hit = get_bits(data,*pos,L_HIT);
  *pos += L_HIT; //jump to next hit.
    
  std::ostringstream os;
  os << "hit: " << std::hex << pixel_hit << std::dec << ": ";
  for(int i = 23; i >= 0; i--) {
    os << ((pixel_hit>>i)&1);
    if(i==4 || i==5|| i==9|| i==12|| i==15|| i==18|| i==21) os << ".";
  }
  LOG(logDEBUG3) << os.str();

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

    LOG(logINFO) << "GET_EVENT::HIT ROC" << std::setw(2) << roc << " | pix " << pixhit->col << " " << pixhit->row << ", raw " << pixhit->raw;
    CMSPixelEventDecoder::statistics.pixels_valid++;
    return 0;
  }
  else {
    // Something went wrong with the decoding:
    LOG(logWARNING) << "GET_EVENT failed to convert address of [" << std::hex << pixel_hit << std::dec << "]: dcol " << dcol << " drow " << drow << " (ROC" << roc << ", raw: " << pulseheight << ")";
    CMSPixelEventDecoder::statistics.pixels_invalid++;
    return DEC_ERROR_INVALID_ADDRESS;
  }
}




/*========================================================================*/
/*          CMSPixel Event Decoder                                        */
/*          child class CMSPixelEventDecoderAnalog                        */
/*          decoding ANALOG chip data                                     */
/*========================================================================*/

CMSPixelEventDecoderAnalog::CMSPixelEventDecoderAnalog(unsigned int rocs, int flags, uint8_t ROCTYPE, levelset addLevels) : CMSPixelEventDecoder(rocs, flags, ROCTYPE)
{
  LOG(logDEBUG) << "Preparing an analog decoder instance...";
  // Loading constants:
  LOG(logDEBUG2) << "Loading constants...";
  load_constants();
    
  // Try to read the address levels file
  addressLevels = addLevels;
}

bool CMSPixelEventDecoderAnalog::preprocessing(std::vector< int16_t > * data) {
  // Restoring analog levels being negative:
  std::stringstream os;
  for(unsigned int i = 0; i < data->size();i++) {
    data->at(i) = data->at(i) & 0x0fff;
    if( data->at(i) & 0x0800 ) data->at(i) -= 4096; //hex 800 = 2048
    os << std::dec << data->at(i) << " ";
  }
  LOG(logDEBUG4) << os.str();
  return true;
}


bool CMSPixelEventDecoderAnalog::find_roc_header(std::vector< int16_t > data, unsigned int * pos, unsigned int roc) {

  // Did we reach max number of ROCs read in from address levels file?
  if(roc >= addressLevels.ROC.size()) return false;
    
  // ROC header signature: UltraBlack, Black, lastDAC
  if(     findBin(data[*pos],2,addressLevels.ROC[roc].level) == 0 
	  && findBin(data[*pos+1],2,addressLevels.ROC[roc].level) == 1 ) {
    LOG(logDEBUG1) << "Found ROC header ("<< std::setw(5) << data[*pos] << " " << std::setw(5) << data[*pos+1] << ") after " << std::dec << *pos << " bits.";
    *pos += L_ROC_HEADER;
    return true;
  }
  else return false;
}

bool CMSPixelEventDecoderAnalog::find_tbm_trailer(std::vector< int16_t > data, unsigned int pos) {
  // TBM trailer signature: UltraBlack, UltraBlack, Black, Black (+ 4 status)
  if(     findBin(data[pos],3,addressLevels.TBM.level) != 0 
	  || findBin(data[pos+1],3,addressLevels.TBM.level) != 0 
	  || findBin(data[pos+2],3,addressLevels.TBM.level) != 1 
	  || findBin(data[pos+3],3,addressLevels.TBM.level) != 1) {
    LOG(logDEBUG1) << "Found TBM trailer at " << pos << ".";
    return false;
  }
  else return true;
}

bool CMSPixelEventDecoderAnalog::find_tbm_header(std::vector< int16_t > data, unsigned int pos) {
  // TBM trailer signature: UltraBlack, UltraBlack, UltraBlack, Black (+ 4 trigger num)
  if(     findBin(data[pos],3,addressLevels.TBM.level) != 0 
	  || findBin(data[pos+1],3,addressLevels.TBM.level) != 0 
	  || findBin(data[pos+2],3,addressLevels.TBM.level) != 0 
	  || findBin(data[pos+3],3,addressLevels.TBM.level) != 1) {
    LOG(logDEBUG1) << "Found TBM header at " << pos << ".";
    return false;
  }
  else return true;
}

int CMSPixelEventDecoderAnalog::decode_hit(std::vector< int16_t > data, unsigned int * pos, unsigned int roc, event * pixhit) {
  if(L_GRANULARITY*data.size() - *pos < L_HIT) {
    LOG(logDEBUG4) << "Not enough data for a pixel hit.";
    *pos = L_GRANULARITY*data.size();   // Set pointer to the end of data.
    return DEC_ERROR_NO_MORE_DATA;
  }
    
  // Find levels and translate them into DCOL and pixel addresses:
  int c1 = findBin( data[*pos+0], 5, addressLevels.address[roc].level );
  int c0 = findBin( data[*pos+1], 5, addressLevels.address[roc].level );
  int a2 = findBin( data[*pos+2], 5, addressLevels.address[roc].level );
  int a1 = findBin( data[*pos+3], 5, addressLevels.address[roc].level );
  int a0 = findBin( data[*pos+4], 5, addressLevels.address[roc].level );
  int aa = data[*pos+5];

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

    LOG(logINFO) << "GET_EVENT::HIT ROC" << std::setw(2) << roc << " | pix " << pixhit->col << " " << pixhit->row << ", raw " << pixhit->raw;
    CMSPixelEventDecoder::statistics.pixels_valid++;
    return 0;
  }
  else {
    LOG(logDEBUG4) << "raw: " << data[*pos+0] << " " << data[*pos+1] << " " << data[*pos+2] << " " << data[*pos+3] << " " << data[*pos+4];
    LOG(logWARNING) << "GET_EVENT failed to convert address: dcol " << dcol << " drow " << drow << " (ROC" << roc << ", raw: " << aa << ")";
    CMSPixelEventDecoder::statistics.pixels_invalid++;
    return DEC_ERROR_INVALID_ADDRESS;
  }
}

int CMSPixelEventDecoderAnalog::findBin(int adc, int nlevel, std::vector< int > level ) {

  if( adc < level[0] ) return 0;
  for( int i = 0; i < nlevel; i++ ) if( adc >= level[i] && adc < level[i+1] ) return i;
  return nlevel;
}


