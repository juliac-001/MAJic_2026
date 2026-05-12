#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/signal.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>

#include <chrono>
#include <thread>

#include "opencv2/opencv.hpp"
#include <opencv2/imgproc.hpp>
#include <lccv.hpp>
#include <unistd.h>
#include <iostream>

using namespace std::chrono_literals;
using namespace std;

using namespace cv;

#define BAUDRATE B115200
#define MODEMDEVICE "/dev/ttyAMA0"

int uart0_filestream = -1;

int flight, blight, fright, bright;
int result = 0;

bool stopMotors = false;
bool initgr = false;
bool onball = false;
int movement = 0;

int rx_length = 0;

lccv::PiCamera cam;

bool contour_cmp(vector<Point> &a, vector<Point> &b) {
	return contourArea(a) < contourArea(b);
}

void loop() {
	char tx_buffer[20];

	sprintf(tx_buffer, "[%d %d %d]", flight, fright, result);

	if (uart0_filestream != -1) {
		int count = 0;
		count = write(uart0_filestream, &tx_buffer[0], strlen(tx_buffer));  //Filestream, bytes to write, number of bytes to write
		if (count < 0) {
			printf("uart: %d\n", uart0_filestream);
			printf("UART TX error\n");

		} else {
			//printf("my name is maya");
		}
	}
}


int greenSquare(Mat img) {
	Mat hsvimg = img.clone();
	Mat saveimg = img.clone();
	
	cvtColor(img, img, COLOR_BGR2GRAY); 
		
	GaussianBlur(img, img, Size(5,5), 0, 0);
		
	threshold(img, img, 130, 255, THRESH_BINARY_INV);	
		
	Mat thresh = img.clone();
		
	vector<vector<Point>> contours;
	vector<vector<Point>> greenSquares;
	findContours(thresh, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	
	cvtColor(hsvimg, hsvimg, COLOR_BGR2HSV);

	inRange(hsvimg, Scalar (56, 108, 87), Scalar (128, 255, 255), hsvimg);
	imshow("hsv", hsvimg);
	findContours(hsvimg, greenSquares, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	drawContours(saveimg, greenSquares, -1, Scalar (100, 255, 100), 3);	
	imshow("w/ GS contours", saveimg);
	
	int leftcount = 0;
	int rightcount = 0;
	int func = 0;
	
	for (int i = 0; i < (int)greenSquares.size(); i++) {
		func = 0;
		if (contourArea(greenSquares[i]) < 500) {
			continue;
		}
		
		Rect box = boundingRect(greenSquares[i]);
		
		Point center(box.x+(box.width/2), box.y+(box.height/2));
		Point above(box.x+(box.width/2), box.y-10);
		Point right(box.x+(box.width) +10, box.y+(box.height/2));
		Point below(box.x+(box.width/2), box.y+(box.height)+10);
		Point left(box.x-10, box.y+(box.height/2));
		
		int abovecolor = thresh.at<unsigned char>(above);
		int rightcolor = thresh.at<unsigned char>(right);
		int belowcolor = thresh.at<unsigned char>(below);
		int leftcolor = thresh.at<unsigned char>(left);

		circle(saveimg, center, 3, Scalar(255,0,255), -1); //center - pink
		circle(saveimg, above, 3, Scalar(0,0,255), -1); // above - red
		circle(saveimg, right, 3, Scalar(0,255,255), -1); //right - yellow
		circle(saveimg, below, 3, Scalar(255,0,0), -1); //below - blue
		circle(saveimg, left, 3, Scalar(255,255, 0), -1); //left - teal
		
		imshow("w/ GS contours", saveimg);
		
		if (belowcolor == 255) {
			continue;
			func = 0;
		}
		else if(abovecolor == 255 && rightcolor == 255) {
			leftcount++;
		}
		else if(abovecolor == 255 && leftcolor == 255) {
			rightcount++;
		}
	}
		
	if (leftcount && rightcount) {
		printf("doublegreen\n");
		func = 3;
	}
	else if(leftcount == 1) {
		printf("left\n");
		func = 2;
	}
	else if(rightcount == 1) {
		printf("right\n");
		func = 1;
	}
	else {
		func = 0;
	}
	
	return func;
}

void runningGreen(Mat img) {
	result = greenSquare(img);
	if (result != 0) {
		result = 4;
		loop();
		
		this_thread::sleep_for(2000ms);
		
		Mat GRimg;
		if(!cam.getVideoFrame(GRimg,99999999)) {
			printf("!ERROR!\n");
		}
		
		imshow("GRimg", GRimg);
		
		result = greenSquare(GRimg);
	}
	loop();
}

void init() {
	uart0_filestream = open(MODEMDEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
	if (uart0_filestream == -1) {
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
		exit(-1);
	}

	struct termios options;
	tcgetattr(uart0_filestream, &options);
	options.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);
}

int main(void) {
	init();

	printf("gp20 found");

	Mat img;

	cam.options->camera = 0;
	cam.options->video_width = 320;
	cam.options->video_height = 240;
	cam.options->framerate = 30;
	cam.options->verbose = true;

	cam.startVideo();

	namedWindow("hi", WINDOW_NORMAL);
	
	unsigned char rx_buffer[256];
	
	while (rx_length <= 0) {
		rx_length = read(uart0_filestream, (void *)rx_buffer, 255);
	}
	
	rx_buffer[rx_length] = '\0';
	printf("Start data recieved: %s\n", rx_buffer);
	
	if (rx_length <= 0);
	else{
		rx_buffer[rx_length] = '\0';
		printf("%i bytes read : %s", rx_length, rx_buffer);
	}
	
	while (rx_length != 10) {
		if(!cam.getVideoFrame(img,99999999)) {
			printf("!ERROR!\n");
			break;
		}
		
		runningGreen(img);
		
		char key = (char) waitKey(1);	
		
		if (key == 'q') {
			break;
		} else if (key == ' ') {
			stopMotors = true;
		} else if (key == 'g') {
			stopMotors = false;
		}
		
		int test = read(uart0_filestream, (void*)rx_buffer, 255);
		
		if (test > 0 ||test==2) {
			rx_length = test;
		}
		
		printf("rxlength: %d", rx_length);
	}
	
	cam.stopVideo();
	destroyAllWindows();

	return 0;
}




/*#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/signal.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>

#include <chrono>
#include <thread>

#include "opencv2/opencv.hpp"
#include <opencv2/imgproc.hpp>
#include <lccv.hpp>
#include <unistd.h>
#include <iostream>

// our libraries
#include "motorfunctions.h"

using namespace std::chrono_literals;
using namespace std;

using namespace cv;

#define BAUDRATE B115200
#define MODEMDEVICE "/dev/ttyAMA0"

extern int fleft, bleft, fright, bright;
int errors, spdChange;

float base_kp = 4.2;
float kp = 4.2;
int targetValue = 160;
int targetSpeed = 50;
int result = 0;

bool stopMotors = false;

int rx_length = 0;

int uart0_filestream = -1;

lccv::PiCamera cam;

bool contour_cmp(vector<Point> &a, vector<Point> &b)
{
	return contourArea(a) < contourArea(b);
}

void loop()
{
	char tx_buffer[20];

	printf("left: %d, right: %d, result: %d\n", fleft, fright, result);

	if (stopMotors == 1)
	{
		motorZero();
	}

	sprintf(tx_buffer, "[%d %d %d]", fleft, fright, result);
	//printf("%s",tx_buffer);

	if (uart0_filestream != -1)
	{
		int count = 0;
		count = write(uart0_filestream, &tx_buffer[0], strlen(tx_buffer)); // Filestream, bytes to write, number of bytes to write
		if (count < 0)
		{
			printf("uart: %d\n", uart0_filestream);
			printf("UART TX error\n");
		}
	}
}

void linetrace(Mat img)
{

	imshow("base image", img);
	Mat base = img.clone();
	Mat saveimg = img.clone();
	Mat hsvimg = img.clone();

	cvtColor(img, img, COLOR_BGR2HSV);

	medianBlur(img, img, 5);

	inRange(img, Scalar(0, 0, 0), Scalar(180, 255, 137), img);

	dilate(img, img, getStructuringElement(MORPH_RECT, Size(7, 7), Point(3, 3)));
	erode(img, img, getStructuringElement(MORPH_RECT, Size(7, 7), Point(3, 3)));

	Mat thresh = img.clone();
	imshow("black and white", thresh);

	vector<vector<Point>> contours;
	vector<vector<Point>> contourLeft;
	vector<vector<Point>> contourRight;
	findContours(thresh, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	drawContours(saveimg, contours, -1, Scalar(100, 255, 100), 3);

	cout << "countersize: " << (int)contours.size() << endl;

	float base_kp = 1.7;
	float kp = base_kp;
	int targetValue = 240;
	int targetSpeed = 65;

	if ((int)contours.size() > 0)
	{
		vector<Point> line = *max_element(contours.begin(), contours.end(), contour_cmp);

		if (contourArea(contours[0]) > 900)
		{

			cout << "countoursize: " << contourArea(contours[0]) << endl;

			Moments m = moments(line);
			int cx = m.m10 / m.m00;
			int cy = m.m01 / m.m00;

			circle(saveimg, Point(cx, cy), 6, Scalar(255, 0, 255), -1);
			imshow("savedimg", saveimg);

			if (cy > 160)
			{
				kp = 4;
			}
			else
			{
				kp = base_kp;
			}

			kp = base_kp;

			errors = targetValue - cx;
			spdChange = errors * kp;
			fleft = targetSpeed - spdChange;
			fright = targetSpeed + spdChange;

			Mat sliceLeft = img(Rect(0, 0, 50, 240));
			findContours(sliceLeft, contourLeft, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

			Mat sliceRight = img(Rect(270, 0, 50, 240));
			findContours(sliceRight, contourRight, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

			loop();
		}
		else
		{
			fleft = targetSpeed;
			fright = targetSpeed;
			loop();
		}

	}

	else
	{
		fleft = targetSpeed;
		fright = targetSpeed;
		loop();
	}
}

void init()
{
	uart0_filestream = open(MODEMDEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
	if (uart0_filestream == -1)
	{
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
		exit(-1);
	}

	struct termios options;
	tcgetattr(uart0_filestream, &options);
	options.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);
}

int main(void)
{
	//printf("TEST!!!\n\n\n\n\n");

	init();

	printf("gp20 found");

	Mat img;

	cam.options->camera = 0;
	cam.options->video_width = 320;
	cam.options->video_height = 240;
	cam.options->framerate = 30;
	cam.options->verbose = true;

	cam.startVideo();

	namedWindow("hi", WINDOW_NORMAL);

	printf("RX LENGTH: %d\n", rx_length);

	unsigned char rx_buffer[256];

	while (rx_length <= 0)
	{
		rx_length = read(uart0_filestream, (void *)rx_buffer, 255);
	}

	rx_buffer[rx_length] = '\0';
	printf("Start data recieved: %s\n", rx_buffer);

	if (rx_length <= 0)
		;
	else
	{
		rx_buffer[rx_length] = '\0';
		printf("%i bytes read : %s\n", rx_length, rx_buffer);
	}

	while (rx_length != 10)
	{
		if (!cam.getVideoFrame(img, 99999999))
		{
			printf("!ERROR!\n");
			break;
		}

		flip(img, img, -1);

		linetrace(img);

		char key = (char)waitKey(1);

		if (key == 'q')
		{
			break;
		}
		else if (key == ' ')
		{
			stopMotors = true;
		}
		else if (key == 'g')
		{
			stopMotors = false;
		}

		int test = read(uart0_filestream, (void *)rx_buffer, 255);

		if (test > 0 || test == 2)
		{
			rx_length = test;
		}

		printf("rxlength: %d\n", rx_length);
	}

	cam.stopVideo();
	destroyAllWindows();

	return 0;
}*/

