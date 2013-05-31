#include "car_count.h"
#include <stdio.h>
#include <stdlib.h> 



/*
configure file format
1 -- number of lanes
1 -- number of additional lanes
1 -- position of first additional lane
0 -- 0 if car first hits original line; 1 if it first hits additional line 
30 -- distance between lines (in feet)

64 -- ROI_x (define ROI)
150 -- ROI_y
80 -- ROI_width
38 -- ROI_height
16 -- VL_L_x (1st lane)
16 -- VL_L_y
44 -- VL_L_width
1 -- VL_L_height
48 -- VL_L_x (2nd lane)
22 -- VL_L_y
8 -- VL_L_width
1 -- VL_L_height
*/




/*
void main()
{
	
	char* video_name = "C:\\Documents and Settings\\imsc\\Desktop\\Intel Project\\Highway Videos\\Best Case\\20120208101941.avi";
	char* config_file_name = "C:\\Documents and Settings\\imsc\\Desktop\\Intel Project\\Highway Videos\\Best Case\\config_20120208101941.txt";
	char* out_file_name = "C:\\Documents and Settings\\imsc\\Desktop\\Intel Project\\Highway Videos\\Best Case\\ouput_20120208101941.txt";
	

	Configure* config = NULL;
		
	readConfigureFile(config_file_name, &config);
	FrameProcessor frame_processor(config);
	frame_processor.process_video(video_name, config);
	
	destroyConfigure(config);

	system("Pause");
}
*/

void main()
{	
	string video_names[MAX_VIDEO_NUM];
	vector<const char*> config_file_names;
	vector<string> output_file_names;
	char* input_file_name = "files.txt";

	ifstream input;
	input.open(input_file_name);
	if(!input.is_open()) {
		printf("failed to open the input file\n");
		exit(0);
	}

	int n=0;
	string line;
	getline(input,line);
	n = atoi(line.c_str());
	int i=0;

	for(int i=0; i<n; i++){
		getline(input,video_names[i]);
		getline(input,line);
		config_file_names.push_back(line.c_str());

		std::string result_file_base = video_names[i].substr(video_names[i].find_last_of("\\")+1);
		result_file_base = result_file_base.substr(0,result_file_base.find_last_of("."));
		std::string result_folder = "";
		result_folder += result_file_base + string("\\");
		std::string result_file = result_folder;
		result_file += result_file_base + string(".txt");
		output_file_names.push_back(result_file);
	}

	input.close();

	MultipleVideoProcessor processor(config_file_names, output_file_names, n);
	processor.process_videos(video_names);
//	delete processor;

	system("Pause");
}