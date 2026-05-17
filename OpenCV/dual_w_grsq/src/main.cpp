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

int errors, spdChange;

float kp = 1.3;
int targetValue = 160;
int targetSpeed = 15;
int result = 0;

int marr[2] = {0};

bool stopMotors = false;
bool turningLeft = false;
bool turningRight = false;
bool top = false;
bool backwards = true;

int rx_length = 0;
int uart0_filestream = -1;
lccv::PiCamera cameras[2];

Point noffset (0, 0);
Point loffset (0, 50);
Point roffset (220, 50);
Point boffset (0, 190);

int leftSize = 0;
int rightSize = 0;

int threshnumber = 180;

int fleft, bleft, fright, bright;

void motorZero() {
    fleft = 0;
    bleft = 0;
    fright = 0;
    bright = 0;
}

Mat green_image_proccessing(string area){
    Mat img;

    if (!cameras[0].getVideoFrame(img, 99999999))
    {
        printf("!ERROR!\n");
    }

    resize(img, img, Size(320, 240));
    //flip(img, img, -1);


    dilate(img, img, getStructuringElement(MORPH_RECT, Size(21, 21)));
    erode(img, img, getStructuringElement(MORPH_RECT, Size(21, 21)));

    cvtColor(img, img, COLOR_BGR2GRAY); 
    GaussianBlur(img, img, Size(5,5), 0, 0);
    threshold(img, img, 130, 255, THRESH_BINARY_INV);	

    if(area == "topRight"){
        img = img(Rect(210, 0, 59, 59));
    }
    else if(area == "topLeft"){
        img = img(Rect(50, 0, 59, 59));
    }

    return img;
}

Mat thresh_image_proccessing(){
    Mat img;

    if (!cameras[0].getVideoFrame(img, 99999999))
    {
        printf("!ERROR!\n");
    }

    resize(img, img, Size(320, 240));
    flip(img, img, -1);

    dilate(img, img, getStructuringElement(MORPH_RECT, Size(21, 21)));
    erode(img, img, getStructuringElement(MORPH_RECT, Size(21, 21)));

    cvtColor(img, img, COLOR_BGR2HSV);
    medianBlur(img, img, 5);
    inRange(img, Scalar(0, 0, 0), Scalar(180, 115, 115), img);

    return img;
}

bool contour_cmp(vector<Point>& a, vector<Point>& b)
{
    return contourArea(a) < contourArea(b);
}

int* getCenter(vector<Point> line, Mat img, Point offset = noffset, bool drawCircle = false){
    Moments m = moments(line);
    
    marr[0] = m.m10 / m.m00; //cx
    marr[1] = m.m01 / m.m00; //cyp

    if(drawCircle) {
        circle(img, Point(marr[0] + offset.x, marr[1] + offset.y), 6, Scalar(255, 127, 255), -1);
    }

    return marr;
}

void PID(vector<vector<Point>> contours, Mat img, int direction = 1)
{
    vector<Point> line;
    if ((int)contours.size() > 0) {
	line = *max_element(contours.begin(), contours.end(), contour_cmp);
    
	 cout << "countoursize: " << contourArea(line) << endl;
    
	int cx = getCenter(line, img)[0];
	int cy = getCenter(line, img)[1];
    
	//imshow("PIDimg", img);
    
	errors = targetValue - cx;
	spdChange = errors * kp;
	fleft = 1 * (targetSpeed - spdChange);
	fright = 1 * (targetSpeed + spdChange);
    }
}

void track(Point center)
{
    int cx = center.x;
    int cy = center.y;

    errors = targetValue - cx;
    spdChange = errors * kp;
    fleft = targetSpeed - spdChange;
    fright = targetSpeed + spdChange;
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

int greenSquare(Mat img) {
    Mat hsvimg = img.clone();
    Mat saveimg = img.clone();

    dilate(img, img, getStructuringElement(MORPH_RECT, Size(21, 21)));
    erode(img, img, getStructuringElement(MORPH_RECT, Size(21, 21)));
	
    cvtColor(img, img, COLOR_BGR2GRAY); 
	    
    GaussianBlur(img, img, Size(5,5), 0, 0);
	    
    threshold(img, img, threshnumber, 255, THRESH_BINARY_INV);	
	    
    Mat thresh = img.clone();
    //imshow("greensquarethresh", thresh);
	    
    vector<vector<Point>> contours;
    vector<vector<Point>> greenSquares;
    findContours(thresh, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    
    cvtColor(hsvimg, hsvimg, COLOR_BGR2HSV);

    inRange(hsvimg, Scalar (50, 55, 56), Scalar (110, 255, 255), hsvimg);
    //imshow("hsv", hsvimg);
    
    findContours(hsvimg, greenSquares, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    drawContours(saveimg, greenSquares, -1, Scalar (100, 255, 100), 3);	
    //imshow("w/ GS contours", saveimg);
    
    int leftcount = 0;
    int rightcount = 0;
    bool inmiddle = false;
    bool dinmiddle = false;
	
    for (int i = 0; i < (int)greenSquares.size(); i++) {
	if (contourArea(greenSquares[i]) < 1500) {
		continue;
	}
	
	Rect box = boundingRect(greenSquares[i]);
	
	Point center(box.x+(box.width/2), box.y+(box.height/2));
	Point above(box.x+(box.width/2), box.y-24);
	Point right(box.x+(box.width) +24, box.y+(box.height/2));
	Point below(box.x+(box.width/2), box.y+(box.height)+24);
	Point left(box.x-24, box.y+(box.height/2));
	
	/*int abovecolor = thresh.at<unsigned char>(above);
	int rightcolor = thresh.at<unsigned char>(right);
	int belowcolor = thresh.at<unsigned char>(below);
	int leftcolor = thresh.at<unsigned char>(left);*/
	
	auto sampleRegion = [&](Point p, int radius = 3) -> int {

	    // Clamp ROI to image bounds
	    int x1 = max(0, p.x - radius);
	    int y1 = max(0, p.y - radius);

	    int x2 = min(thresh.cols - 1, p.x + radius);
	    int y2 = min(thresh.rows - 1, p.y + radius);

	    int width = x2 - x1 + 1;
	    int height = y2 - y1 + 1;

	    // Safety check
	    if (width <= 0 || height <= 0) {
		return 0;
	    }

	    Rect roi(x1, y1, width, height);

	    // Average brightness of region
	    Scalar avg = mean(thresh(roi));

	    // Return average grayscale value
	    return (int)avg[0];
	};

	int abovecolor = sampleRegion(above);
	int rightcolor = sampleRegion(right);
	int belowcolor = sampleRegion(below);
	int leftcolor = sampleRegion(left);

	circle(saveimg, center, 3, Scalar(255,0,255), -1); //center - pink
	circle(saveimg, above, 3, Scalar(0,0,255), -1); // above - red
	circle(saveimg, right, 3, Scalar(0,255,255), -1); //right - yellow
	circle(saveimg, below, 3, Scalar(255,0,0), -1); //below - blue
	circle(saveimg, left, 3, Scalar(255,255, 0), -1); //left - teal
	
	printf("Right Color: %d", rightcolor);
	printf("\tLeft Color: %d", leftcolor);
	
	imshow("w/ GS contours", saveimg);
	
	if (belowcolor > 200) {
	    continue;
	}
	if(abovecolor > 200 && rightcolor > 200 && leftcolor < 200) {
	    leftcount++;
	}
	if(abovecolor > 200 && leftcolor > 200 && rightcolor < 200) {
	    rightcount++;
	}
	//else {
	  //  result = 0;
	//}
	
	if (center.y > 130 && center.y < 200) {
	    inmiddle = true;
	}
        if (center.y > 140) {
	    dinmiddle = true;
	}
    }
    
    printf("Amount of Left Contours: %d\n", leftcount);
    printf("Amount of Right Contours: %d\n", rightcount);
    printf("In Middle: %d", inmiddle);
    printf("Din Middle: %d", dinmiddle);
		
    if (leftcount && rightcount && dinmiddle) {
	printf("doublegreen\n");
	result = 3;
    }
    else if(leftcount && inmiddle && (rightcount == 0)) {
	printf("left\n");
	/*if(!dinmiddle){
	    fright = 10;
	    fleft = 10;
	    loop();
	}
	else {*/
	    result = 1;
	//}
    }
    else if(rightcount && inmiddle && (leftcount == 0)) {
	printf("right\n");
	/*if(!dinmiddle){
	    fright = 10;
	    fleft = 10;
	    loop();
	}
	else {*/
	    result = 2;
	//}
    }
    else {
	result = 0;
    }
	
}

//---------------------------------------------------------------------------------------------
void linetrace(Mat img)
{
    //imshow("base image", img);

    Mat thresh = img.clone();
    Mat landrimg = img.clone();
    Mat saveimg = img.clone();
    Mat greenimg = img.clone();

    dilate(thresh, thresh, getStructuringElement(MORPH_RECT, Size(21, 21)));
    erode(thresh, thresh, getStructuringElement(MORPH_RECT, Size(21, 21)));

    Mat PIDimg = thresh.clone();

    cvtColor(thresh, thresh, COLOR_BGR2HSV);
    medianBlur(thresh, thresh, 5);
    inRange(thresh, Scalar(0, 0, 0), Scalar(180, 115, 115), thresh);
    
    Mat topThresh = thresh(Rect(0, 0, 319, 49));
    Mat bottomThresh = thresh(Rect(boffset.x, boffset.y, 319, 49));
    //Mat leftThresh = thresh(Rect(0, 0, 49, 239));
    //Mat rightThresh = thresh(Rect(270, 0, 49, 239));
    Mat leftThresh = thresh(Rect(loffset.x, loffset.y, 99, 139));
    Mat rightThresh = thresh(Rect(roffset.x, roffset.y, 99, 139));
    Mat thinLeft = thresh(Rect(0, 0, 19, 239));
    Mat thinRight = thresh(Rect(300, 0, 19, 239));;
    //imshow("black_and_white_image", thresh);

    vector<vector<Point>> contours;
    vector<vector<Point>> contourTop;
    vector<vector<Point>> contourBottom;
    vector<vector<Point>> contourLeft;
    vector<vector<Point>> contourRight;
    vector<vector<Point>> contourThinLeft;
    vector<vector<Point>> contourThinRight;

    findContours(thresh, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    findContours(topThresh, contourTop, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    findContours(bottomThresh, contourBottom, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    findContours(leftThresh, contourLeft, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    findContours(rightThresh, contourRight, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    findContours(thinLeft, contourThinLeft, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    findContours(thinRight, contourThinRight, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	drawContours(saveimg, contours, -1, Scalar(255, 0, 0), 3);
	drawContours(saveimg, contourTop, -1, Scalar(0, 255, 0), 3);
	drawContours(saveimg, contourBottom, -1, Scalar(0, 255, 255), 3, LINE_8, noArray(), INT_MAX, boffset);
	drawContours(saveimg, contourLeft, -1, Scalar(0, 0, 255), 3, LINE_8, noArray(), INT_MAX, loffset);
	//drawContours(saveimg, contourRight, -1, Scalar(255, 0, 255), 3, LINE_8, noArray(), INT_MAX, Point(270, 0));
    drawContours(saveimg, contourRight, -1, Scalar(255, 0, 255), 3, LINE_8, noArray(), INT_MAX, roffset);

    greenSquare(greenimg);
	
    top = false;
    if ((int)contourTop.size() > 0)
    {
        vector<Point> lineTop = *max_element(contourTop.begin(), contourTop.end(), contour_cmp);
        printf("\nContour Area of Top: %f\n", contourArea(lineTop));
        if (contourArea(lineTop) > 1500)
        {
            PID(contourTop, PIDimg, 1);
            top = true;
            turningRight = false;
            turningLeft = false;
        }
    }

    if ((int)contourLeft.size() > 0)
    {
        backwards = true;
        leftSize = contourArea(*max_element(contourLeft.begin(), contourLeft.end(), contour_cmp));
    }
    if ((int)contourRight.size() > 0)
    {
        backwards = true;
        rightSize = contourArea(*max_element(contourRight.begin(), contourRight.end(), contour_cmp));
    }

    if (((int)contourLeft.size() > 0 && (int)contourBottom.size() > 0 && !top)  && (!turningRight || (leftSize > rightSize)))
    //else if(lr_cmp) //returns true for turning LEFT
    {
        turningLeft = true;
        //if (contourArea(contourLeft[0]) > 1000)
        //{
            //stopMotors = true;
        //}
        vector<Point> lineLeft = *max_element(contourLeft.begin(), contourLeft.end(), contour_cmp);
        vector<Point> lineBottom = *max_element(contourBottom.begin(), contourBottom.end(), contour_cmp);

        int lcx = getCenter(lineLeft, saveimg, loffset)[0];
        int lcy = getCenter(lineLeft, saveimg, loffset, 1)[1];

        int bcx = getCenter(lineBottom, saveimg, boffset)[0];
        int bcy = getCenter(lineBottom, saveimg, boffset, 1)[1];

        line(saveimg, Point(lcx + loffset.x, lcy + loffset.y), Point(bcx + boffset.x, bcy + boffset.y), Scalar(255, 0, 255), 2, cv::LINE_AA);

        fleft = -targetSpeed;
        fright = targetSpeed;
    }
    if ((int)contourRight.size() > 0 && (int)contourBottom.size() > 0 && !top && (!turningLeft || (rightSize > leftSize)))
    //else if(!lr_cmp) //returns false for turning RIGHT
    {
        vector<Point> lineBottom = *max_element(contourBottom.begin(), contourBottom.end(), contour_cmp);
        vector<Point> lineRight = *max_element(contourRight.begin(), contourRight.end(), contour_cmp);

        turningRight = true;
        
        int rcx = getCenter(lineRight, saveimg, roffset)[0];
        int rcy = getCenter(lineRight, saveimg, roffset, 1)[1];

        int bcx = getCenter(lineBottom, saveimg, boffset)[0];
        int bcy = getCenter(lineBottom, saveimg, boffset, 1)[1];

        line(saveimg, Point(rcx + roffset.x, rcy + roffset.y), Point(bcx + boffset.x, bcy + boffset.y), Scalar(255, 0, 255), 2, cv::LINE_AA);
        
        fleft = targetSpeed;
        fright = -targetSpeed;

    }

    if((int)contourTop.size() > 0 && ((int)contourThinRight.size() || (int)contourThinLeft.size()) && (result == 0) && (int)contourLeft.size() == 0 && (int)contourRight.size() == 0){
        fleft = targetSpeed;
        fright = targetSpeed;
    }

    if((int)contourTop.size() == 0 && (int)contourRight.size() == 0 && (int)contourLeft.size() == 0){
        printf("\n------------GAPS!-------------\n");
        if (contours.size()) {
            if (backwards){
                do
                {
                    Mat newimg = thresh_image_proccessing();
                    Mat newTopimg = newimg(Rect(0, 0, 319, 49));
                    findContours(newimg, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
                    findContours(newTopimg, contourTop, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
                    PID(contours, PIDimg, -1);
                    loop();
                } while (contourTop.size() == 0);
                do
                {
                    Mat newimg = thresh_image_proccessing();
                    Mat newTopimg = newimg(Rect(0, 0, 319, 49));
                    findContours(newimg, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
                    findContours(newTopimg, contourTop, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
                    PID(contours, PIDimg, 1);
                    loop();
                } while (contourTop.size() > 0);

                backwards = false;
            }
            else {
                fright = targetSpeed;
                fleft = targetSpeed;
            }
        }
        else {
            fright = targetSpeed;
            fleft = targetSpeed;
        }
    }
	
    imshow("savedimage", saveimg);

    loop();
    result = 0;
}

//---------------------------------------------------------------------------------------------

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
    init();

    printf("gp20 found");

    Mat bottomimg;
    Mat frontimg;
    
    int index = 0;
    
    for (auto& cam : cameras){
	cam.options->camera = index++;
	cam.options->video_width = 1640; //320
	cam.options->video_height = 1232; //240
	cam.options->framerate = 30;
	cam.options->verbose = true;
	cam.startVideo();
    }

    namedWindow("firstWindow", WINDOW_NORMAL);

    printf("RX LENGTH: %d\n", rx_length);

    unsigned char rx_buffer[256];

    while (rx_length <= 0)
    {
        rx_length = read(uart0_filestream, (void*)rx_buffer, 255);
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

    while (rx_length != 11)
    {
        if (!cameras[0].getVideoFrame(bottomimg, 99999999))
        {
            printf("!ERROR!\n");
            break;
        }
	if (!cameras[1].getVideoFrame(frontimg, 99999999))
        {
            printf("!ERROR!\n");
            break;
        }

        resize(bottomimg, bottomimg, Size(320, 240));

        //camera was flipped in new position
        //flip(bottomimg, bottomimg, -1);
        
        linetrace(bottomimg);

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

        int test = read(uart0_filestream, (void*)rx_buffer, 255);

        if (test > 0 || test == 2)
        {
            rx_length = test;
        }

        printf("rxlength: %d\n", rx_length);
    }

    cameras[0].stopVideo();
    cameras[1].stopVideo();
    destroyAllWindows();

    return 0;
}
