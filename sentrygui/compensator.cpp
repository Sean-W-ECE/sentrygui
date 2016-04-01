#include "compensator.h"

using namespace std;

#define TILTSIZE 1024
#define RANGESIZE 4001

//file that data is read from / stored to
fstream srcFile;

compensator::compensator()
{
	//init matrix to NULL
	compensation = NULL;
}


compensator::~compensator()
{
	if (compensation != NULL)
	{
		for (int i = 0; i < TILTSIZE; i++)
		{
			delete[] compensation[i];
			compensation[i] = NULL;
		}
		delete[] compensation;
		compensation = NULL;
	}
	if (srcFile.is_open())
	{
		srcFile.close();
	}
}

/*
	init()

	Initialization routine
	Loads or creates compensation matrix
*/
void compensator::init()
{
	//only run if compensation matrix not initialized
	if (compensation == NULL)
	{
		//allocate the matrix
		compensation = new int*[TILTSIZE];
		for (int i = 0; i < TILTSIZE; i++)
		{
			compensation[i] = new int[RANGESIZE];
		}
		
		//try to read from file to fill matrix
		srcFile.open("compensation.txt");
		if (srcFile.is_open())
		{
			string line;
			while (getline(srcFile, line))
			{
				//TODO: parse line
			}
		}
		//file not found, init matrix to 0
		else
		{
			for (int i = 0; i < TILTSIZE; i++)
			{
				for (int j = 0; j < RANGESIZE; j++)
				{
					compensation[i][j] = 0;
				}
			}
		}
	}
}

/*
	compensate(unsigned int TiltWord, unsigned int Range)

	Returns the compensation angle for given tilt and range
*/
compData compensator::compensate(unsigned int TiltWord, unsigned int Range)
{
	//create return object
	compData retVal = compData();
	//error checking
	if (TiltWord > (TILTSIZE - 1) || Range > (RANGESIZE - 1))
	{
		//status = 1 means input error
		retVal.Tilt = 0;
		retVal.status = 1;
	}
	else
	{
		//if not in error, look up modifier in table
		int mod = compensation[TiltWord][Range];
		//if mod is positive or 0, add to TiltWord, but bound to TILTSIZE
		if (mod >= 0)
		{
			retVal.Tilt = TiltWord + mod;
			if (!(retVal.Tilt < TILTSIZE))
			{
				retVal.Tilt = TILTSIZE - 1;
				retVal.status = 10;
			}
		}
		//if mod is negative, subtract from TiltWord, but bound to 0
		else
		{
			mod = abs(mod);
			if (mod > TiltWord)
			{
				retVal.Tilt = 0;
				retVal.status = 11;
			}
			else
			{
				retVal.Tilt = TiltWord - mod;
			}
		}
	}
	return retVal;
}