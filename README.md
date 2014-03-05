CMSPixelDecoder
===============

CMSPixelDecoder is a generic decoding class for PSI46-type pixel detector readout
 chips. The CMSPixelDecoder classes aim to provide a full-featured data decoder 
from raw data read from any kind of CMS PSI46 Pixel Readout Chips or combinations
 of them such as modules (16 ROCs) or telescopes.

  The decoder can handle data
  * from any number of ROCs (single ROCs, modules, other setups as telescopes)
  * taken with TBM, with TBM emulation or without any TBM headers
  * from both PSI testboards (ATB, DTB) and RAL testboards (IPBus)

<!-- comment -->

  Both single events and full files can be decoded.

# Usage #
  * Decoding of single events
    In order to decode single events (data streams read from arbitrary input)
    one needs to instanciate a `CMSPixelEventDecoder` for either analog or
    digital ROCs. The constructor needs to know the flags (see below), the
    number of ROCs expected as well as the ROC type (see below). For analog
    data it furthermore needs the address levels of the data stream using the
    "levelset" struct (see `CMSPixelDecoder.h`)

    Constructor for analog ROC data:

    ```
    $ CMSPixelEventDecoderAnalog(unsigned int rocs, int flags, uint8_t ROCTYPE,
    levelset addLevels);
    ```

    Constructor for digital ROC data:
    
    ```
    $ CMSPixelEventDecoderDigital(unsigned int rocs, int flags, uint8_t ROCTYPE);
    ```

    the event decoding is done by calling the `get_event()` method
    
    ```
    $ int get_event(std::vector< int16_t > data, std::vector<pixel> * evt);
    ```

    where "data" is a `int16_t` vector containing the raw data from the readout
    system and "evt" is the decoded event, stored in a standard vector of `pixel`
    structs (see below).

    The decoding statistics can be obtained by accessing the "statistics" member
    of the object. This is possible until the object is destructed. See below.
    
  * Decoding of full data files
    Decoding a full data file works similar to the single event decoding. First
    one needs to instanciate a fiel decoder. Again several parameters have to be
    specified beforehand. Beside the known parameters from single event decoding
    (number of ROCs, ROC type and the decoding flags) the file names of data file
    and address parameters have to be provided:

    ```
    CMSPixelFileDecoder(const char *FileName, unsigned int rocs, int flags,
    uint8_t ROCTYPE, const char *addressFile);
    ```
    
    The decoder returns the next event read and decoded from the file when 
    calling the `get_event method`. In addition to single event decoding the time
    stamp of the trigger is returned, if this data is present in the testboard
    event headers:

    ```
    int get_event(std::vector<pixel> * decevt, timing & dectime);
    ```

    "decevt" is the decoded event (as pixel strcut vector) and "dectime" the
    decoded timing information (see below).

    The decoding statistics can be obtained for either the most recently decoded
    event or the accumulated statistics for the whole run (see below).

# Output Format and Returned Data #

  CMSPixelDecoder provides two overloaded `get_event()` methods, mostly for
  backwards compatibility. For the CMSPixelEventDecoder these are:

  ```
  int get_event(std::vector< uint16_t > & data, std::vector<pixel> * evt);
  int get_event(std::vector< uint16_t > & data, std::vector<uint16_t> * readbac
  ```

  where "evt" is the actual pixel data which is provided in any case as standard 
  vector of `pixel` structs, where every pixel consists of:
    
  * `int roc`:  the ID of the ROC from which the pixel hit has been received
  * `int col`:  the decoded column ID of the pixel hit
  * `int row`:  the decoded row ID of the pixel hit
  * `int raw`:  the raw (uncalibrated) pulse height of the hit
  * `double vcal`: placeholder for the calibrated pulse height, not filled.

<!-- comment -->

  The second possible return is `readback`, a vector of `uint16_t` values
  representing the latest values recorded using the readback mechanism
  of each ROC. Note that these values are only filled if a digital ROC V2+ is
  attached and correctly configured (see "Configuration" below).

  The readback mechanism updates the values every 16 tokens, for events inbetween
  the old values are returned. If the datastream doesn't contain enough
  consecutive events with valid ROC headers, the values will not be updated until
  the next 16 valid events in a row.

  The CMSPixelFileDecoder class also provides two versions of its `get_event()`
  method:

  ```
  int get_event(std::vector<pixel> * decevt, timing & evt_timing);
  int get_event(std::vector<pixel> * decevt, std::vector<uint16_t> * readback, timing & evt_timing);
  ```
  
  With the tesboard headers being present it is possible to return additional
  information for every event. This is done via the `timing` struct. Note that 
  not all test boards provide all information, so some fields might be left
  empty. The struct contains the following information:

  * `int64_t timestamp`: the timestamp provided by the test board, either in
    clock ticks of the applied clock (PSI boards) or in ticks of 1us (RAL boards)
  * `uint32_t trigger_number`: trigger counter starting from the begin if the run
  * `uint32_t token_number`: counter of tokens sent to the detector since begin
    of the run
  * `char triggers_stagged`: number of stacked triggers while this event was
    read out
  * `char trigger_phase`: trigger phase with respect to clock edge
  * `char data_phase`: data phase with respect to clock edge

<!-- comment -->

  Additionally the CMSPixelFileDecoder provides a function to retrieve the raw
  data blob from the event decoded the last, independend of the success of
  this decoding attempt:

  ```
  std::vector<uint16_t> get_rawdata();
  ```
  
  The function returns a vector of `uint16_t` containing the fill data blob as
  read from the input file. This gives e.g. the possibility to filter out corrupt
  events for a closer analysis. The data is buffered to be fetched until the
  next `get_event()` call, when it is replaced with the new raw data:

  ```
  if(decoder->get_event(evt, time) < DEC_ERROR_EMPTY_EVENT) {
    std::vector<uint16_t> raw = decoder->get_rawdata();
  }
  ```

# Configuration #
  In order to work correctly the decoder needs to be initialized with the 
  correct data type and number of ROCs.

  Currently the following ROC types are supported:

  * ROC_PSI46V2
  * ROC_PSI46XDB
  * ROC_PSI46DIG
  * ROC_PSI46DIG_TRIG
  * ROC_PSI46DIGV2_B
  * ROC_PSI46DIGV2
  * ROC_PSI46DIGV21

<!-- comment -->

  Furthermore the behaviour of the decoder can be influenced using flags. The
  following flags are defined:

  * **FLAG_ALLOW_CORRUPT_ROC_HEADERS** - allows to decode events with malformed ROC
    headers. This can be e.g. digital ROCs with `0x3fc` headers (bit shift) or
    the analog single ROC anomaly when not operated with TBM (sending
    "Black UltraBlack" instead of "UltraBlack Black").
    Do not use unless you know your data and need it desperately
  * **FLAG_HAVETBM** - tells the decoder whether to look for TBM tokens or not. Be
    aware that a wronmg TBBM token setting will most likely screw up
    the decoding since wrong starting positions will be determined.
  * **FLAG_12BITS_PER_WORD** - digital ROCs only - used to set the number of bits per
    word which are actually filled with data. The standard configu-
    ration for the PSI ATB board was 4 bits, so

    ```
    0000 0000 0000 XXXX -> XXXX
    ```

    However, modified firmwares or the PSI DTB are using the space
    available more efficient and pack 12 bits of data into one short:

    ```
    0000 XXXX XXXX XXXX -> XXXX XXXX XXXX
    ```

  * **FLAG_16BITS_PER_WORD** - digital ROCs only - the RAL IPBus readout setup 
    allows to fully use the available data length by filling all 16bits of the
    short. Use this flag to set for single event decoding of IPBus data.
    when using the `CMSPixelFileDecoderRAL` the flag is set automatically.
  * **FLAG_OVERWRITE_ROC_HEADER_POS** - some analog chips seem to be broken in
    the sense that they don't send valid ROC headers or the header is cut of.
    This flags allows to decode that data by fixing an assumed ROC header pos.
  * **FLAG_OLD_RAL_FORMAT** - this enables data decoding of RAL IPBus data 
    taken with the first readout/ Bridge board version used exclusively in 2012 
    beam tests. Do not use unless you know why!
  * **FLAG_NOT_DISCARD_OUTOFORDER_PIXELS** - usually the data stream is checked
    for the readout order of the pixels following the readout token within the
    ROC. This flag allows to disable checking for the right pixel readout order.
  
# Decoding Statistics #
  The `CMSPixelDecoder` collects decoding statistics during the run. These 
  can either be obtained by accessing the statistics struct of the respective
  class or by printing the full statistics summary to screen.
  
  Return values provide additional information about the most recent decoding
  attempt.

  * Statistics for Single Event Decoding
    can be obtained until the object is destructed by calling

    ```       
    $ eventdec->statistics.<variable>
    ```
  
    For a list of available <variable>s see below.

  * Statistics for File Decoding
    can be obtained until the object is destructed by calling

    for the total statistics of the file decoding process:

    ```
    $ filedec->statistics.<variable>
    ```
     
    for the statistics of the most recently decoded event:

    ```
    $ filedec->evt->statistics.<variable>
    ```
     
    For a list of available <variable>s see below.
     
    If the logging level is set to SUMMARY or higher (see below) the destructor
    of the File Decoder class will print the statistics summary.

  * List of statistics variables
    Currently the following values are collected and stored until the repsective
    object is destroyed. Additional statistics can be implemented quite easily.

    * `uint32_t data_blocks`:
      Number of 16bit event data words processed in total. Note that this does
      contain header blocks such as trigger and timestamp numbers but only the
      event data which is delivered to the Single Event Decoder.
    * `uint32_t head_data`:
      Number of detected testboard data markers
    * `uint32_t head_trigger`:
      Number of detected testboard trigger markers
    * `uint32_t evt_valid`:
      Number of valid events (everything allright)
    * `uint32_t evt_empty`:
      Number of empty events (fine, but contained no pixel)
    * `uint32_t evt_invalid`:
      Number of invalid events (something is fishy with this)
      * No ROC headers / wrong number of ROC headers
      * Missing TBM Header or Trailer
    * `uint32_t pixels_valid`:
      Number of correctly decoded pixel hits
      * `map<unsigned int, unsigned int> rocmap_valid`:
        Number of valid pixels on each ROC, ordered by their ID
      * `uint32_t pixels_valid_zeroph`:
        Number of pixels with correctly decoded address but pulse height zero
    * `uint32_t pixels_invalid`:
      Number of pixel hits with invalid address or zero-bit (undecodable)
      Events containing only some invalid pixels are still delivered, only
      return value is set.
      * `map<unsigned int, unsigned int> rocmap_invalid`:
        Number of invalid pixels on each ROC, ordered by their ID
      * `uint32_t pixels_invalid_eor`:
        Number of bad pixels occuring as the last ones in a ROC readout. After
        a bad pixel the next ROC header should appear in order to be counted.

# Error Codes of the get_event Method #
   The following return values are defined to give you even more information
   about the most recent decoding attempt (call of the `get_event()` method):

   * **DEC_ERROR_EMPTY_EVENT** - the event has been empty. If not caring about
                           trigger correlation just discard this event.
   * **DEC_ERROR_INVALID_ROC_HEADER** - the event contained a non-standard ROC
                           header but could be decoded.
   * **DEC_ERROR_INVALID_ADDRESS** - the event contained one or more pixels with
                           undecodable address. Check the event statistics
                           to learn about the absolute numbers.
   * **DEC_ERROR_NO_TBM_HEADER** - even though the configuration tells the data
                           should contain TBM tokens none could be found.
   * **DEC_ERROR_NO_ROC_HEADER** - the current event contained not a single ROC
                           header and has been discarded.
   * **DEC_ERROR_NO_OF_ROCS** - the number of ROC headers found in the event
                           does not correspond to the number set in the
                           initialization. This event has been discarded.
   * **DEC_ERROR_HUGE_EVENT** - Returned in case of an insanely long event, most
                           likely endless readout loop. This event has
                           been discarded.
   * **DEC_ERROR_INVALID_EVENT** - the event contained some other undecodable
                           feature and has been discarded.
   * **DEC_ERROR_NO_MORE_DATA** - the decoder reached the end of file.
   * **DEC_ERROR_INVALID_FILE** - the file pointer provided is invalid.

<!-- comment -->

   The calling procedure can decide what to do with events of a given return
   value. The errors are ordered accoring to their severity. This makes it
   possible to use if statements like

   ```
   $ if(dec->get_event(...) <= DEC_ERROR_NO_MORE_DATA) break;
   ```

   in order to stop processing when there is no more data or the file pointer
   specified is invalid. Of course one can also check for specific return values
   and take appropriate action.

# Logging and Debugging #
  CMSPixelDecoder has a builtin logging feature which can be used to debug the
  data decoding. It provides several logging levels, the output is directed to
  stderr. You can chose from the following detail levels:

  * **QUIET**    - No output at all.
  * **SUMMARY**  - As QUITE but prints the statistics summary when destructed
  * **ERROR**    - Prints all errors encountered during the decoding (invalid events)
  * **WARNING**  - Prints additional information about failed pixel decoding.
  * **INFO**     - Prints information about found hits
  * **DEBUG**    - Informs about the current status of the decoding steps
  * **DEBUG1-4** - additional debugging levels printing even more stuff to stderr.

<!-- comment -->

  The current logging level can be changed at any time using
  
  ```
  $ Log::ReportingLevel() = Log::FromString(<LEVEL>);
  ```

  Be aware of the fact that enabled logging slows down the encoding by quite a
  bit. So only turn it on if you need it!

# Utilities in the Repository #
  * d2h - data2histograms


  * unit_tests - Testing changes in the Decoder
    This small tool performs some tests for decoding data from various sources
    (covering most of the configuration possibilities) and compares the obtained
    statistics again some hardcoded values. An error is issued if any of the
    numbers do not meet the expectations.
     
    ```
    $ ./unit_tests
    ```
     
    This helps to find quirks and bugs introduced in the latest version. Just
    run this test before commiting changes.
     
    Some timing measurements are done but they are not really reliable (yet).
  
  * tentative_dec - command line program to decode files
    A small tool to quickly decode some events of a given file. Invoke with:

    ```
    $ ./tentative_dec LEVEL NUMEVT File[s...]
    ```
     
    where LEVEL is the desired decoder verbosity level (see above), NUMEVT is
    the number of events to decode from every file, and File[s...] are any
    number of data files. Check the correct decoding settings in the source code!

  * stat_tool - collects statistics
    This tool just runs through a number of files, decodes them and collects
    their statistics, adds them up and prints them finally. Useful to run
    over a large set of files. Usage:
     
    ```
    $ ./stat_tool [-v VERBOSITY -o OUTPUTFILE -n NUMROCS] File[s...]
    ```
     
    where VERBOSITY is the desired verbosity level and File[s...] are any number
    of data files. OUTPUTFILE provides the possibility to log all bad events
    into a new binary file with the name provided to have a closer look at them.
    NUMROCS can be used to adjust the number of ROCs in the readout chain on the
    fly. Check the correct decoding settings in the source code!

  * mc_background - simulation for random bit error estimates
    This tool generates random numbers as hit patterns, preceeded by a valid ROC
    header. This can be used to estimate the number of fake hits from randomly
    distributed bit errors in the readout.
