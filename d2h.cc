// d2h - data2histograms
// Fast statistics and histogramming tool for PSI46 ROC data,
// based on a script by D. Pitzl, 7.8.2011
//
// 
// make d2h

// cd data
// ../offline/d2h -tbm -r 2000 | less [no TBM emu]
// ../offline/d2h      -r 4029 | less [TBM emu]
// ../offline/d2h -a4 -c 202 -r 4029 | less [chip 202]
// ../offline/d2h -m -r 107 | less   module = 16 ROCs

#include "CMSPixelDecoder.h"
#include "d2h.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <math.h>
#include <cmath>
#include <ctype.h>
#include <limits.h>

#include <TMath.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile2D.h>
#include <TProfile.h>
#include <TRandom.h>

#include <vector>

using namespace CMSPixel;

int flags;
int fCluCut = 1; // clustering: 1 = no gap (15.7.2012)
CMSPixelFileDecoder * decoder;


void sig_handler(int s){
  std::cout << "Caught signal " << s << std::endl;
  delete decoder;
  exit(0);
}

int main( int argc, char **argv )
{

  struct sigaction sigIntHandler;

   sigIntHandler.sa_handler = sig_handler;
   sigemptyset(&sigIntHandler.sa_mask);
   sigIntHandler.sa_flags = 0;

   sigaction(SIGINT, &sigIntHandler, NULL);

  int run = 0;
  std::string path;

  int nroc = 1;
  bool fullNameSpecified = 0;

  int chip = 8; // default chip
  std::string gainFileName;
  bool gain_set = false;
  
  std::string fileName;
  std::string histoName;
  std::string address = "addressParameters.dat";

  Log::ReportingLevel() = Log::FromString("SUMMARY");

  bool weib = 0;

  // process input flags:
  for( int i = 1; i < argc; i++ ) {

    // Runnumber, picks the run folder and appends "/mtb.bin"
    if( !strcmp( argv[i], "-r" ) ) {
      run = atoi( argv[++i] );
      char buff[100];
      sprintf(buff, "bt05r%06d", run );
      path = buff;
    }

    // For specifying the full filename, not run number based
    else if( !strcmp( argv[i], "-f" ) ) {
      fullNameSpecified = 1;
      fileName = argv[++i];
    }

    // Switch to 12/16bit data packaging on the testboard
    else if( !strcmp( argv[i], "-a4" ) ) flags |= FLAG_12BITS_PER_WORD;

    // Select Module w/ 16 ROCs
    else if( !strcmp( argv[i], "-m" ) ) nroc = 16;

    // Select number of ROCs
    else if( !strcmp( argv[i], "-nroc" ) ) nroc = atoi(argv[++i]);

    // Switch on TBM usage
    else if( !strcmp( argv[i], "-tbm" ) ) flags |= FLAG_HAVETBM;

    // Allow all (also corrupt) ROC headers:
    else if( !strcmp( argv[i], "-all" ) ) flags |= FLAG_ALLOW_CORRUPT_ROC_HEADERS;

    // Specify decoding flags by hand:
    else if( !strcmp( argv[i], "-flg" ) ) flags |= atoi(argv[++i]);

    // Address Parameters:
    else if( !strcmp( argv[i], "-add" ) ) address = argv[++i];

    // Specify chip number
    else if( !strcmp( argv[i], "-c" ) ) {
      chip = atoi( argv[++i] );
    }
    
    // Specify gain file
    else if( !strcmp( argv[i], "-g" ) ) {
      gainFileName = argv[++i];
      gain_set = true;
    }

    // Set Weibull flag for calibration:
    else if(!strcmp( argv[i], "-w" ) ) {
      weib = true;
    }

    // Set debug level
    else if( !strcmp(argv[i], "-d" ) ) {
      Log::ReportingLevel() = Log::FromString(argv[++i]);      
    }
  }

 std::cout << "nroc = " << nroc << std::endl;

  // input binary file:
  if( !fullNameSpecified ){
    fileName = path + "/mtb.bin";
  }
 std::cout << "fileName: "<< fileName << std::endl;

  // Get the ROC type
  // Naming scheme:
  // 000-099: old analog ROC
  // 100-199: intermediate analog XDB chip
  // 200-299: intermediate digital DIG chip
  // 300-xxx: final DIG V2 chip
  uint8_t roctype;
  if(chip < 100) roctype = ROC_PSI46V2;
  else if(chip < 200) roctype = ROC_PSI46XDB;
  else if(chip < 300) roctype = ROC_PSI46DIG;
  else roctype = ROC_PSI46DIGV2;
 std::cout << "roctype: " << (int)roctype << std::endl;

  decoder = new CMSPixelFileDecoderPSI_ATB(fileName.c_str(), nroc, flags, roctype, address.c_str());

  // gain file:
  double amax[52][80];
  double Gain[52][80];
  double horz[52][80];
  double vert[52][80];
  double expo[52][80];

  double keV = 0.450; // large Vcal -> keV

  if(!gain_set) {
    std::cout << "No gain file specified." << std::endl;
    return(2);
  }

  ifstream gainFile( gainFileName.c_str() );

  if( !gainFile ) {
   std::cout << "Gain file " << gainFileName << " is not accessible." << std::endl;
    return(2);
  }
  else {
    char ih[99];
    int icol;
    int irow;
    double am;
    double ho;
    double ga;
    double vo;
    double ex = 0.0;
    double aa = 1.0;
    if( chip ==   7 ) aa = 0.25; // gainmap taken in TBM mode
    if( chip ==  10 ) aa = 0.25; // gainmap taken in TBM mode
    if( chip ==  11 ) aa = 0.25; // gainmap taken in TBM mode
    if( chip == 102 ) aa = 0.25; // gainmap taken in TBM mode
    if( chip ==  11 ) keV = 0.360; // from Landau peak
    if( chip == 102 ) keV = 0.350; // from Landau peak at 24 keV
    if( chip == 202 ) keV = 0.350; // from Landau peak at 24 keV
    if( chip == 203 ) keV = 0.350; // digital: 50e/small Vcal
    if( chip ==  39 ) keV = 0.350; // digital: 7 = large/small Vcal
    if( chip ==  47 ) keV = 0.350; // copy

   std::cout << std::endl;
   std::cout << "Gain file: " << gainFileName << std::endl;
    if(weib) std::cout << "  Weibull gain calibration." << std::endl;
    else std::cout << " TanH gain calibration." << std::endl;

    while( gainFile >> ih ){
      gainFile >> icol;
      gainFile >> irow;
      if( weib ) {
	gainFile >> ho;//horz offset
	gainFile >> ga;//width
	gainFile >> ex;//exponent
	gainFile >> am;//gain
	gainFile >> vo;//vert offset
      }
      else {
	gainFile >> am;//Amax
	gainFile >> ho;//horz offset
	gainFile >> ga;//gain [ADC/large Vcal]
	gainFile >> vo;//vert offset
      }

      amax[icol][irow] = am*aa;
      Gain[icol][irow] = ga;
      horz[icol][irow] = ho;
      vert[icol][irow] = vo*aa;
      expo[icol][irow] = ex;
    }
   std::cout << std::endl;
  }//gainFile


  // (re-)create root file:
  if(run != 0) histoName = "data" + ZeroPadNumber(run,6) + ".root";
  else histoName = "data-norun.root";
  TFile* histoFile = new TFile( histoName.c_str(), "RECREATE");

  std::cout << "Created ROOT file " << histoName << std::endl;

  // Booking the histograms:
  if(!bookHistograms()) return(2);

   int nw = 0;
  int nd = 0;
  int ng = 0;

  double amin[52][80];
  for( int i = 0; i < 52; ++i )
    for( int j = 0; j < 80; ++j )
      amin[i][j] = 99;

  int ph0[52][80] = {{0}};
  long sumdph[52][80] = {{0}};
  long sumdphsq[52][80] = {{0}};
  int Nhit[52][80] = {{0}};

  unsigned int ndata = 0;
  unsigned int ndataetrig = 0;
  unsigned int ntrig = 0;
  unsigned int nres = 0;
  unsigned int ncal = 0;
  unsigned int nbig = 0;
  unsigned int nwrong = 0;
  unsigned int npixel = 0;
  unsigned int badadd = 0;
  unsigned int ntbmres = 0;
  unsigned int ninfro = 0;
  unsigned int nover = 0;
  unsigned int ncor = 0;
  unsigned int nclus = 0;

  CMSPixel::timing evt_timing;
  unsigned long ttime1 = 0; // 64 bit
  unsigned long oldtime = 0; // 64 bit

  long difftime = 0;
  long utime = 0;
  unsigned long prevtime = 0;
  int invtime = 0;
  int ned = 0;

  // read events from the binary file:
  int status = 0;
  std::vector<CMSPixel::pixel> * decevt;

  while(status > DEC_ERROR_NO_MORE_DATA) {
    decevt = new std::vector<pixel>;
    status = decoder->get_event(decevt, evt_timing);

    // Count all data words:
    nd += decoder->evt->statistics.data_blocks;

    // Infinite readout detected:
    if(status == DEC_ERROR_HUGE_EVENT) nbig++;

    // Invalid address during decoding:
    if(status >= DEC_ERROR_INVALID_ADDRESS) {
      ng += decoder->evt->statistics.data_blocks;
      badadd += decoder->evt->statistics.pixels_invalid;
    }

    h001->Fill(decoder->evt->statistics.data_blocks);
    h000->Fill( log(decoder->evt->statistics.data_blocks)/log(10.0) );
    int pixels = decoder->evt->statistics.pixels_valid
               + decoder->evt->statistics.pixels_invalid; 
    h002->Fill(pixels); 

    // Timing histograms:
    t100->Fill( evt_timing.timestamp/39936E3 );
    t300->Fill( evt_timing.timestamp/39936E3 );
    t600->Fill( evt_timing.timestamp/39936E3 );
    t1200->Fill( evt_timing.timestamp/39936E3 );
    t2000->Fill( evt_timing.timestamp/39936E3 );
    t4000->Fill( evt_timing.timestamp/39936E3 );
    h004->Fill( evt_timing.timestamp/39936E3 );

    difftime = evt_timing.timestamp - prevtime;
    prevtime = evt_timing.timestamp;
    if( decoder->statistics.head_data == 1 ) ttime1 = evt_timing.timestamp;
    utime = evt_timing.timestamp - ttime1;

    if( oldtime > evt_timing.timestamp )
      invtime++; //inverted time?
    oldtime = evt_timing.timestamp;

    h005->Fill( utime % 39 ); // flat
    h006->Fill( difftime/39 ); // 39 clocks/turn
    h007->Fill( log(difftime/39)/log(10.0) );
    int dd = difftime % 39; // 0..38
    if( dd > 19 ) dd = dd-39; // -19..-1
    h008->Fill( dd ); // -19..19
    d600->Fill( utime/39936E3, dd );
    d1200->Fill( utime/39936E3, dd );
    d2000->Fill( utime/39936E3, dd );
    d4000->Fill( utime/39936E3, dd );
    int i0 = 0;
    if( dd == 0 ) i0 = 1;
    if( abs( dd ) < 1.5 ) i600->Fill( utime/39936E3, i0 );

    // Event display:
    bool led = 0;
    if( decevt->size() > 5 && ned < 99 ) { // beam
      led = 1;
      if( ned < 10 ){
	hed[ned] = new TH2D( Form( "e80%i", ned ), "event;col;row;pixel [ADC]",   52, -0.5, 51.5, 80, -0.5, 79.5 );
	hcl[ned] = new TH2D( Form( "e90%i", ned ), "event;col;row;cluster [ADC]", 52, -0.5, 51.5, 80, -0.5, 79.5 );
      }
      else {
	hed[ned] = new TH2D( Form( "e8%i", ned ), "event;col;row;pixel [ADC]",   52, -0.5, 51.5, 80, -0.5, 79.5 );
	hcl[ned] = new TH2D( Form( "e9%i", ned ), "event;col;row;cluster [ADC]", 52, -0.5, 51.5, 80, -0.5, 79.5 );
      }
    }

    int npxdcol[26] = {0};

    // Loop over all pixels found:
    for( std::vector<pixel>::iterator px = decevt->begin(); px != decevt->end(); px++ ){

      // Event display:
      if( led ) hed[ned]->Fill( (*px).col, (*px).row, (*px).raw );

      // Recalculate Dcol and Drow:
      int drow = 2*abs((*px).row - 80) + (*px).row%2;
      int dcol = ((*px).col-drow%2)/2;
				
      h015->Fill( (*px).raw );
      h020->Fill( dcol );
      h021->Fill( drow );
      h022->Fill( (*px).col );
      h023->Fill( (*px).row );
      h024->Fill( (*px).col, (*px).row );

      // Pixels per DCOL:
      npxdcol[dcol]++;

      // Calibrate into Vcal and keV:
      if( Nhit[(*px).col][(*px).row] == 0 ) ph0[(*px).col][(*px).row] = (*px).raw;
      int dph = (*px).raw - ph0[(*px).col][(*px).row];
      h081->Fill( dph );

      sumdph[(*px).col][(*px).row] += dph;
      sumdphsq[(*px).col][(*px).row] += dph*dph;
      Nhit[(*px).col][(*px).row]++;

      double Aout = (*px).raw;
      double Ared = Aout - vert[(*px).col][(*px).row]; //vert off, gaintanh2ps.C
      double ma9 = amax[(*px).col][(*px).row];

      //if( Ared >  ma9-1 ) ma9 = Ared + 2;
      if( Ared >  ma9-1 ) ma9 = 1.000001* Ared;
      if( Ared <  -ma9+1 ) ma9 = Ared - 2;

      // calibrate into ke units:
      if( weib ) 
	(*px).vcal = (pow(-log( 1 - Ared / ma9 ), 1/expo[(*px).col][(*px).row])
		      * Gain[(*px).col][(*px).row] + horz[(*px).col][(*px).row])
	              * keV; // Weibull [ke]
      else
	(*px).vcal = (TMath::ATanH( Ared / ma9 ) *
		      Gain[(*px).col][(*px).row] + horz[(*px).col][(*px).row] ) * keV; // [ke]

      // Calibration across col psi46
      double acor = 1.0 + 0.20 * ( (*px).col - 25.5 ) / 51.0; 
      // Not necessary anymore for everything from PSI46XDB:
      if( chip > 100 ) acor = 1.0;
      double pxq = (*px).vcal / acor;

      h016->Fill( pxq );
      h216->Fill( pxq );
      q600->Fill( utime/39936E3, pxq ); // 2D
      p600->Fill( utime/39936E3, pxq ); // prof
      p10->Fill( utime/39936E3, pxq ); // prof

      h017->Fill( (*px).col, (*px).row, pxq );
      h018->Fill( (*px).col, pxq );
      h019->Fill( (*px).row, pxq );
      h025->Fill( (*px).col, (*px).row, pxq );

      if( pxq < amin[(*px).col][(*px).row] ) amin[(*px).col][(*px).row] = pxq;

      // Fill the 2px cluster plots:
      if(decevt->size() == 2) {
	  h026->Fill( (*px).raw );
	  h027->Fill( (*px).vcal );
      }

    } //  end of run over pixels

    // Fill the number of pixels per dcol:
    for( int idc = 0; idc < 26; ++idc )
      h009->Fill( npxdcol[idc] );

    // Do all the cluster magic counting:
    // find clusters in pb:
    unsigned int kclus = 0;
    std::vector<cluster> clust = getHits(decevt);

    if(ndata%1000 == 0) {
    std::cout << "(clk " << evt_timing.timestamp/39936E3 
	      << "s, trg " << decoder->statistics.head_trigger 
	      << ", px" << decevt->size() 
	      << ", cl " << clust.size() << ")" << std::endl;
    }

    nclus += clust.size();
    kclus = clust.size();

    h003->Fill( clust.size() );

    // look at clusters:
    for( std::vector<cluster>::iterator c = clust.begin(); c != clust.end(); c++ ){
      h030->Fill( c->size );
      h031->Fill( c->charge );

      if( c->col >= 0 ) hcc[int(c->col)]->Fill( c->charge );

      if( c->size == 1 )
	h032->Fill( c->charge );
      else if( c->size == 2 )
	h033->Fill( c->charge );
      else if( c->size == 3 )
	h034->Fill( c->charge );
      else
	h035->Fill( c->charge );

      h036->Fill( c->col );
      h037->Fill( c->row );
      h038->Fill( c->col, c->row, c->size );

      double acor = 1.0 + 0.16 * ( c->col - 25.5 ) / 51.0; // DP calibration across col

      h039->Fill( c->charge / acor );
      h058->Fill( c->col, c->charge );
      h059->Fill( c->col, c->charge / acor );

      int colmin = 99;
      int colmax = -1;
      int rowmin = 99;
      int rowmax = -1;
      double a1 = 0;
      double a2 = 0;

      // Check pixels inside the current cluster:
      for( std::vector<pixel>::iterator px = c->vpix.begin(); px != c->vpix.end(); px++ ){
	if( px->col < colmin ) colmin = px->col;
	if( px->col > colmax ) colmax = px->col;
	if( px->row < rowmin ) rowmin = px->row;
	if( px->row > rowmax ) rowmax = px->row;

	if( fabs(px->vcal) > a1 ) {
	  a2 = a1;
	  a1 = fabs(px->vcal);
	}
	else if( fabs(px->vcal) > a2 ) {
	  a2 = fabs(px->vcal);
	}

      }//pix

      int ncol = colmax - colmin + 1;
      int nrow = rowmax - rowmin + 1;
      double a12 = a1 + a2;
      double eta = ( a1 - a2 ) / a12;

      h040->Fill( ncol );
      h041->Fill( nrow );

      if( c->size == 2 ) {
	h042->Fill(a1);
	h043->Fill(a2);
	if( ncol == 2 ) h044->Fill( eta );
	if( nrow == 2 ) h045->Fill( eta );
	h046->Fill( c->col, eta );
	h047->Fill( c->row, eta );
      }//2-pix clus

      // Mask out edges (fiducial volume):
      if( colmin > 0 && colmax < 51 && rowmin > 0 && rowmax < 79 ){

	h051->Fill( c->size );
	h052->Fill( c->charge );

	if     ( c->size == 1 ) h053->Fill( c->charge );
	else if( c->size == 2 ) h054->Fill( c->charge );
	else if( c->size == 3 ) h055->Fill( c->charge );
	else                    h056->Fill( c->charge );

	h060->Fill( ncol );
	h061->Fill( nrow );

	if( c->size == 2 ) {

	  h062->Fill(a1);
	  h063->Fill(a2);
	  if( ncol == 2 ) h064->Fill( eta );
	  if( nrow == 2 ) h065->Fill( eta );
	  if( nrow == 2 ) h066->Fill( a2 / a12 );
	  if( nrow == 2 ) h067->Fill( 91 +  9 * 2 * a2 / a12 );//guess, for vertical incidence
	  if( nrow == 2 ) h068->Fill( 91 +  9 * 2 * a2 / a12 );//
	  else h068->Fill( 50 + 42*gRandom->Rndm() );//smear to get flat x distribution

	}//2-pix clus
	else h068->Fill( 50 + 42*gRandom->Rndm() );

      }//fiducial

      // Event display:
      if( led ) hcl[ned]->Fill( c->col, c->row, c->charge );

    }//cluster loop

    // Event display:
    if( led ) ned++;

    // Yield plots:
    bool ic = kclus > 0; 
    h098->Fill( ndata, ic );
    c100->Fill( utime/39936E3, ic );
    c300->Fill( utime/39936E3, ic );
    c600->Fill( utime/39936E3, ic );
    y600->Fill( utime/39936E3, ic );
    c1200->Fill( utime/39936E3, ic );
    c2000->Fill( utime/39936E3, ic );
    c4000->Fill( utime/39936E3, ic );

    delete decevt;
    // Increase event number:
    ndata++;
  }

  nw = 0; // words in total FIXME
  ntrig = decoder->statistics.head_trigger;
  nres = 0; //FIXME
  ncal = 0; //FIXME
  ntbmres = 0; // FIXME 0x8020
  ninfro = 0; // FIXME 0x8040
  ndataetrig = 0; //FIXME 0x8003
  nover = 0; // FIXME overflow 0x8080
  npixel = decoder->statistics.pixels_valid + decoder->statistics.pixels_invalid;

  for( int i = 0; i < 52; ++i ) {
    for( int j = 0; j < 80; ++j ) {
      h072->Fill( i, j, amin[i][j] );
      h073->Fill( i, amin[i][j] );
      h074->Fill( amin[i][j] );
    }
  }

  double tsecs = (evt_timing.timestamp-ttime1)/39936E3;
 std::cout << std::endl;
 std::cout << "time:    " << evt_timing.timestamp-ttime1 << " clocks = " << tsecs << " s" << std::endl;
 std::cout << "words:   " << nw << std::endl;
 std::cout << "datas:   " << nd << " words" << " (" << 100.0*nd/nw << "%)" << std::endl;
 std::cout << "good:    " << ng << " words" << " (" << 100.0*ng/std::max(1,nd) << "%)" << std::endl;
  //cout << "nres:    " << nres << std::endl;
  //cout << "ncal:    " << ncal << std::endl;
 std::cout << "ntrig:  " << ntrig << " (" << ntrig/tsecs << " Hz)" << std::endl;
 std::cout << "ndata:   " << ndata << " events" << std::endl;
 std::cout << "nclus:   " << nclus << " (" << 100.0*nclus/std::max((unsigned int)1,ndata) << "%)" << std::endl;
 std::cout << "npix:    " << npixel << " (" << 1.0*npixel/std::max((unsigned int)1,ndata) << "/ev)" << std::endl;
 std::cout << std::endl;
 std::cout << "nwrong:  " << nwrong << std::endl;
 std::cout << "badadd:  " << badadd << " (" << (badadd*100.0)/std::max((unsigned int)1,npixel) << "%)" << std::endl;
 std::cout << "nbig  :  " << nbig << std::endl;
  //cout << "ntbmres: " << ntbmres << std::endl;
  //cout << "ninfro:  " << ninfro << std::endl;
  //cout << "nover:   " << nover << std::endl;
  //cout << "data&trg:" << ndataetrig << std::endl;
  // std::cout << "ncor:    " << ncor << " headers" << std::endl;
 std::cout << "invtime: " << invtime << std::endl;

  std::cout << std::endl << "All histograms stored in " << histoName << std::endl;

  histoFile->Write();
  histoFile->Close();

  decoder->statistics.print();
  delete decoder;
  return(0);
}

std::vector<cluster> getHits(std::vector<CMSPixel::pixel> * pixbuff){

  // returns clusters with local coordinates
  // decodePixels should have been called before to fill pixel buffer pb 
  // simple clusterization
  // cluster search radius fCluCut ( allows fCluCut-1 empty pixels)

  std::vector<cluster> v;
  if( pixbuff->empty() ) return v;

  int* gone = new int[pixbuff->size()];

  for(size_t i=0; i<pixbuff->size(); i++){
    gone[i] = 0;
  }

  size_t seed = 0;
  while( seed < pixbuff->size() ) {
    // start a new cluster
    cluster c;
    c.vpix.push_back(pixbuff->at(seed));
    gone[seed]=1;
    c.charge = 0;
    c.size = 0;
    c.col = 0;
    c.row = 0;

    // let it grow as much as possible:
    int growing;
    do{
      growing = 0;
      for(size_t i = 0; i < pixbuff->size(); i++ ){
        if( !gone[i] ){//unused pixel
          for( unsigned int p = 0; p < c.vpix.size(); p++ ){//vpix in cluster so far
            int dr = c.vpix.at(p).row - pixbuff->at(i).row;
            int dc = c.vpix.at(p).col - pixbuff->at(i).col;
            if( (   dr>=-fCluCut) && (dr<=fCluCut) 
		&& (dc>=-fCluCut) && (dc<=fCluCut) ) {
              c.vpix.push_back(pixbuff->at(i));
	      gone[i] = 1;
              growing = 1;
              break;//important!
            }
          }//loop over vpix
        }//not gone
      }//loop over all pix
    }while( growing);

    // added all I could. determine position and append it to the list of clusters:
    for( std::vector<pixel>::iterator p=c.vpix.begin();  p!=c.vpix.end();  p++){
      double Qpix = p->vcal; // calibrated [keV]
      if( Qpix < 0 ) Qpix = 1; // DP 1.7.2012
      c.charge += Qpix;
      c.col += (*p).col*Qpix;
      c.row += (*p).row*Qpix;
    }

    c.size = c.vpix.size();

    if( ! c.charge == 0 ) {
      c.col = c.col/c.charge;
      c.row = c.row/c.charge;
      c.xy[0] /= c.charge;
      c.xy[1] /= c.charge;
    }
    else {
      c.col = (*c.vpix.begin()).col;
      c.row = (*c.vpix.begin()).row;
     std::cout << "GetHits: cluster with zero charge" << std::endl;
    }

    v.push_back(c);//add cluster to vector

    //look for a new seed = unused pixel:
    while( (++seed<pixbuff->size()) && (gone[seed]) );
  }//while over seeds

  // nothing left,  return clusters
  delete gone;
  return v;
}

gain::gain(Char_t* chipdir)//Constructor
{

//	Read and Fill the 12 parameters
//
//	Strategy:
//	PQ 	: from -0.5 to point Q use tanh
//	QR	:	straight line to point R
//	RS	: from R to 255.5 use tanh
//	
//	Echo 12 values  
//	P0 from RS
//	P1 from RS
//	P2 from RS
//	P3 from RS
//	Vcal from point R
//	P0 from QR : offset
//	P1 from QR : gradient
//	Vcal from point Q
//	P0 from PQ
//	P1 from PQ
//	P2 from PQ
//	P3 from PQ

  TString name(chipdir);

 std::cout << "gain using " << name << std::endl;

  ifstream ROCCAL(name);
  for( Int_t i = 0; i < 52; ++i ) {//col's
    for( Int_t j = 0; j < 80; ++j ) {//row's
      for( Int_t k = 0; k < 12; ++k ) {//par's
	ROCCAL >> ROCPAR[i][j][k];
      }	
    }
  }

}

Double_t gain::GetVcal( Int_t col, Int_t row, Double_t Aout ){

  //Assume its on the range R to 255.5
  Double_t a = ( Aout - ROCPAR[col][row][3] ) / ROCPAR[col][row][2];  

  if( a > 0.999999 )
    a = 0.999999;
  else if( a < -0.999999 )
    a = -0.999999;

  Double_t v = ( TMath::ATanH(a) - ROCPAR[col][row][1] ) / ROCPAR[col][row][0];	

  //check if it's below R:
  if( v < ROCPAR[col][row][4] ) {
    //use linear range QR
    v = ( Aout - ROCPAR[col][row][5] ) / ROCPAR[col][row][6];

    //finally check to see if there is a tanh in low vcal
    if( ROCPAR[col][row][7] > 1.0 && v < ROCPAR[col][row][7] ) {
      //use lower tanh
      a = ( Aout - ROCPAR[col][row][11] ) / ROCPAR[col][row][10];  
      if( a < -0.999999 ) a = -0.999999;
      v = ( TMath::ATanH(a) - ROCPAR[col][row][9] ) / ROCPAR[col][row][8];	
    }
  }
	
  return v;
}

std::string ZeroPadNumber(int num, int len)
{
  std::ostringstream ss;
  ss << std::setw( len ) << std::setfill( '0' ) << num;
  return ss.str();
}

bool bookHistograms() {
  try {
    //                      name   title;xtit;ytit   nbins xmin xmax
    h000 = new TH1D( "h000", "log words per event;log(event size [words]);triggers", 60, 0, 6 );
    h001 = new TH1D( "h001", "words per event;event size [words];triggers", 101, -1, 201 );
    h002 = new TH1D( "h002", "pixels per event;pixels/event;triggers", 100,  0.5,  100.5 );
    h003 = new TH1D( "h003", "clusters per event;clusters/event;triggers", 100,  0.5, 100.5 );

    t100  = new TH1D( "t100",  "time;t [s];triggers / s", 100, 0,  100 );
    t300  = new TH1D( "t300",  "time;t [s];triggers / s", 300, 0,  300 );
    t600  = new TH1D( "t600",  "time;t [s];triggers / s", 600, 0,  600 );
    t1200 = new TH1D( "t1000", "time;t [s];triggers / 10 s", 120, 0, 1200 );
    t2000 = new TH1D( "t2000", "time;t [s];triggers / 10 s", 200, 0, 2000 );
    t4000 = new TH1D( "t4000", "time;t [s];triggers / 10 s", 400, 0, 4000 );

    h004 = new TH1D( "h004", "one second;t [s];triggers / 10 ms", 100, 29, 30 );
    h005 = new TH1D( "h005", "time mod 39;t mod 39 clocks;triggers", 39, -0.5, 38.5 );
    h006 = new TH1D( "h006", "time between triggers;#Delta t [turns];triggers", 500, 0, 1000 );
    h007 = new TH1D( "h007", "time between triggers;log_{10}(#Delta t [turns]);triggers", 400, 2, 6 );
    h008 = new TH1D( "h008", "difftime mod 39;#Delta t mod 39 clocks;triggers", 39, -19.5, 19.5 );
    h009 = new TH1D( "h009", "pixel per double column;pixels/DC;double columns",  50,  0.5,  50.5 );

    d600  = new TH2D( "d600",  "dt mod 39 clocks vs time;time [s];dt mod 39 clocks", 140, 0,  700, 39, -19.5, 19.5 );
    d1200 = new TH2D( "d1200", "dt mod 39 clocks vs time;time [s];dt mod 39 clocks", 130, 0, 1300, 39, -19.5, 19.5 );
    d2000 = new TH2D( "d2000", "dt mod 39 clocks vs time;time [s];dt mod 39 clocks", 200, 0, 2000, 39, -19.5, 19.5 );
    d4000 = new TH2D( "d4000", "dt mod 39 clocks vs time;time [s];dt mod 39 clocks", 400, 0, 4000, 39, -19.5, 19.5 );

    i600  = new TProfile( "i600", "correct dt;time [s];fraction correct dt", 140, 0,  700, -0.5, 1.5 );
    q600  = new TH2D( "q600",  "pixel charge vs time;time [s];pixel charge [ke]", 140, 0,  700, 100, 0, 20 );
    p600  = new TProfile( "p600", "pixel charge vs time;time [s];<pixel charge> [ke]", 140, 0,  700, 0, 99 );
    p10  = new TProfile( "p10", "pixel charge vs time;time [s];<pixel charge> [ke]", 100, 0, 10, 0, 99 );

    //h011 = new TH1D( "h011", "UB;UB [ADC];events", 500, -400, 100 );
    //h012 = new TH1D( "h012", "B;B [ADC];events", 200, -100, 100 );
    //h013 = new TH1D( "h013", "LD;LD [ADC];events", 500, -100, 400 );
    //h014 = new TH1D( "h014", "address;address;events", 16, -0.5, 15.5 );
    h015 = new TH1D( "h015", "PH;PH [ADC];pixels", 256, -0.5, 255.5 );
    h016 = new TH1D( "h016", "pixel charge;pixel charge [ke];pixels", 120, -10, 50 );
    h216 = new TH1D( "h216", "pixel charge;pixel charge [ke];pixels", 100, 0, 25 );

    h017  = new TProfile2D( "h017", "mean charge/pixel;col;row;<pixel charge> [ke]",52, -0.5, 51.5, 80, -0.5, 79.5, 0, 999 );
    h018 = new TProfile( "h018", "mean pixel charge/col;col;<pixel charge> [ke]",52, -0.5, 51.5, 0, 999 );
    h019 = new TProfile( "h019", "mean pixel charge/row;row;<pixel charge> [ke]",80, -0.5, 79.5, 0, 999 );

    h020 = new TH1D( "h020", "dcol;double column;pixels", 26, -0.5, 25.5 );
    h021 = new TH1D( "h021", "drow;double row;pixels", 217, -0.5, 216.5 );
    h022 = new TH1D( "h022", "col;column;pixels", 52, -0.5, 51.5 );
    h023 = new TH1D( "h023", "row;row;pixels",    80, -0.5, 79.5 );
    h024 = new TH2D( "h024", "hit map;column;row;events", 52, -0.5, 51.5, 80, -0.5, 79.5 );
    h025 = new TH2D( "h025", "charge map;column;row;sum charge [ke]", 52, -0.5, 51.5, 80, -0.5, 79.5 );
    h026 = new TH1D( "h026", "Aout 2-pix clus;2-pix clus Aout [ADC];pixels", 500, -600, 1400 );
    h027 = new TH1D( "h027", "Aout cal 2-pix clus;2-pix clus Aout [ke];pixels", 500, -10, 90 );

    h030 = new TH1D( "h030", "cluster size;pixels/cluster;cluster", 21, -0.5, 20.5 );
    h031 = new TH1D( "h031", "cluster charge;        cluster charge [ke];clusters", 200, 0, 100 );
    h032 = new TH1D( "h032", "cluster charge;1-pixel cluster charge [ke];clusters", 200, 0, 100 );
    h033 = new TH1D( "h033", "cluster charge;2-pixel cluster charge [ke];clusters", 200, 0, 100 );
    h034 = new TH1D( "h034", "cluster charge;3-pixel cluster charge [ke];clusters", 200, 0, 200 );
    h035 = new TH1D( "h035", "cluster charge;4-pixel cluster charge [ke];clusters", 200, 0, 200 );

    h036 = new TH1D( "h036", "cluster col;cluster column;clusters", 52, -0.5, 51.5 );
    h037 = new TH1D( "h037", "cluster row;cluster row;clusters",    80, -0.5, 79.5 );

    h038  = new TProfile2D( "h038", "mean pix/clus;col;row;<pixel/cluster>",52, -0.5, 51.5, 80, -0.5, 79.5, 0, 999 );

    h039 = new TH1D( "h039", "cluster charge / a;cluster charge / a [ke];clusters", 200, 0, 100 );

    h040 = new TH1D( "h040", "col/clus;col/cluster;cluster", 11, -0.5, 10.5 );
    h041 = new TH1D( "h041", "row/clus;row/cluster;cluster", 11, -0.5, 10.5 );
    h042 = new TH1D( "h042", "2-pix cluster A1;2-pixel cluster A1 [ke];clusters", 200, 0, 100 );
    h043 = new TH1D( "h043", "2-pix cluster A2;2-pixel cluster A2 [ke];clusters", 200, 0, 100 );
    h044 = new TH1D( "h044", "eta 2-col cluster;2-col eta;cluster", 100, 0, 1 );
    h045 = new TH1D( "h045", "eta 2-row cluster;2-row eta;cluster", 100, 0, 1 );
    h046  = new TProfile( "h046", "eta vs col;col;<eta>",52, -0.5, 51.5, -1, 2 );
    h047  = new TProfile( "h047", "eta vs row;row;<eta>",80, -0.5, 79.5, -1, 2 );

    h051 = new TH1D( "h051", "cluster size fiducial;pixels/cluster;fiducial cluster", 21, -0.5, 20.5 );
    h052 = new TH1D( "h052", "cluster charge fiducial;        cluster charge [ke];fiducial clusters", 200, 0, 100 );
    h053 = new TH1D( "h053", "cluster charge fiducial;1-pixel cluster charge [ke];fiducial clusters", 200, 0, 100 );
    h054 = new TH1D( "h054", "cluster charge fiducial;2-pixel cluster charge [ke];fiducial clusters", 200, 0, 100 );
    h055 = new TH1D( "h055", "cluster charge fiducial;3-pixel cluster charge [ke];fiducial clusters", 200, 0, 200 );
    h056 = new TH1D( "h056", "cluster charge fiducial;4-pixel cluster charge [ke];fiducial clusters", 200, 0, 200 );

    h058 = new TProfile( "h058", "mean cluster charge vs col;col;<cluster charge> [ke]",52, -0.5, 51.5, 0, 99 );
    h059 = new TProfile( "h059", "mean cluster charge / a vs col;col;<cluster charge / a> [ke]",52, -0.5, 51.5, 0, 99 );

    h060 = new TH1D( "h060", "col/clus;col/cluster;cluster", 11, -0.5, 10.5 );
    h061 = new TH1D( "h061", "row/clus;row/cluster;cluster", 11, -0.5, 10.5 );
    h062 = new TH1D( "h062", "2-pix cluster A1;2-pixel cluster A1 [ke];clusters", 200, 0, 100 );
    h063 = new TH1D( "h063", "2-pix cluster A2;2-pixel cluster A2 [ke];clusters", 200, 0, 100 );
    h064 = new TH1D( "h064", "eta 2-col cluster;2-col eta;cluster", 100, 0, 1 );
    h065 = new TH1D( "h065", "eta 2-row cluster;2-row eta;cluster", 100, 0, 1 );
    h066 = new TH1D( "h066", "A2/Asum 2-row cluster;A2/(A1+A2);cluster", 100, 0, 0.5 );
    h067 = new TH1D( "h067", "x 2-row cluster;x [um];cluster", 100, 50, 100 );
    h068 = new TH1D( "h068", "x 2-row cluster;x [um];cluster", 100, 50, 100 );


    h072 = new TProfile2D( "h072", "smallest pixel charge;col;row;smallest pixel charge [ke]",52, -0.5, 51.5, 80, -0.5, 79.5, 0, 10 );
    h073 = new TProfile( "h073", "smallest pixel charge per col;col;smallest pixel charge [ke]",52, -0.5, 51.5, 0, 99 );
    h074 = new TH1D( "h074", "smallest charge per pixel;smallest pixel charge [ke];pixels", 100, 0, 10 );
    
    h081 = new TH1D( "h081", "ph - ph0;ph - ph0 [ADC];pixel", 101, -50.5, 50.5 );
    
    h098 = new TProfile( "h098", "cluster yield;trigger;<events with clusters>", 100, 0, 100000, -1, 9 );
    
    c100  = new TProfile( "c100",  "yield vs time;t [s];<events with clusters>/1s", 100, 0, 100, -1, 9 );
    c300  = new TProfile( "c300",  "yield vs time;t [s];<events with clusters>/3s", 100, 0, 300, -1, 9 );
    c600  = new TProfile( "c600",  "yield vs time;t [s];<events with clusters>/5s", 140, 0, 700, -1, 9 );
    c1200 = new TProfile( "c1200", "yield vs time;t [s];<events with clusters>/10s", 130, 0, 1300, -1, 9 );
    c2000 = new TProfile( "c2000", "yield vs time;t [s];<events with clusters>/10s", 200, 0, 2000, -1, 9 );
    c4000 = new TProfile( "c4000", "yield vs time;t [s];<events with clusters>/10s", 400, 0, 4000, -1, 9 );
    
    y600  = new TProfile( "y600",  "yield vs time;t [s];<events with clusters>/5s", 700*12.5, 0, 700, -1, 9 ); // DESY cycles, drop in all

    // Book some additional ROOT histograms: h100-h151
    for( int i = 0 ; i < 52 ; i++)  {
      hcc[i] = new TH1D( Form( "h1%02i", i ), Form( "cluster charge for col %i  ; cluster charge [ke] ; entries", i), 100, 0, 100 );
    }

    std::cout << "Booked histograms." << std::endl;
    return true;
  }
  catch(...) {
    std::cout << "Unable to book histograms." << std::endl;
    return false;
  }
}
