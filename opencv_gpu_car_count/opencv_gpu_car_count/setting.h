#ifndef __SETTING_H__
#define __SETTING_H__

#define BUFFER_SIZE		        	2048
#define NUM_FRAMES                  80000
#define BACKGROUND_ADJUSTMENT       3.5
#define BACKGROUND_THRESHOLD        2.0f
#define MAX_IMAGE_WIDTH             1920		// Maximum FULLHD resolution
#define MAX_IMAGE_HEIGHT            1080
#define IMAGE_THRESHOLD             37
#define IMAGE_THRESHOLD_NIGHT       150
#define BUFFER_SIZE                 2048
#define DURATION_S                  100
#define MAX_INSTANCE                32
#define INIT_FRAME                  20          // in frames
#define SPEED_WINDOW                20          // in seconds 
#define STAT_WINDOW                 15          // in seconds
#define FLOW_LOWER_THRES            0.2f
#define FLOW_UPPER_THRES            0.5f
#define MAX_NIGHT_THRES             0.83f
#define DAY_NIGHT_THRES             20
#define OUTPUT_MODE                 1           // 0 for testing; 1 for streaming service
#define DISPLAY_PORT                1 
#define SKIP_FACTOR					1
#define SOFTWARE_MODE    			0
#define WHITE_COLOR					255
#define CONVERT_SPEED(x)		(x * 3600 / 5280);  //convert from miles per meter to miles per hour
#define MAX_VIDEO_NUM				8
#define MAX_FILE_NAME_LEN			200
#define DISPLAY_VIDEO_ON			1


#endif