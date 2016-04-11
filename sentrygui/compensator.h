#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include "compData.h"

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
	int writeback();
	compData compensate(unsigned int TiltWord, unsigned int Range);

private:
	//matrix of base compensation values.
	//Stored values are modifiers on input values
	//will be [tilt][distance]
	float** compensation;
	int interpolate(int TiltWord, int roundrange);
};