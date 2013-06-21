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
std::string ZeroPadNumber(int num, int len);
bool bookHistograms();

TH1D *h000, *h001, *h002, *h003, *h004, *h005, *h006, *h007, *h008, *h009;
//TH1D *h011, *h012, *h013, *h014;
TH1D *h015, *h016;

TH1D *t100, *t300, *t600, *t1200, *t2000, *t4000;
TH2D *d600, *d1200, *d2000, *d4000;

TProfile *i600;
TH2D *q600;
TProfile *p600;
TProfile *p10;

TH1D *h216;

TProfile2D *h017;
TProfile *h018, *h019;

TH1D *h020, *h021, *h022, *h023, *h026, *h027;
TH2D *h024, *h025;
TH1D *h030, *h031, *h032, *h033, *h034, *h035, *h036, *h037;
TProfile2D *h038;

TH1D *h039;

TH1D *h040, *h041, *h042, *h043, *h044, *h045;
TProfile *h046;
TProfile *h047;

TH1D *h051, *h052, *h053, *h054, *h055, *h056;
TProfile *h058, *h059;

TH1D *h060, *h061, *h062, *h063, *h064, *h065, *h066, *h067, *h068;

TProfile2D *h072;
TProfile *h073;
TH1D *h074;

TH1D *h081;

TProfile *h098;

TProfile *c100, *c300, *c600, *c1200, *c2000, *c4000;

TProfile *y600;
TH2D *hed[99];
TH2D *hcl[99];

TH1D *hcc[52];
