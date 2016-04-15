#include "compensator.h"

using namespace std;

#define TILTSIZE 1024
#define RANGESIZE 801
#define PI 3.14159265

//file that data is read from / stored to
fstream srcFile;
//filename for compensation matrix data
string filename = "compensation.txt";
//int that records the last adjustment direction
int lastDir = 0;
//last adjustment angle
double lastAdj = 0;

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
						compensation[i][j] = 20;// std::numeric_limits<float>::quiet_NaN();
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
					compensation[i][j] = -20;//std::numeric_limits<float>::quiet_NaN();
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

// Performs nearest neighbor interpolation along X and Y axes (bilinear) to
// estimate a value for an uninitialized compensation angle
int compensator::interpolate(int TiltWord, int roundrange)
{
	int mod;
	int nearest[4];
	int i = -1;
	// Find nearest neighbor to the left
	nearest[0] = TiltWord;
	while (TiltWord + i >= 0) {
		if (!isnan(compensation[TiltWord + i][roundrange])) {
			nearest[0] = TiltWord + i;
			break;
		}
		i--;
	}
	i = 1;
	// Find nearest neighbor to the right
	nearest[1] = TiltWord;
	while (TiltWord + i < TILTSIZE) {
		if (!isnan(compensation[TiltWord + i][roundrange])) {
			nearest[1] = TiltWord + i;
			break;
		}
		i++;
	}
	i = -1;
	// Find nearest neighbor to the bottom
	nearest[2] = roundrange;
	while (roundrange + i >= 0) {
		if (!isnan(compensation[TiltWord][roundrange + i])) {
			nearest[2] = roundrange + i;
			break;
		}
		i--;
	}
	i = 1;
	// Find nearest neighbor to the top
	nearest[3] = roundrange;
	while (roundrange + i < RANGESIZE) {
		if (!isnan(compensation[TiltWord][roundrange + i])) {
			nearest[3] = roundrange + i;
			break;
		}
		i++;
	}
	// Calculate distance weighted average
	int num = 0;
	int denum = 0;
	for (i = 0; i < 2; i++) {
		if (nearest[i] != TiltWord) {
			num += (int)compensation[nearest[i]][roundrange] / abs(TiltWord - nearest[i]);
			denum += 1 / abs(TiltWord - nearest[i]);
		}
	}
	for (i = 2; i < 4; i++) {
		if (nearest[i] != roundrange) {
			num += (int)compensation[TiltWord][nearest[i]] / abs(roundrange - nearest[i]);
			denum += 1 / abs(roundrange - nearest[i]);
		}
	}
	if (denum = 0) mod = 0;
	else mod = (int)num / denum;
	return mod;
}


/*
	compensate(unsigned int TiltWord, unsigned int Range)

	Returns the compensation angle for given tilt and range
*/
compData compensator::compensate(unsigned int TiltWord, unsigned int Range)
{
	//create return object
	compData retVal = compData();

	//round range to nearest 5, use as index into matrix
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
			mod = interpolate(TiltWord, roundRange);
		else
			mod = (int)rawVal;

		//add mod to original tilt
		retVal.Tilt = TiltWord + mod;
	}
#if 0
		//if mod is positive or 0, add to TiltWord (+ -> down), but bound to TILTSIZE
		if (mod >= 0)
		{
			retVal.Tilt = TiltWord + mod;
			if (!(retVal.Tilt < TILTSIZE))
			{
				retVal.Tilt = TILTSIZE - 1;
				retVal.status = 10;
			}
		}
		//if mod is negative, still add the value from TiltWord (- -> up, but bound to 0
		else
		{
			if (abs(mod) > TiltWord)
			{
				retVal.Tilt = 0;
				retVal.status = 11;
			}
			else
			{
				retVal.Tilt = TiltWord + mod;
			}
		}
	}
#endif
	return retVal;
}

//Update a compensation value stored in the matrix with new one
void compensator::update(float newCompVal, unsigned int tilt, unsigned int range)
{
	compensation[tilt][range] = newCompVal;
}

/*
   adjust: used during feedback phase to adjust tilt
   comp angle (big changes) = arctan(13.47 / range) 
   comp angle (small changes) = arctan((13.47 / 2) / range)
*/
compData compensator::adjust(unsigned int Tilt, double tiltIncr, unsigned int Range, int dir)
{
	//create return object
	compData retVal = compData();
	double newAngle;
	unsigned int newTilt;

	if (lastDir == 0)
	{
		//Very High
		if (abs(dir) >= 2)
		{
			//move 5 inches up/down on target
			//compute angle difference
			newAngle = atan2(13.47, (double)Range) * 180.0 / PI;
		}
		else if (abs(dir) == 1)
		{
			//move 2.5 inches up/down on target
			//compute angle difference
			newAngle = atan2((13.47 / 2), (double)Range) * 180.0 / PI;
		}
		else
		{
			newAngle = 0.0;
		}
	}
	//otherwise, use lastAdj as basis for future adjustments
	else
	{
		//every time directions switch, divide tilt by 2
		//if same direction as before, divide tilt by 2 if dir lower in magnitude
		if (!samesgn(dir, lastDir) || (abs(dir) < abs(lastDir)))
			newAngle = lastAdj / 2.0;
		else
			newAngle = lastAdj; //same direction & magnitude: keep angle same
	}

	//convert angle to word
	newTilt = (int)round((double)newAngle / tiltIncr);

	//move turret according to dir (<0 = sub, >0 = add)
	if (dir < 0)
	{
		//subtract newTilt from original tilt to make gun go up
		retVal.Tilt = Tilt - newTilt;
		//check for out of bounds
		if (newTilt > Tilt)
		{
			retVal.Tilt = 0;
			retVal.status = 11;
		}
	}
	if (dir > 0)
	{
		//add newTilt from original tilt to make gun go down
		retVal.Tilt = Tilt + newTilt;
		//check for out of bounds
		if (!(retVal.Tilt < TILTSIZE))
		{
			retVal.Tilt = TILTSIZE - 1;
			retVal.status = 10;
		}
	}
	
	//update lastDir and lastAdj
	lastDir = dir;
	lastAdj = newAngle;

	//return 
	return retVal;
}

//returns if 2 numbers have the same sign
//return 1 if same, 0 if different
int compensator::samesgn(int v1, int v2)
{
	int v1sign = (0 < v1) - (v1 < 0);
	int v2sign = (0 < v2) - (v2 < 0);

	return (v1sign == v2sign);
}

//reset the lastDir and lastAdj. Called from recognition
void compensator::resetDir()
{
	lastDir = 0;
	lastAdj = 0;
}