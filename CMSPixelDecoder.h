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

#ifndef CMSPIXELDECODER_H_
#define CMSPIXELDECODER_H_

#include <fstream>
#include <vector>
#include <map>
#include <iostream>
#include <stdint.h>
#include <sstream>
#include <string>
#include <stdio.h>
#include <iomanip>
#include <sys/time.h>
#include <sys/types.h>

// Flags:
#define FLAG_ALLOW_CORRUPT_ROC_HEADERS 1
#define FLAG_HAVETBM 2
#define FLAG_12BITS_PER_WORD 4
#define FLAG_16BITS_PER_WORD 8
#define FLAG_OVERWRITE_ROC_HEADER_POS 16
#define FLAG_OLD_RAL_FORMAT 32

// Decoder errors:
#define DEC_ERROR_EMPTY_EVENT -1
#define DEC_ERROR_INVALID_ROC_HEADER -2
#define DEC_ERROR_INVALID_ADDRESS -3
#define DEC_ERROR_NO_TBM_HEADER -4
#define DEC_ERROR_NO_ROC_HEADER -5
#define DEC_ERROR_NO_OF_ROCS -6
#define DEC_ERROR_HUGE_EVENT -7
#define DEC_ERROR_INVALID_EVENT -8
#define DEC_ERROR_NO_MORE_DATA -9
#define DEC_ERROR_INVALID_FILE -10


// Sensor properties:
#define ROCNUMDCOLS 26
#define ROCNUMCOLS 52
#define ROCNUMROWS 80

namespace CMSPixel {

  // ROC types we have to care about:
  enum {
    ROC_PSI46V2       = 0x01,
    ROC_PSI46XDB      = 0x02,
    ROC_PSI46DIG      = 0x04,
    ROC_PSI46DIG_TRIG = 0x08,
    ROC_PSI46DIGV2_B  = 0x10,
    ROC_PSI46DIGV2    = 0x20
  };

  // Struct for raw data readout
  typedef struct {
    int roc;
    int col;
    int row;
    int raw;
    double vcal;
  } pixel;

  typedef struct {
    int64_t timestamp;
    uint32_t trigger_number;
    char trigger_phase;
  } timing;

  // Struct for Decoder levels
  class levels {
  public:
  levels() : level() {};
    std::vector< int > level;
  };
    
  class levelset {
  public:
  levelset() :
    TBM(), ROC(), address() {};
    levels TBM;
    std::vector< levels > ROC;
    std::vector< levels > address;
  };

  class CMSPixelStatistics {
  public:
    CMSPixelStatistics() :
    data_blocks(0),
      head_data(0),
      head_trigger(0),
      evt_valid(0),
      evt_empty(0),
      evt_invalid(0),
      pixels_valid(0),
      pixels_invalid(0)	{ init(); };
    void init();
    void update(CMSPixelStatistics stats);
    void print();
    std::string get();
    // Raw data blocks:
    uint32_t data_blocks;
    // Number of detected testboard data markers
    uint32_t head_data;
    // Number of detected testboard trigger markers
    uint32_t head_trigger;
    // Number of valid events (everything allright)
    uint32_t evt_valid;
    // Number of empty events (fine, but contained no pixel)
    uint32_t evt_empty;
    // Number of invalid events (something is fishy with this)
    //  * No ROC headers / wrong number of ROC headers
    //  * Missing TBM Header or Trailer
    uint32_t evt_invalid;
    // Number of correctly decoded pixel hits
    uint32_t pixels_valid;
    // Number of pixel hits with invalid address or zero-bit (undecodable)
    // Events containing only some invalid pixels are still delivered, only return value is set.
    uint32_t pixels_invalid;

    //    int data_notrailer;
    //    int data_huge;
    //    int data_norocs;
    //    int data_fewrocs;
    //    std::map<unsigned int,int> data_diffrocs;
    //    std::map<unsigned int,int> data_rocheads;
    //    int headers_dropped;
  };



  /*========================================================================*/
  /*          CMSPixel Event Decoder                                        */
  /*          parent class CMSPixelDecoder                                  */
  /*========================================================================*/

  class CMSPixelEventDecoder {
  public:
    CMSPixelEventDecoder(unsigned int rocs, int flags, uint8_t ROCTYPE);
    virtual ~CMSPixelEventDecoder();
    int get_event(std::vector< uint16_t > & data, std::vector<pixel> * evt);
    CMSPixelStatistics statistics;

  protected:
    bool convertDcolToCol(int dcol, int pix, int & col, int & row);

    unsigned int L_HEADER, L_TRAILER, L_EMPTYEVT, L_GRANULARITY, L_HIT, L_ROC_HEADER, L_HUGE_EVENT;
    const int flag;
    const unsigned int noOfROC;
    const uint8_t theROC;

  private:
    // Purely virtual, to be implemented in the child classes (digital/analog):
    inline virtual void load_constants(int flags) = 0;
    virtual bool find_roc_header(std::vector< uint16_t > data, unsigned int * pos, unsigned int roc) = 0;
    virtual bool find_tbm_header(std::vector< uint16_t > data, unsigned int pos) = 0;
    virtual bool find_tbm_trailer(std::vector< uint16_t > data, unsigned int pos) = 0;
    virtual int decode_hit(std::vector< uint16_t > data, unsigned int * pos, unsigned int roc, pixel * hit) = 0;
    virtual std::string print_data(std::vector< uint16_t> * data) = 0;
            
    // These functions are the same no matter what data format we have:
    int pre_check_sanity(std::vector< uint16_t > * data, unsigned int * pos);
    int post_check_sanity(std::vector< pixel > * evt, unsigned int rocs);

  };


  /*========================================================================*/
  /*          CMSPixel Event Decoder                                        */
  /*          child class CMSPixelEventDecoderAnalog                        */
  /*          decoding ANALOG chip data                                     */
  /*========================================================================*/
 
  class CMSPixelEventDecoderAnalog : public CMSPixelEventDecoder {
  public:
    CMSPixelEventDecoderAnalog(unsigned int rocs, int flags, uint8_t ROCTYPE, levelset addLevels);

  protected:
    inline void load_constants(int flags) {
      // Silence unised variable warning
      (void)flags;
      // Lenth of different tokens:
      // Analog: all values given in data words (16bit)
      L_ROC_HEADER = 3;   // ROC header
      L_HIT = 6;          // Hit length
      L_GRANULARITY = 1;  // Data granularity (analog: words)

      // Check whether we should have a TBM header or not:
      L_HEADER = 8;     // FPGA header without TBM emu: 1 word;
      L_TRAILER = 8;    // FPGA trailer without TBM emu: 6 words;
    };

    bool find_roc_header(std::vector< uint16_t > data, unsigned int * pos, unsigned int roc);
    bool find_tbm_header(std::vector< uint16_t > adc, unsigned int pos);    
    bool find_tbm_trailer(std::vector< uint16_t > adc, unsigned int pos);
    int decode_hit(std::vector< uint16_t > data, unsigned int * pos, unsigned int roc, pixel * hit);

  private:
    int findBin(int adc, int nlevel, std::vector< int > level);
    int16_t sign(uint16_t val);
    std::string print_data(std::vector< uint16_t> * data);
    levelset addressLevels;
  };

  
  /*========================================================================*/
  /*          CMSPixel Event Decoder                                        */
  /*          child class CMSPixelEventDecoderDigital                       */
  /*          decoding DIGITAL chip data                                    */
  /*========================================================================*/
 
  class CMSPixelEventDecoderDigital : public CMSPixelEventDecoder {
  public:
    CMSPixelEventDecoderDigital(unsigned int rocs, int flags, uint8_t ROCTYPE);

  protected:
    inline void load_constants(int flags) {
      // Lenth of different tokens:
      // Digital: all values given in single bits
      L_ROC_HEADER = 12;   // ROC header
      L_HIT = 24;          // Hit length
      
      // Data granularity (digital: bits per word)
      if(flags & FLAG_12BITS_PER_WORD)
	L_GRANULARITY = 12;
      else if(flags & FLAG_16BITS_PER_WORD)
 	L_GRANULARITY = 16;
      else
 	L_GRANULARITY = 4;

      // Check whether we should have a TBM header or not:
      L_HEADER = 28;
      L_TRAILER = 28;
    };

    bool find_roc_header(std::vector< uint16_t > data, unsigned int * pos, unsigned int roc);
    bool find_tbm_header(std::vector< uint16_t > adc, unsigned int pos);
    bool find_tbm_trailer(std::vector< uint16_t > adc, unsigned int pos);
    int decode_hit(std::vector< uint16_t > data, unsigned int * pos, unsigned int roc, pixel * hit);

  private:
    int get_bit(std::vector< uint16_t > data, int bit_offset);
    int get_bits(std::vector< uint16_t > data, int bit_offset,int number_of_bits);
    std::string print_data(std::vector< uint16_t> * data);
    std::string print_hit(int hit);
  };



  /*========================================================================*/
  /*          CMSPixel File Decoder                                         */
  /*          class to decode full CMSPixel data files                      */
  /*========================================================================*/

  class CMSPixelFileDecoder {
  public:
    CMSPixelFileDecoder(const char *FileName, unsigned int rocs, int flags, uint8_t ROCTYPE, const char *addressFile);
    virtual ~CMSPixelFileDecoder();

    int get_event(std::vector<pixel> * decevt, timing & evt_timing);

    virtual bool word_is_data(unsigned short word) = 0;
    virtual bool word_is_trigger(unsigned short word) = 0;
    virtual bool word_is_header(unsigned short word) = 0;
    virtual bool word_is_2nd_header(unsigned short word) = 0;
    virtual bool process_rawdata(std::vector< uint16_t > * rawdata) = 0;

    CMSPixelStatistics statistics;
    CMSPixelEventDecoder * evt;

  protected:
    uint8_t theROC;
    virtual bool readWord(uint16_t &word);
    FILE * mtbStream;
    //int64_t cmstime;
    timing cms_t;

  private:
    bool chop_datastream(std::vector< uint16_t > * rawdata);
    bool read_address_levels(const char* levelsFile, unsigned int rocs, levelset & addressLevels);
    std::string print_addresslevels(levelset addLevels);
    levelset addressLevels;
  };

  class CMSPixelFileDecoderRAL : public CMSPixelFileDecoder {
  public:
  CMSPixelFileDecoderRAL(const char *FileName, unsigned int rocs, int flags, uint8_t ROCTYPE) : CMSPixelFileDecoder(FileName, rocs, addflags(flags), ROCTYPE, ""), ral_flags(flags) {};
    ~CMSPixelFileDecoderRAL() {};
  private:
    int ral_flags;
    inline int addflags(int flags) {
      return (flags | FLAG_16BITS_PER_WORD);
    };
    bool readWord(uint16_t &word);
    inline bool word_is_data(unsigned short word) {
      // IPBus format starts with 0xFFFFFFFF, no other headers allowed.
      if(word == 0xFFFF) return true;
      else return false;
    };
    inline bool word_is_trigger(unsigned short word) {
      // IPBus format doesn't know about trigger headers.
      (void)word;
      return false;
    };
    inline bool word_is_header(unsigned short word) {
      // IPBus format doesn't know about headers other than data headers.
      if(word == 0xFFFF) return true;
      else return false;
    };
    inline bool word_is_2nd_header(unsigned short word) {
      // IPBus header is 32bit, so check second part (also 0xFFFF):
      return word_is_header(word);
    };
    bool process_rawdata(std::vector< uint16_t > * rawdata);
  };

class CMSPixelFileDecoderPSI_ATB : public CMSPixelFileDecoder {
  public:
  CMSPixelFileDecoderPSI_ATB(const char *FileName, unsigned int rocs, int flags, uint8_t ROCTYPE, const char *addressFile) : CMSPixelFileDecoder(FileName, rocs, flags, ROCTYPE, addressFile) {};
    ~CMSPixelFileDecoderPSI_ATB() {};
  private:
    inline bool word_is_data(unsigned short word) {
      if(word == 0x8001 || word == 0x8081 || word == 0x8005) return true;
      else return false;
    };
    inline bool word_is_trigger(unsigned short word) {
      if(word == 0x8004 || word == 0x8002) return true;
      else return false;
    };
    inline bool word_is_header(unsigned short word) {
      if(word == 0x8001 || word == 0x8081 || word == 0x8005 || word == 0x8004 || word == 0x8002 || word == 0x8008 || word == 0x8010) return true;
      else return false;
    };
    inline bool word_is_2nd_header(unsigned short word) {
      // PSI ATB features 16bit headers only.
      (void)word;
      return true;
    };
    bool process_rawdata(std::vector< uint16_t > * rawdata);
  };

class CMSPixelFileDecoderPSI_DTB : public CMSPixelFileDecoder {
  public:
  CMSPixelFileDecoderPSI_DTB(const char *FileName, unsigned int rocs, int flags, uint8_t ROCTYPE, const char *addressFile) : CMSPixelFileDecoder(FileName, rocs, flags, ROCTYPE, addressFile) {};
    ~CMSPixelFileDecoderPSI_DTB() {};
  private:
    inline bool word_is_data(unsigned short word) {
      if(word == 0x8501 || word == 0x8581 || word == 0x8505) return true;
      else return false;
    };
    inline bool word_is_trigger(unsigned short word) {
      if(word == 0x8504) return true;
      else return false;
    };
    inline bool word_is_header(unsigned short word) {
      if(word == 0x8501 || word == 0x8581 || word == 0x8505 || word == 0x8504 || word == 0x8502 || word == 0x8508 || word == 0x8510) return true;
      else return false;
    };
    inline bool word_is_2nd_header(unsigned short word) {
      // PSI DTB features 16bit headers only.
      (void)word;
      return true;
    };
    bool process_rawdata(std::vector< uint16_t > * rawdata);
  };




  enum TLogLevel {logQUIET,logSUMMARY,logERROR, logWARNING, logINFO, logDEBUG, logDEBUG1, logDEBUG2, logDEBUG3, logDEBUG4};

  class Log
  {
  public:
    Log();
    virtual ~Log();
    std::ostringstream& Get(TLogLevel level = logINFO);
  public:
    static TLogLevel& ReportingLevel();
    static std::string ToString(TLogLevel level);
    static TLogLevel FromString(const std::string& level);
  protected:
    std::ostringstream os;
  private:
    Log(const Log&);
    Log& operator =(const Log&);
    std::string NowTime();
  };

  inline Log::Log() : os()
  {
  }

  inline std::string Log::NowTime()
  {
    char buffer[11];
    time_t t;
    time(&t);
    tm r = * localtime(&t);//{0};
    strftime(buffer, sizeof(buffer), "%X", localtime_r(&t, &r));
    struct timeval tv;
    gettimeofday(&tv, 0);
    char result[100] = {0};
    std::sprintf(result, "%s.%03ld", buffer, static_cast<long>(tv.tv_usec) / 1000); 
    return result;
  }

  inline std::ostringstream& Log::Get(TLogLevel level)
  {
    os << "- " << NowTime();
    os << " " << std::setw(7) << ToString(level) << ": ";
    os << std::string(level > logDEBUG ? level - logDEBUG : 0, '\t');
    return os;
  }

  inline Log::~Log()
  {
    os << std::endl;
    fprintf(stderr, "%s", os.str().c_str());
    fflush(stderr);
  }

  inline TLogLevel& Log::ReportingLevel()
  {
    static TLogLevel reportingLevel = logSUMMARY;
    return reportingLevel;
  }

  inline std::string Log::ToString(TLogLevel level)
  {
    static const char* const buffer[] = {"QUIET","SUMMARY","ERROR", "WARNING", "INFO", "DEBUG", "DEBUG1", "DEBUG2", "DEBUG3", "DEBUG4"};
    return buffer[level];
  }

  inline TLogLevel Log::FromString(const std::string& level)
    {
      if (level == "DEBUG4")
	return logDEBUG4;
      if (level == "DEBUG3")
	return logDEBUG3;
      if (level == "DEBUG2")
	return logDEBUG2;
      if (level == "DEBUG1")
	return logDEBUG1;
      if (level == "DEBUG")
	return logDEBUG;
      if (level == "INFO")
	return logINFO;
      if (level == "WARNING")
	return logWARNING;
      if (level == "ERROR")
	return logERROR;
      if (level == "SUMMARY")
	return logSUMMARY;
      if (level == "QUIET")
	return logQUIET;
      Log().Get(logWARNING) << "Unknown logging level '" << level << "'. Using WARNING level as default.";
      return logWARNING;
    }

#define LOG(level)				\
  if (level > Log::ReportingLevel()) ;		\
  else Log().Get(level)

}

#endif /*CMSPIXELDECODER_H_*/
