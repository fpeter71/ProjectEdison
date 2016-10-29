#ifdef _WIN64

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <windows.h>

#include "opencv2/opencv.hpp"
#include "SpermDetectors.h"

using namespace std;

#define SHOWIT

// detectionmethod if -1, it comes from the settings file
int AnalyseVideoPC(const char *filename, char *settings, int detectionmethod, int howmanyquarters,  int analysistime, char *statisticsfilename, char *trackfilename, char *snapshotfilename, double *CABCD)
{
//	FILE *fuck;
//	fuck = fopen("fuck.dat", "w");

	char nonconstfilename[256];
	strcpy(nonconstfilename, filename);

	ReadConfig config(settings);
	double MicronPerPixel = config.GetParam("MicronPerPixel", 1);
	double DeltaTime = config.GetParam("DeltaTime", 0.042);
	double ChannelHeight = config.GetParam("ChannelHeight", 20);
	int SubWindowNum;
	int TrackDataStored;

	// method selection if -1, read from config
	if (detectionmethod == -1)
		detectionmethod = (int)config.GetParam("DetectionMethod", 0);
	
	if (analysistime == -1)
		analysistime = (int)config.GetParam("AnalysisTime", 10);
	if ((analysistime>60)||(analysistime<1))
		analysistime = 30;

	// howmanyquarters selection if -1, read from config
	if (howmanyquarters == -1)
		SubWindowNum = (int)config.GetParam("ProcessedSubWindows", 1);
	if( (SubWindowNum != 1) && (SubWindowNum != 2) && (SubWindowNum != 4))
		SubWindowNum = 1;

	TrackDataStored = (int)config.GetParam("TrackDataStored", 0);

	// input video open
	CapRead capture;
	capture.openfile(nonconstfilename);
  
	if(!capture.is_open())
	{
		cerr << "Problem opening video source" << endl;
		return -1;
	}
	
	FILE *statfile = NULL;
	FILE *trackfile = NULL;
	if (TrackDataStored != 0)
	{
		statfile = fopen(statisticsfilename, "w");
		if (statfile == NULL)
		{
			cerr << "Statistics file is not opened." << endl;
		}
		trackfile = fopen(trackfilename, "w");
		if (trackfile == NULL)
		{
			cerr << "Trackfile is not opened." << endl;
		}
	}

	// let it show
	Mat rgbframe;	
	Scalar Colors[]={Scalar(255,0,0),Scalar(0,255,0),Scalar(0,0,255),Scalar(255,255,0),Scalar(0,255,255),Scalar(255,0,255),Scalar(255,127,255),Scalar(127,0,255),Scalar(127,0,127)};

	// get a frame to a mat
	// fölösleges, mert csak 1 byte lesz és kész.
	Mat frame(capture.height(), capture.width(), CV_8UC1);

	uint8_t *img_buffer;
	uint8_t *img_buffer_ptr;
	img_buffer = (uint8_t*)malloc(capture.framesize());
	
	// stat
	Statistics stats(config);

	// the most important tracker params
	// float _dt, float _Accel_noise_mag, double _dist_thres, int _maximum_allowed_skipped_frames,int _max_trace_length)
	double KFA = config.GetParam("KalmanFilterAcceleration", 0.0042);
	CTracker tracker[4] = {
		CTracker (DeltaTime, KFA, statfile, trackfile, 20, 10, -1), 
		CTracker (DeltaTime, KFA, statfile, trackfile, 20, 10, -1), 
		CTracker (DeltaTime, KFA, statfile, trackfile, 20, 10, -1), 
		CTracker (DeltaTime, KFA, statfile, trackfile, 20, 10, -1)};
		
	// detectors
	SpermDetectors detector[4] = {SpermDetectors(config), SpermDetectors(config), SpermDetectors(config), SpermDetectors(config)};
	vector<Point2d> centers[4];
	Rect myROI;

	// limit ROI size to 250 per frame according to sperm count
	unsigned int EstimatedSpermCount;
	int ROIwidth = 400;
	SpermDetectors singleusedetector(config);

	// ide kell valami exit condicio
	int cyclecounter = 1;
	// max a minute :-)
	int zs=1;
			
	while( waitKey(1) != 27 ) 
	{
		if (zs==capture.get_frame_count())
			break;

		capture.getnext(img_buffer);
		
		if (capture.islast() || (capture.get_err_num() != 0))
			break;
				
		// fölösleges, mert csak 1 byte lesz és kész.
		uint32_t w,h;
		img_buffer_ptr = img_buffer;
		for(h=0; h < capture.height(); h++)
			for(w=0; w < capture.width(); w++)
			{
				frame.at<uint8_t>(Point(w,h))= *img_buffer_ptr;
				img_buffer_ptr ++;
			}

/*
cv::Mat sperm = ColorSperms(detectionmethod, frame, 240, 320, "MFL_settings.ini");
transpose(sperm, sperm);
flip(sperm, sperm, 1);
imshow("focus",sperm);
*/
		// eddig folosleges
		// save 1st frame
		if ((TrackDataStored != 0) && (cyclecounter == 1)) {
			try 
			{ 
				imwrite(snapshotfilename, frame ); 
			}
		catch (runtime_error& ex) 
			{
				cerr << "Snapshot is not opened." << endl;
			}}
	
		// limit ROI size to 250 per frame according to sperm count
		if (cyclecounter == 1)
		{
			EstimatedSpermCount = CountSpermsMat(detectionmethod, frame, singleusedetector)/SubWindowNum;
			if (EstimatedSpermCount<250)
				ROIwidth = 400;
			else if	(EstimatedSpermCount<300)
				ROIwidth = 350;
			else
				ROIwidth = 300;
		}
		// limit ROI size to 250 per frame according to sperm count

		if (SubWindowNum != 1)
		{
			cvtColor(2*(frame - 50), rgbframe, CV_GRAY2RGB);
			
		};

		// subwindow cycle
		for(int subwindow = 0;subwindow <SubWindowNum; subwindow++)
		{
			// fps changed!
			if (capture.deltatime > 2*DeltaTime) {
				// clear tracks
				tracker[subwindow].Restart();
			}

			// selection of amount
			if (SubWindowNum == 1)
				myROI = Rect(200, 150, ROIwidth, 300);
			else if (SubWindowNum == 2) {
				switch(subwindow) {
				case 0: myROI = Rect(0+400-ROIwidth, 150, ROIwidth, 300); break;
				default: myROI = Rect(399, 150, ROIwidth, 300); break;
				}
			}
			else if (SubWindowNum == 4) {
				switch(subwindow) {
				case 0: myROI = Rect(0+400-ROIwidth, 0, ROIwidth, 300); break;
				case 1: myROI = Rect(0+400-ROIwidth, 299, ROIwidth, 300); break;
				case 2: myROI = Rect(399, 0, ROIwidth, 300); break;
				default: myROI = Rect(399, 299, ROIwidth, 300); break;
				}
			} 
			else
				// the impossible
				Rect myROI(200, 150, ROIwidth, 300);
			
			Mat inputimage =  frame(myROI);
				
			// Detect the sperms
			// 0 - small circular like sperms, quick
			// 1 - oval, larger sperms, slower
			switch (detectionmethod) {
				case 0: 
					detector[subwindow].Quick(inputimage, centers[subwindow]); 
					break;
				default:
					detector[subwindow].Dense(inputimage, centers[subwindow]); 
					break;	
			}
			
			// show before convert to microns
			// show
			cv::Point a,b;
			double xoffs, yoffs;
			if (SubWindowNum == 1) {
				cvtColor(2*(inputimage - 50), rgbframe, CV_GRAY2RGB);
				xoffs = 0;
				yoffs = 0;
			} else
			{
				xoffs = myROI.x;
				yoffs = myROI.y;
			}
#ifdef SHOWIT			
			for(unsigned int i=0;i<centers[subwindow].size();i++)
			{ 
				a.x = centers[subwindow][i].x + xoffs;
				a.y = centers[subwindow][i].y + yoffs;

				circle(rgbframe, a, 1, Scalar(128,255,128),1, 1);  
			}
#endif			
			// valid tracks
			/*
			for(unsigned int i=0;i<tracker[subwindow].tracks.size();i++)
			{ 
				if (tracker[subwindow].tracks[i]->valid) {
					
					for(int j=0;j<tracker[subwindow].tracks[i]->realtrace.size()-1;j++)
					{
						//	line(rgbframe,tracker.tracks[i]->trace[j],tracker.tracks[i]->trace[j+1],Colors[tracker.tracks[i]->track_id%9], 1, CV_AA);
						a.x = tracker[subwindow].tracks[i]->realtrace[j].x/MicronPerPixel + xoffs;
						a.y = tracker[subwindow].tracks[i]->realtrace[j].y/MicronPerPixel + yoffs;
						b.x = tracker[subwindow].tracks[i]->realtrace[j+1].x/MicronPerPixel + xoffs;
						b.y = tracker[subwindow].tracks[i]->realtrace[j+1].y/MicronPerPixel + yoffs;
						
						line(rgbframe,a,b,Colors[ (int)tracker[subwindow].tracks[i]->track_id % 9], 1, CV_AA);
					}
					
					/*
					cv::Point ccenter;
					ccenter.x = tracker[subwindow].tracks[i]->trace[tracker[subwindow].tracks[i]->trace.size()-1].x/MicronPerPixel + xoffs;
					ccenter.y = tracker[subwindow].tracks[i]->trace[tracker[subwindow].tracks[i]->trace.size()-1].y/MicronPerPixel + yoffs;

					circle(rgbframe, ccenter, 2, Scalar(128,128, 255)); 
				
				}
			}
				*/	
			// convert to micrometers
			for(unsigned int i=0;i<centers[subwindow].size();i++)
				centers[subwindow][i] = centers[subwindow][i] * MicronPerPixel;

			// the real stuff, tracking done here
			if(centers[subwindow].size()>0)
				tracker[subwindow].Update(centers[subwindow], capture.lasttime);

			// statistics
			// concentration in 1 milliliter  =  1e+12 cubic micrometer
			// in millions
			stats.UpdateGrages(tracker[subwindow].tracks, 1 / ((ChannelHeight * MicronPerPixel*300 * MicronPerPixel * ROIwidth)/1e12) / 1e6);
			
			//fprintf(fuck, "%d ", (int)centers[subwindow].size() );
		
		} // subwindows
		//fprintf(fuck, "\n");

#ifdef SHOWIT			
		imshow("main",rgbframe);
#endif

		// logs
		std::cout << "concentration [millions/ml]:   " << stats.GetConcentration()  << std::endl;
		std::cout << "sperm count:   " << stats.GetSpermCount()*SubWindowNum  << std::endl;
		std::cout << stats.GetGradeA() << " " << stats.GetGradeB() << " " << stats.GetGradeC() << " " << stats.GetGradeD() << " " << std::endl;
		std::cout << cyclecounter * DeltaTime << " seconds " << std::endl  << std::endl;

		cyclecounter++;
		zs++;
	}

	// close all open tracks to use the open statfile before it closes
	for(int subwindow = 0;subwindow < SubWindowNum; subwindow++)
	{
		tracker[subwindow].Restart();
		detector[subwindow].~SpermDetectors();
	}
	capture.shut();
	
	// results, concentration, abcd grades
	CABCD[0] = stats.GetConcentration();
	CABCD[1] = stats.GetGradeA();
	CABCD[2] = stats.GetGradeB();
	CABCD[3] = stats.GetGradeC();
	CABCD[4] = stats.GetGradeD();
	
	CABCD[5] = cyclecounter * 24;
	CABCD[6] = SubWindowNum;
	CABCD[7] = stats.GetSpermCount() * SubWindowNum;
	

	if (statfile != NULL) {
		fclose(statfile);
	}
	if (trackfile != NULL) {
		fclose(trackfile);
	}

	free(img_buffer);
	//fclose(fuck);
	return 0;
}

void findgolden(const char *name, double *abcd)
{
	// find golden
	FILE *golden;
	char buf[256];

	golden = fopen("golden.prn", "r");
	if (golden == NULL)
	{
		cerr << "golden.prn file is not opened." << endl;
		return;
	}

	fseek(golden, 0, SEEK_SET);
	while(! feof(golden)) {
		fscanf(golden, "%s %lf %lf %lf %lf\n", buf, &abcd[0], &abcd[1], &abcd[2], &abcd[3]);
		if (strcmp(name, buf) == 0) {
			fclose(golden);	
			return;
		}
	}

	abcd[0] = 0;
	abcd[1] = 0;
	abcd[2] = 0;
	abcd[3] = 0;
	fclose(golden);	
}

int main(int argc, char **argv)
{
	FILE *statfile = NULL;
	statfile = fopen("results.csv", "w");
	if (statfile == NULL)
	{
		cerr << "Statistics file is not opened." << endl;
	}
//	fprintf(statfile, "Filename; Concentration; Grade A; Grade B; Grade C; Grade D; Sperm count\n");

	FILE *dirfile = NULL;
	char buf[256];
	vector <string> filelist;

	system("dir /b *.cap > dirlist.txt");
	dirfile = fopen("dirlist.txt", "r");
	if (dirfile == NULL)
	{
		cerr << "No file list..." << endl;
		return -1;
	}

	fseek(dirfile, 0, SEEK_SET);
	while(! feof(dirfile)) {
		if (fscanf(dirfile, "%s", buf) > 0)
			filelist.push_back(string(buf));
		}
	fclose(dirfile);
	
	if (filelist.size() == 0) {
		cerr << "No file list..." << endl;
		return -1;
	}

	cout << filelist.size() << " video files found." << endl;
	
	// analzis
	int i=0;
	double CABCDtimearea[8];
	char settings[256] = "MFL_settings.ini";
	int analysistime = 100; // limit will be the cap file length
	char statisticsfilename[256] = "mfl.stat";
	char trackfilename[256]  = "track.stat";
	char snapshotfilename[256]  = "spot.jpg";
	char dummy[256]  = "";
	string dstr;

	for (i=0; i<filelist.size(); i++)
	{
		
#ifdef SHOWIT			
		namedWindow("main",WINDOW_AUTOSIZE);
#endif
		sprintf(statisticsfilename, "%d.stat", i);
		AnalyseVideoPC(filelist[i].c_str(), settings, -1, -1,  analysistime, statisticsfilename, trackfilename, snapshotfilename, CABCDtimearea);
		
#ifdef SHOWIT			
		destroyAllWindows();
#endif

		std::cout << "Final data of " << filelist[i] << std::endl;

		std::cout << "Video time: " << CABCDtimearea[5] << std::endl;
		std::cout << "Area: " << CABCDtimearea[6] << std::endl;

		std::cout << "concentration [millions/ml]:   " << CABCDtimearea[0]  << std::endl;
		std::cout << "grade A: " <<  CABCDtimearea[1] << std::endl;
		std::cout << "grade B: " <<  CABCDtimearea[2] << std::endl;
		std::cout << "grade C: " <<  CABCDtimearea[3] << std::endl;
		std::cout << "grade D: " <<  CABCDtimearea[4] << std::endl;

//		sprintf(dummy, "%.1lf;%.1lf;%.1lf;%.1lf;%.1lf;%d", CABCDtimearea[0],  CABCDtimearea[1],  CABCDtimearea[2],  CABCDtimearea[3],  CABCDtimearea[4], (int)CABCDtimearea[7]);
//		dstr = string(dummy);
//		replace(dstr.begin(), dstr.end(), '.',',');
//		fprintf(statfile, "%s;%s\n", filelist[i].c_str(), dstr.c_str() );

		fprintf(statfile, "%.1lf %.1lf %.1lf %.1lf ", CABCDtimearea[1],  CABCDtimearea[2],  CABCDtimearea[3],  CABCDtimearea[4]);
		findgolden(filelist[i].c_str(), CABCDtimearea);
		fprintf(statfile, "%.1lf %.1lf %.1lf %.1lf\n", CABCDtimearea[0],  CABCDtimearea[1],  CABCDtimearea[2],  CABCDtimearea[3]);
	}

	fclose(statfile);
	std::cout << "Ready" << std::endl;

	return 0;
}

#endif
