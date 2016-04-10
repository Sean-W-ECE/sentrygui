#include "compensator.h"

using namespace std;

#define TILTSIZE 1024
#define RANGESIZE 801

//file that data is read from / stored to
fstream srcFile;
//filename for compensation matrix data
string filename = "compensation.txt";

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
	Loads from file or creates compensation matrix
*/
void compensator::init()
{
	//only run if compensation matrix not initialized
	if (compensation == NULL)
	{
		//allocate the matrix
		compensation = new float*[TILTSIZE];
		for (int i = 0; i < TILTSIZE; i++)
		{
			compensation[i] = new float[RANGESIZE];
		}
		
		//try to read from file to fill matrix
		srcFile.open(filename, fstream::in);
		if (srcFile.is_open())
		{
			string line;
			int i = 0; //row iterator (tilt)
			float tempStore;

			while (getline(srcFile, line))
			{
				int j = 0; //column iterator (range)
				string::size_type sz = 0; //size_t
				string::size_type sz2 = sz; //duplicate
				//if i exceeds # of rows, break
				if (i > (TILTSIZE - 1))
					break;
				//convert values in the string to floats
				do
				{
					//if j exceeds # fo columns, break
					if (j > (RANGESIZE - 1))
						break;
					//parse first float appearing in string, save position after end of float
					try
					{
						tempStore = stof(line.substr(sz2), &sz);
						compensation[i][j] = tempStore;
					}
					catch (const invalid_argument& ia)
					{
						//if an invalid argument exception arises, fill in matrix with NAN
						compensation[i][j] = NAN;
					}
					sz2 = sz; //update to compute substring
					j++; //next column
				} while (line[sz] != '\n');
				i++; //next line
			}

			//close file when done reading
			srcFile.close();
		}
		//file not found, init matrix to NAN
		else
		{
			for (int i = 0; i < TILTSIZE; i++)
			{
				for (int j = 0; j < RANGESIZE; j++)
				{
					compensation[i][j] = NAN;
				}
			}
		}
	}
}

/*
	int writeback()

	Writes the compensation table back to file

	Return values:
	0 - success
	1 - file not opened beforehand
	2 - file write failure
*/
int compensator::writeback()
{
	int row;
	int col;
	string out = "";

	//file opened check
	if (srcFile.is_open())
	{
		//move put position to beginning of file
		srcFile.seekp(0);
	}
	else
	{
		//if not open, open it
		srcFile.open(filename, fstream::out);
	}

	//print each row as a line, space delimited
	for (row = 0; row < TILTSIZE; row++)
	{
		//for each row, convert floats to string, then append to out
		//NANs are converted to string "nan" rather than any other form
		for (col = 0; col < (RANGESIZE - 1); col++)
		{
			out += (isnan(compensation[row][col]) ? "NAN" : to_string(compensation[row][col])) + " ";
		}
		//append last column with newline
		out += (isnan(compensation[row][col]) ? "NAN" : to_string(compensation[row][col])) + "\n";
	}
	//write string to file
	srcFile << out;

	//close file
	srcFile.close();

	//successful return
	return 0;
}

/*
	compensate(unsigned int TiltWord, unsigned int Range)

	Returns the compensation angle for given tilt and range
*/
compData compensator::compensate(unsigned int TiltWord, unsigned int Range)
{
	//create return object
	compData retVal = compData();

	//round range to nearest 5
	int roundRange = (Range % 5 < 3) ? (Range / 5) : (Range / 5 + 1);

	//error checking
	if (TiltWord > (TILTSIZE - 1) || Range > (RANGESIZE - 1))
	{
		//status = 1 means input error
		retVal.Tilt = 512; //return to neutral
		retVal.status = 1;
	}
	else
	{
		//if not in error, look up modifier in table
		float rawVal = compensation[TiltWord][roundRange];
		//modifier to be added to tilt
		int mod;
		//check for NAN
		if (isnan(rawVal))
			mod = 0;
		else
			mod = (int)rawVal;

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