#include "Ctracker.h"
using namespace cv;
using namespace std;

size_t CTrack::NextTrackID=0;

CTrack::CTrack(Point2f pt, float dt, float Accel_noise_mag, FILE *statfileid,  FILE *trackfileid,float starttime)
{
	track_id=NextTrackID;

	NextTrackID++;
	// Каждый трек имеет свой фильтр Кальмана,
	// при помощи которого делается прогноз, где должна быть следующая точка.
	KF = new TKalmanFilter(pt,dt,Accel_noise_mag);
	// Здесь хранятся координаты точки, в которой трек прогнозирует следующее наблюдение (детект).
	prediction=pt;
	skipped_frames=0;

	_dt = dt;
	_statfileid = statfileid;
	_trackfileid = trackfileid;
	_starttime = starttime;
	valid = false;
}

// ---------------------------------------------------------------------------
double CTrack::get_filterred_distance()
{
	Point2d diff;
	double steps = 0.0;

	for (int i=1; i<trace.size(); i++)
	{
		diff = trace[i] - trace[i-1];
		steps += sqrtf(diff.x*diff.x+diff.y*diff.y); 
	}

	return steps;
}

// ---------------------------------------------------------------------------
double CTrack::get_real_distance()
{
	Point2d diff;
	double steps = 0.0;

	for (int i=1; i<realtrace.size(); i++)
	{
		diff = realtrace[i] - realtrace[i-1];
		steps += sqrtf(diff.x*diff.x+diff.y*diff.y); 
	}

	return steps;
}

// ---------------------------------------------------------------------------
double CTrack::get_endtoend_distance()
{
	Point2d diff = trace[0] - trace[trace.size()-1];
	return sqrtf(diff.x*diff.x+diff.y*diff.y); 
}

// ---------------------------------------------------------------------------
double CTrack::get_sluggingamplitude()
{
	Point2d diff;
	double steps = 0.0;

	for (int i=0; i<realtrace.size(); i++)
	{
		diff = trace[i] - realtrace[i];
		steps += sqrtf(diff.x*diff.x+diff.y*diff.y); 
	}
	steps /= realtrace.size();

	return steps;
}


// ---------------------------------------------------------------------------
/* 
# 1 Track ID
# 2 Start time of the sprem track in seconds
# 3 The sperm of this row followed how long in seconds
# 4 Average path length in microns
# 5 Curvilinear or real path including slugging in micron
# 6 VAP Average path velocity (micron/sec)
# 7 VCL Curvilinear velocity  (micron/sec)
# 8 VSL Straight-line velocity (micron/sec)
# 9 ALH Amplitude of lateral head displacement (micron)
# 10 LIN Linearity  (percent)
# 11 End-to-end distance (micron)
#
# description:
# Curvilinear velocity: Curvilinear velocity (VCL) is the measure of the rate of travel of the centroid of the sperm head over a given time period.
# Average path velocity: Average path velocity(VAP) is the velocity along the average path of the spermatozoon.
# Straight-line velocity: Straight-line velocity (VSL) is the linear or progressive velocity of the cell measured from start to end point for a track.
# Linearity: Linearity of forward progression (LIN) is the ratio of VSL to VCL and is expressed as percentage.
# Amplitude of lateral head displacement: Amplitude of lateral head displacement (ALH) of the sperm head is calculated from the amplitude of its lateral deviation about the cell's axis of progression or average path.
*/
// ---------------------------------------------------------------------------
CTrack::~CTrack()
{
	// on destroy, save the statistics!
	if (_statfileid != NULL)
	{
		if (trace.size() > 1/_dt)
			fprintf(_statfileid, "%d %.2lf %.2lf %.2lf %.2lf %.2lf %.2lf %.2lf %.2lf %.2lf %.2lf\n", 
				track_id,
				_starttime,
				trace.size() * _dt, 
				get_filterred_distance(), 
				get_real_distance(), 
				get_filterred_speed(), 
				get_real_speed(), 
				get_endtoend_distance() / (trace.size() * _dt + 1e-9),
				get_sluggingamplitude(),
				get_endtoend_distance() / (trace.size() * _dt) / (get_real_speed() + 1e-9) * 100,
				get_endtoend_distance() 
				);
	}

	// on destroy, save the statistics!
	if (_trackfileid != NULL)
	{
		for(int i=0; i<trace.size(); i++)
		{
			fprintf(_trackfileid, "%d %.2lf %.2lf\n", track_id, trace[i].x, trace[i].y);
		}
	}

	delete KF;
}

// ---------------------------------------------------------------------------
// Трекер. Производит управление треками. Создает, удаляет, уточняет.
// ---------------------------------------------------------------------------
CTracker::CTracker(float _dt, float _Accel_noise_mag, FILE *statfileid,  FILE *trackfileid, double _dist_thres, int _maximum_allowed_skipped_frames,int _max_trace_length)
{
	dt=_dt;
	Accel_noise_mag=_Accel_noise_mag;
	dist_thres=_dist_thres;
	maximum_allowed_skipped_frames=_maximum_allowed_skipped_frames;
	max_trace_length=_max_trace_length;
	_statfileid = statfileid;
	_trackfileid = trackfileid;
}

void CTracker::Update(vector<Point2d>& detections, float starttime)
{
	// -----------------------------------
	// Если треков еще нет, то начнем для каждой точки по треку
	// -----------------------------------
	if(tracks.size()==0)
	{
		// Если еще нет ни одного трека
		for(int i=0;i<detections.size();i++)
		{
			CTrack* tr=new CTrack(detections[i],dt,Accel_noise_mag, _statfileid, _trackfileid, starttime);
			tracks.push_back(tr);
		}	
	}

	// -----------------------------------
	// Здесь треки уже есть в любом случае
	// -----------------------------------
	int N=tracks.size();		// треки
	int M=detections.size();	// детекты

	// Матрица расстояний от N-ного трека до M-ного детекта.
	vector< vector<double> > Cost(N,vector<double>(M));
	vector<int> assignment; // назначения

	// -----------------------------------
	// Треки уже есть, составим матрицу расстояний
	// -----------------------------------
	double dist;
	for(int i=0;i<tracks.size();i++)
	{	
		// Point2d prediction=tracks[i]->prediction;
		// cout << prediction << endl;
		for(int j=0;j<detections.size();j++)
		{
			Point2d diff=(tracks[i]->prediction-detections[j]);
			dist=sqrtf(diff.x*diff.x+diff.y*diff.y);
			Cost[i][j]=dist;
		}
	}
	// -----------------------------------
	// Решаем задачу о назначениях (треки и прогнозы фильтра)
	// -----------------------------------
	AssignmentProblemSolver APS;
	APS.Solve(Cost,assignment,AssignmentProblemSolver::optimal);

	// -----------------------------------
	// почистим assignment от пар с большим расстоянием
	// -----------------------------------
	// Не назначенные треки
	vector<int> not_assigned_tracks;

	for(int i=0;i<assignment.size();i++)
	{
		if(assignment[i]!=-1)
		{
			if(Cost[i][assignment[i]]>dist_thres)
			{
				assignment[i]=-1;
				// Отмечаем неназначенные треки, и увеличиваем счетчик пропущеных кадров,
				// когда количество пропущенных кадров превысит пороговое значение, трек стирается.
				not_assigned_tracks.push_back(i);
			}
		}
		else
		{			
			// Если треку не назначен детект, то увеличиваем счетчик пропущеных кадров.
			tracks[i]->skipped_frames++;
		}

	}


	// -----------------------------------
	// Если трек долго не получает детектов, удаляем
	// -----------------------------------
	for(int i=0;i<tracks.size();i++)
	{
		if(tracks[i]->skipped_frames>maximum_allowed_skipped_frames)
		{
			delete tracks[i];
			tracks.erase(tracks.begin()+i);
			assignment.erase(assignment.begin()+i);
			i--;
		}
	}
	// -----------------------------------
	// Выявляем неназначенные детекты
	// -----------------------------------
	vector<int> not_assigned_detections;
	vector<int>::iterator it;
	for(int i=0;i<detections.size();i++)
	{
		it=find(assignment.begin(), assignment.end(), i);
		if(it==assignment.end())
		{
			not_assigned_detections.push_back(i);
		}
	}

	// -----------------------------------
	// и начинаем для них новые треки
	// -----------------------------------
	if(not_assigned_detections.size()!=0)
	{
		for(int i=0;i<not_assigned_detections.size();i++)
		{
			CTrack* tr=new CTrack(detections[not_assigned_detections[i]],dt,Accel_noise_mag, _statfileid,  _trackfileid, starttime);
			tracks.push_back(tr);
		}	
	}

	// Апдейтим состояние фильтров

	for(int i=0;i<assignment.size();i++)
	{
		// Если трек апдейтился меньше одного раза, то состояние фильтра некорректно.
		tracks[i]->KF->GetPrediction();

		if(assignment[i]!=-1) // Если назначение есть то апдейтим по нему
		{
			tracks[i]->skipped_frames=0;
			tracks[i]->prediction=tracks[i]->KF->Update(detections[assignment[i]],1);
		}
		else				  // Если нет, то продолжаем прогнозировать
		{
			tracks[i]->prediction=tracks[i]->KF->Update(Point2f(0,0),0);	
		}

		/*
		if(tracks[i]->trace.size()>max_trace_length)
		{
			tracks[i]->trace.erase(tracks[i]->trace.begin(),tracks[i]->trace.end()-max_trace_length);
		}
		*/
		
		tracks[i]->trace.push_back(tracks[i]->prediction);

		// store real path or the prediction if missed position
		if(assignment[i]!=-1)
		{
			tracks[i]->realtrace.push_back(detections[assignment[i]]);
		}
		else
		{
			tracks[i]->realtrace.push_back(tracks[i]->prediction);
		}

		tracks[i]->KF->LastResult=tracks[i]->prediction;
	}

}
// ---------------------------------------------------------------------------
void CTracker::Restart(void)
{
	for(int i=0;i<tracks.size();i++)
	{
	delete tracks[i];
	}
	tracks.clear();
}

// ---------------------------------------------------------------------------
//
// ---------------------------------------------------------------------------
CTracker::~CTracker(void)
{
	for(int i=0;i<tracks.size();i++)
	{
	delete tracks[i];
	}
	tracks.clear();
}
