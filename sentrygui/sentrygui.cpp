#include "sentrygui.h"
#include "recognition.h"
#include <iostream>
#include <QGraphicsPixmapItem>

bool initialized = false;
bool calibrated = false;
bool capturing = false;

sentrygui::sentrygui(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	setup();
}

sentrygui::~sentrygui()
{
	//stop targeting
	emit endProcess();
	//terminate thread
	thread->exit();
}

void sentrygui::setup()
{
	
	thread = new QThread(this);
	//create targeting module in new thread
	recognition* recog = new recognition(NULL);
	
	/* Connections for Signals and Slots */

	//connect basics
	connect(thread, &QThread::finished, recog, &QObject::deleteLater);
	connect(this, &sentrygui::endProcess, recog, &recognition::endProcess);
	//link mat to qimage converter
	connect(recog, &recognition::sendImage, this, &sentrygui::receiveImage);
	//attach cam status updater
	connect(recog, &recognition::sendCamStatus, this, &sentrygui::updateCamStatus);
	//attach console text writer
	connect(recog, &recognition::sendConsoleText, this, &sentrygui::printConsole);

	/* Button Connections */
	//connect reset button to recognition
	connect(ui.resetButton, &QAbstractButton::released, recog, &recognition::reset);
	//connect calibrate button directly to recognition
	connect(ui.calibrateButton, &QAbstractButton::released, recog, &recognition::startCalibrate);
	//connect start/stop button to GUI
	connect(ui.stopButton, &QAbstractButton::released, this, &sentrygui::stopstart);
	//connect feedback buttons to GUI
	connect(ui.feedbackGroup, static_cast<void(QButtonGroup::*)(QAbstractButton*)>(&QButtonGroup::buttonReleased), this, &sentrygui::feedbackHandler);

	//connection for initialization
	connect(this, &sentrygui::targetInit, recog, &recognition::init);
	//connection for calibration
	connect(this, &sentrygui::targetCalibrate, recog, &recognition::calibrate);
	//connect start/stop button to calibration when starting up
	connect(this, &sentrygui::startCalibration, recog, &recognition::startCalibrate);
	//connection for starting/stopping autoprocess
	connect(this, &sentrygui::startProcess, recog, &recognition::process);
	connect(this, &sentrygui::switchCapture, recog, &recognition::toggleCapture);

	/* Startup Signal Chain */
	//connect init flag
	connect(recog, &recognition::sendInit, this, &sentrygui::getInitialized);
	//connect calibrated flag
	connect(recog, &recognition::sendCalibrated, this, &sentrygui::getCalibration);
	//connect user feedback system
	connect(this, &sentrygui::shotFeedback, recog, &recognition::shotFeedback);

	/* Graphics Stuff */
	//create the Pixmap item
	mapItem = new QGraphicsPixmapItem();
	//set view in UI to scene
	ui.camView->setScene(&scene);
	mapItem = new QGraphicsPixmapItem();
	//add item to scene
	scene.addItem(mapItem);
	
	/* GUI Console */
	//initialize console
	ui.console->setReadOnly(1);
	//move recog object to thread and start
	recog->moveToThread(thread);
	thread->start();
	//init the targeting module
	emit targetInit();
	//Calibration and scan start done via signal chain
}

//FUNCTION: register key press events
void sentrygui::keyPressed(QKeyEvent* ev)
{
	//press g to start calibration process after init
	if (ev->key() == Qt::Key_G)
	{
		printConsole(QString("G pressed"));
		emit targetCalibrate();
	}
	//press space as test
	if (ev->key() == Qt::Key_Space)
	{
		printConsole(QString("Space was pressed"));
	}
}

//SLOT: receive image, convert to QImage, display to UI
void sentrygui::receiveImage(QImage image)
{
	mapItem->setPixmap(QPixmap::fromImage(image));
}

//SLOT: print text to console
void sentrygui::printConsole(QString text)
{
	ui.console->append(text);
}

//SLOT: update the status field
void sentrygui::updateCamStatus(QString text)
{
	ui.statusDisplay->setText(text);
}

//SLOT: once target initialized, emit signal to calibrate
void sentrygui::getInitialized()
{
	capturing = true;
	emit targetCalibrate();
}

//SLOT: once calibrated, start scanning
void sentrygui::getCalibration()
{
	state = 2;
	ui.stopButton->setText(QString("STOP"));
	emit startProcess();
}

//SLOT: if s is true, set capturing to true, if s is false, capturing = false
void sentrygui::stopstart()
{
	//if not calibrated yet, then pressing button will begin calibration
	if (!calibrated)
	{
		//emit startCalibration();
	}
	else
	{
		if (capturing == false)
		{
			capturing = true;
			ui.stopButton->setText(QString("START"));
			emit switchCapture(true);
		}
		else
		{
			capturing = false;
			ui.stopButton->setText(QString("STOP"));
			emit switchCapture(false);
		}
	}
}

//SLOT: handles button pressed from the feedback buttons
//param is pointer to the button that was pressed
void sentrygui::feedbackHandler(QAbstractButton* qb)
{
	//get text of button
	QString txt = qb->text();
	//handle Hit clicked
	if (!txt.compare(QString("HIT"), Qt::CaseSensitive))
	{
		//send 0 to recog shotFeedback
		emit shotFeedback(0);
	}
	//handle Very Low clicked
	if (!txt.compare(QString("VERY LOW"), Qt::CaseSensitive))
	{
		//send -2 to recog shotFeedback
		emit shotFeedback(-2);
	}
	//handle Low clicked
	if (!txt.compare(QString("LOW"), Qt::CaseSensitive))
	{
		//send -1 to recog shotFeedback
		emit shotFeedback(-1);
	}
	//handle High clicked
	if (!txt.compare(QString("HIGH"), Qt::CaseSensitive))
	{
		//send 1 to recog shotFeedback
		emit shotFeedback(1);
	}
	//handle Very High clicked
	if (!txt.compare(QString("VERY HIGH"), Qt::CaseSensitive))
	{
		//send 2 to recog shotFeedback
		emit shotFeedback(2);
	}
}