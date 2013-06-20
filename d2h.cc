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

using namespace std;
using namespace CMSPixel;

int flags;

int fCluCut = 1; // clustering: 1 = no gap (15.7.2012)

int main( int argc, char **argv )
{
  int run = 0;
  string path;

  int nroc = 1;
  bool fullNameSpecified = 0;

  int chip = 8; // default chip
  string gainFileName;
  bool gain_set = false;
  
  string fileName;
  string histoName;

  Log::ReportingLevel() = Log::FromString("SUMMARY");

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

    // Switch on TBM usage
    else if( !strcmp( argv[i], "-tbm" ) ) flags |= FLAG_HAVETBM;

    // Specify chip number
    else if( !strcmp( argv[i], "-c" ) ) {
      chip = atoi( argv[++i] );
    }
    
    // Specify gain file
    else if( !strcmp( argv[i], "-g" ) ) {
      gainFileName = argv[++i];
      gain_set = true;
    }

    // Set debug level
    else if( !strcmp(argv[i], "-d" ) ) {
      Log::ReportingLevel() = Log::FromString(argv[++i]);      
    }
  }

  cout << "nroc = " << nroc << endl;

  // input binary file:
  if( !fullNameSpecified ){
    fileName = path + "/mtb.bin";
  }
  cout << "fileName: "<< fileName << endl;

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
  cout << "roctype: " << (int)roctype << endl;

  CMSPixelFileDecoder * decoder;
  decoder = new CMSPixelFileDecoderPSI_ATB(fileName.c_str(), nroc, flags, roctype, "addressParameters.dat");

  // gain file:
  double amax[52][80];
  double Gain[52][80];
  double horz[52][80];
  double vert[52][80];
  double expo[52][80];

  bool weib = 0;
  double keV = 0.450; // large Vcal -> keV

  if(!gain_set) {
    if(      chip ==  6 ) gainFileName = "/home/pitzl/psi/slc5/psi46expert/chip6/gaintanh_C6.dat";
    else if( chip ==  7 ) gainFileName = "/home/pitzl/psi/slc5/psi46expert/chip7/gaintanh_C7_tbm.dat";
    else if( chip == 10 )
      gainFileName = "/home/pitzl/psi/slc5/psi46expert/chip10-tb/gaintanh-chip10-ia24-board9-bias150.dat"; // Jun 2012
    else if( chip == 11 )
      gainFileName = "/home/pitzl/psi/slc5/psi46expert/chip11/gaintanh-chip11-board9-tbm-Jun.dat";
    else if( chip == 14 ) gainFileName = "/home/pitzl/psi/slc5/psi46expert/chip14/gaintanh.dat";
    else if( chip == 8 ) gainFileName = "gaintanh_C8.dat";
    else if( chip == 102 ) {
      gainFileName = "/home/pitzl/psi/slc5/psi46expert/xdb2/gainweib-xdb2-ia35c-trim30-bias150-Friday.dat";
      weib = 1;
    }
    else if( chip == 202 ){ // digital
      gainFileName = "/home/pitzl/psi/digi/chip202/gainweib-Ia40-Vdig14-trim18.dat"; // lab
      weib = 1;
    }
    else if( chip == 203 ) { // digital
      gainFileName = "/home/pitzl/psi/digi/chip203/gaindigi-chip203-Ia30-Vdig14-trim35.dat";
      weib = 1;
    }
    else if( chip == 204 ) { // digital
      gainFileName = "/home/pitzl/psi/digi/chip204/gaindigi.dat";
      weib = 1;
    }
    else if( chip == 205 ) { // digital
      gainFileName = "gaindigi-Vdig15-Ia25-trim28-Mar4.dat";
      weib = 1;
    }
    else if( chip == 39 ) { // digital
      gainFileName = "/home/pitzl/psi/digi/chip39/gaindigi-Ia25-Vdig12-trim2424.dat";
      weib = 1;
    }
    else if( chip == 47 ) { // digital
      gainFileName = "/home/pitzl/psi/digi/chip47-Vdig15/gaindigi-Vdig15-Ia25-trim30.dat";
      weib = 1;
    }
    else if( chip == 70 ) { // digital
      gainFileName = "gainweib.dat";
      weib = 1;
    }
    else
      gainFileName = "gaintanh.dat";
  }

  ifstream gainFile( gainFileName.c_str() );

  if( !gainFile ) {
    cout << "no gain file " << gainFileName << endl;
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

    cout << endl;
    cout << "gain: " << gainFileName << endl;
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
    cout << endl;
  }//gainFile


  // (re-)create root file:
  histoName = printf("data%06d.root", run );
  TFile* histoFile = new TFile( histoName.c_str(), "RECREATE");

  // Book some additional ROOT histograms:
  for( int i = 0 ; i < 52 ; i++)  {
    hcc[i] = new TH1D( Form( "h1%02i", i ), Form( "cluster charge for col %i  ; cluster charge [ke] ; entries", i), 100, 0, 100 ); //h100-h151
  }

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

  int64_t time = 0; // 64 bit
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
    status = decoder->get_event(decevt, time);

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
    t100->Fill( time/39936E3 );
    t300->Fill( time/39936E3 );
    t600->Fill( time/39936E3 );
    t1200->Fill( time/39936E3 );
    t2000->Fill( time/39936E3 );
    t4000->Fill( time/39936E3 );
    h004->Fill( time/39936E3 );

    difftime = time - prevtime;
    prevtime = time;
    if( decoder->statistics.head_data == 1 ) ttime1 = time;
    utime = time - ttime1;

    if( oldtime > time )
      invtime++; //inverted time?
    oldtime = time;

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
    for( vector<pixel>::iterator px = decevt->begin(); px != decevt->end(); px++ ){

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

      //if( led )
      if( led ) cout << "gain: " << (*px).col << "  " << (*px).row
		     << "  " << (*px).raw << "  " << (*px).vcal << endl;

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

    cout << "(clk " << time/39936E3 
	 << "s, trg " << decoder->statistics.head_trigger 
	 << ", px" << decevt->size() 
	 << ", cl " << clust.size() << ")" << endl;

    nclus += clust.size();
    kclus = clust.size();

    h003->Fill( clust.size() );

    // look at clusters:
    for( vector<cluster>::iterator c = clust.begin(); c != clust.end(); c++ ){
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
      for( vector<pixel>::iterator px = c->vpix.begin(); px != c->vpix.end(); px++ ){
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

  double tsecs = (time-ttime1)/39936E3;
  cout << endl;
  cout << "time:    " << time-ttime1 << " clocks = " << tsecs << " s" << endl;
  cout << "words:   " << nw << endl;
  cout << "datas:   " << nd << " words" << " (" << 100.0*nd/nw << "%)" << endl;
  cout << "good:    " << ng << " words" << " (" << 100.0*ng/max(1,nd) << "%)" << endl;
  //cout << "nres:    " << nres << endl;
  //cout << "ncal:    " << ncal << endl;
  cout << "ntrig:  " << ntrig << " (" << ntrig/tsecs << " Hz)" << endl;
  cout << "ndata:   " << ndata << " events" << endl;
  cout << "nclus:   " << nclus << " (" << 100.0*nclus/max((unsigned int)1,ndata) << "%)" << endl;
  cout << "npix:    " << npixel << " (" << 1.0*npixel/max((unsigned int)1,ndata) << "/ev)" << endl;
  cout << endl;
  cout << "nwrong:  " << nwrong << endl;
  cout << "badadd:  " << badadd << " (" << (badadd*100.0)/max((unsigned int)1,npixel) << "%)" << endl;
  cout << "nbig  :  " << nbig << endl;
  //cout << "ntbmres: " << ntbmres << endl;
  //cout << "ninfro:  " << ninfro << endl;
  //cout << "nover:   " << nover << endl;
  //cout << "data&trg:" << ndataetrig << endl;
  //  cout << "ncor:    " << ncor << " headers" << endl;
  cout << "invtime: " << invtime << endl;

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
    for( vector<pixel>::iterator p=c.vpix.begin();  p!=c.vpix.end();  p++){
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
      cout << "GetHits: cluster with zero charge" << endl;
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

  cout << "gain using " << name << endl;

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

