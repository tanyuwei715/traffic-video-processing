#ifndef __MULTI_VIDEO_PROCESSOR_H__
#define __MULTI_VIDEO_PROCESSOR_H__

#include "frame_processor.h"

class MultipleVideoProcessor {
private:
	int num_of_videos;
	vector<FrameProcessor*> video_processors;
	vector<Configure*> config;
	vector<FILE*> output_files;

public:
	MultipleVideoProcessor(vector<const char*> configure_file_names, vector<string> output_file_names, int n);
	~MultipleVideoProcessor();
	void process_videos(const string* input_file_names);
	bool stat_window_full(int frame_count);
	void update_background(vector<cv::gpu::GpuMat> d_background_vec, vector<cv::gpu::GpuMat> d_ROI_vec, 
		vector<cv::gpu::Stream> str, vector<cv::gpu::GpuMat> bg_minus_img_vec, vector<cv::gpu::GpuMat> img_minus_bg_vec, 
		vector<cv::gpu::GpuMat> bg_minus_img_thres_vec, vector<cv::gpu::GpuMat> img_minus_bg_thres_vec); 
};

#endif