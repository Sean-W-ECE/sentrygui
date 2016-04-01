#include "sentrygui.h"
#include "recognition.h"
#include <iostream>
#include <QGraphicsPixmapItem>

bool initialized = false;
bool calibrated = false;

sentrygui::sentrygui(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	setup();
}

sentrygui::~sentrygui()
{
	//terminate thread
	thread->exit();
}

void sentrygui::setup()
{
	
	thread = new QThread(this);
	//create targeting module in new thread
	recognition* recog = new recognition(NULL);
	//connect basics
	connect(thread, &QThread::finished, recog, &QObject::deleteLater);
	connect(this, &sentrygui::startProcess, recog, &recognition::process);
	connect(this, &sentrygui::endProcess, recog, &recognition::endCapture);
	//link mat to qimage converter
	connect(recog, &recognition::sendImage, this, &sentrygui::receiveImage);
	//attach cam status updater
	connect(recog, &recognition::sendCamStatus, this, &sentrygui::updateCamStatus);
	//attach console text writer
	connect(recog, &recognition::sendConsoleText, this, &sentrygui::printConsole);

	//connect reset button to recognition
	connect(ui.resetButton, &QAbstractButton::released, recog, &recognition::reset);
	//connect calibrate button
	connect(ui.calibrateButton, &QAbstractButton::released, recog, &recognition::startCalibrate);

	//connect init flag
	connect(recog, &recognition::sendInit, this, &sentrygui::getInitialized);
	//connect calibrated flag
	connect(recog, &recognition::sendCalibrated, this, &sentrygui::getCalibration);

	//move recog object to thread and start
	recog->moveToThread(thread);
	thread->start();
		
	//create the Pixmap item
	mapItem = new QGraphicsPixmapItem();
	//set view in UI to scene
	ui.camView->setScene(&scene);
	mapItem = new QGraphicsPixmapItem();
	//add item to scene
	scene.addItem(mapItem);
	
	//initialize console
	ui.console->setReadOnly(1);

	//init the targeting module
	emit targetInit();
	//spin until initialized
	while (!initialized)
	{
		QCoreApplication::processEvents(0);
	}
	//calibrate camera
	emit targetCalibrate();
	while (!calibrated)
	{
		QCoreApplication::processEvents(0);
	}
	//start scanning
	emit startProcess();
}

//receive image, convert to QImage, display to UI
void sentrygui::receiveImage(QImage image)
{
	mapItem->setPixmap(QPixmap::fromImage(image));
}

//print text to console
void sentrygui::printConsole(QString text)
{
	ui.console->append(text);
}

//update the status field
void sentrygui::updateCamStatus(QString text)
{
	ui.statusDisplay->setText(text);
}

//sets the initialized flag
void sentrygui::getInitialized()
{
	initialized = true;
}

//sets the calibrated flag
void sentrygui::getCalibration()
{
	calibrated = true;
}