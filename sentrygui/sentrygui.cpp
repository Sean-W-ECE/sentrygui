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

	//connection for initialization
	connect(this, &sentrygui::targetInit, recog, &recognition::init);
	//connection for calibration
	connect(this, &sentrygui::targetCalibrate, recog, &recognition::calibrate);

	//connect init flag
	connect(recog, &recognition::sendInit, this, &sentrygui::getInitialized);
	//connect calibrated flag
	connect(recog, &recognition::sendCalibrated, this, &sentrygui::getCalibration);
	//connect manual mode
	connect(this, &sentrygui::startManual, recog, &recognition::manual);

	//create the Pixmap item
	mapItem = new QGraphicsPixmapItem();
	//set view in UI to scene
	ui.camView->setScene(&scene);
	mapItem = new QGraphicsPixmapItem();
	//add item to scene
	scene.addItem(mapItem);
	
	//initialize console
	ui.console->setReadOnly(1);
	//move recog object to thread and start
	recog->moveToThread(thread);
	thread->start();
	//init the targeting module
	emit targetInit();
	//Calibration and scan start done via signal chain
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

//once target initialized, emit signal to calibrate
void sentrygui::getInitialized()
{
	emit targetCalibrate();
}

//once calibrated, start idling
void sentrygui::getCalibration()
{
	state = 2;
	emit startProcess();
}