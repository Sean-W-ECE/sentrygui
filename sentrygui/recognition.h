#ifndef RECOGNITION_H
#define RECOGNITION_H

#include <QObject>
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
#include "compData.h"

class recognition : public QObject
{
	Q_OBJECT

public:
	recognition(QObject *parent);
	~recognition();
	/*static double computeReprojectionErrors(const vector<vector<Point3f> >& objectPoints,
		const vector<vector<Point2f> >& imagePoints,
		const vector<Mat>& rvecs, const vector<Mat>& tvecs,
		const Mat& cameraMatrix, const Mat& distCoeffs,
		vector<float>& perViewErrors);*/

private:
	cv::Mat frame;
	cv::Mat I2;
	Serial *SP;

signals:
	//void done();
	void sendImage(QImage image);
	void sendCamStatus(int stat);
	void sendConsoleText(QString text);
	
public slots:
	void process();
	void endCapture();
};

#endif // RECOGNITION_H
