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
	void sendTurretAngles(double pan, double tilt);
	void startProcess();
	void endProcess();

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

	//setup for thread
	void setup();

	//function to update the UI
	int updateUI();
	//send movement command to Arduino
	int moveTurret();

public slots:
	//receive Mat from thread
	void receiveImage(QImage image);
	//receive status messages about camera from recog
	void updateCamStatus(int stat);
	//receive console text and output to console pane
	void printConsole(QString text);
};

#endif // SENTRYGUI_H
