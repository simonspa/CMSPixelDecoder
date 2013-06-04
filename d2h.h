#include <TArrayI.h>
#include <TArrayD.h>
#include <TRandom3.h>
#include <TCanvas.h>
#include <TPostScript.h>
#include <TLine.h>


#include <TMath.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile2D.h>
#include <TProfile.h>
#include <TRandom.h>


#include "CMSPixelDecoder.h"

struct cluster {
  std::vector<CMSPixel::pixel> vpix;
  int size;
  int sumA;//DP
  float charge;
  float col,row;
  int layer;
  double xy[2]; // local coordinates
  double xyz[3];
};

class gain {
 public:
  gain(Char_t *chipdir);
  Double_t GetVcal(Int_t col, Int_t row, Double_t Aout);
 private:
  Double_t ROCPAR[52][80][12];// 52 x 80 pixels each with 12 calibration parameters
};
		
std::vector<cluster> getHits(std::vector<CMSPixel::pixel> * pixbuff);

// book histos:
//                      name   title;xtit;ytit   nbins xmin xmax
TH1D *h000 = new TH1D( "h000", "log words per event;log(event size [words]);triggers", 60, 0, 6 );
TH1D *h001 = new TH1D( "h001", "words per event;event size [words];triggers", 101, -1, 201 );
TH1D *h002 = new TH1D( "h002", "pixels per event;pixels/event;triggers", 100,  0.5,  100.5 );
TH1D *h003 = new TH1D( "h003", "clusters per event;clusters/event;triggers", 100,  0.5, 100.5 );

TH1D *t100  = new TH1D( "t100",  "time;t [s];triggers / s", 100, 0,  100 );
TH1D *t300  = new TH1D( "t300",  "time;t [s];triggers / s", 300, 0,  300 );
TH1D *t600  = new TH1D( "t600",  "time;t [s];triggers / s", 600, 0,  600 );

TH1D *t1200 = new TH1D( "t1000", "time;t [s];triggers / 10 s", 120, 0, 1200 );
TH1D *t2000 = new TH1D( "t2000", "time;t [s];triggers / 10 s", 200, 0, 2000 );
TH1D *t4000 = new TH1D( "t4000", "time;t [s];triggers / 10 s", 400, 0, 4000 );

TH1D *h004 = new TH1D( "h004", "one second;t [s];triggers / 10 ms", 100, 29, 30 );
TH1D *h005 = new TH1D( "h005", "time mod 39;t mod 39 clocks;triggers", 39, -0.5, 38.5 );

TH1D *h006 = new TH1D( "h006", "time between triggers;#Delta t [turns];triggers", 500, 0, 1000 );
TH1D *h007 = new TH1D( "h007", "time between triggers;log_{10}(#Delta t [turns]);triggers", 400, 2, 6 );
TH1D *h008 = new TH1D( "h008", "difftime mod 39;#Delta t mod 39 clocks;triggers", 39, -19.5, 19.5 );
TH1D *h009 = new TH1D( "h009", "pixel per double column;pixels/DC;double columns",  50,  0.5,  50.5 );

TH2D *d600  = new TH2D( "d600",  "dt mod 39 clocks vs time;time [s];dt mod 39 clocks", 140, 0,  700, 39, -19.5, 19.5 );
TH2D *d1200 = new TH2D( "d1200", "dt mod 39 clocks vs time;time [s];dt mod 39 clocks", 130, 0, 1300, 39, -19.5, 19.5 );
TH2D *d2000 = new TH2D( "d2000", "dt mod 39 clocks vs time;time [s];dt mod 39 clocks", 200, 0, 2000, 39, -19.5, 19.5 );
TH2D *d4000 = new TH2D( "d4000", "dt mod 39 clocks vs time;time [s];dt mod 39 clocks", 400, 0, 4000, 39, -19.5, 19.5 );

TProfile *i600  = new TProfile( "i600", "correct dt;time [s];fraction correct dt", 140, 0,  700, -0.5, 1.5 );
TH2D *q600  = new TH2D( "q600",  "pixel charge vs time;time [s];pixel charge [ke]", 140, 0,  700, 100, 0, 20 );
TProfile *p600  = new TProfile( "p600", "pixel charge vs time;time [s];<pixel charge> [ke]", 140, 0,  700, 0, 99 );

TProfile *p10  = new TProfile( "p10", "pixel charge vs time;time [s];<pixel charge> [ke]", 100, 0, 10, 0, 99 );

//TH1D *h011 = new TH1D( "h011", "UB;UB [ADC];events", 500, -400, 100 );
//TH1D *h012 = new TH1D( "h012", "B;B [ADC];events", 200, -100, 100 );
//TH1D *h013 = new TH1D( "h013", "LD;LD [ADC];events", 500, -100, 400 );
//TH1D *h014 = new TH1D( "h014", "address;address;events", 16, -0.5, 15.5 );
TH1D *h015 = new TH1D( "h015", "PH;PH [ADC];pixels", 256, -0.5, 255.5 );
TH1D *h016 = new TH1D( "h016", "pixel charge;pixel charge [ke];pixels", 120, -10, 50 );
TH1D *h216 = new TH1D( "h216", "pixel charge;pixel charge [ke];pixels", 100, 0, 25 );

TProfile2D *h017  = new TProfile2D( "h017", "mean charge/pixel;col;row;<pixel charge> [ke]",52, -0.5, 51.5, 80, -0.5, 79.5, 0, 999 );

TProfile *h018 = new TProfile( "h018", "mean pixel charge/col;col;<pixel charge> [ke]",52, -0.5, 51.5, 0, 999 );

TProfile *h019 = new TProfile( "h019", "mean pixel charge/row;row;<pixel charge> [ke]",80, -0.5, 79.5, 0, 999 );

TH1D *h020 = new TH1D( "h020", "dcol;double column;pixels", 26, -0.5, 25.5 );
TH1D *h021 = new TH1D( "h021", "drow;double row;pixels", 217, -0.5, 216.5 );
TH1D *h022 = new TH1D( "h022", "col;column;pixels", 52, -0.5, 51.5 );
TH1D *h023 = new TH1D( "h023", "row;row;pixels",    80, -0.5, 79.5 );
TH2D *h024 = new TH2D( "h024", "hit map;column;row;events", 52, -0.5, 51.5, 80, -0.5, 79.5 );
TH2D *h025 = new TH2D( "h025", "charge map;column;row;sum charge [ke]", 52, -0.5, 51.5, 80, -0.5, 79.5 );

TH1D *h026 = new TH1D( "h026", "Aout 2-pix clus;2-pix clus Aout [ADC];pixels", 500, -600, 1400 );
TH1D *h027 = new TH1D( "h027", "Aout cal 2-pix clus;2-pix clus Aout [ke];pixels", 500, -10, 90 );

TH1D *h030 = new TH1D( "h030", "cluster size;pixels/cluster;cluster", 21, -0.5, 20.5 );
TH1D *h031 = new TH1D( "h031", "cluster charge;        cluster charge [ke];clusters", 200, 0, 100 );
TH1D *h032 = new TH1D( "h032", "cluster charge;1-pixel cluster charge [ke];clusters", 200, 0, 100 );
TH1D *h033 = new TH1D( "h033", "cluster charge;2-pixel cluster charge [ke];clusters", 200, 0, 100 );
TH1D *h034 = new TH1D( "h034", "cluster charge;3-pixel cluster charge [ke];clusters", 200, 0, 200 );
TH1D *h035 = new TH1D( "h035", "cluster charge;4-pixel cluster charge [ke];clusters", 200, 0, 200 );

TH1D *h036 = new TH1D( "h036", "cluster col;cluster column;clusters", 52, -0.5, 51.5 );
TH1D *h037 = new TH1D( "h037", "cluster row;cluster row;clusters",    80, -0.5, 79.5 );

TProfile2D *h038  = new TProfile2D( "h038", "mean pix/clus;col;row;<pixel/cluster>",52, -0.5, 51.5, 80, -0.5, 79.5, 0, 999 );

TH1D *h039 = new TH1D( "h039", "cluster charge / a;cluster charge / a [ke];clusters", 200, 0, 100 );

TH1D *h040 = new TH1D( "h040", "col/clus;col/cluster;cluster", 11, -0.5, 10.5 );
TH1D *h041 = new TH1D( "h041", "row/clus;row/cluster;cluster", 11, -0.5, 10.5 );
TH1D *h042 = new TH1D( "h042", "2-pix cluster A1;2-pixel cluster A1 [ke];clusters", 200, 0, 100 );
TH1D *h043 = new TH1D( "h043", "2-pix cluster A2;2-pixel cluster A2 [ke];clusters", 200, 0, 100 );
TH1D *h044 = new TH1D( "h044", "eta 2-col cluster;2-col eta;cluster", 100, 0, 1 );
TH1D *h045 = new TH1D( "h045", "eta 2-row cluster;2-row eta;cluster", 100, 0, 1 );
TProfile *h046  = new TProfile( "h046", "eta vs col;col;<eta>",52, -0.5, 51.5, -1, 2 );
TProfile *h047  = new TProfile( "h047", "eta vs row;row;<eta>",80, -0.5, 79.5, -1, 2 );

TH1D *h051 = new TH1D( "h051", "cluster size fiducial;pixels/cluster;fiducial cluster", 21, -0.5, 20.5 );
TH1D *h052 = new TH1D( "h052", "cluster charge fiducial;        cluster charge [ke];fiducial clusters", 200, 0, 100 );
TH1D *h053 = new TH1D( "h053", "cluster charge fiducial;1-pixel cluster charge [ke];fiducial clusters", 200, 0, 100 );
TH1D *h054 = new TH1D( "h054", "cluster charge fiducial;2-pixel cluster charge [ke];fiducial clusters", 200, 0, 100 );
TH1D *h055 = new TH1D( "h055", "cluster charge fiducial;3-pixel cluster charge [ke];fiducial clusters", 200, 0, 200 );
TH1D *h056 = new TH1D( "h056", "cluster charge fiducial;4-pixel cluster charge [ke];fiducial clusters", 200, 0, 200 );

TProfile *h058 = new TProfile( "h058", "mean cluster charge vs col;col;<cluster charge> [ke]",52, -0.5, 51.5, 0, 99 );
TProfile *h059 = new TProfile( "h059", "mean cluster charge / a vs col;col;<cluster charge / a> [ke]",52, -0.5, 51.5, 0, 99 );

TH1D *h060 = new TH1D( "h060", "col/clus;col/cluster;cluster", 11, -0.5, 10.5 );
TH1D *h061 = new TH1D( "h061", "row/clus;row/cluster;cluster", 11, -0.5, 10.5 );
TH1D *h062 = new TH1D( "h062", "2-pix cluster A1;2-pixel cluster A1 [ke];clusters", 200, 0, 100 );
TH1D *h063 = new TH1D( "h063", "2-pix cluster A2;2-pixel cluster A2 [ke];clusters", 200, 0, 100 );
TH1D *h064 = new TH1D( "h064", "eta 2-col cluster;2-col eta;cluster", 100, 0, 1 );
TH1D *h065 = new TH1D( "h065", "eta 2-row cluster;2-row eta;cluster", 100, 0, 1 );
TH1D *h066 = new TH1D( "h066", "A2/Asum 2-row cluster;A2/(A1+A2);cluster", 100, 0, 0.5 );
TH1D *h067 = new TH1D( "h067", "x 2-row cluster;x [um];cluster", 100, 50, 100 );
TH1D *h068 = new TH1D( "h068", "x 2-row cluster;x [um];cluster", 100, 50, 100 );

TProfile2D *h072 = new TProfile2D( "h072", "smallest pixel charge;col;row;smallest pixel charge [ke]",52, -0.5, 51.5, 80, -0.5, 79.5, 0, 10 );
TProfile *h073 = new TProfile( "h073", "smallest pixel charge per col;col;smallest pixel charge [ke]",52, -0.5, 51.5, 0, 99 );
TH1D *h074 = new TH1D( "h074", "smallest charge per pixel;smallest pixel charge [ke];pixels", 100, 0, 10 );

TH1D *h081 = new TH1D( "h081", "ph - ph0;ph - ph0 [ADC];pixel", 101, -50.5, 50.5 );

TProfile *h098 = new TProfile( "h098", "cluster yield;trigger;<events with clusters>", 100, 0, 100000, -1, 9 );

TProfile *c100  = new TProfile( "c100",  "yield vs time;t [s];<events with clusters>/1s", 100, 0, 100, -1, 9 );
TProfile *c300  = new TProfile( "c300",  "yield vs time;t [s];<events with clusters>/3s", 100, 0, 300, -1, 9 );
TProfile *c600  = new TProfile( "c600",  "yield vs time;t [s];<events with clusters>/5s", 140, 0, 700, -1, 9 );
TProfile *c1200 = new TProfile( "c1200", "yield vs time;t [s];<events with clusters>/10s", 130, 0, 1300, -1, 9 );
TProfile *c2000 = new TProfile( "c2000", "yield vs time;t [s];<events with clusters>/10s", 200, 0, 2000, -1, 9 );
TProfile *c4000 = new TProfile( "c4000", "yield vs time;t [s];<events with clusters>/10s", 400, 0, 4000, -1, 9 );

TProfile *y600  = new TProfile( "y600",  "yield vs time;t [s];<events with clusters>/5s", 700*12.5, 0, 700, -1, 9 ); // DESY cycles, drop in all

TH2D *hed[99];
TH2D *hcl[99];

TH1D *hcc[52];
