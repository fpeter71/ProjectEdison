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
#define TRACK
//#define EVAL
#define TRACKLINES


class SampleMovementCheck {
	double xdrift, ydrift;
	int	count;
	int detected;
public:
	void Update(Point2f current, Point2f prediction) 
	{
		count ++;
		
		xdrift += (current.x - prediction.x) ;
		ydrift += (current.y - prediction.y) ;

	};

	double CheckLimits() 
	{  
		// means drift
		detected |= sqrt(xdrift*xdrift + ydrift*ydrift)/count > 12;
		return detected ;
	};

	void ResetCounter() {  count = 1;  xdrift = 0;ydrift = 0; detected=0;}

	SampleMovementCheck() { count = 1; xdrift = 0;ydrift = 0;detected=0;};
};

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
		CTracker (DeltaTime, KFA, statfile, trackfile, 20, 5, -1), 
		CTracker (DeltaTime, KFA, statfile, trackfile, 20, 5, -1), 
		CTracker (DeltaTime, KFA, statfile, trackfile, 20, 5, -1), 
		CTracker (DeltaTime, KFA, statfile, trackfile, 20, 5, -1)};
		
	// detectors
	SpermDetectors detector[4] = {SpermDetectors(config), SpermDetectors(config), SpermDetectors(config), SpermDetectors(config)};
	vector<Point2d> centers[4];
	Rect myROI;

	// limit ROI size to 250 per frame according to sperm count
	unsigned int EstimatedSpermCount;
	int ROIwidth = 400;
	SpermDetectors singleusedetector(config);
	SampleMovementCheck movementcheck;

	// skip first 10 frames
	int cyclecounter = 10;
	int zs=10;
	for (int i=0;i<10;i++)
		capture.getnext(img_buffer);

	char key = 0; 
	SubWindowNum = -1;
	movementcheck.ResetCounter();

	while( key != 27 ) 
	{
		key= cv::waitKey(30);

		if (zs>=capture.get_frame_count())
			break;

		capture.getnext(img_buffer);

		if (capture.islast() || (capture.get_err_num() != 0))
			break;
				
		// wait
		if ( key == 32 ) {
			cout<<"Press spacebar to continue!"<<endl;
			while(waitKey(100) != 32) {;};
		}
		if ((key!=0)&&(key!=-1)&&(key != 32))
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
	
		// limit ROI size to 250-300 per frame according to sperm count
		if (SubWindowNum == -1)
		{
			EstimatedSpermCount = CountSpermsMat(detectionmethod, frame, singleusedetector);
			if (EstimatedSpermCount>1800)
			{
				SubWindowNum = 1;
				cout << " too much " << endl;
				break;
			}
			if (EstimatedSpermCount>1200)
				SubWindowNum = 1;
			else if	(EstimatedSpermCount>700)
				SubWindowNum = 2;
			else
				SubWindowNum = 4;
			
			cout << "EstimatedSpermCount " << EstimatedSpermCount << " selection " << SubWindowNum << endl;
			
		}
		// limit ROI size to 250 per frame according to sperm count

		cvtColor(1.5*(frame - 50), rgbframe, CV_GRAY2RGB);
		
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
				//case 0: myROI = Rect(0+400-ROIwidth, 150, ROIwidth, 300); break;
				//default: myROI = Rect(399, 150, ROIwidth, 300); break;
				case 0: myROI = Rect(200, 0, ROIwidth, 300); break;
				default: myROI = Rect(200, 299, ROIwidth, 300); break;
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
			xoffs = myROI.x;
			yoffs = myROI.y;

#ifdef SHOWIT			
			for(unsigned int i=0;i<centers[subwindow].size();i++)
			{ 
				a.x = centers[subwindow][i].x + xoffs;
				a.y = centers[subwindow][i].y + yoffs;

				circle(rgbframe, a, 1, Scalar(128,255,128),1, 1);  
			}
#endif			
			// valid tracks
			for(unsigned int i=0;i<tracker[subwindow].tracks.size();i++)
			{ 
				// movement
				if (tracker[subwindow].tracks[i]->valid) {
					if ((tracker[subwindow].tracks[i]->get_filterred_speed() < 35) && (tracker[subwindow].tracks[i]->get_filterred_speed() > 5))
						movementcheck.Update(tracker[subwindow].tracks[i]->trace[0], tracker[subwindow].tracks[i]->trace[tracker[subwindow].tracks[i]->trace.size()-1]);
				
#ifdef TRACKLINES
				if ((tracker[subwindow].tracks[i]->get_filterred_speed() < 35) && (tracker[subwindow].tracks[i]->get_filterred_speed() > 5))
					//for(int j=0;j<tracker[subwindow].tracks[i]->trace.size()-1;j++)
					{
						//	line(rgbframe,tracker.tracks[i]->trace[j],tracker.tracks[i]->trace[j+1],Colors[tracker.tracks[i]->track_id%9], 1, CV_AA);
						a.x = tracker[subwindow].tracks[i]->trace[0].x/MicronPerPixel + xoffs;
						a.y = tracker[subwindow].tracks[i]->trace[0].y/MicronPerPixel + yoffs;
						b.x = tracker[subwindow].tracks[i]->trace[tracker[subwindow].tracks[i]->trace.size()-1].x/MicronPerPixel + xoffs;
						b.y = tracker[subwindow].tracks[i]->trace[tracker[subwindow].tracks[i]->trace.size()-1].y/MicronPerPixel + yoffs;
						
						line(rgbframe,a,b,Colors[ (int)tracker[subwindow].tracks[i]->track_id % 9], 1, CV_AA);
					}


					cv::Point ccenter;
					ccenter.x = tracker[subwindow].tracks[i]->KF->GetPrediction().x /MicronPerPixel + xoffs;
					ccenter.y = tracker[subwindow].tracks[i]->KF->GetPrediction().y /MicronPerPixel + yoffs;

					circle(rgbframe, ccenter, 2, Scalar(255,128, 120)); 


#else					
					cv::Point ccenter;
					ccenter.x = tracker[subwindow].tracks[i]->trace[tracker[subwindow].tracks[i]->trace.size()-1].x/MicronPerPixel + xoffs;
					ccenter.y = tracker[subwindow].tracks[i]->trace[tracker[subwindow].tracks[i]->trace.size()-1].y/MicronPerPixel + yoffs;

					circle(rgbframe, ccenter, 2, Scalar(128,128, 255)); 
#endif
				}
			}
			
			// convert to micrometers
			for(unsigned int i=0;i<centers[subwindow].size();i++)
				centers[subwindow][i] = centers[subwindow][i] * MicronPerPixel;

			// the real stuff, tracking done here
#ifdef TRACK
			if(centers[subwindow].size()>0)
				tracker[subwindow].Update(centers[subwindow], capture.lasttime);
#endif 

			// statistics
			// concentration in 1 milliliter  =  1e+12 cubic micrometer
			// in millions
			stats.UpdateGrages(tracker[subwindow].tracks, 1 / ((ChannelHeight * MicronPerPixel*300 * MicronPerPixel * ROIwidth)/1e12) / 1e6);
			
			//fprintf(fuck, "%d ", (int)centers[subwindow].size() );
			
		} // subwindows
		//fprintf(fuck, "\n");
				
#ifdef SHOWIT			
		imshow("ESC to step next video, space to stop/start!",rgbframe);
#endif
		// check sanity
		int totalsperms = 0;
		bool spermsarewelldistributed = true;
		
		for (int subwindow=0;subwindow<SubWindowNum;subwindow++) 
			totalsperms += centers[subwindow].size();
		
		cout << movementcheck.CheckLimits()  << endl;
		
		if (cyclecounter == 20)
			std::cout << "totalsperms " << totalsperms << endl;

		for (int subwindow=0;subwindow<SubWindowNum;subwindow++) 
			if ( centers[subwindow].size() > 500) 
			{
				spermsarewelldistributed = false;
				std::cout << "Too much: " << totalsperms << endl;
				key = (int)'1';
				break;
			}

		double std_dev = 0, mean_val = totalsperms/SubWindowNum;
		for (int subwindow=0;subwindow<SubWindowNum;subwindow++) 
			std_dev += (centers[subwindow].size() - mean_val)  * (centers[subwindow].size() - mean_val);
		// below 200 not checked at all
		if (totalsperms < 200)
			std_dev = 0;
		else
		{
			std_dev = sqrt( std_dev / SubWindowNum );
			std_dev /= mean_val;
		}

		if (std_dev > 0.3) {
			spermsarewelldistributed = false;
			cout << "Very wrong distributed! " << std_dev << endl;
			key = (int)'2';
			break;
		}

		// minoseg
		/*
		int totaltracks = 0;
		int totalvalidtracks = 0;
		for (int subwindow=0;subwindow<SubWindowNum;subwindow++) 
			for(unsigned int i=0;i<tracker[subwindow].tracks.size();i++)
			{ 
				totaltracks++;
				if (tracker[subwindow].tracks[i]->valid) {
					totalvalidtracks ++;
				}
			}
		cout << " track " << totaltracks << ", valid track " << totalvalidtracks << " per total sperm " << totalsperms << " = " << totalvalidtracks/totalsperms << endl;			
		*/

#ifdef TRACK
		// logs
//		std::cout << "concentration [millions/ml]:   " << stats.GetConcentration()  << std::endl;
	//	std::cout << "sperm count:   " << stats.GetSpermCount()*SubWindowNum  << std::endl;
//		std::cout << stats.GetGradeA() << " " << stats.GetGradeB() << " " << stats.GetGradeC() << " " << stats.GetGradeD() << " " << std::endl;
	//	std::cout << cyclecounter * DeltaTime << " seconds " << std::endl  << std::endl;
#endif
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

#ifdef EVAL
	if (key == -1)
	{
		cout << "Classes in this file:"<< endl;
		cout << "0 perfect" << endl;
		cout << "1 too dense, not visible correctly" << endl;
		cout << "2 bubbles, other artifacts in the video" << endl;
		cout << "3 motion of the sample" << endl;
		cout<<"Press opinion (0,1,2..) or ESC to finish to continue!"<<endl;
		while( key == -1) {
			key = waitKey(100) ;
		};
	}
#endif

	return key;
}


// detectionmethod if -1, it comes from the settings file
int SaveAVI(const char *filename, const char *videoname)
{
	char nonconstfilename[256];
	strcpy(nonconstfilename, filename);
	char noncostavi[256];
	strcpy(noncostavi, videoname);

	// input video open
	CapRead capture;
	capture.openfile(nonconstfilename);
  
	if(!capture.is_open())
	{
		cerr << "Problem opening video source" << endl;
		return -1;
	}
	
	// let it show
	Mat rgbframe;	
	
	// get a frame to a mat
	// fölösleges, mert csak 1 byte lesz és kész.
	Mat frame(capture.height(), capture.width(), CV_8UC1);

	uint8_t *img_buffer;
	uint8_t *img_buffer_ptr;
	img_buffer = (uint8_t*)malloc(capture.framesize());
	
	// ide kell valami exit condicio
	int cyclecounter = 1;
	// max a minute :-)
	int zs=1;
		
	char key = 0; 
	
	Size SizeOfFrame = cv::Size( 800, 600);
	VideoWriter video(noncostavi, CV_FOURCC('M', 'J', 'P', 'G'), 24, SizeOfFrame, true);
		
	while( key != 27 ) 
	{
		
		key= cv::waitKey(30);

		if (zs==capture.get_frame_count())
			break;

		capture.getnext(img_buffer);
		
		if (capture.islast() || (capture.get_err_num() != 0))
			break;
				
		// wait
		if ( key == 32 ) {
			cout<<"Press spacebar to continue!"<<endl;
			while(waitKey(100) != 32) {;};
		}
		if ((key!=0)&&(key!=-1)&&(key != 32))
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

		Mat inputimage =  frame;
		double min, max;
		cv::minMaxLoc(inputimage, &min, &max);
		inputimage = inputimage - min + 10;
		cv::minMaxLoc(inputimage, &min, &max);
		inputimage = inputimage/max*240;
		cvtColor(inputimage, rgbframe, CV_GRAY2RGB);

		video.write(rgbframe);
		
#ifdef SHOWIT			
		imshow("ESC to step next video, space to stop/start!",rgbframe);
#endif
		cyclecounter++;
		zs++;
	}

	free(img_buffer);
	video.release();

	return key;
}




int findgolden(const char *name, double *abcd)
{
	// find golden
	FILE *golden;
	char buf[256];

	golden = fopen("goldenref_csak_jok_casaval.csv", "r");
	if (golden == NULL)
	{
		cerr << "goldenref_csak_jok_casaval.csv file is not opened." << endl;
		return 0;
	}

	fseek(golden, 0, SEEK_SET);
	while(! feof(golden)) {
		fscanf(golden, "%s %lf %lf %lf %lf\n", buf, &abcd[0], &abcd[1], &abcd[2], &abcd[3]);
		strcat(buf, ".cap");
		if (strcmp(name, buf) == 0) {
			fclose(golden);		
			return 1;
		}
	}

	abcd[0] = 0;
	abcd[1] = 0;
	abcd[2] = 0;
	abcd[3] = 0;
	fclose(golden);	
	return 0;
}

int main(int argc, char **argv)
{
	FILE *statfile = NULL;
	FILE *evalfile = NULL;
#ifdef TRACK
	statfile = fopen("results.dat", "w");
#endif
#ifdef EVAL
	evalfile = fopen("jorossz.csv", "w");
	cout << "Classes in this file:"<< endl;
	cout << "0 perfect" << endl;
	cout << "1 too dense, not visible correctly" << endl;
	cout << "2 bubbles, other artifacts in the video" << endl;
	cout << "3 motion of the sample" << endl;

#endif

	FILE *dirfile = NULL;
	char buf[256];
	vector <string> filelist;

	system("dir /b /s *.cap > dirlist.txt");
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
	system("del dirlist.txt");

	if (filelist.size() == 0) {
		cerr << "No file list..." << endl;
		return -1;
	}

	cout << filelist.size() << " video files found." << endl;
	cout << "ESC to step next video, space to stop/start!" << endl;

	// analzis
	int i=0;
	double CABCDtimearea[8];
	double CABCDref[4];
	char settings[256] = "MFL_settings.ini";
	int analysistime = 100; // limit will be the cap file length
	char statisticsfilename[256] = "mfl.stat";
	char trackfilename[256]  = "track.stat";
	char snapshotfilename[256]  = "spot.jpg";
	char dummy[256]  = "";
	string dstr;

	// 45 84 116 121
	for (i=116; i<filelist.size(); i++)
	{
	//	if (findgolden( (filelist[i].substr(filelist[i].find_last_of("/\\") + 1)).c_str(), CABCDref) )
		{
			cout << endl << endl << i << "/" << filelist.size() << endl;
			cout << filelist[i] << " " << i << "/" << filelist.size() << endl;

	#ifdef SHOWIT			
			namedWindow("ESC to step next video, space to stop/start!",WINDOW_NORMAL); // _AUTOSIZE
	#endif
			sprintf(statisticsfilename, "%d.stat", i );
	
			int key = AnalyseVideoPC(filelist[i].c_str(), settings, -1, -1,  analysistime, statisticsfilename, trackfilename, snapshotfilename, CABCDtimearea);
			//int key = SaveAVI(filelist[i].c_str(), (filelist[i].substr(0,filelist[i].find_last_of(".")) + ".avi").c_str());
			//int key;
			//Interface dummy;
			//key= cv::waitKey(30);
			//char filename[256];
			//strcpy(filename, filelist[i].c_str());
			//AnalyseVideo(filename,dummy, settings, -1, -1,  analysistime, statisticsfilename, trackfilename, snapshotfilename, CABCDtimearea);

		
	#ifdef SHOWIT			
			destroyAllWindows();
	#endif
			if (key == 27)
				break;

	#ifdef EVAL
			fprintf(evalfile, "%s %s %d\n", filelist[i].c_str(), (filelist[i].substr(filelist[i].find_last_of("/\\") + 1)).c_str(), key - (int)'0');
	#endif


	#ifdef TRACK
			if (key !=  (int)'1')
			{
				std::cout << "Final data of " << filelist[i] << std::endl;

				std::cout << "Video time: " << CABCDtimearea[5] << std::endl;
				std::cout << "Area: " << CABCDtimearea[6] << std::endl;

				std::cout << "concentration [millions/ml]:   " << CABCDtimearea[0]  << std::endl;
				std::cout << "grade A: " <<  CABCDtimearea[1] << std::endl;
				std::cout << "grade B: " <<  CABCDtimearea[2] << std::endl;
				std::cout << "grade C: " <<  CABCDtimearea[3] << std::endl;
				std::cout << "grade D: " <<  CABCDtimearea[4] << std::endl;

				std::cout << "grade D: " <<  CABCDtimearea[4] << std::endl;

				/*
				if (i==0)
					fprintf(statfile, "Filename; Concentration; Grade A; Grade B; Grade C; Grade D; Sperm count\n");
		
				sprintf(dummy, "%.1lf;%.1lf;%.1lf;%.1lf;%.1lf;%d", CABCDtimearea[0],  CABCDtimearea[1],  CABCDtimearea[2],  CABCDtimearea[3],  CABCDtimearea[4], (int)CABCDtimearea[7]);
				dstr = string(dummy);
				replace(dstr.begin(), dstr.end(), '.',',');
				fprintf(statfile, "%s;%s\n", filelist[i].c_str(), dstr.c_str() );
				*/

				fprintf(statfile, "%s ",(filelist[i].substr(filelist[i].find_last_of("/\\") + 1)).c_str());
				fprintf(statfile, "%.1lf %.1lf %.1lf %.1lf    ",CABCDtimearea[1],  CABCDtimearea[2],  CABCDtimearea[3],  CABCDtimearea[4]);
				fprintf(statfile, "%.1lf %.1lf  ",CABCDtimearea[1] + CABCDtimearea[2] +  CABCDtimearea[3],  CABCDtimearea[1] + CABCDtimearea[2] );
				fprintf(statfile, "%.1lf %.1lf\n", CABCDref[0],  CABCDref[1]);
			
				cout << CABCDtimearea[1] + CABCDtimearea[2] +  CABCDtimearea[3] <<  "/" <<  CABCDtimearea[1] + CABCDtimearea[2] << "   ";
				cout << CABCDref[0] << "/" <<  CABCDref[1] << endl;
			}

#endif		

		} // in golden		
	}
	if(statfile!=NULL)
		fclose(statfile);
	if(evalfile!=NULL)
		fclose(evalfile);

	std::cout << "Ready" << std::endl;

	return 0;
}

#endif
