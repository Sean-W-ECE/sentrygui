#ifndef RECOGNITION_H
#define RECOGNITION_H

#include <QObject>
#include <qcoreapplication.h>
#include <qimage.h>
#include <opencv2/core/core.hpp>
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/calib3d/calib3d_c.h"
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/imgproc/detail/distortion_model.hpp"
#include <opencv/cv.hpp>
#include <iostream>
#include <stdlib.h>
#include <Windows.h>
#include <string.h>
#include <time.h>
#include <cctype>
#include <iterator>
#include "SerialClass.h"
#include "compensator.h"
#include "compData.h"

class recognition : public QObject
{
	Q_OBJECT

public:
	recognition(QObject *parent);
	~recognition();
	void init();
	int calibrate();

private:
	cv::Mat frame;
	cv::Mat I2;
	Serial *SP;
	compensator *comp;

	/*Calibration Values*/
	cv::Mat cameraMatrix, distCoeffs;
	double hfov, vfov;

	void sendFrame(cv::Mat& frame);
	void moveTurret();

signals:
	//void done();
	void sendImage(QImage image);
	void sendCamStatus(QString text);
	void sendConsoleText(QString text);
	void sendInit();
	void sendCalibrated();
	void sendModeStatus(bool mode);
	
public slots:
	void startCalibrate();
	void process();
	void endProcess();
	void toggleCapture(bool on);
	void reset();
	void shotFeedback(int stat);
};

#endif // RECOGNITION_H
