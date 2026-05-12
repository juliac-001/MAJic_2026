#include <lccv.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/ocl.hpp>

#include <iostream>

#include <fstream>


using namespace cv;
using namespace std;

const float SCORE_THRESHOLD = 0.35;
const float NMS_THRESHOLD = 0.35;


cv::Size2f imageShape(cv::Size(320,320));

std::vector<std::string> class_list{"alive_victim", "az", "dead_victim", "dz"};

std::string camTitle(int num){
    std::string title("Video");
    title += std::to_string(num);
    return title;
}

bool is_cuda = false;

int main() {
	//dnn::Net net;
	//auto result = cv::dnn::readNetFromONNX("/home/pi/Jeremy/yolo/model100.onnx");
	
	// can use just "readNet" and it will auto recognize the onnx format
	//cv::dnn::Net net = cv::dnn::readNetFromONNX("/home/pi/Jeremy/yolo/model100.onnx");
	//cv::dnn::Net net = cv::dnn::readNet("/home/pi/Jeremy/yolo/model100.onnx");
	
	
	//cv::dnn::Net net = cv::dnn::readNet("/home/pi/Jeremy/yolo/model100_gpu_test.onnx");
	//cv::dnn::Net net = cv::dnn::readNet("/home/pi/Downloads/evac_room_7_17.onnx");
	cv::dnn::Net net = cv::dnn::readNet("/home/pi/Downloads/2026evac_5-10.onnx");

	if(net.empty()) {
		std::cerr << "Failed to load model\n";
		return -1;
	}

	//cv::dnn::Net net = cv::dnn::readNet("/home/pi/Downloads/2026evac_5-10.onnx");
	
	
	
	if(is_cuda){
		net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
		net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
	}
	else{
		net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
		net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
	}
	
	//net = result;   // why
	
	// camera stuff
	lccv::PiCamera cameras[1];
    int index = 0;
    for (auto& cam : cameras){
        cam.options->camera = index++;
        cam.options->video_width=320;//640;
        cam.options->video_height=240;//480;
        cam.options->framerate=30;
        cam.options->verbose=true;
        cam.startVideo();
        auto title = camTitle(cam.options->camera);
        std::cout << title << " Started:"<< std::endl;
        cv::namedWindow(title,cv::WINDOW_NORMAL);
    }

	// create image matrix
	Mat img;
	while(true){
		// new ppi 5 version
		int counter = 0;
		for (auto& cam : cameras){
            auto title = camTitle(cam.options->camera);
            if(!cam.getVideoFrame(img,99999999)){
                std::cout<<"Timeout error " << title << "\t" <<counter <<std::endl;
                counter++;            
            }
            else{
                //cv::imshow(title.data(),img);
            }
		}
		
		flip(img, img, -1);
		blur( img, img, Size( 3, 3 ), Point(-1,-1) );
		
		//resize(img, img, Size(320,240), 0, 0, INTER_LINEAR);
		
		
		Mat blob;		
		// does necessary scaling and stuff for NN usage
		dnn::blobFromImage(img, blob, 1.0/255.0, imageShape, Scalar(), true, false);
		//dnn::blobFromImage(img, blob, 1.0, imageShape);//, Scalar(), true, false);
		//dnn::blobFromImage(img, blob, 1.0/255, imageShape, Scalar(0,0,0), false, false, CV_32F);
		
		
		vector<Mat> outputs;

		int frameCount = 0;
		// inside while loop:
		if (frameCount++ % 10 == 0) {  // only run inference every 5 frames
			if (blob.rows > 0){
				net.setInput(blob);
				net.forward(outputs, net.getUnconnectedOutLayersNames());
			}
		}
		/*
		
		// some info about the output
		//int rows = outputs[0].size[2];
		//int dimensions = outputs[0].size[1];
		int rows = outputs[0].size[1];
		int dimensions = outputs[0].size[2];
		if(dimensions > rows){
			printf("\n\nYOLO 8\n\n");
			
		}
		else{
			printf("\n\nYOLO 5\n\n");
		}
		//printf("Rows: %5d\tDimensions: %5d\n", rows, dimensions);     // 2100 rows and 8 dimensions
		
		// swap for yolov8 
		rows = outputs[0].size[2];
		dimensions = outputs[0].size[1];
		
		// changing around dimensions (it changed for no reason in yolov8 so we need to switch it back)
		outputs[0] = outputs[0].reshape(1, dimensions);
		cv::transpose(outputs[0], outputs[0]);
		
		float *data = (float *)outputs[0].data;
		
		vector<int> class_ids;
		vector<float> confidences;
		vector<Rect> boxes;
		float x_factor = img.cols / imageShape.width;
		float y_factor = img.rows / imageShape.height;
		
		
		// loop through all the rows in the results
		
		
		for(int i = 0; i < rows; i++){
			float *classes_scores = data+4;
			
			
			
			Mat scores(1, class_list.size(), CV_32FC1, classes_scores);
			Point class_id;
			double maxClassScore;
			
			
			minMaxLoc(scores, 0, &maxClassScore, 0, &class_id);
			
			
			
			if(maxClassScore > 0.5){//SCORE_THRESHOLD){
				printf("i = %d\tscore: %.2f\n", i, maxClassScore);
				
				confidences.push_back(maxClassScore);
				class_ids.push_back(class_id.x);
				
				printf("class id: %d\n", class_id.x);
				
				float x = data[0];
				float y = data[1];
				float w = data[2];
				float h = data[3];
				
				//printf("x,y => %.2f, %.2f\n", x, y);
				
				
				
				
				int left = x * img.cols - w * img.cols / 2;
				int top = y * img.rows - h * img.rows / 2;
				
				int width = int(w * img.cols);
				int height = int(h * img.rows);
				
				//int left = int((x - 0.5 * w) * x_factor);
				//int top = int((y - 0.5 * h) * y_factor);
				
				//int width = int(w * x_factor);
                //int height = int(h * y_factor);


				boxes.push_back(Rect(left, top, width, height));
				//rectangle(img, Rect(left, top, width, height), Scalar(0,255,0), 1);
			}			data += dimensions;
		}
		
		printf("does it make it this far?????");
		
		// all done, time to draw
		vector<int> nms_result;
		dnn::NMSBoxes(boxes, confidences, SCORE_THRESHOLD, NMS_THRESHOLD, nms_result);
		
		
		for(unsigned long i = 0; i < nms_result.size(); i++){
			int idx = nms_result[i];
			rectangle(img, boxes[idx], Scalar(0,255,0), 1);
			putText(img, to_string( int(confidences[idx] * 100)) + "% " + class_list[class_ids[idx]], Point(boxes[idx].x, boxes[idx].y), 1, 3, Scalar(0,255,0), 2);
		}
		
		
		
		*/


		imshow("main", img);
		
		char input = waitKey(1);
		
		if(input == 'q'){
			break;
		}
		
		
	}
	
	
	// loop ended, clean up time
	// close cameras and stuff
	for (auto& cam : cameras){
        auto title = camTitle(cam.options->camera);
        
        std::cout << title << std::endl;
        //sleep(2);
        cv::destroyWindow(title);
        //sleep(2);
        cam.stopVideo();
        //sleep(2);
    }
	
	
	
	
	
	return 0;
}
