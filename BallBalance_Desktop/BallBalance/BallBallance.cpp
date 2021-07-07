//#define DEBUG
#define windowname "Ball Ballance Bot - University of Ruse 'Angel Kunchev' by Berran Remzi"
#define windowname2 "Ball Ballance Bot - University of Ruse 'Angel Kunchev' by Berran Remzi"

//#include <afx.h>

#include <iostream>
#include <atlstr.h>
#include <cstring>
#include <sstream>
#include <math.h>
#include "opencv2/opencv.hpp"

using namespace cv;
using namespace std;

bool setAngles(int corX, int corY, CString PortSpecifier)
{
	stringstream ss("");
	string buff;

	ss << corX << " " << corY << "\n";
	buff = ss.str();
	
	DCB dcb;
	DWORD byteswritten;
	bool retVal = 0;
	CString data;
	HANDLE hPort = CreateFile(
		PortSpecifier,
		GENERIC_READ | GENERIC_WRITE, 
		0, 
		0, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		0);

	if (!GetCommState(hPort, &dcb)){
		wcout << "Can't open serial port " << PortSpecifier.GetString() << endl;
		return false;
	}

	dcb.BaudRate = CBR_9600; //9600 Baud
	dcb.ByteSize = 8; //8 data bits
	dcb.Parity = NOPARITY; //no parity
	dcb.StopBits = ONESTOPBIT; //1 stop
	dcb.fDtrControl = DTR_CONTROL_DISABLE; //DTR_CONTROL_ENABLE, DTR_CONTROL_HANDSHAKE

	if (!SetCommState(hPort, &dcb))
		return false;

	for (size_t i = 0; i < buff.length(); i++){
		data = buff.at(i);
		WriteFile(hPort, data, 1, &byteswritten, NULL);
	}
	CloseHandle(hPort); //close the handle
	return retVal;
}

int main(int argc, char** argv)
{	
	cout << "Ball Ballance Bot - University of Ruse 'Angel Kunchev'" << endl;
	cout << "by Berran Remzi" << endl;

	String arg1;
	CString serialPort;

	char *p;
	int cameraNo = 0;

	if (argc < 3){
		cout << "Command line parameters are missing!!!" << endl;
		cout << "Loading default parameters:" << endl;

		serialPort = "\\\\.\\COM3";
		cameraNo = 0;
	}
	else{
		cout << "Loading command line parameters:" << endl;

		arg1 = "\\\\.\\" + String(argv[1]);
		serialPort= arg1.c_str();
		cameraNo = strtol(argv[2], &p, 10);

	}
	wcout << "   SerialPort(" << serialPort.GetString() <<")"<<endl;
	cout << "   Camera(" << cameraNo <<")"<< endl;

	VideoCapture cap(cameraNo); //capture the video from webcam
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);

	if (!cap.isOpened())  // if not success, exit program
	{
		cout << "Can't open a webcam" << endl;
		cout << "Press ENTER to exit...";
		getchar();
		return -1;
	}
	
	const int rectSide = 70;
	const int circSide = 90;
	const int triSide = 50;

	const int rectDelay = 70;
	const int triDelay = 60;

	int rect[2][4] = { 
		{ 320 - rectSide, 320 + rectSide, 320 + rectSide, 320 - rectSide },
		{ 240 - rectSide, 240 - rectSide, 240 + rectSide, 240 + rectSide }
	};

	int circle[2][37] = {};
	for (int j = 0; j < 37; j++){
		double sinCalc = sin(j * 10 * 3.14 / 180);
		double cosCalc = cos(j * 10 * 3.14 / 180);
		circle[0][j] = (int)(sinCalc*circSide) + 320;
		circle[1][j] = (int)(cosCalc*circSide) + 240;
	}

	
	int triangle[2][3] = {
		{ 320 - (triSide * 2), 320 + (triSide * 2), 320},
		{ 240 - triSide, 240 - triSide, 240 + (triSide*2)}
	};

	int iLowH = 0;
	int iHighH = 179;

	int iLowS = 213;
	int iHighS = 255;

	int iLowV = 70;
	int iHighV = 175;

	int P1000 = 100;
	int I1000 = 10;
	int D100 = 100;
	float P = 0;
	float I = 0;
	float D = 0;

	int sliderX = 320;
	int sliderY = 240;
	int setX = sliderX;
	int setY = sliderY;
	int moveDelay = 0;
	int i = 0;
	int errorX = 0, errorY = 0;
	int lastErrorX = 0, lastErrorY = 0;
	int sumErrorX = 0, sumErrorY = 0;

	int iMax = 1000;
	int iMin = iMax*(-1);
	int iLastX = -1;
	int iLastY = -1;
	int pattern = 1;

	//Create trackbars in "Control" window
	namedWindow(windowname, CV_WINDOW_AUTOSIZE); //create a window called "Control"
	resizeWindow(windowname, 310, 600);

	createTrackbar("LowH", windowname, &iLowH, 179); //Hue (0 - 179)
	createTrackbar("LowS", windowname, &iLowS, 255); //Saturation (0 - 255)
	createTrackbar("LowV", windowname, &iLowV, 255);//Value (0 - 255)
	createTrackbar("Xpos", windowname, &sliderX, 640);

	createTrackbar("HighH", windowname, &iHighH, 179);
	createTrackbar("HighS", windowname, &iHighS, 255);
	createTrackbar("HighV", windowname, &iHighV, 255);
	createTrackbar("Ypos", windowname, &sliderY, 480);

	createTrackbar("P (x0.001)", windowname, &P1000, 150);
	createTrackbar("I (x0.001)", windowname, &I1000, 20);
	createTrackbar("D (x0.01)", windowname, &D100, 150);

	//Прочита временна снимка от камерата
	Mat imgTmp;
	Mat hImg;
	Mat colorThresholded;
	cap.read(imgTmp);

	//Създава черна снимка с размера на изхода на камерата
	Mat imgLines = Mat::zeros(imgTmp.size(), CV_8UC3);

	while (true)
	{
		Mat imgOriginal;

		bool bSuccess = cap.read(imgOriginal); //Прочита нов кадър от камерата

		if (!bSuccess) //Ако не успее да прочете кадър, прекъсваме цикъла
		{
			cout << "Can't read a frame from video stream" << endl;
			cout << "Press ENTER to exit...";
			getchar();
			break;
		}

		Mat imgHSV;
		Mat imgThresholded;

		cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Преобразуваме дадения кадър от BGR тип на HSV тип
		inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image

		//morphological opening (removes small objects from the foreground)
		erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(4, 4)));
		dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(4, 4)));

		//Calculate the moments of the thresholded image
		Moments oMoments = moments(imgThresholded);

		double dM01 = oMoments.m01;
		double dM10 = oMoments.m10;
		double dArea = oMoments.m00;

		iMin = iMax*(-1);

		if (pattern == 1){
			setX = sliderX;
			setY = sliderY;
		}
		if (pattern == 2){
			if (moveDelay > rectDelay){
				setX = rect[0][i];
				setY = rect[1][i];
				i++;
				moveDelay = 0;
				if (i > 3) i = 0;
			}
		}
		if (pattern == 3){
			if (moveDelay > 1){
				setX = circle[0][i];
				setY = circle[1][i];
				i++;
				moveDelay = 0;
				if (i > 35) i = 0;
			}
		}
		if (pattern == 4){
			if (moveDelay > triDelay){
				setX = triangle[0][i];
				setY = triangle[1][i];
				i++;
				moveDelay = 0;
				if (i > 2) i = 0;
			}
		}

		moveDelay++;
		//Ако площа на обекта <=50000, предполагаме, че няма обект в дадения кадър поради наличие на много шум.
		if (dArea > 50000)
		{
			//изчисляваме позицията на топчето
			int posX = (int)(dM10 / dArea);
			int posY = (int)(dM01 / dArea);

			iLastX = posX;
			iLastY = posY;

			//Начало на ПИД алгоритъма, с мащабиране на коефициентите
			P = (float)P1000 / 1000;
			I = (float)I1000 / 1000;
			D = (float)D100 / 100;

			errorX = posX - setX;
			errorY = posY - setY;

			//Формула на ПИД регулатор
			long PIDx = (long)(P * errorX + D * (errorX - lastErrorX) + I * (sumErrorX));
			long PIDy = (long)(P * errorY + D * (errorY - lastErrorY) + I * (sumErrorY));
			PIDy *= -1;

			lastErrorX = errorX;
			lastErrorY = errorY;

			sumErrorX += errorX;
			sumErrorY += errorY;

			//скалиране на интегралната съставка
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

			PIDx = 90 - PIDx;
			PIDy = PIDy + 90;

			//Край на изчисленията на ПИД алгоритъма
			if (pattern != 0){
				setAngles((int)PIDx, (int)PIDy, serialPort);
			}
				
			line(imgOriginal, Point(posX - 30, posY), Point(posX + 30, posY), Scalar(0, 255, 255), 5);
			line(imgOriginal, Point(posX, posY - 30), Point(posX, posY + 30), Scalar(0, 255, 255), 5);
		}

		line(imgOriginal, Point(setX - 30, setY), Point(setX + 30, setY), Scalar(255, 255, 0), 2);
		line(imgOriginal, Point(setX, setY - 30), Point(setX, setY + 30), Scalar(255, 255, 0), 2);
		
		cvtColor(imgThresholded, colorThresholded, CV_GRAY2RGB);
		hconcat(imgOriginal, colorThresholded, hImg);	//Залепя двата кадъра един до друг

		imshow(windowname, hImg);	//Показва оригиналния и филтрирания кадър
		char c = 0;
		c = waitKey(33);
		if (c == 27)				//Изчаква натискане на ESC за 30ms. Ако е натиснат прекъсва главния цикъл 
		{
			cout << "ESC key is pressed by user" << endl;
			break;
		}
		switch (c){
		case '1':
			pattern = 1;
			i = 0;
			cout << "Pattern is disabled" << endl;
			break;
		case '2':
			pattern = 2;
			i = 0;
			cout << "Rectangle pattern is selected" << endl;
			break;
		case '3':
			pattern = 3;
			i = 0;
			cout << "Circle pattern is selected" << endl;
			break;
		case '4':
			pattern = 4;
			i = 0;
			cout << "Triangle pattern is selected" << endl;
			break;
		case '0':
			pattern = 0;
			i = 0;
			cout << "Pattern is disabled!!!" << endl;
			cout << "Select pattern (1, 2, 3, 4) to start balancing!!!" << endl;
			break;
		case '9':
			pattern = 0;
			i = 0;
			cout << "Home position" << endl;
			cout << "Select pattern (1, 2, 3, 4) to start balancing!!!" << endl;
			setAngles(90, 90, serialPort);
			break;
		default: 
			break;
		}
	}
	return 0;
}