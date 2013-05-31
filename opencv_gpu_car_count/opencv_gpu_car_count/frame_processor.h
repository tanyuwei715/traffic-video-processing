#ifndef __FRAME_PROCESSOR_H__
#define __FRAME_PROCESSOR_H__

#include "setting.h"

#include <cv.h>
#include <string>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/gpu/gpu.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include<iostream>
#include<fstream>
#include <vector>

using namespace std;
using namespace cv;

/*
frame_processor class defines a single video processor
*/

class Lane{
public:
	cv::Rect line1, line2;
	int line_dist;
	Lane() {
		line_dist = 0;
		line1 = cv::Rect();
		line2 = cv::Rect();
	}
};

class Configure{
public: 
	int num_of_lanes;
	int num_of_add_lines;
	int hit_type;
	Lane* lanes;
	cv::Rect ROI;
	float p1; //count the car as passing by
	float p2; //decrease the car as left

	Configure() {
		num_of_lanes = 0;
		hit_type = 0;
		lanes = NULL;	
		ROI = cv::Rect();
		p1 = 0.0;
		p2 = 0.0;
	}
};


class WindowStatInfo {
public:
	int count;          // number of cars over a period of time (stat_window)
	double speed;       // avg speed over a period of time
	WindowStatInfo() {
		count = 0;
		speed = 0.0;
	}
};

class FrameProcessor {
private:
	Configure* config;
	int stat_window_size; 
	int num_of_lanes;
	int direction;  // car flow direction: 1-- from first VL to additional VL;  0-- from AVL to VL
	int frame_interval_size; // process one frame every frame_interval_size frames  
	int time_interval_size; //get avg every time_interval_time secs
	int fps;				//frame_per_sec: depends on the video
	int frame_counter;
	double old_avg_speed;
	
	vector<bool*> hit_buffer;
	vector<bool> line_hit;
	vector<double>old_lane_speed;
	WindowStatInfo * window_stat_info;
	cv::VideoCapture cap;

	bool line_hit_by_car(cv::Mat ROIFrame, cv::Rect line, float thrshold);
//	void add_frame_stat(FrameStatInfo* frame_stat_);

public:
	FrameProcessor(Configure* conf) {
		config = conf;
		num_of_lanes = conf->num_of_lanes;
		direction = conf->hit_type;

		fps = 0;
		frame_counter = 0;
		window_stat_info = new WindowStatInfo();
		old_avg_speed = 0.0;
		time_interval_size = STAT_WINDOW;
		frame_interval_size = SKIP_FACTOR;

		for(int i=0; i<config->num_of_add_lines+config->num_of_lanes; i++) {
			line_hit.push_back(false);
			old_lane_speed.push_back(0.0);
		}
	}

	~FrameProcessor() {
		delete window_stat_info;
		line_hit.clear();
	}
	
	double get_time(int frame_num) {
		return (double)frame_num / (double)fps;
	}
	double get_time() {
		return (double)(frame_counter - INIT_FRAME) / (double)fps;
	}
	double get_period_length(int frame_num1, int frame_num2) {
		return abs((double)(frame_num1 - frame_num2))/ (double)fps;
	}
	void process_video(const string inputFile, Configure* config);
	void process_per_frame(cv::Mat ROIframe, Configure* config);
	void calc_window_stat(FILE* fout = NULL);
	bool stat_window_full();
	bool initialize();
	bool skip_frame();
	bool open_video(const string inputFile);
	bool end_of_video();

	void read_grey_frame(cv::Mat &mat, cv::Mat &original);
	void update_background(cv::gpu::GpuMat d_background, cv::gpu::GpuMat d_ROI, cv::Mat ROI); 
	void clear_buffer();
	double calc_avg_speed(int* hit_count, int* num_of_cars);
};	

void readRectangle(FILE *infile, cv::Rect* rect);
void readConfigureFile(const char* in_file_name, Configure** conf1);
void destroyConfigure(Configure* conf);
void draw_virtual_line(cv::Mat ROI, Configure* config);
string convertInt(int number);
void show_image(const string name, cv::Mat image);

#endif