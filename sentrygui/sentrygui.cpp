#include "sentrygui.h"
#include "recognition.h"
#include <iostream>
#include <QGraphicsPixmapItem>

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

	//move recog object to thread and start
	recog->moveToThread(thread);
	thread->start();
	//start the process
	emit startProcess();
	
	//create the Pixmap item
	mapItem = new QGraphicsPixmapItem();
	//set view in UI to scene
	ui.camView->setScene(&scene);
	mapItem = new QGraphicsPixmapItem();
	//add item to scene
	scene.addItem(mapItem);
	
	//initialize console
	ui.console->setReadOnly(1);
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

void sentrygui::updateCamStatus(int stat)
{
	if (stat == -1)
	{
		ui.statusDisplay->setText("No Arduino connected");
	}
	if (stat == 0)
	{
		ui.statusDisplay->setText("No camera found");
	}
	if (stat == 1)
	{
		ui.statusDisplay->setText("Calibrating");
	}
	if (stat == 2)
	{
		ui.statusDisplay->setText("Scanning");
	}
	if (stat == 3)
	{
		ui.statusDisplay->setText("Ready!");
	}
	if (stat == 4)
	{
		ui.statusDisplay->setText("Arduino connected");
	}
}