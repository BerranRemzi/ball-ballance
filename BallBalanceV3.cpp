
#define _AFXDLL
#define port "\\\\.\\COM3"
#define _WIN32_WINNT_MAXVER
#define DEBUGError

#include <cstring>
#include <sstream>
#include <afx.h>

#include "stdafx.h"
#include <iostream>
//#include <cv.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"


using namespace cv;
using namespace std;

bool WriteComPort2(string sendData)
{
	CString PortSpecifier = port;
	DCB dcb;
	DWORD byteswritten;
	bool retVal;
	CString data;
	HANDLE hPort = CreateFile(
		PortSpecifier,
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
		);

	if (!GetCommState(hPort, &dcb))
		return false;

	dcb.BaudRate = CBR_115200; //9600 Baud
	dcb.ByteSize = 8; //8 data bits
	dcb.Parity = NOPARITY; //no parity
	dcb.StopBits = ONESTOPBIT; //1 stop
	if (!SetCommState(hPort, &dcb))
		return false;

	for (size_t i = 0; i < sendData.length(); i++){
		//char d;
		data = sendData.at(i); // data
		//cout << sendData.at(i); //gets '2'
		/*retVal = */WriteFile(hPort, data, 1, &byteswritten, NULL);
	}
	//cout << "Serial data : " << sendData;
	CloseHandle(hPort); //close the handle
	return retVal;
}

int main(int argc, char** argv)
{
	VideoCapture cap(0); //capture the video from webcam

	if (!cap.isOpened())  // if not success, exit program
	{
		cout << "Cannot open the web cam" << endl;
		return -1;
	}

	namedWindow("Control", CV_WINDOW_NORMAL); //create a window called "Control"
	namedWindow("Filter", CV_WINDOW_NORMAL); //create a window called "Control"
	resizeWindow("Filter", 400, 480);
	resizeWindow("Control", 400, 480);
	int iLowH = 0;
	int iHighH = 4;

	int iLowS = 255;
	int iHighS = 255;

	int iLowV = 56;
	int iHighV = 255;

	//Create trackbars in "Control" window
	createTrackbar("LowH", "Filter", &iLowH, 179); //Hue (0 - 179)
	createTrackbar("HighH", "Filter", &iHighH, 179);

	createTrackbar("LowS", "Filter", &iLowS, 255); //Saturation (0 - 255)
	createTrackbar("HighS", "Filter", &iHighS, 255);

	createTrackbar("LowV", "Filter", &iLowV, 255);//Value (0 - 255)
	createTrackbar("HighV", "Filter", &iHighV, 255);

	int P10 = 8;
	int I10 = 2;
	int D10 = 10;
	double P = 0;
	double I = 0;
	double D = 0;
	int setX = 387;
	int setY = 252;

	int iMax = 1000;
	int iMin = iMax*(-1);

	createTrackbar("P (x100)", "Control", &P10, 50);
	createTrackbar("I (x100)", "Control", &I10, 20);
	createTrackbar("D (x10)", "Control", &D10, 100);

	createTrackbar("iMax", "Control", &iMax, 2000);

	createTrackbar("X", "Control", &setX, 500);
	createTrackbar("Y", "Control", &setY, 500);

	int iLastX = -1;
	int iLastY = -1;

	//Capture a temporary image from the camera
	Mat imgTmp;
	cap.read(imgTmp);

	//Create a black image with the size as the camera output
	Mat imgLines = Mat::zeros(imgTmp.size(), CV_8UC3);
	Mat imgThresholded = Mat::zeros(imgTmp.size(), CV_8UC3);;
	Mat gray = Mat::zeros(imgTmp.size(), CV_8UC3);

	stringstream ss;
	string buff;

	long ms;
	while (true)
	{
		Mat imgOriginal;

		bool bSuccess = cap.read(imgOriginal); // read a new frame from video


		if (!bSuccess) //if not success, break loop
		{
			cout << "Cannot read a frame from video stream" << endl;
			break;
		}

		Mat imgHSV;

		cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV

		

		inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image

		//morphological opening (removes small objects from the foreground)
		erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(2, 2)));
		
		//Calculate the moments of the thresholded image
		Moments oMoments = moments(imgThresholded);

		double dM01 = oMoments.m01;
		double dM10 = oMoments.m10;
		double dArea = oMoments.m00;

		int errorX, errorY;
		int lastErrorX, lastErrorY;
		int sumErrorX, sumErrorY;

		iMin = iMax*(-1);
		// if the area <= 10000, I consider that the there are no object in the image and it's because of the noise, the area is not zero 
		if (dArea > 50000)
		{
			//calculate the position of the ball
			int posX = dM10 / dArea;
			int posY = dM01 / dArea;

			iLastX = posX;
			iLastY = posY;

			//PID algorith start pos<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

			P = (double)P10 / 100;
			I = (double)I10 / 100;
			D = (double)D10 / 10;

			errorX = posX - setX;
			errorY = -1*(posY - setY);

			//generalized PID formula
			long PIDx = P * errorX + D * (errorX - lastErrorX) + I * (sumErrorX);
			long PIDy = P * errorY + D * (errorY - lastErrorY) + I * (sumErrorY);
			//PIDy *= -1;

#ifdef DEBUGError
			//cout << "PIDx : "<<PIDx << endl;
			cout << "Error : " << sumErrorX << ", " << sumErrorY << endl;
#endif
			lastErrorX = errorX;
			lastErrorY = errorY;

			sumErrorX += errorX;
			sumErrorY += errorY;

			//scale the sum for the integral term
			if (sumErrorX > iMax) {
				sumErrorX = iMax;
			}
			else if (sumErrorX < iMin) {
				sumErrorX = iMin;
			}

			if (sumErrorY > iMax) {
				sumErrorY = iMax;
			}
			else if (sumErrorY < iMin) {
				sumErrorY = iMin;
			}

			if (PIDx > 30)
				PIDx = 30;
			if (PIDx < -30)
				PIDx = -30;

			if (PIDy > 30)
				PIDy = 30;
			if (PIDy < -30)
				PIDy = -30;

			PIDx = PIDx + 90;
			PIDy = PIDy + 90;
			//PID algorithm end
#ifdef DEBUG
			cout << " PWM1 : " << PIDx << " PWM2 : " << PIDy << endl;
			//cout << posX << " " << posY << " " << setX << " " << setY << endl;
			//Serial send <<<<<<<<<<<<<<<<<<<<<
			//cout << "X:" << posX << " Y:" << posY << endl;
#endif
			ss << (int)PIDx << " " << (int)PIDy << "\n";
			buff = ss.str();
			WriteComPort2(buff);
			ss.str("");
			line(imgOriginal, Point(posX - 30, posY), Point(posX + 30, posY), Scalar(0, 255, 255), 5);
			line(imgOriginal, Point(posX, posY - 30), Point(posX, posY + 30), Scalar(0, 255, 255), 5);

			//Serial send end
		}

		line(imgOriginal, Point(setX - 30, setY), Point(setX + 30, setY), Scalar(255, 255, 0), 2);
		line(imgOriginal, Point(setX, setY - 30), Point(setX, setY + 30), Scalar(255, 255, 0), 2);

		imshow("Thresholded", imgThresholded); //show the thresholded image

		imshow("Original", imgOriginal); //show the original image

		if (waitKey(33) == 27) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
		{
			cout << "esc key is pressed by user" << endl;
			break;
		}

	}

	return 0;
}