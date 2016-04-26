#ifndef SENTRYGUI_H
#define SENTRYGUI_H

#include <QtWidgets/QWidget>
#include <qimage.h>
#include <qthread.h>
#include <QKeyEvent>
#include "ui_sentrygui.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv/cv.hpp>
#include <string.h>
#include <stdlib.h>

class sentrygui : public QWidget
{
	Q_OBJECT

public:
	sentrygui(QWidget *parent = 0);
	~sentrygui();

signals:
	void targetInit();
	void targetCalibrate();
	void startCalibration();
	void startProcess();
	void endProcess();
	void switchCapture(bool on);
	void shotFeedback(int stat);

private:
	Ui::sentryguiClass ui;
	//thread for the recognition function
	QThread* thread;
	//scene to be displayed
	QGraphicsScene scene;
	//pixmap from image
	QPixmap map;
	//item for scene
	QGraphicsPixmapItem *mapItem;

	//state of operation
	//0 = setup/init, 1 = manual, 2 = auto
	int state = 0;

	//setup for thread
	void setup();
	
protected:
	void keyPressed(QKeyEvent*);

public slots:
	//receive Mat from thread
	void receiveImage(QImage image);
	//receive status messages about camera from recog
	void updateCamStatus(QString text);
	//receive console text and output to console pane
	void printConsole(QString text);
	//receive initialization condition
	void getInitialized();
	//receive calibrated status from targeting module
	void getCalibration();
	//handler for STOP/START button
	void stopstart();
	//updates the stop/start button and mode displays
	void updateMode(bool mode);
	//handle user feedback from GUI buttons
	void feedbackHandler(QAbstractButton*);
	//update UI displays for turret position
	void updateTurret(double tilt, double pan, bool linked);
	//update range
	void updateRange(unsigned int r, int valid);
};

#endif // SENTRYGUI_H
