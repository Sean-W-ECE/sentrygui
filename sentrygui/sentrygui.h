#ifndef SENTRYGUI_H
#define SENTRYGUI_H

#include <QtWidgets/QWidget>
#include <qimage.h>
#include <qthread.h>
#include "ui_sentrygui.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv/cv.hpp>

class sentrygui : public QWidget
{
	Q_OBJECT

public:
	sentrygui(QWidget *parent = 0);
	~sentrygui();

signals:
	void targetInit();
	void targetCalibrate();
	void startProcess();
	void switchCapture(bool on);

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
};

#endif // SENTRYGUI_H
