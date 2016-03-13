#include <opencv2/core/core.hpp>
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/calib3d/calib3d_c.h"
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/imgproc/detail/distortion_model.hpp"
#include "SerialClass.h"
#include <opencv/cv.hpp>
#include <iostream>
#include <stdlib.h>
#include <Windows.h>
#include <string.h>
#include <time.h>
#include <cctype>
#include <iterator>

using namespace cv;
using namespace std;

#define SCANINCREMENT 30;

char key;
int thresh = 127;
int max_thresh = 255;
RNG rng(12345);

char outdata[64];
string servocomm = "";

bool target_found = false;
bool target_centered = false;

int cornersH = 6;
int cornersV = 9;
int numSquares = cornersH*cornersV;

double thetapPxH, thetapPxW;

// These will have to change depending on what the previous angle was set to.
double PanAngle = 90.0;
double TiltAngle = 90.0;

unsigned int PanWord = 0;
unsigned int TiltWord = 0;

double NewPanDelta = 0;
double NewTiltDelta = 0;
double OldPanDelta = 0;
double OldTiltDelta = 0;

Point view_center;
Point target_confirmed_points;

double pan_increment = (double)180.0 / 1024.0;
double tilt_increment = (double)180.0 / 1024.0;

time_t start_time;
time_t end_time;

// For current target size, 5.5 inches in diameter,
// 79 inches is the maximum detectable range.
// 30 ft = 360 inches
// So target will have to be at least 4.55x wider.
// Target must be at least 25 inches across

enum { DETECTION = 0, CAPTURING = 1, CALIBRATED = 2 };
enum Pattern { CHESSBOARD, CIRCLES_GRID, ASYMMETRIC_CIRCLES_GRID };

static double computeReprojectionErrors(
	const vector<vector<Point3f> >& objectPoints,
	const vector<vector<Point2f> >& imagePoints,
	const vector<Mat>& rvecs, const vector<Mat>& tvecs,
	const Mat& cameraMatrix, const Mat& distCoeffs,
	vector<float>& perViewErrors)
{
	vector<Point2f> imagePoints2;
	int i, totalPoints = 0;
	double totalErr = 0, err;
	perViewErrors.resize(objectPoints.size());

	for (i = 0; i < (int)objectPoints.size(); i++)
	{
		projectPoints(Mat(objectPoints[i]), rvecs[i], tvecs[i],
			cameraMatrix, distCoeffs, imagePoints2);
		err = norm(Mat(imagePoints[i]), Mat(imagePoints2), NORM_L2);
		int n = (int)objectPoints[i].size();
		perViewErrors[i] = (float)std::sqrt(err*err / n);
		totalErr += err*err;
		totalPoints += n;
	}

	return std::sqrt(totalErr / totalPoints);
}

static void calcChessboardCorners(Size boardSize, float squareSize, vector<Point3f>& corners)
{
	corners.resize(0);

	for (int i = 0; i < boardSize.height; i++)
		for (int j = 0; j < boardSize.width; j++)
			corners.push_back(Point3f(float(j*squareSize),
			float(i*squareSize), 0));

}

static bool runCalibration(vector<vector<Point2f> > imagePoints,
	Size imageSize, Size boardSize,
	float squareSize, float aspectRatio,
	int flags, Mat& cameraMatrix, Mat& distCoeffs,
	vector<Mat>& rvecs, vector<Mat>& tvecs,
	vector<float>& reprojErrs,
	double& totalAvgErr)
{
	cameraMatrix = Mat::eye(3, 3, CV_64F);
	if (flags & CALIB_FIX_ASPECT_RATIO)
		cameraMatrix.at<double>(0, 0) = aspectRatio;

	distCoeffs = Mat::zeros(8, 1, CV_64F);

	vector<vector<Point3f> > objectPoints(1);
	calcChessboardCorners(boardSize, squareSize, objectPoints[0]);
	//bool success = false;
	objectPoints.resize(imagePoints.size(), objectPoints[0]);
	//bool success = true;
	//cout << "Got past the resize" << endl;
	

		double rms = calibrateCamera(objectPoints, imagePoints, imageSize, cameraMatrix,
			distCoeffs, rvecs, tvecs, flags);
	
	///*|CALIB_FIX_K3*/|CALIB_FIX_K4|CALIB_FIX_K5);
	printf("RMS error reported by calibrateCamera: %g\n", rms);

	bool ok = checkRange(cameraMatrix) && checkRange(distCoeffs);
	if (ok) printf("Printing parameters to xml.\n");
	totalAvgErr = computeReprojectionErrors(objectPoints, imagePoints,
		rvecs, tvecs, cameraMatrix, distCoeffs, reprojErrs);

	return ok;
	
}

static void saveCameraParams(const string& filename,
	Size imageSize, Size boardSize,
	float squareSize, float aspectRatio, int flags,
	const Mat& cameraMatrix, const Mat& distCoeffs,
	const vector<Mat>& rvecs, const vector<Mat>& tvecs,
	const vector<float>& reprojErrs,
	const vector<vector<Point2f> >& imagePoints,
	double totalAvgErr)
{
	FileStorage fs(filename, FileStorage::WRITE);

	time_t tt;
	time(&tt);
	struct tm *t2 = localtime(&tt);
	char buf[1024];
	strftime(buf, sizeof(buf) - 1, "%c", t2);

	fs << "calibration_time" << buf;

	if (!rvecs.empty() || !reprojErrs.empty())
		fs << "nframes" << (int)max(rvecs.size(), reprojErrs.size());
	fs << "image_width" << imageSize.width;
	fs << "image_height" << imageSize.height;
	fs << "board_width" << boardSize.width;
	fs << "board_height" << boardSize.height;
	fs << "square_size" << squareSize;

	if (flags & CALIB_FIX_ASPECT_RATIO)
		fs << "aspectRatio" << aspectRatio;

	if (flags != 0)
	{
		sprintf(buf, "flags: %s%s%s%s",
			flags & CALIB_USE_INTRINSIC_GUESS ? "+use_intrinsic_guess" : "",
			flags & CALIB_FIX_ASPECT_RATIO ? "+fix_aspectRatio" : "",
			flags & CALIB_FIX_PRINCIPAL_POINT ? "+fix_principal_point" : "",
			flags & CALIB_ZERO_TANGENT_DIST ? "+zero_tangent_dist" : "");
		//cvWriteComment( *fs, buf, 0 );
	}

	fs << "flags" << flags;

	fs << "camera_matrix" << cameraMatrix;
	fs << "distortion_coefficients" << distCoeffs;

	fs << "avg_reprojection_error" << totalAvgErr;
	if (!reprojErrs.empty())
		fs << "per_view_reprojection_errors" << Mat(reprojErrs);

	if (!rvecs.empty() && !tvecs.empty())
	{
		CV_Assert(rvecs[0].type() == tvecs[0].type());
		Mat bigmat((int)rvecs.size(), 6, rvecs[0].type());
		for (int i = 0; i < (int)rvecs.size(); i++)
		{
			Mat r = bigmat(Range(i, i + 1), Range(0, 3));
			Mat t = bigmat(Range(i, i + 1), Range(3, 6));

			CV_Assert(rvecs[i].rows == 3 && rvecs[i].cols == 1);
			CV_Assert(tvecs[i].rows == 3 && tvecs[i].cols == 1);
			//*.t() is MatExpr (not Mat) so we can use assignment operator
			r = rvecs[i].t();
			t = tvecs[i].t();
		}
		//cvWriteComment( *fs, "a set of 6-tuples (rotation vector + translation vector) for each view", 0 );
		fs << "extrinsic_parameters" << bigmat;
	}

	if (!imagePoints.empty())
	{
		Mat imagePtMat((int)imagePoints.size(), (int)imagePoints[0].size(), CV_32FC2);
		for (int i = 0; i < (int)imagePoints.size(); i++)
		{
			Mat r = imagePtMat.row(i).reshape(2, imagePtMat.cols);
			Mat imgpti(imagePoints[i]);
			imgpti.copyTo(r);
		}
		fs << "image_points" << imagePtMat;
	}
}

static bool readStringList(const string& filename, vector<string>& l)
{
	l.resize(0);
	FileStorage fs(filename, FileStorage::READ);
	if (!fs.isOpened())
		return false;
	FileNode n = fs.getFirstTopLevelNode();
	if (n.type() != FileNode::SEQ)
		return false;
	FileNodeIterator it = n.begin(), it_end = n.end();
	for (; it != it_end; ++it)
		l.push_back((string)*it);
	return true;
}

static bool readXMLList(const string& filename, 
	//const vector<vector<Point2f> >& imagePoints, 
	Mat& cameraMatrix, Mat& distCoeffs)
{
	//l.resize(0);
	FileStorage fs(filename, FileStorage::READ);
	if (!fs.isOpened())
		return false;
	FileNode n = fs.getFirstTopLevelNode();
	if (n.type() != FileNode::SEQ)
		return false;
	//fs["image_points"] >> imagePoints;
	fs["camera_matrix"] >> cameraMatrix;
	fs["distortion_coefficients"] >> distCoeffs;
	fs.release();
	/*FileNodeIterator it = n.begin(), it_end = n.end();
	for (; it != it_end; ++it)
		l.push_back((string)*it);
	return true;*/
	return true;
}

static bool runAndSave(const string& outputFilename,
	const vector<vector<Point2f> >& imagePoints,
	Size imageSize, Size boardSize, float squareSize,
	float aspectRatio, int flags, Mat& cameraMatrix,
	Mat& distCoeffs, bool writeExtrinsics, bool writePoints)
{
	vector<Mat> rvecs, tvecs;
	vector<float> reprojErrs;
	double totalAvgErr = 0;

	bool ok = runCalibration(imagePoints, imageSize, boardSize, squareSize,
		aspectRatio, flags, cameraMatrix, distCoeffs,
		rvecs, tvecs, reprojErrs, totalAvgErr);
	printf("%s. avg reprojection error = %.2f\n",
		ok ? "Calibration succeeded" : "Calibration failed",
		totalAvgErr);

	if (ok)
		saveCameraParams(outputFilename, imageSize,
		boardSize, squareSize, aspectRatio,
		flags, cameraMatrix, distCoeffs,
		writeExtrinsics ? rvecs : vector<Mat>(),
		writeExtrinsics ? tvecs : vector<Mat>(),
		writeExtrinsics ? reprojErrs : vector<float>(),
		writePoints ? imagePoints : vector<vector<Point2f> >(),
		totalAvgErr);
	return ok;
}


int main()
{
	
	// Creates window to display camera view
	namedWindow("camera_feed", WINDOW_AUTOSIZE);
	namedWindow("thresholded_view", WINDOW_AUTOSIZE);
	namedWindow("HLS view", WINDOW_AUTOSIZE);
	Mat frame;
	Serial *SP = new Serial(_T("COM4"));
	if (SP->IsConnected()){
		cout << "Arduino connected." << endl;
		Sleep(3000);
	}
	else {
		cerr << "Could not connect to Arduino." << endl;
		Sleep(3000);
		return -1;
	}

	// Creates videocapture object to capture from camera
	VideoCapture capture;
	if (!capture.open(1)) {
		cerr << "No camera stream found.";
		//return -1;
	}
	const string outputFilename = "calibration.xml";
	//CvCapture * capture = cvCaptureFromCAM(CV_CAP_ANY);
	Size cb_size = Size(cornersV, cornersH);
	//int numBoards =4;
	int H, W, i, j;
	int mode = DETECTION;
	int delay = 10;
	clock_t prevTimestamp = 0;

	Mat cameraMatrix, distCoeffs;
	vector<vector<Point3f>> cb_points;
	vector < vector<Point2f>> image_points;

	//Mat intrinsic = Mat(3, 3, CV_32FC1);
	//Mat distCoeffs;
	vector<Mat> rvecs;
	vector<Mat> tvecs;

	//intrinsic.ptr<float>(0)[0] = 1;
	//intrinsic.ptr<float>(1)[1] = 1;
	double pixel_size = 1.34/1000;
	double aperW = 1920 * pixel_size;
	double aperH = 1080 * pixel_size;
	double hfov, vfov;
	double focalLength;
	Point2d princiPoint;
	double aspectRatio = 4/3;
	double squareSize = 1.0;

	int nframes = 6;
	int flags = CALIB_FIX_ASPECT_RATIO;

	capture >> frame;
	H = frame.rows;
	W = frame.cols;
	//vector<Point3f> checkboard;
	/*for (i = 0; i < numSquares; i++) {
		checkboard.push_back(Point3f(i / cornersH, i % cornersH, 0.0f));
	}
	*/
	Size imsize(frame.cols, frame.rows);
	// First, check that a calibration xml exists.
	bool loadfromxml = readXMLList(outputFilename, cameraMatrix, distCoeffs);
	cout << "Loaded from xml : " << loadfromxml << endl;

	// If not, run calibration routine.
	if (!loadfromxml) {
		while (1) {
			Mat gray_frame;
			vector<Point2f> corners;
			bool blink = false;

			capture >> frame;

			cvtColor(frame.clone(), gray_frame, CV_BGR2GRAY);
			bool found = findChessboardCorners(frame.clone(), cb_size, corners, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FAST_CHECK | CV_CALIB_CB_NORMALIZE_IMAGE);
			if (found) {
				cornerSubPix(gray_frame, corners, Size(11, 11), Size(-1, -1), TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 30, 0.1));
			}

			if (mode == CAPTURING && found &&
				(!capture.isOpened() || clock() - prevTimestamp > delay*1e-3*CLOCKS_PER_SEC))
			{
				image_points.push_back(corners);
				prevTimestamp = clock();
				blink = capture.isOpened();
			}

			if (found)
				drawChessboardCorners(frame, cb_size, Mat(corners), found);

			string msg = mode == CAPTURING ? "100/100" :
				mode == CALIBRATED ? "Calibrated" : "Press 'g' to start";
			int baseLine = 0;
			Size textSize = getTextSize(msg, 1, 1, 1, &baseLine);
			Point textOrigin(frame.cols - 2 * textSize.width - 10, frame.rows - 2 * baseLine - 10);

			if (mode == CAPTURING)
			{
					msg = format("%d/%d", (int)image_points.size(), nframes);
			}

			putText(frame, msg, textOrigin, 1, 1,
				mode != CALIBRATED ? Scalar(0, 0, 255) : Scalar(0, 255, 0));


			if (blink)
				bitwise_not(frame, frame);

			/*if (mode == CALIBRATED)
			{
			Mat temp = frame.clone();
			//undistort(temp, frame, cameraMatrix, distCoeffs);
			}*/
			imshow("camera_feed", frame);
			//
			key = cvWaitKey(50);
			if (char(key) == 27)
				return 0;

			if (capture.isOpened() && key == 'g')
			{
				mode = CAPTURING;
				image_points.clear();
			}
			//key = cvWaitKey(0);
			//if (char(key) = 27) return 0;

			/*if (char(key) = ' ' && found != 0) {
				image_points.push_back(corners);
				cb_points.push_back(checkboard);
				hits++;
				if (hits >= numBoards) break;
				}*/

			if (mode == CAPTURING && image_points.size() > nframes)
			{

				if (runAndSave(outputFilename, image_points, imsize,
					cb_size, squareSize, aspectRatio,
					flags, cameraMatrix, distCoeffs,
					true, true))
					//if (ok)
					mode = CALIBRATED;
				else
					mode = DETECTION;
				if (!capture.isOpened())
					break;
			}
			if (mode == CALIBRATED) {
				cout << "Camera successfully calibrated!" << endl;
				Sleep(3000);
				break;
			}

		}
	}
	cout << "Camera calibrated and calibration information saved." << endl;
	string msg = "Camera calibrated and calibration information saved.";
	int baseLine = 0;
	Size textSize = getTextSize(msg, 1, 1, 1, &baseLine);
	Point textOrigin(frame.cols - 2 * textSize.width - 10, frame.rows - 2 * baseLine - 10);
	putText(frame, msg, textOrigin, 1, 1, Scalar(0, 255, 0));

	
	// Get FOV and a few other parameters using the new camera matrix
	calibrationMatrixValues(cameraMatrix, imsize, aperH, aperW, hfov, vfov, focalLength, princiPoint, aspectRatio);
	
	double NewTiltAngle = 0.0;
	double NewPanAngle = 0.0;
	int inc = 1;
	bool send_success;
	bool begin_wait = false;
	target_found = false;
	target_centered = false;

	vector<Point> found_points;
	int counts = 0;

	PanWord = (int)round((double)PanAngle / pan_increment);
	TiltWord = (int)round((double)TiltAngle / tilt_increment);
	cout << "Initializing position." << endl;
	cout << "PanWord:";
	cout << PanWord << endl;
	servocomm += to_string(PanWord);
	servocomm += ",";
	cout << "TiltWord:";
	cout << TiltWord << endl;
	servocomm += to_string(TiltWord);
	servocomm += "\n";
	for (int i = 0; i < servocomm.length(); i++) {
		outdata[i] = servocomm[i];
	}
	send_success = SP->WriteData(outdata, servocomm.length());
	if (send_success) cout << "Commands successful!" << endl;
	else cout << "Commands not successful." << endl;
	for (int i = 0; i < servocomm.length(); i++) {
		outdata[i] = 0;
	}
	cout << "servocomm: ";
	cout << servocomm << endl;
	//cout << data << endl;
	servocomm = "";

	time(&start_time);
	
	while (1) {
		time(&end_time);
		if (end_time - start_time > 1)
			begin_wait = false;

		Mat frame_ud;
		Mat I2;
		// Put camera capture into matrix object
		capture >> frame;
		if (frame.empty()) break;

		undistort(frame, frame_ud, cameraMatrix, distCoeffs);

		// Convert screencap into HLS from RGB
		cvtColor(frame_ud, I2, CV_BGR2HLS);
		medianBlur(I2, I2, 5);
		//imshow("HLS view", I2);
		H = I2.rows;
		W = I2.cols;
		// Mark center on camera view
		view_center.x = round(W / 2);
		view_center.y = round(H / 2);
		circle(frame, view_center, 4, (255, 0, 0), -1);
		Mat I3(H,W,CV_8UC1);

		thetapPxH = vfov / H;
		thetapPxW = hfov / W;

		// Mask out any non-green objects in the frame and
		// return a thresholded image
		// Hue = [0:180]
		// Lightness = [0:255]
		// Saturation = [0:255]

		// Green hues located between 60 and 90
		for (i = 0; i < H; i++) {
			for (j = 0; j < W; j++) {
				Vec3b hls = I2.at<Vec3b>(i, j);
				uchar hue = hls.val[0];
				uchar light = hls.val[1];
				uchar satu = hls.val[2];
				//I2.at<uchar>((i*W + j) * 3 + 0)
				if (hue < 50 || hue > 90
					|| light < 25 || light > 200
					|| satu < 30 || satu > 220)
				{
					I3.at<uchar>(i,j) = 0;
				}
				else
					I3.at<uchar>(i,j) = 255;
			}
		}

		imshow("thresholded_view", I3);

		vector<vector<Point>>contours;
		vector<Vec4i> hierarchy;
		
		
		//Mat contours = I3.clone();
		//medianBlur(I3, I3, 3);
		//Canny(I3, I3, 1, 127, 3);
		/*int erosion_size = 0;
		Mat element = getStructuringElement(MORPH_RECT,
			Size(2 * erosion_size + 1, 2 * erosion_size + 1),
			Point(erosion_size, erosion_size));
		erode(I3, I3, element);
		*/

		// Finds contours in the masked image
		findContours(I3.clone(),contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

		// Draw contours on the frame
		for (i = 0; i< contours.size(); i++)
		{
			Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
			drawContours(frame, contours, i, color, 1, 8, hierarchy, 0, Point());
			//drawContours(I3, contours, i, color, 1, 8, hierarchy, 0, Point());
		}
		
		
		vector<Moments> momnts(contours.size());
		vector<Point2f> center(contours.size());
		vector<Point2f> center_circle(contours.size());
		vector<float>radii(contours.size());
		int num_contours = contours.size();
		// Now search for circles in the image and find their centers
		if (!contours.empty()) {
			int z = 0;
			for (i = 0; i < num_contours; i++) {
				// If the number of vertices in the contour exceeds 9, you
				// can safely assume that is a circle.
				if (contours[i].size()>9) {
					momnts[i] = moments(contours[i], false);
					if (momnts[i].m00 != 0) {
						//cout << "z = " << z << " , m00 = " << momnts[i].m00 << endl;
						//cout << "z = " << z << " , m10/m00 = " << momnts[i].m10/momnts[i].m00 << endl;
						//cout << "z = " << z << " , m01/m00 = " << momnts[i].m01 / momnts[i].m00 << endl;
						// Find image moments to calculate the x and y coordinates
						// of the center.
						float xin = momnts[i].m10 / momnts[i].m00;
						float yin = momnts[i].m01 / momnts[i].m00;
						center[z] = Point2f(xin, yin);
						//center[z].x = momnts[i].m10 / momnts[i].m00;
						//center[z].y = momnts[i].m01 / momnts[i].m00;
						minEnclosingCircle(contours[i], center_circle[i], radii[z]);
						z++;
					}
				}
			}
			//imshow("thresholded_view", I3);
			//cout << "z = " << z << endl;
			//cout << "centerx1 = " << center[0].x << ", centery1 = " << center[1].y << endl;
			//cout << "centerx1 = " << center[1].x << ", centery1 = " << center[1].y << endl;
			//cout << "centerx1 = " << center[1].x << ", centery1 = " << center[1].y << endl;

			Point centerfin;
			bool found = false;
			int count;
			vector<float>radius_target(z);
			// Now verify that the circles make up the bullseye
			for (j = 0; j < z; j++) {
				centerfin = center[j];
				//cout << "center.x" << z << " = " << centerfin.x << ", center.y" << z << " = " << centerfin.y << endl;
				count = 0;
				if (centerfin.x > 0 && centerfin.y > 0) {
					for (i = 0; i < z; i++) {
						// If the centers are within 2% of each other,
						// assume they are the same point
						if (abs((center[i].x - centerfin.x) / centerfin.x) < 0.02 &&
							abs((center[i].y - centerfin.y) / centerfin.y) < 0.02)
						{
							radius_target[count] = radii[i];
							count++;
						}
						// If found circles exceeds 3, assume target found.
						if (count > 4) {
							found = true;
							break;
						}
					}
					if (found) break;
				}
			}
			// If target was found, mark it on frame and say where it was found.
			if (found) {
				Point targetpoint;
				targetpoint.x = (int)centerfin.x;
				targetpoint.y = (int)centerfin.y;
				circle(frame, targetpoint, 4, (0, 0, 255), -1);
				
				cout << "\rTarget found! Center at ( " << targetpoint.x << " , " << targetpoint.y << " )" << endl;
				if (abs(targetpoint.x - view_center.x) < 3 && abs(targetpoint.y - view_center.y) < 3) {
					cout << "Camera locked on target at " << targetpoint.x << " , " << targetpoint.y << "! Initiating firing procedure!" << endl;
					target_centered = true;
				}
				else target_centered = false;

				//target_confirmed_points = targetpoint;
				NewPanDelta = thetapPxW*round(view_center.x - targetpoint.x);
				NewTiltDelta = thetapPxH*round(view_center.y - targetpoint.y);
				if (!begin_wait) {
					NewPanAngle = (PanAngle + NewPanDelta);
					PanWord = (int)round((double)NewPanAngle / pan_increment);
					NewTiltAngle = (TiltAngle + NewTiltDelta);
					TiltWord = (int)round((double)NewTiltAngle / tilt_increment);
					
					//begin_wait = true;
				}


				string msg = format("New pan angle: %f (%d), New Tilt Angle: %f (%d)", PanAngle, PanWord, TiltAngle, TiltWord);
				int baseLine = 0;
				Size textSize = getTextSize(msg, 1, 1, 1, &baseLine);
				Point textOrigin(frame.cols - 2 * textSize.width + 500, frame.rows - 2 * baseLine - 10);
				//cout << "Set Pan Angle to : " << PanAngle << " , " << PanWord << endl;
				//cout << "Set Tilt Angle to : " << TiltAngle << " , " << TiltWord << endl;
				putText(frame, msg, textOrigin, 1, 1, Scalar(0, 255, 0));

				target_found = true;

				/*if (found_points.size()<5)
					found_points.push_back(targetpoint);
				else {
					
					for (i = 0; i < found_points.size(); i++) {
						for (j = 0; i < found_points.size(); i++) {
							if (abs(found_points[i].x - found_points[j].x) < 5 &&
								abs(found_points[i].y - found_points[j].y) < 5)
								counts++;
							if (counts >= 3) {
								target_confirmed_points = found_points[i];
								NewPanDelta = thetapPxW*round(target_confirmed_points.x - view_center.x);
								NewTiltDelta = thetapPxH*round(target_confirmed_points.y - view_center.y);
								if (!begin_wait) {
									NewPanAngle = (PanAngle + NewPanDelta);
									PanWord = (int)round((double)NewPanAngle / pan_increment);
									NewTiltAngle = (TiltAngle + NewTiltDelta);
									TiltWord = (int)round((double)NewTiltAngle / tilt_increment);
									time(&start_time);
									//begin_wait = true;
								}


								string msg = format("New pan angle: %f (%d), New Tilt Angle: %f (%d)", PanAngle, PanWord, TiltAngle, TiltWord);
								int baseLine = 0;
								Size textSize = getTextSize(msg, 1, 1, 1, &baseLine);
								Point textOrigin(frame.cols - 2 * textSize.width + 500, frame.rows - 2 * baseLine - 10);
								//cout << "Set Pan Angle to : " << PanAngle << " , " << PanWord << endl;
								//cout << "Set Tilt Angle to : " << TiltAngle << " , " << TiltWord << endl;
								putText(frame, msg, textOrigin, 1, 1, Scalar(0, 255, 0));

								target_found = true;
								break;
							}
						}
						if (counts >= 3) {
							found_points.clear();
							counts = 0;
							break;
						}
					}
				}*/

				//OldPanDelta = NewPanDelta;
				//OldTiltDelta = NewTiltDelta;
				
				
				//PanAngle = PanAngle + NewPanAngle;
				//cout << "Increase pan angle by " << NewPanDelta << endl;
				//cout << "Increase tilt angle by " << NewTiltDelta << endl;

				
			}
			// Otherwise, say no target was found
			else {
				target_found = false; // cout << "\rObjects processed. Target not found..." << endl;
			}
		}
		// If no green contours detected, say nothing found.
		else {
			cout << "\rNo green objects detected. Target not found..." << endl;
			
		}
		if (target_found == false && SP->IsConnected() && begin_wait == false) {
			if (PanAngle > 145.0 || PanAngle < 35.0)
				inc = -1*inc;
			PanAngle = PanAngle + inc*SCANINCREMENT;
			//TiltAngle = (double)TiltWord*tilt_increment;
			PanWord = PanAngle / pan_increment;
			//TiltAngle = TiltAngle / tilt_increment;
			//cout << "PanWord:";
			//cout << PanWord << endl;
			servocomm += to_string(PanWord);
			servocomm += ",";
			//cout << "TiltWord:";
			//cout << TiltWord << endl;
			servocomm += to_string(TiltWord);
			servocomm += "\n";
			for (int i = 0; i < servocomm.length(); i++) {
				outdata[i] = servocomm[i];
			}
			send_success = SP->WriteData(outdata, servocomm.length());
			if (send_success) cout << "Commands successful!" << endl;
			else cout << "Commands not successful." << endl;
			for (int i = 0; i < servocomm.length(); i++) {
				outdata[i] = 0;
			}
			cout << "servocomm: ";
			cout << servocomm << endl;
			servocomm = "";

			
			time(&start_time);
			begin_wait = true;
		}
		//time(&end_time);
		
		if (target_found == true && SP->IsConnected() && begin_wait == false) {
			cout << "PanWord:";
			cout << PanWord << endl;
			servocomm += to_string(PanWord);
			servocomm += ",";
			cout << "TiltWord:";
			cout << TiltWord << endl;
			servocomm += to_string(TiltWord);
			servocomm += "\n";
			for (int i = 0; i < servocomm.length(); i++) {
				outdata[i] = servocomm[i];
			}
			send_success = SP->WriteData(outdata, servocomm.length());
			if (send_success) cout << "Commands successful!" << endl;
			else cout << "Commands not successful." << endl;
			for (int i = 0; i < servocomm.length(); i++) {
				outdata[i] = 0;
			}
			cout << "servocomm: ";
			cout << servocomm << endl;
			servocomm = "";

			PanAngle = (double)PanWord*pan_increment;
			TiltAngle = (double)TiltWord*tilt_increment;
			time(&start_time);
			begin_wait = true;
		}

		//cout << "Center at " << view_center.x << " , " << view_center.y << endl;
		imshow("camera_feed", frame);
		key = cvWaitKey(10);
		if (char(key) == 27) break;

	}
	//cvReleaseCapture(&capture);
	
	cvDestroyWindow("camera_feed");
	cvDestroyWindow("thresholded_view");
	capture.release();
	return 0;
}

/*
Original Python code:
=======================================================================
def target_acquisition():
	I_1 = imrotate(IP.imresize(IP.imread('E:/Franco/Documents/Schoolwork/Final Project/007.jpg'),0.15),90)
	I2 = cv2.cvtColor(I_1,cv2.COLOR_RGB2HLS)
	h,w,hsi = I2.shape
	I3 = IP.zeros((h,w),np.uint8)
	for i in range (0,h):
		for j in range (0,w):
			if I2[i,j,0] < 60 or I2[i,j,0] > 90 or I2[i,j,1] < 25 or I2[i,j,1] > 105 or I2[i,j,2] < 30 or I2[i,j,2] > 160:
				I3[i,j] = 0
			else:
				I3[i,j] = 255 #I2[i,j,:]

	_, contours, hierarchy = cv2.findContours( I3.copy(), cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
	
	cx = zeros(len(contours))
	cy = zeros(len(contours))
	z = 0
	for i in range (0,len(contours)):
		if (len(contours[i])>6):
			M[z] = cv2.moments(contours[i])
		if (M[z]['m00']!=0):
			cx[z] = M[z]['m10']/(M[z]['m00'])
			cy[z] = M[z]['m01']/(M[z]['m00'])
			z+=1
	for i in range (0,z):
		cxfin = cx[i]
		cyfin = cy[i]
		count = 0
		found = False
		for j in range (0,z):
			if (abs((cx[j]-cxfin)/cxfin) < 0.05 and abs((cy[j]-cyfin)/cyfin) < 0.05):
				count+=1
			if count>4:
				found=True
				break
		if found==True:
		break
	if (found==True):
        cv2.circle(I_1,(int(cxfin),int(cyfin)),4,(0,0,255),-1)
        imshow(I_1)
        print (int(cxfin),int(cyfin))
    else:
        print ("Target not found.")

*/
