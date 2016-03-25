#include "recognition.h"
#include <iostream>

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

// Creates videocapture object to capture from camera
VideoCapture capture;

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

recognition::recognition(QObject *parent)
	: QObject(parent)
{
	//init serial to null
	SP = NULL;
}

recognition::~recognition()
{
	capture.release();
}

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

void recognition::process()
{
	//initializes serial connection
	Serial *SP = new Serial(_T(L"COM4"));
	while (!(SP->IsConnected()))
	{
		emit sendConsoleText(QString("No Arduino found. Searching..."));
		emit sendCamStatus(-1);
		Sleep(3000);
	}
	//when connected, alert and wait for 3 seconds
	emit sendConsoleText(QString("Arduino connected"));
	emit sendConsoleText(QString("Waiting for 3 seconds"));
	emit sendCamStatus(4);
	Sleep(3000);
	
	//repeatedly check if camera is connected and try to connect
	while (!capture.open(0)) {
		emit sendConsoleText(QString("No camera found. Searching..."));
		emit sendCamStatus(0);
		Sleep(3000);
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
	double pixel_size = 1.34 / 1000;
	double aperW = 1920 * pixel_size;
	double aperH = 1080 * pixel_size;
	double hfov, vfov;
	double focalLength;
	Point2d princiPoint;
	double aspectRatio = 4 / 3;
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
		//tell UI cam is calibrating
		emit sendCamStatus(1);

		int calibrationStarted = 0;
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

			//convert frame to QImage during calibration
			QImage image(frame.data, frame.size().width, frame.size().height, frame.step, QImage::Format_RGB888);
			image = image.rgbSwapped();
			//send frame to UI
			emit sendImage(image);

			//key = cvWaitKey(50);
			//if (char(key) == 27)
			//	return;

			if (capture.isOpened() && calibrationStarted == 0) //start calibration automatically
			{
				mode = CAPTURING;
				image_points.clear();
				calibrationStarted = 1;
			}

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
				emit sendConsoleText(QString("Camera successfully calibrated."));
				emit sendCamStatus(3);
				Sleep(3000);
				break;
			}

		}
	}
	emit sendConsoleText(QString("Camera calibrated and calibration information saved."));
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
	emit sendConsoleText(QString("Initializing position."));
	QString consoleMessage = "PanWord: " + PanWord;
	emit sendConsoleText(QString(consoleMessage));
	/*cout << "PanWord:";
	cout << PanWord << endl;*/
	servocomm += to_string(PanWord);
	servocomm += ",";
	consoleMessage = "TiltWord: " + TiltWord;
	emit sendConsoleText(consoleMessage);
	/*cout << "TiltWord:";
	cout << TiltWord << endl;*/
	servocomm += to_string(TiltWord);
	servocomm += "\n";
	for (int i = 0; i < servocomm.length(); i++) {
		outdata[i] = servocomm[i];
	}
	send_success = SP->WriteData(outdata, servocomm.length());
	if (send_success) emit sendConsoleText(QString("Commands successful"));
	else emit sendConsoleText(QString("Commands failed"));
	for (int i = 0; i < servocomm.length(); i++) {
		outdata[i] = 0;
	}
	emit sendConsoleText(QString("servocomm: ") + QString(servocomm.c_str()));
	//cout << data << endl;
	servocomm = "";

	time(&start_time);

	//tell UI cam is entering scanning mode
	emit sendCamStatus(2);

	//main scanning function
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
		Mat I3(H, W, CV_8UC1);

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
					I3.at<uchar>(i, j) = 0;
				}
				else
					I3.at<uchar>(i, j) = 255;
			}
		}

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
		findContours(I3.clone(), contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

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

				consoleMessage = QString("\rTarget found! Center at ( %1 , %2 )").arg(targetpoint.x,targetpoint.y);
				emit sendConsoleText(consoleMessage);
				if (abs(targetpoint.x - view_center.x) < 3 && abs(targetpoint.y - view_center.y) < 3) {
					consoleMessage = QString("Camera locked on target at %1 , %2! Initiating firing procedure!").arg(targetpoint.x, targetpoint.y);
					emit sendConsoleText(consoleMessage);
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
			}
			// Otherwise, say no target was found
			else {
				cout << "\rObjects processed. Target not found..." << endl;
			}
		}
		// If no green contours detected, say nothing found.
		else {
			emit sendConsoleText(QString("No green objects detected. Target not found..."));
			target_found = false;
		}

		//servo control when no target found
		if (target_found == false && SP->IsConnected() && begin_wait == false) {
			if (PanAngle > 145.0 || PanAngle < 35.0)
				inc = -1 * inc;
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
			if (send_success) emit sendConsoleText(QString("Commands successful"));
			else emit sendConsoleText(QString("Commands failed"));
			for (int i = 0; i < servocomm.length(); i++) {
				outdata[i] = 0;
			}
			consoleMessage = "servocomm: " + QString(servocomm.c_str());
			emit sendConsoleText(consoleMessage);
			servocomm = "";


			time(&start_time);
			begin_wait = true;
		}
		//time(&end_time);

		//servo control when target is found
		if (target_found == true && SP->IsConnected() && begin_wait == false) {
			cout << "PanWord:";
			cout << PanWord << endl;
			consoleMessage = "PanWord:" + PanWord;
			emit sendConsoleText(consoleMessage);
			servocomm += to_string(PanWord);
			servocomm += ",";
			consoleMessage = "TiltWord:" + TiltWord;
			emit sendConsoleText(consoleMessage);
			cout << "TiltWord:";
			cout << TiltWord << endl;
			servocomm += to_string(TiltWord);
			servocomm += "\n";
			for (int i = 0; i < servocomm.length(); i++) {
				outdata[i] = servocomm[i];
			}
			send_success = SP->WriteData(outdata, servocomm.length());
			if (send_success) emit sendConsoleText(QString("Commands successful"));
			else emit sendConsoleText(QString("Commands failed"));
			for (int i = 0; i < servocomm.length(); i++) {
				outdata[i] = 0;
			}
			consoleMessage = "servocomm: " + QString(servocomm.c_str());
			emit sendConsoleText(consoleMessage);
			servocomm = "";

			PanAngle = (double)PanWord*pan_increment;
			TiltAngle = (double)TiltWord*tilt_increment;
			time(&start_time);
			begin_wait = true;
		}

		consoleMessage = QString("Center at %1 , %2").arg(view_center.x, view_center.y);
		emit sendConsoleText(consoleMessage);
		//convert frame to QImage
		QImage image(frame.data, frame.size().width, frame.size().height, frame.step, QImage::Format_RGB888);
		image = image.rgbSwapped();
		//send frame to UI
		emit sendImage(image);

	}
	//execution will never reach here in GUI version
	capture.release();
	return;
}

void recognition::receiveTurretAngles(double pan, double tilt)
{
	//TODO
}