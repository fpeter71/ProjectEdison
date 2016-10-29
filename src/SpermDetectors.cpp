#include "SpermDetectors.h"

// igy minden ".." string lesz es nem char*
using namespace std;

SpermDetectors::SpermDetectors(ReadConfig &config)
{
	_config = &config;
}

SpermDetectors::~SpermDetectors(void)
{
}

// sensitivity: final threshold level. meaningfull 2..10; best 4-6
void SpermDetectors::Quick(Mat &frame, vector<Point2d> &centers)
{
	double gausslow = _config->GetParam("SmallSpermDOGInnerGaussian", 1.2);
	double gausshigh = _config->GetParam("SmallSpermDOGOuterGaussian", 2.5);
	double sensitivity = _config->GetParam("SmallSpermThreshold", 6.0);
	double disclow = _config->GetParam("SmallSpermSizeLow", 0.0);
	double dischigh = _config->GetParam("SmallSpermSizeHigh", 25.0);

	// find the human sperms
	Mat g1, g2, diff;
	GaussianBlur(frame, g1, Size(13,13), gausshigh);
	GaussianBlur(frame, g2, Size(13,13), gausslow);
	diff = g1 - g2;
		
	// histeresis threshold
	Mat binaryImage;
	Mat thrlow, thrhigh;
	threshold(diff, thrhigh, sensitivity, 255, CV_THRESH_BINARY_INV);
	threshold(diff, thrlow, 3.0, 255, CV_THRESH_BINARY_INV);
		
	// dilate, and, ..
	erode(thrhigh, binaryImage, Mat());
	bitwise_or(thrlow, binaryImage, binaryImage);
	erode(binaryImage, thrhigh, Mat());
	bitwise_or(thrlow, thrhigh, thrhigh);
	erode(thrhigh, binaryImage, Mat());
	bitwise_or(thrlow, binaryImage, binaryImage);
		
	bitwise_not(binaryImage, binaryImage);
		
	CBlobResult res(binaryImage, Mat(), 1);
	CBlobResult resfilterred;

	// FilterAction::FLT_EXCLUDE FilterCondition::FLT_OUTSIDE
	res.Filter(resfilterred, 2, CBlobGetArea(), 10,  disclow, dischigh);
	
	// transfer to tracker
	centers.resize( resfilterred.GetNumBlobs() );
	for(int i=0;i<resfilterred.GetNumBlobs();i++) { 
		Point p;
		p = resfilterred.GetBlob(i)->getCenter();
		centers[i] = Point2d( (double) p.x, (double)p.y ); 
	}

}

const Mat Bkernel1 = (Mat_<float>(7, 7) << 0.000 ,0.152 ,0.076 ,0.076 ,0.076 ,0.152 ,0.000 ,0.000 ,0.152 ,-0.168 ,-0.168 ,-0.168 ,0.152 ,0.000 ,0.000 ,0.152 ,-0.168 ,-0.168 ,-0.168 ,0.152 ,0.000 ,0.000 ,0.152 ,-0.168 ,-0.244 ,-0.168 ,0.152 ,0.000 ,0.000 ,0.152 ,-0.168 ,-0.168 ,-0.168 ,0.152 ,0.000 ,0.000 ,0.152 ,-0.168 ,-0.168 ,-0.168 ,0.152 ,0.000 ,0.000 ,0.152 ,0.076 ,0.076 ,0.076 ,0.152 ,0.000 );
const Mat Bkernel2 = (Mat_<float>(7, 7) << 0.000 ,0.000 ,0.065 ,0.101 ,0.075 ,0.030 ,0.000 ,0.000 ,0.031 ,0.147 ,-0.111 ,-0.117 ,0.026 ,0.059 ,0.000 ,0.121 ,-0.041 ,-0.168 ,-0.168 ,-0.158 ,0.123 ,0.087 ,0.030 ,-0.168 ,0.066 ,-0.168 ,0.030 ,0.087 ,0.123 ,-0.158 ,-0.168 ,-0.168 ,-0.041 ,0.121 ,0.000 ,0.059 ,0.026 ,-0.117 ,-0.111 ,0.147 ,0.031 ,0.000 ,0.000 ,0.030 ,0.075 ,0.101 ,0.065 ,0.000 ,0.000  );
const Mat Bkernel3 = (Mat_<float>(7, 7) << 0.000 ,0.000 ,0.000 ,0.022 ,0.069 ,0.117 ,0.032 ,0.026 ,0.073 ,0.120 ,0.121 ,0.022 ,0.014 ,0.040 ,0.130 ,0.014 ,-0.084 ,-0.168 ,-0.168 ,-0.116 ,0.064 ,0.040 ,-0.168 ,-0.168 ,-0.070 ,-0.168 ,-0.168 ,0.040 ,0.064 ,-0.116 ,-0.168 ,-0.168 ,-0.084 ,0.014 ,0.130 ,0.040 ,0.014 ,0.022 ,0.121 ,0.120 ,0.073 ,0.026 ,0.032 ,0.117 ,0.069 ,0.022 ,0.000 ,0.000 ,0.000  );
const Mat Bkernel4 = (Mat_<float>(7, 7) << 0.032 ,0.117 ,0.069 ,0.022 ,0.000 ,0.000 ,0.000 ,0.040 ,0.014 ,0.022 ,0.121 ,0.120 ,0.073 ,0.026 ,0.064 ,-0.116 ,-0.168 ,-0.168 ,-0.084 ,0.014 ,0.130 ,0.040 ,-0.168 ,-0.168 ,-0.070 ,-0.168 ,-0.168 ,0.040 ,0.130 ,0.014 ,-0.084 ,-0.168 ,-0.168 ,-0.116 ,0.064 ,0.026 ,0.073 ,0.120 ,0.121 ,0.022 ,0.014 ,0.040 ,0.000 ,0.000 ,0.000 ,0.022 ,0.069 ,0.117 ,0.032  );
const Mat Bkernel5 = (Mat_<float>(7, 7) << 0.000 ,0.030 ,0.075 ,0.101 ,0.065 ,0.000 ,0.000 ,0.059 ,0.026 ,-0.117 ,-0.111 ,0.147 ,0.031 ,0.000 ,0.123 ,-0.158 ,-0.168 ,-0.168 ,-0.041 ,0.121 ,0.000 ,0.087 ,0.030 ,-0.168 ,0.066 ,-0.168 ,0.030 ,0.087 ,0.000 ,0.121 ,-0.041 ,-0.168 ,-0.168 ,-0.158 ,0.123 ,0.000 ,0.031 ,0.147 ,-0.111 ,-0.117 ,0.026 ,0.059 ,0.000 ,0.000 ,0.065 ,0.101 ,0.075 ,0.030 ,0.000  );

void SpermDetectors::Dense(Mat &frame, vector<Point2d> &centers)
{
	double sensitivity = _config->GetParam("OvalSpermThreshold", 25.0);
	double histthreshold = _config->GetParam("OvalSpermUseHysteresisThreshold", 0);
	double sensitivitylow = _config->GetParam("OvalSpermHysteresisThresholdLow", 15.0);
	double disclow = _config->GetParam("OvalSpermSizeLow", 2.0);
	double dischigh = _config->GetParam("OvalSpermSizeHigh", 30.0);
	
	// find the bull sperms
	Mat sumofkernels;
	Mat kerneloutput;

	filter2D(frame, sumofkernels, -1, Bkernel1);
	filter2D(frame, kerneloutput, -1, Bkernel2); sumofkernels += kerneloutput;
	filter2D(frame, kerneloutput, -1, Bkernel3); sumofkernels += kerneloutput;
	filter2D(frame, kerneloutput, -1, Bkernel4); sumofkernels += kerneloutput;
	filter2D(frame, kerneloutput, -1, Bkernel5); sumofkernels += kerneloutput;
	GaussianBlur(sumofkernels, sumofkernels, Size(3,3), 1);

	// threshold
	Mat binaryImage;
		
	if (histthreshold != 0) {
		// histeresis threshold
		Mat thrlow, thrhigh;
		threshold(sumofkernels, thrhigh, sensitivity, 255, CV_THRESH_BINARY_INV);
		threshold(sumofkernels, thrlow, sensitivitylow, 255, CV_THRESH_BINARY_INV);
		
		// dilate, and, ..
		erode(thrhigh, binaryImage, Mat());
		bitwise_or(thrlow, binaryImage, binaryImage);
		erode(binaryImage, thrhigh, Mat());
		bitwise_or(thrlow, thrhigh, thrhigh);
		erode(thrhigh, binaryImage, Mat());
		bitwise_or(thrlow, binaryImage, binaryImage);
		
		bitwise_not(binaryImage, binaryImage);		
	}
	else
	{
		threshold(sumofkernels, binaryImage, sensitivity, 255, CV_THRESH_BINARY);
	}

	CBlobResult res(binaryImage, Mat(), 1);
	CBlobResult resfilterred;

	res.Filter(resfilterred, 2, CBlobGetArea(), 10, disclow, dischigh);
	
	// transfer to tracker
	centers.resize( resfilterred.GetNumBlobs() );
	for(int i=0;i<resfilterred.GetNumBlobs();i++) { 
		Point p;
		p = resfilterred.GetBlob(i)->getCenter();
		centers[i] = Point2d( (double) p.x, (double)p.y ); 
	}

}


// statistics
void Statistics::UpdateGrages(vector<CTrack*> tracks, double concentration_multiplier) 
{
	double VCL, VAP, VSL, GradeNonLinearity;
	double GradeAThreshold, GradeBThreshold, GradeNonLinearityLimit, GradeCThreshold;
	GradeAThreshold = _config->GetParam("GradeAThreshold");
	GradeBThreshold = _config->GetParam("GradeBThreshold");
	GradeCThreshold = _config->GetParam("GradeCThreshold");
	GradeNonLinearityLimit = _config->GetParam("GradeNonLinearityLimit");
	bool anyfound = false;

	for (unsigned int i=0;i<tracks.size();i++)
	{
		if (tracks[i]->trace.size() > (1/_config->GetParam("DeltaTime")))
		{
			anyfound = true;
			tracks[i]->valid = true;

			_gradesamplenum++; 

			VCL = tracks[i]->get_real_speed();
			VAP = tracks[i]->get_filterred_speed();
			// dt is fixed for 0.042
			VSL = tracks[i]->get_endtoend_distance() / (tracks[i]->trace.size() * 0.042 + 1e-9);
			GradeNonLinearity = tracks[i]->get_filterred_distance() /	tracks[i]->get_endtoend_distance();
			
			if ((VAP >= GradeAThreshold) && (GradeNonLinearity >= GradeNonLinearityLimit))
				gradeA++;
			else if ((VAP >= GradeBThreshold) && ((VAP < GradeAThreshold) || (GradeNonLinearity < GradeNonLinearityLimit)))
				gradeB++;
			else if ((VAP < GradeBThreshold) && (VAP > GradeCThreshold))
				gradeC++;
			else 
				gradeD++;
			/*
			if ((VSL >= GradeAThreshold) && (GradeNonLinearity >= GradeNonLinearityLimit))
				gradeA++;
			else if ((VSL >= GradeBThreshold) && ((VSL < GradeAThreshold) || ((VSL >= GradeAThreshold)&&(GradeNonLinearity < GradeNonLinearityLimit))))
				gradeB++;
			else if ((VSL < GradeBThreshold) && (VSL > GradeCThreshold))
				gradeC++;
			else if (VSL <= GradeCThreshold)
				gradeD++;
			*/
			// count it for the concentration
			Concentration += concentration_multiplier; 
			SpermCount++;
		}
		else
			tracks[i]->valid = false;

	}

	// count it for the concentration
	if (anyfound)
		_samplenum++; 
}

Statistics::Statistics() 
{ 
	_gradesamplenum = 0; _samplenum = 0; Concentration=0; SpermCount = 0; gradeA = 0; gradeB = 0; gradeC = 0; gradeD = 0; 
}

Statistics::Statistics(ReadConfig &config) 
{
	_config = &config; _gradesamplenum = 0; _samplenum = 0; Concentration=0;SpermCount = 0; 
	gradeA = 0; gradeB = 0; gradeC = 0; gradeD = 0;
}

// inputimage C2!
double FocusHits = 0;
Mat ColorSperms(int detectionmethod, Mat inputimage, int width, int height, char *settings)
{
	// detectors
	ReadConfig config(settings);
	SpermDetectors detector(config);

	// select center region only

	Mat inputwindow = inputimage(cv::Range(180, 180 + height/2), cv::Range(180, 180 + width/2));
	Mat inputfull = inputimage(cv::Range(180, 180  + 350), cv::Range(180, 180  + 350));

	// Detect the sperms
	// 0 - small circular like sperms, quick
	// 1 - oval, larger sperms, slower
	vector<Point2d> centers;
	switch (detectionmethod) {
		case 0:
			detector.Quick(inputwindow, centers);
			break;
		default:
			detector.Dense(inputwindow, centers);
			break;
	}
	vector<Point2d> centersfull;
	switch (detectionmethod) {
		case 0:
			detector.Quick(inputfull, centersfull);
			break;
		default:
			detector.Dense(inputfull, centersfull);
			break;
	}

	Mat rgbframe;
	Mat resizedinput;
	Point p;
	
	Rect myROI(0, 0, 120, 30);
	inputwindow = 2*(inputwindow - 50);
	inputwindow(myROI) = Scalar(0);
	char buf[32];
	FocusHits = FocusHits * 0.5 + 0.5*centersfull.size();
	sprintf(buf, "Hits: %d", (int)FocusHits);
	
	resize(inputwindow, resizedinput, Size(), 2,2, CV_INTER_CUBIC);
	putText(resizedinput, buf, Point2f(0,40), FONT_HERSHEY_COMPLEX_SMALL, 2, Scalar(255,255,255),2);
	
	cvtColor(resizedinput, rgbframe, CV_GRAY2RGB);

	for(unsigned int i=0;i<centers.size();i++)
	{
		p.x = centers[i].x*2;
		p.y = centers[i].y*2;
		if (p.y > 60) {
			circle(rgbframe, p, 1, Scalar(0,255,0));
		}
	}

	transpose(rgbframe, rgbframe);
	flip(rgbframe, rgbframe, 1);
	// spermcounter
	return rgbframe;
}

// inputimage C2!
int CountSpermsMat(int detectionmethod, Mat inputimage, SpermDetectors &detector)
{
	// Detect the sperms
	// 0 - small circular like sperms, quick
	// 1 - oval, larger sperms, slower
	vector<Point2d> centers;
	switch (detectionmethod) {
		case 0:
			detector.Quick(inputimage, centers);
			break;
		default:
			detector.Dense(inputimage, centers);
			break;
	}

	return centers.size();
}


unsigned int CountSperms(char *filename, char *settings, int detectionmethod)
{
	ReadConfig config(settings);

	// input video open
	CapRead capture;
	capture.openfile(filename);
  
	if(!capture.is_open())
	{
		cerr << "Problem opening video source" << endl;
		return -1;
	}

	// detectors
	SpermDetectors detector(config);
	vector<Point2d> centers;
	
	// get a frame to a mat
	// fĂ¶lĂ¶sleges, mert csak 1 byte lesz Ă©s kĂ©sz.
	Mat frame(capture.height(), capture.width(), CV_8UC1);

	uint8_t *img_buffer;
	uint8_t *img_buffer_ptr;
	img_buffer = (uint8_t*)malloc(capture.framesize());
	
	// method selection if -1, read from config
	if (detectionmethod == -1)
		detectionmethod = (int)config.GetParam("DetectionMethod", 0);

//namedWindow("Video");
while(!capture.islast()){

	// ide kell valami exit condicio
	capture.getnext(img_buffer);
		
	if (capture.get_err_num() != 0)
		return -1;
	
	// fĂ¶lĂ¶sleges, mert csak 1 byte lesz Ă©s kĂ©sz.
	uint32_t w,h;
	img_buffer_ptr = img_buffer;
	for(h=0; h < capture.height(); h++)
		for(w=0; w < capture.width(); w++)
		{
			frame.at<uint8_t>(Point(w,h))= *img_buffer_ptr;
			img_buffer_ptr++;
		}
					
	// eddig folosleges
	// select center region only
	Rect myROI(180, 150, 240, 320);
	Mat inputimage =  frame(myROI);
				
	// Detect the sperms
	// 0 - small circular like sperms, quick
	// 1 - oval, larger sperms, slower
	switch (detectionmethod) {
		case 0: 
			detector.Quick(inputimage, centers); 
			break;
		default:
			detector.Dense(inputimage, centers); 
			break;	
	}

	// create image-ecske
	Mat rgbframe;	
	cvtColor(inputimage, rgbframe, CV_GRAY2RGB);
	for(unsigned int i=0;i<centers.size();i++)
			{ circle(rgbframe, centers[i], 8, Scalar(128,255,128));  }
//	imshow("Video",rgbframe);
//	waitKey(1);
}
//destroyAllWindows();

	return centers.size();
}	

// detectionmethod if -1, it comes from the settings file
int AnalyseVideo(char *filename, Interface &etinterf, char *settings, int detectionmethod, int howmanyquarters,  int analysistime, char *statisticsfilename, char *trackfilename, char *snapshotfilename, double *CABCD)
{
	ReadConfig config(settings);
	double MicronPerPixel = config.GetParam("MicronPerPixel", 1);
	double DeltaTime = config.GetParam("DeltaTime", 0.042);
	double ChannelHeight = config.GetParam("ChannelHeight", 20);
	double MaxStepDistanceInPixels = config.GetParam("MaxStepDistanceInPixels", 20);
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
	capture.openfile(filename);
  
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

	// get a frame to a mat
	// fĂ¶lĂ¶sleges, mert csak 1 byte lesz Ă©s kĂ©sz.
	Mat frame(capture.height(), capture.width(), CV_8UC1);

	uint8_t *img_buffer;
	uint8_t *img_buffer_ptr;
	img_buffer = (uint8_t*)malloc(capture.framesize());
	
	// stat
	Statistics stats(config);

	// the most important tracker params
	// float _dt, float _Accel_noise_mag, double _dist_thres, int _maximum_allowed_skipped_frames,int _max_trace_length)
	double KFA = config.GetParam("KalmanFilterAcceleration", 1);
	CTracker tracker[4] = {
		CTracker (DeltaTime, KFA, statfile, trackfile, MaxStepDistanceInPixels, 10, -1), 
		CTracker (DeltaTime, KFA, statfile, trackfile, MaxStepDistanceInPixels, 10, -1), 
		CTracker (DeltaTime, KFA, statfile, trackfile, MaxStepDistanceInPixels, 10, -1), 
		CTracker (DeltaTime, KFA, statfile, trackfile, MaxStepDistanceInPixels, 10, -1)};
		
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
	for(int zs=0;zs<analysistime*24;zs++)
	{
		if (zs == capture.get_frame_count())
			break;
			
		etinterf.calc_screen_update((double)zs / (double)capture.get_frame_count());

		capture.getnext(img_buffer);
		
		if (capture.islast() || (capture.get_err_num() != 0))
			break;
				
		// fĂ¶lĂ¶sleges, mert csak 1 byte lesz Ă©s kĂ©sz.
		uint32_t w,h;
		img_buffer_ptr = img_buffer;
		for(h=0; h < capture.height(); h++)
			for(w=0; w < capture.width(); w++)
			{
				frame.at<uint8_t>(Point(w,h))= *img_buffer_ptr;
				img_buffer_ptr ++;
			}
					
		// eddig folosleges
		// save 1st frame
		//if ((TrackDataStored != 0) && (cyclecounter == 1)) {
		if (cyclecounter == 1) {
			try
			{ 
				imwrite("test.jpg", frame );
				cout << "snapshot ok." << endl;
			}
		catch (runtime_error& ex) 
			{
				cerr << "Snapshot is not opened." << endl;
			}}
	
		// limit ROI size to 250 per frame according to sperm count
		if (cyclecounter == 1)
		{
			EstimatedSpermCount = CountSpermsMat(detectionmethod, frame, singleusedetector);
			cout << "estimated: " << EstimatedSpermCount << endl;
			if (EstimatedSpermCount<250)
				ROIwidth = 400;
			else if	(EstimatedSpermCount<300)
				ROIwidth = 350;
			else
				ROIwidth = 300;
		}
		// limit ROI size to 250 per frame according to sperm count

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
				
			// convert to micrometers
			for(unsigned int i=0;i<centers[subwindow].size();i++)
				centers[subwindow][i] = centers[subwindow][i] * MicronPerPixel;

			// the real stuff, tracking done here
			if(centers[subwindow].size()>0)
				tracker[subwindow].Update(centers[subwindow], capture.lasttime);

			// statistics
			// concentration in 1 milliliter  =  1e+12 cubic micrometer
			// in millions
			stats.UpdateGrages(tracker[subwindow].tracks, 1/ ((ChannelHeight * MicronPerPixel*300 * MicronPerPixel*ROIwidth)/1e12) / 1e6);
		} // subwindows

		// logs
		std::cout << "concentration [millions/ml]:   " << stats.GetConcentration()  << std::endl;
		std::cout << "sperm count:   " << stats.GetSpermCount()*SubWindowNum  << std::endl;
		std::cout << stats.GetGradeA() << " " << stats.GetGradeB() << " " << stats.GetGradeC() << " " << stats.GetGradeD() << " " << std::endl;
		std::cout << cyclecounter * DeltaTime << std::endl;

		cyclecounter++;
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
	CABCD[5] = stats.GetSpermCount() * SubWindowNum;
	
	if (statfile != NULL) {
		fclose(statfile);
	}
	if (trackfile != NULL) {
		fclose(trackfile);
	}

	free(img_buffer);
	
	return 0;
}


