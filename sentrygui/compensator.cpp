#include "compensator.h"

using namespace std;

#define TILTSIZE 1024
#define RANGESIZE 2048

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
		compensation = new double*[TILTSIZE];
		for (int i = 0; i < TILTSIZE; i++)
		{
			compensation[i] = new double[RANGESIZE];
		}
		
		//try to read from file to fill matrix
		fstream srcFile;
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