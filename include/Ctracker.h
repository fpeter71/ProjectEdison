#ifndef __CTRACKER_H__
#define __CTRACKER_H__

#include "Kalman.h"
#include "HungarianAlg.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
using namespace cv;
using namespace std;

class CTrack
{
	FILE *_statfileid;
	FILE *_trackfileid;

	double _dt;
	float _starttime;

public:
	vector<Point2d> trace;
	vector<Point2d> realtrace;

	bool valid;

	static size_t NextTrackID;
	size_t track_id;
	size_t skipped_frames; 
	Point2d prediction;
	TKalmanFilter* KF;

	// statistics
	double get_filterred_distance();
	double get_real_distance();
	double get_filterred_speed() { return get_filterred_distance() / trace.size() / _dt; } ;
	double get_real_speed() { return get_real_distance() / realtrace.size() / _dt; } ;
	double get_sluggingamplitude();
	
	double get_endtoend_distance();
	
	CTrack(Point2f p, float dt, float Accel_noise_mag, FILE *statfileid, FILE *trackfileid, float starttime);
	~CTrack();
};

class CTracker
{
	FILE *_statfileid;
	FILE *_trackfileid;

public:
	
	// Ўаг времени опроса фильтра
	float dt; 

	float Accel_noise_mag;

	// ѕорог рассто€ни€. ≈сли точки наход€тс€ дуг от друга на рассто€нии,
	// превышающем этот порог, то эта пара не рассматриваетс€ в задаче о назначени€х.
	double dist_thres;
	// ћаксимальное количество кадров которое трек сохран€етс€ не получа€ данных о измерений.
	int maximum_allowed_skipped_frames;
	// ћаксимальна€ длина следа
	int max_trace_length;

	vector<CTrack*> tracks;
	void Update(vector<Point2d>& detections, float starttime);
	void Restart();

	CTracker(float _dt, float _Accel_noise_mag, FILE *statfileid, FILE *trackfileid, double _dist_thres=60, int _maximum_allowed_skipped_frames=10,int _max_trace_length=10);
	~CTracker(void);
};



#endif
