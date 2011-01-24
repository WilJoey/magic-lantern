// 720x480, changes when external monitor is connected
#define YUV422_LV_BUFFER 0x40D07800 
#define YUV422_LV_PITCH 1440
#define YUV422_LV_PITCH_RCA 1080
#define YUV422_LV_PITCH_HDMI 3840
#define YUV422_LV_HEIGHT 480
#define YUV422_LV_HEIGHT_RCA 540
#define YUV422_LV_HEIGHT_HDMI 1080


// changes during record
#define YUV422_HD_BUFFER 0x44000080

#define YUV422_HD_PITCH_IDLE 2112
#define YUV422_HD_HEIGHT_IDLE 704

#define YUV422_HD_PITCH_ZOOM 2048
#define YUV422_HD_HEIGHT_ZOOM 680

#define YUV422_HD_PITCH_REC_FULLHD 3440
#define YUV422_HD_HEIGHT_REC_FULLHD 974

// guess
#define YUV422_HD_PITCH_REC_720P 2560
#define YUV422_HD_HEIGHT_REC_720P 580

#define YUV422_HD_PITCH_REC_480P 1280
#define YUV422_HD_HEIGHT_REC_480P 480

#define FOCUS_CONFIRMATION 0x41d0
#define DISPLAY_SENSOR 0x2dec
#define DISPLAY_SENSOR_MAYBE 0xC0220104

// for gui_main_task
#define GMT_NFUNCS 8
#define GMT_FUNCTABLE 0xFF453E14

// button codes as received by gui_main_task
#define BGMT_PRESS_LEFT 0x1c
#define BGMT_UNPRESS_LEFT 0x1d
#define BGMT_PRESS_UP 0x1e
#define BGMT_UNPRESS_UP 0x1f
#define BGMT_PRESS_RIGHT 0x1a
#define BGMT_UNPRESS_RIGHT 0x1b
#define BGMT_PRESS_DOWN 0x20
#define BGMT_UNPRESS_DOWN 0x21

#define BGMT_PRESS_SET 0x4
#define BGMT_UNPRESS_SET 0x5

#define BGMT_TRASH 0xA
#define BGMT_MENU 6
#define BGMT_DISP 7
#define BGMT_Q 8

#define BGMT_PRESS_HALFSHUTTER 0x3F
#define BGMT_UNPRESS_HALFSHUTTER 0x40

// these are not sent always
#define BGMT_PRESS_ZOOMOUT_MAYBE 0xD
#define BGMT_UNPRESS_ZOOMOUT_MAYBE 0xE

#define BGMT_PRESS_ZOOMIN_MAYBE 0xB
#define BGMT_UNPRESS_ZOOMIN_MAYBE 0xC


#define SENSOR_RES_X 5202
#define SENSOR_RES_Y 3465

#define FLASH_BTN_MOVIE_MODE ((*(int*)0x14c1c) & 0x40000)
#define CLK_25FPS 0x1e24c  // this is updated at 25fps and seems to be related to auto exposure
