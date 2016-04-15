#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
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
	void update(float newCompVal, unsigned int tilt, unsigned int range);
	compData adjust(unsigned int Tilt, double tiltIncr, unsigned int Range, int dir);
	void resetDir();

private:
	//matrix of base compensation values.
	//Stored values are modifiers on input values
	//will be [tilt][distance]
	float** compensation;
	//find nearest initialied neighbor
	int interpolate(int TiltWord, int roundrange);
	//get sign of number
	int samesgn(int v1, int v2);
};