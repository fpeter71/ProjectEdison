#ifndef __SPERMDETECTORS_H__
#define __SPERMDETECTORS_H__

#include <vector>
#include <opencv2/opencv.hpp>
#include "BlobResult.h"
#include "ReadConfig.h"
#include "Ctracker.h"
#include "capread.h"
#include "blob.h"

#ifndef _WIN64
#include "interface.h"
#else
namespace etm {
class Interface {
public:
	void calc_screen_update(double percent) {;}
	} ;
};
#endif

using namespace cv;
using namespace std;
using namespace etm;

#ifdef _WIN64

typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;

#endif

class SpermDetectors{
private:
	ReadConfig *_config;

public:
	SpermDetectors(ReadConfig &config);
	~SpermDetectors(void);

	void Quick(Mat &frame, vector<Point2d> &centers);
	void Dense(Mat &frame, vector<Point2d> &centers);
	};

// statistics
class Statistics {
	int		_samplenum;
	int		_gradesamplenum;

	double Concentration;
	int		SpermCount;
	ReadConfig *_config;

public:
	double gradeA;
	double gradeB;
	double gradeC;
	double gradeD;

	void UpdateGrages(vector<CTrack*> tracks, double concentration_multiplier);

	double GetConcentration() { if (_samplenum==0) return 0; else return Concentration/_samplenum; }
	double GetSpermCount() { if (_samplenum==0) return 0; else return SpermCount/_samplenum; }

	double GetGradeA() { return (_gradesamplenum==0) ? -1 : floor(1000*gradeA/_gradesamplenum + 0.5)/10; };
	double GetGradeB() { return (_gradesamplenum==0) ? -1 : floor(1000*gradeB/_gradesamplenum + 0.5)/10; };
	double GetGradeC() { return (_gradesamplenum==0) ? -1 : floor(1000*gradeC/_gradesamplenum + 0.5)/10; };
	double GetGradeD() { return (_gradesamplenum==0) ? -1 : floor(1000*gradeD/_gradesamplenum + 0.5)/10; };

	Statistics();
	Statistics(ReadConfig &config);
};


// a produkcio
// filename a *.cap
// settings config file neve
// detectionmethod 0, 1 human vagy bika sperma
// howmanyquarters hany kepdarab, 1,2,4 
// analsistime, 5,10,30 measuretime
// char *statisticsfilename, char *trackfilename, char *snapshotfilename, -> "", "", ""
// CABCD a kimeneti double[5], lasd results_filled.jpg
int AnalyseVideo(char *filename, Interface &etinterf, char *settings, int detectionmethod, int howmanyquarters,  int analysistime, char *statisticsfilename, char *trackfilename, char *snapshotfilename, double *CABCD);

// detectionmethod = -1, akkor kell a settings file neve, 0, 1 human vagy bika sperma
// inputimage 800x600 8bit-C2
// width height, mekkora legyen a vege
// settings file nev, MFL_parameter.ini
// return MAT, RGB a width, highet mereteben
Mat ColorSperms(int detectionmethod, Mat inputimage, int width, int height, char *settings);

unsigned int CountSperms(char *filename, char *settings, int detectionmethod);
int CountSpermsMat(int detectionmethod, Mat inputimage, SpermDetectors &detector);

#endif
