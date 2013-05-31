#include "frame_processor.h"

string convertInt(int number)
{
	stringstream ss;//create a stringstream
	ss << number;//add number to the stream
	return ss.str();//return a string with the contents of the stream
}

void show_image(const string name, cv::Mat image)
{
#ifdef DISPLAY_VIDEO_ON
	cv::imshow(name, image);
#endif
}

void draw_virtual_line(cv::Mat ROI, Configure* config) 
{
	for(int i=0; i<config->num_of_lanes; i++) {
		cv::Mat line1(ROI, config->lanes[i].line1);
		cv::Mat line2(ROI, config->lanes[i].line2);
		line1 = 255;
		line2 = 255;
	}
}

void readRectangle(FILE *infile, cv::Rect* rect)
{
	char comment[100];
	fscanf(infile, "%d", &(rect->x));
	fgets(comment, 100, infile);
	fscanf(infile, "%d", &(rect->y));
	fgets(comment, 100, infile);
	fscanf(infile, "%d", &(rect->width));
	fgets(comment, 100, infile);
	fscanf(infile, "%d", &(rect->height));
	fgets(comment, 100, infile);
}

//read configure files
void readConfigureFile(const char* in_file_name, Configure** conf1)
{
	FILE *in_file = fopen(in_file_name, "r");
	if(in_file == NULL) {
		printf("can't open configuration file %s\n", in_file_name);
		exit(1);
	}
	int lane_num1 = 0, lane_num2 = 0, hit=0, line_dist=0, pos=0;
	Configure* conf = new Configure();
	char comment[100];
	fscanf(in_file, "%d", &lane_num1);
	fgets(comment, 100, in_file);
	fscanf(in_file, "%d", &lane_num2);
	fgets(comment, 100, in_file);
	fscanf(in_file, "%d", &pos);
	fgets(comment, 100, in_file);
	fscanf(in_file, "%d", &hit);
	fgets(comment, 100, in_file);
	fscanf(in_file, "%d", &line_dist);
	fgets(comment, 100, in_file);

	conf->num_of_lanes = lane_num1;
	conf->num_of_add_lines = lane_num2;
	conf->hit_type = hit;
	conf->lanes = new Lane[conf->num_of_lanes];

	readRectangle(in_file, &(conf->ROI));

	for(int i=0; i<conf->num_of_lanes; i++) {
		conf->lanes[i].line_dist = line_dist;
		conf->lanes[i].line1 = cv::Rect();
		readRectangle(in_file, &(conf->lanes[i].line1));
		conf->lanes[i].line2 = cv::Rect();
		readRectangle(in_file, &(conf->lanes[i].line2));
	}

	conf->p1 = FLOW_LOWER_THRES;
	conf->p2 = FLOW_UPPER_THRES;

	(*conf1) = conf;
}

void destroyConfigure(Configure* conf)
{
	delete[] conf->lanes;
	delete(conf);
}

bool FrameProcessor::line_hit_by_car(cv::Mat ROIFrame, cv::Rect line, float thrshold)
{
	cv::Mat line_boolean; 
	line_boolean = cv::Mat(ROIFrame, line);

	float sum = (float)(cv::sum(line_boolean)[0]);  
	float occupacy = sum /(float)(WHITE_COLOR * line_boolean.cols);
	//if 20% of line is blocked, then hit
	if(occupacy > thrshold){
		return true;
	}
	return false;
}


void FrameProcessor::process_per_frame(cv::Mat ROIframe, Configure* config)
{
	bool* hit_status = new bool[config->num_of_lanes + config->num_of_add_lines];
	
	for(int i=0; i<config->num_of_lanes; i++) {	
		hit_status[i] = false;
		if(!line_hit[i] && line_hit_by_car(ROIframe, config->lanes[i].line1, config->p2)){
			line_hit[i] = true;
			hit_status[i] = true;
		}
		else if(line_hit[i] && !line_hit_by_car(ROIframe, config->lanes[i].line1, config->p1)){	
			line_hit[i] = false;
		}
	}

	int n = config->num_of_lanes;
	for(int i=config->num_of_lanes; i<config->num_of_lanes+config->num_of_add_lines; i++) {		
		hit_status[i] = false;
		if(!line_hit[i] && line_hit_by_car(ROIframe, config->lanes[i-n].line2, config->p2)){
			line_hit[i] = true;
			hit_status[i] = true;
		}
		else if(line_hit[i] && !line_hit_by_car(ROIframe, config->lanes[i-n].line2, config->p1)){	
			line_hit[i] = false;
		}
	}
	hit_buffer.push_back(hit_status);
}

void FrameProcessor::clear_buffer() 
{
	frame_counter = INIT_FRAME;
	for(int i=0; i<num_of_lanes; i++) {
		hit_buffer.clear();
	}
}
double FrameProcessor::calc_avg_speed(int* hit_count, int* num_of_cars)
{
	double avg_speed = 0.0;
	double lane_speed = 0.0;
	int lane_count = 0;
	for(int i=0; i<num_of_lanes; i++) {
		lane_speed = 0.0;
		if(num_of_cars[i] > 0) {
			hit_count[i] /= num_of_cars[i];
			double time_interval = (double)(hit_count[i] * frame_interval_size) / (double)fps;
			lane_speed = (double)(config->lanes[i].line_dist) / time_interval;
			lane_speed = CONVERT_SPEED(lane_speed);
		}
		if(lane_speed > 100 || lane_speed == 0.0){
			lane_speed = old_lane_speed[i];
		}else{
			old_lane_speed[i] = lane_speed;
		}
		if(lane_speed > 0) { 
			avg_speed += lane_speed;
			lane_count++;
		}
	}
	avg_speed /= lane_count;

	return avg_speed;
}

void FrameProcessor::calc_window_stat(FILE*fout)
{
	double avg_speed = 0.0;
	int avg_num = 0;
	int lane_count = 0;
	int sum = 0;
	int* hit = new int[num_of_lanes];
	int* hit_count = new int[num_of_lanes];
	int* num_of_cars = new int[num_of_lanes];
	bool* hit_status = new bool[num_of_lanes];

	for(int i=0; i<num_of_lanes; i++) {
		hit[i] = 0;
		hit_count[i] = 0;
		num_of_cars[i] = 0;
		hit_status[i] = false;
	}

	for(int i=hit_buffer.size()-1; i>=0; i--) {
		for(int j=0; j<num_of_lanes; j++) {
			if(hit_status[j]) {
				if(hit_buffer[i][j]){
					hit_count[j] += hit[j];
					hit[j] = 0;
					hit_status[j] = false;
					num_of_cars[j]++;
				} 
			}
			if(hit_buffer[i][j+num_of_lanes]){
				hit[j] = 0;
				hit_status[j] = true;
			}
			if(hit_status[j]){
				hit[j]++;
			}
		}
	}
	//calc avg speed
	avg_speed = calc_avg_speed(hit_count, num_of_cars);
	
	if(avg_speed > 0 && avg_speed < 100) {
		old_avg_speed = avg_speed;
	}
	for(int i=0; i<num_of_lanes; i++) {
		avg_num += num_of_cars[i];
	}
	window_stat_info->speed = avg_speed;
	window_stat_info->count = avg_num;

	if(fout != NULL) {
		fprintf(fout, "average speed %.1f miles per hour\n", old_avg_speed);
		fprintf(fout, "average car number in %d sec: %d\n", STAT_WINDOW, avg_num);
	}
	else {
		printf("average speed %.1f miles per hour\n", old_avg_speed);
		printf("average car number in %d sec: %d\n", STAT_WINDOW, avg_num);
	}

	clear_buffer();
	delete[] hit;
	delete[] hit_count;
	delete[] num_of_cars;
	delete[] hit_status;
}


void FrameProcessor::update_background(cv::gpu::GpuMat d_background, cv::gpu::GpuMat d_ROI, cv::Mat ROI) 
{
	cv::gpu::GpuMat bg_minus_img(ROI), img_minus_bg(ROI), bg_minus_img_thres(ROI), img_minus_bg_thres(ROI);

	cv::gpu::subtract(d_background, d_ROI, bg_minus_img); 
	cv::gpu::subtract(d_ROI, d_background, img_minus_bg);
	cv::gpu::threshold(bg_minus_img, bg_minus_img_thres, BACKGROUND_THRESHOLD, BACKGROUND_ADJUSTMENT, CV_THRESH_BINARY_INV);
	cv::gpu::threshold(img_minus_bg, img_minus_bg_thres, BACKGROUND_THRESHOLD, BACKGROUND_ADJUSTMENT, CV_THRESH_BINARY_INV);
	cv::gpu::add(d_background, bg_minus_img_thres, d_background);
	cv::gpu::subtract(d_background, img_minus_bg_thres, d_background);
}

bool FrameProcessor::open_video(const string input_file)
{
	cap = cv::VideoCapture(input_file);
    
	fps = cap.get(CV_CAP_PROP_FPS);
	stat_window_size = time_interval_size * fps; //the num of cars is the average over stat_frame_num frames frames
	return cap.isOpened();
}


void FrameProcessor::read_grey_frame(cv::Mat &greyscale, cv::Mat &original) {
	cv::Mat frame;
	cap >> frame;
	original = frame.clone();
	if(original.channels() > 1){
		cv::cvtColor(original, greyscale, cv::COLOR_BGR2GRAY); // turn it to grey, 1 channel 
	}
	frame_counter++;
}

bool FrameProcessor::skip_frame() {
	return ((frame_counter - INIT_FRAME) % frame_interval_size > 0);
}

bool FrameProcessor::stat_window_full() {
	return ((frame_counter-INIT_FRAME) % stat_window_size == 0);
}

bool FrameProcessor::end_of_video() {
	return (!cap.grab() || frame_counter >= NUM_FRAMES);
}

void FrameProcessor::process_video(const string input_file_name, Configure* config)
{
	open_video(input_file_name);
	char* out_file_name = "C:\\Documents and Settings\\imsc\\Desktop\\Intel Project\\Highway Videos\\Best Case\\ouput_20120208101941.txt";
	FILE* fout = fopen(out_file_name, "w");

    cv::Mat greyscale, original;
	cv::Mat	ROI, ROI_boolean;
	cv::Mat element(11,11,CV_8U,cv::Scalar(1));
	cv::Rect myROI(config->ROI);

	read_grey_frame(greyscale, original);
	ROI = cv::Mat(greyscale,myROI);
	
	cv::gpu::GpuMat d_background(ROI);
	cv::gpu::GpuMat d_foreground(ROI);
	cv::gpu::GpuMat d_ROI(ROI);
	cv::gpu::GpuMat d_motion(ROI);
	cv::gpu::GpuMat d_ROI_boolean(ROI);
	cv::gpu::GpuMat d_result(ROI);

	while(frame_counter < INIT_FRAME) {
		read_grey_frame(greyscale, original);
		ROI = cv::Mat(greyscale,myROI);
		d_ROI.upload(ROI);
		update_background(d_background, d_ROI, ROI); 
	}

	int count = 0;
	float speed = 0;
	int frame_count = 1;
	int stat_count = 0;

	clock_t start, end;
	start = clock();

	while(cap.grab() && frame_counter < NUM_FRAMES)
    {
		read_grey_frame(greyscale, original);
		
		if(!skip_frame()) {
			ROI = cv::Mat(greyscale,myROI);
			
			d_ROI.upload(ROI);
			draw_virtual_line(ROI, config);
			cv::imshow("ROI", ROI);

			//calculate forground
			cv::gpu::absdiff(d_ROI, d_background, d_foreground); // fg = |ROI - bg|
			cv::gpu::threshold(d_foreground, d_ROI_boolean, IMAGE_THRESHOLD, 255, CV_THRESH_BINARY); // d_bool = thresh(fg)

			//morphology operations	  
			cv::gpu::morphologyEx(d_ROI_boolean, d_motion, MORPH_CLOSE, element);
			cv::gpu::morphologyEx(d_motion, d_ROI_boolean, MORPH_OPEN, element);

			d_ROI_boolean.download(ROI_boolean);


			//adjust background
			update_background(d_background, d_ROI, ROI); 

			//process frame statistics
			process_per_frame(ROI_boolean, config);
			
			draw_virtual_line(ROI_boolean, config);
			cv::imshow("ROI_boolean", ROI_boolean);
#ifdef DISPLAY_VIDEO
			cvWaitKey(0);
#endif
		}

		if(((frame_counter-INIT_FRAME) % stat_window_size) == 0) {	
			end = clock();
			double duration = (double)(end-start);
			int frame_num = (frame_counter - INIT_FRAME) / frame_interval_size;
			calc_window_stat();
			printf("processed %d frame, resolution: %d * %d, \ntime: %.1f, on average %.1f msec per frame \n\n",
				frame_num, original.cols, original.rows, duration, duration/frame_num);
			start = clock();
		}
    }
	fclose(fout);
}
