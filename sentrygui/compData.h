#pragma once

/*
	compData

	Class used to contain data returned from compensator
	Includes new tilt value from compensator and status messages
*/
class compData
{
public:
	compData();
	~compData();
	//new tilt value
	unsigned int Tilt;
	//int for status messages
	//0 means ok
	//1 means input error
	//10 means output is capped by upper tilt bound
	//11 means output is capped by lower tilt bound
	int status;
};

