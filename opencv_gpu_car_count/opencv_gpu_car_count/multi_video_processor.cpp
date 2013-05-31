#include "multi_video_processor.h"

MultipleVideoProcessor::MultipleVideoProcessor(vector<const char *>configure_file_names, vector<string> output_file_names, int n) {
	num_of_videos = n;

	for(int i=0; i<n; i++) {
		Configure * config1 = NULL;
		readConfigureFile(configure_file_names[i], &(config1));
		config.push_back(config1);
		video_processors.push_back(new FrameProcessor(config[i]));
		output_files.push_back(fopen(output_file_names[i].c_str(),"w"));
	}
}

MultipleVideoProcessor::~MultipleVideoProcessor()
{
	for(int i=0; i<num_of_videos; i++) {
		//fclose(output_files[i]);
		delete(video_processors[i]);
		delete(config[i]);
	}
	output_files.clear();
	video_processors.clear();
	config.clear();
}

void MultipleVideoProcessor::update_background(vector<gpu::GpuMat> d_background_vec, vector<gpu::GpuMat> d_ROI_vec, 
											   vector<gpu::Stream> str, vector<gpu::GpuMat> bg_minus_img_vec, vector<gpu::GpuMat> img_minus_bg_vec, 
											   vector<gpu::GpuMat> bg_minus_img_thres_vec, vector<gpu::GpuMat> img_minus_bg_thres_vec) 
{	
	for(int i=0; i<num_of_videos; i++) {
		cv::gpu::subtract(d_background_vec[i], d_ROI_vec[i], bg_minus_img_vec[i], gpu::GpuMat(), -1 ,str[i]); 
	}
	for(int i=0; i<num_of_videos; i++) {
		cv::gpu::subtract(d_ROI_vec[i], d_background_vec[i], img_minus_bg_vec[i], gpu::GpuMat(), -1 ,str[i]);
	}
	for(int i=0; i<num_of_videos; i++) {
		cv::gpu::threshold(bg_minus_img_vec[i], bg_minus_img_thres_vec[i], BACKGROUND_THRESHOLD, 
			BACKGROUND_ADJUSTMENT, CV_THRESH_BINARY_INV, str[i]);
	}
	for(int i=0; i<num_of_videos; i++){
		cv::gpu::threshold(img_minus_bg_vec[i], img_minus_bg_thres_vec[i], BACKGROUND_THRESHOLD, 
			BACKGROUND_ADJUSTMENT, CV_THRESH_BINARY_INV, str[i]);
	}
	for(int i=0; i<num_of_videos; i++){
		cv::gpu::add(d_background_vec[i], bg_minus_img_thres_vec[i], d_background_vec[i], gpu::GpuMat(), -1 ,str[i]);
	}
	for(int i=0; i<num_of_videos; i++){
		cv::gpu::subtract(d_background_vec[i], img_minus_bg_thres_vec[i], d_background_vec[i], gpu::GpuMat(), -1 ,str[i]);
	}
}

bool MultipleVideoProcessor::stat_window_full(int frame_count)
{
	return (frame_count % (300*8) == 0);
}
void MultipleVideoProcessor::process_videos(const string* input_file_names)
{
	for(int i=0; i<num_of_videos; i++) {
		video_processors[i]->open_video(input_file_names[i]);
	}

    vector<cv::Mat> greyscale_vec; 
	vector<cv::Mat> original_vec;
	vector<cv::Mat>	ROI_vec;
	vector<cv::Mat>	ROI_boolean_vec;

	vector<cv::gpu::GpuMat> d_background_vec;
	vector<cv::gpu::GpuMat> d_foreground_vec;
	vector<cv::gpu::GpuMat> d_ROI_vec;
	vector<cv::gpu::GpuMat> d_motion_vec;
	vector<cv::gpu::GpuMat> d_ROI_boolean_vec;
	vector<cv::gpu::Stream> stream;

	vector<cv::gpu::GpuMat> bg_minus_img_vec, img_minus_bg_vec, bg_minus_img_thres_vec, img_minus_bg_thres_vec;
	vector<cv::gpu::GpuMat> buf1_vec, buf2_vec;
	cv::Mat element(11,11,CV_8U,cv::Scalar(1));
	int frame_count = 0;
	
	//CPU memory initialization
	for(int i=0; i<num_of_videos; i++) {
		cv::Rect myROI(config[i]->ROI);
		greyscale_vec.push_back(Mat(myROI.size(),CV_8U));
		original_vec.push_back(Mat(myROI.size(),CV_8U));
		video_processors[i]->read_grey_frame(greyscale_vec[i], original_vec[i]);
		cv::Mat ROI = cv::Mat(greyscale_vec[i],myROI);
		ROI_vec.push_back(ROI);
		ROI_boolean_vec.push_back(ROI);
	}

	//GPU memory initialization
	for(int i=0; i<num_of_videos; i++) {
		cv::Rect myROI(config[i]->ROI);
		d_background_vec.push_back(cv::gpu::GpuMat(ROI_vec[i]));
		d_foreground_vec.push_back(cv::gpu::GpuMat(ROI_vec[i]));
		d_ROI_vec.push_back(cv::gpu::GpuMat(ROI_vec[i]));
		d_motion_vec.push_back(cv::gpu::GpuMat(ROI_vec[i]));
		d_ROI_boolean_vec.push_back(cv::gpu::GpuMat(ROI_vec[i]));
		
		bg_minus_img_vec.push_back(cv::gpu::GpuMat(ROI_vec[i]));
		img_minus_bg_vec.push_back(cv::gpu::GpuMat(ROI_vec[i]));
		bg_minus_img_thres_vec.push_back(cv::gpu::GpuMat(ROI_vec[i]));
		img_minus_bg_thres_vec.push_back(cv::gpu::GpuMat(ROI_vec[i]));

		buf1_vec.push_back(cv::gpu::GpuMat(ROI_vec[i]));
		buf2_vec.push_back(cv::gpu::GpuMat(ROI_vec[i]));
		
		stream.push_back(gpu::Stream::Stream());
	}

	//initialize background image on GPU
	while(frame_count < INIT_FRAME) { 
		frame_count++;
		for(int i=0; i<num_of_videos; i++) {
			video_processors[i]->read_grey_frame(greyscale_vec[i], original_vec[i]);
			ROI_vec[i] = cv::Mat(greyscale_vec[i],config[i]->ROI);
			stream[i].enqueueUpload(ROI_vec[i], d_ROI_vec[i]);
		}
		for(int i=0; i<num_of_videos; i++) {
			stream[i].waitForCompletion();
		}
		update_background(d_background_vec, d_ROI_vec, stream, 
			bg_minus_img_vec, img_minus_bg_vec, bg_minus_img_thres_vec, img_minus_bg_thres_vec); 
	}
	
	for(int i=0; i<num_of_videos; i++) {
		stream[i].waitForCompletion();
	}

	frame_count = 0;
	clock_t start, end;
	start = clock();
	bool end_of_processing = false;

	while(!end_of_processing){
		end_of_processing = true;

		for(int i=0; i < num_of_videos; i++) {
			
			if(!video_processors[i]->end_of_video())
			{
				video_processors[i]->read_grey_frame(greyscale_vec[i], original_vec[i]);
			}	
			while(!video_processors[i]->end_of_video()&& video_processors[i]->skip_frame()) {
				video_processors[i]->read_grey_frame(greyscale_vec[i], original_vec[i]);
			}
			if(!video_processors[i]->end_of_video()) { // get a valid frame, needs to be processed. 
				end_of_processing = false;
			}
		}
		
		if(end_of_processing) {
			break;
		}

		for(int i=0; i < num_of_videos; i++){
			if(video_processors[i]->end_of_video()){
				continue;
			}
			
			const string original_window = "original_" + convertInt(i);
			show_image(original_window, original_vec[i]);

			frame_count++;
			ROI_vec[i] = cv::Mat(greyscale_vec[i],config[i]->ROI);
			
			//d_ROI.upload(ROI);
			stream[i].enqueueUpload(ROI_vec[i], d_ROI_vec[i]);
			draw_virtual_line(ROI_vec[i], config[i]);
			const string roi_window = "ROI_" + convertInt(i);
			show_image(roi_window, ROI_vec[i]);
		}
		
		// wait until uploading is done
		for(int i=0; i<num_of_videos; i++) {
			stream[i].waitForCompletion();
		}

		//calculate forground
		for(int i=0; i < num_of_videos; i++){
			cv::gpu::absdiff(d_ROI_vec[i], d_background_vec[i], d_foreground_vec[i], stream[i]); // fg = |ROI - bg|
		}
		
		for(int i=0; i < num_of_videos; i++){
			cv::gpu::threshold(d_foreground_vec[i], d_ROI_boolean_vec[i], IMAGE_THRESHOLD, 255, CV_THRESH_BINARY, stream[i]); // d_bool = thresh(fg)
		}

		//morphology operations	  
		for(int i=0; i < num_of_videos; i++){	
			cv::gpu::morphologyEx(d_ROI_boolean_vec[i], d_motion_vec[i], MORPH_CLOSE, element, 
				buf1_vec[i], buf2_vec[i], Point(-1,-1), 1, stream[i]);
		}

		for(int i=0; i < num_of_videos; i++){	
			cv::gpu::morphologyEx(d_motion_vec[i], d_ROI_boolean_vec[i], MORPH_OPEN, element,
				buf1_vec[i], buf2_vec[i], Point(-1,-1), 1, stream[i]);
		}
		
		//adjust background
		update_background(d_background_vec, d_ROI_vec, stream,
			bg_minus_img_vec, img_minus_bg_vec, bg_minus_img_thres_vec, img_minus_bg_thres_vec); 
		
		
		for(int i=0; i<num_of_videos; i++) {
			stream[i].waitForCompletion();
		}
		//d_ROI_boolean.download(ROI_boolean);
		for(int i=0; i < num_of_videos; i++){	
			stream[i].enqueueDownload(d_ROI_boolean_vec[i], ROI_boolean_vec[i]);
		}
		
		//process frame statistics
		for(int i=0; i<num_of_videos; i++) {
			stream[i].waitForCompletion();
		}

		for(int i=0; i < num_of_videos; i++){	
			video_processors[i]->process_per_frame(ROI_boolean_vec[i], config[i]);
				
			draw_virtual_line(ROI_boolean_vec[i], config[i]);
			const string bool_window = "ROI_bool_" + convertInt(i);
			show_image(bool_window, ROI_boolean_vec[i]);
		
			if(video_processors[i]->stat_window_full()) {	
				video_processors[i]->calc_window_stat(output_files[i]);
			}
		}
#if DISPLAY_VIDEO_ON
		cvWaitKey(0);
#endif
		if(stat_window_full(frame_count)){	
			end = clock();
			double duration = (double)(end-start);
			printf("processed %d frame \ntime: %.1f, on average %.1f msec per frame \n\n",
			frame_count, duration, duration/frame_count);
			frame_count = 0;
			start = clock();
		}
	}
}