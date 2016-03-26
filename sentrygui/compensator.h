#pragma once
#include <iostream>
#include <fstream>
#include <string>

/*
	Compensator module
	
	Used to calculate adjustments for target distance
	and miss correction
*/
class compensator
{
public:
	compensator();
	~compensator();
	void init();

private:
	//matrix of base compensation values
	//will be [tilt][distance]
	double** compensation;
};