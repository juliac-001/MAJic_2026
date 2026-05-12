#include <lccv.hpp>
#include <opencv2/opencv.hpp>
#include <unistd.h>

using namespace cv;
const int max_value_H = 360 / 2;
const int max_value = 255;
const String window_capture_name = "Video Capture";
const String window_detection_name = "Object Detection";
int low_H = 0, low_S = 0, low_V = 0;
int high_H = max_value_H, high_S = max_value, high_V = max_value;
static void on_low_H_thresh_trackbar(int, void*)
{
	low_H = min(high_H - 1, low_H);
	setTrackbarPos("Low H", window_detection_name, low_H);
}
static void on_high_H_thresh_trackbar(int, void*)
{
	high_H = max(high_H, low_H + 1);
	setTrackbarPos("High H", window_detection_name, high_H);
}
static void on_low_S_thresh_trackbar(int, void*)
{
	low_S = min(high_S - 1, low_S);
	setTrackbarPos("Low S", window_detection_name, low_S);
}
static void on_high_S_thresh_trackbar(int, void*)
{
	high_S = max(high_S, low_S + 1);
	setTrackbarPos("High S", window_detection_name, high_S);
}
static void on_low_V_thresh_trackbar(int, void*)
{
	low_V = min(high_V - 1, low_V);
	setTrackbarPos("Low V", window_detection_name, low_V);
}
static void on_high_V_thresh_trackbar(int, void*)
{
	high_V = max(high_V, low_V + 1);
	setTrackbarPos("High V", window_detection_name, high_V);
}
int main(int argc, char* argv[])
{
	std::cout << "Sample program for LCCV video capture" << std::endl;
	std::cout << "Press ESC to stop." << std::endl;
	cv::Mat cam_0_im;
	lccv::PiCamera cam_0;
	cam_0.options->camera = 0;
	cam_0.options->video_width = 640;
	cam_0.options->video_height = 480;
	cam_0.options->framerate = 30;
	cam_0.options->verbose = true;
	cv::namedWindow(window_detection_name, cv::WINDOW_NORMAL);
	cv::namedWindow(window_capture_name, cv::WINDOW_NORMAL);
	cam_0.startVideo();
	std::cout << "Camera 0 Started" << std::endl;

	namedWindow(window_capture_name);
	namedWindow(window_detection_name);
	// Trackbars to set thresholds for HSV values
	createTrackbar("Low H", window_detection_name, &low_H, max_value_H, on_low_H_thresh_trackbar);
	createTrackbar("High H", window_detection_name, &high_H, max_value_H, on_high_H_thresh_trackbar);
	createTrackbar("Low S", window_detection_name, &low_S, max_value, on_low_S_thresh_trackbar);
	createTrackbar("High S", window_detection_name, &high_S, max_value, on_high_S_thresh_trackbar);
	createTrackbar("Low V", window_detection_name, &low_V, max_value, on_low_V_thresh_trackbar);
	createTrackbar("High V", window_detection_name, &high_V, max_value, on_high_V_thresh_trackbar);
	Mat frame, frame_HSV, frame_threshold;
	while (true) {
		if (!cam_0.getVideoFrame(cam_0_im, 99999999)) {
			std::cout << "Timeout error " << std::endl;
		}
		else {
			cv::imshow(window_capture_name, cam_0_im);
			// Convert from BGR to HSV colorspace
			resize(cam_0_im, cam_0_im, Size(320, 240));
			flip(cam_0_im, cam_0_im, -1);
			dilate(cam_0_im, cam_0_im, getStructuringElement(MORPH_RECT, Size(21, 21)));
			erode(cam_0_im, cam_0_im, getStructuringElement(MORPH_RECT, Size(21, 21)));
			cvtColor(cam_0_im, frame_HSV, COLOR_BGR2HSV);
			medianBlur(frame_HSV, frame_HSV, 5);
			// Detect the object based on HSV Range Values
			inRange(frame_HSV, Scalar(low_H, low_S, low_V), Scalar(high_H, high_S, high_V), frame_threshold);
			imshow(window_detection_name, frame_threshold);
			char key = (char)cv::waitKey(1);
			if (key == 'q' || key == 27)
			{
				break;
			}
		}
	}

	cam_0.stopVideo();
	std::cout << "\n\n\ninRange(frame_HSV, Scalar(";
	std::cout << low_H << "," << low_S << "," << low_V;

	std::cout << "), Scalar(";
	std::cout << high_H << "," << high_S << "," << high_V;

	std::cout << "), frame_threshold);\n\n\n" << std::endl;

	destroyAllWindows();
	return 0;
}
