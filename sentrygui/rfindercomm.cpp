#include <iostream>
#include <Windows.h>
#include <stdio.h>
#include "SerialClass.h"
#include <string>
#include <tchar.h>
#include <iostream>

#define RFIND_BYTES 6

using namespace std;
/* Weird things with this code:
	The first and second readings are always correct,
	but the third, fourth, and fifth readings are not.

	Therefore, a potential way around this is to run the reading
	algorithm is to call it five times, and taking the first reading
	returned.

	I have no idea why this happens, but I'm still looking into it.
*/

/* Update 3/30/16, 1:25 PM
	I think I figured out the issue. It wasn't reading enough
	bytes and the unread bytes accumulated in the serial port.
*/

int process() {
	string dist = "";
	char read_rfinder[2];
	string data = "";
	bool send_success;
	char outdata[16];
	char distance[16];
	int reads = 0;
	long unsigned int tar_dist = 0;
	cout << "Arduino communication test" << endl;
	Serial *SP = new Serial(_T(L"COM4"));
	if (SP->IsConnected())
		cout << "Arduino connected." << endl;
	Sleep(5000);
	while (SP->IsConnected()) {
		// Take input from user; you can change this to a bool.
		// If 1, the Arduino will read a new distance and return it.
		// If 0, the Arduino will not read a new distance and return the existing one.
		cin >> read_rfinder;

		// If the user inputs x on the keyboard, ends the program.
		// This is to make sure the Arduino disconnects from serial.
		if ((char)read_rfinder[0] == 120) break;

		data += read_rfinder;
		data += "\n";
		for (int i = 0; i < data.length(); i++) {
			outdata[i] = data[i];
		}
		send_success = SP->WriteData(outdata, data.length());
		if (send_success) {
			cout << "Byte successfully sent!" << endl;
		}
		else cout << "Send command not successful." << endl;
		for (reads = 0; reads < 1; reads++) {
			// Sends the user byte to the Arduino.
			
			Sleep(100);

			// Now Arduino will read the distance from the sensor
			// and return it.
			send_success = SP->ReadData(distance, RFIND_BYTES);
			if (send_success) {
				cout << "Distance read successful!" << endl;
				int j = 0;
				for (int i = 0; i < RFIND_BYTES; i++) {
					if (distance[i] != '\n') {
						dist += distance[i];
						j++;
					}
					//cout << distance[i];
				}
				//cout << endl;
			}
			else cout << "Receive command not successful." << endl;
			/*for (int i = 0; i < data.length(); i++) {
				outdata[i] = 0;
				}*/
			//cout << data << endl;
			data = "";
			Sleep(700);

			// Due to the issue mentioned above, the program will only
			// keep the first value returned through serial.
			if (reads == 0) {
				//cout << "dist : " << dist << endl;
				tar_dist = stol(dist, nullptr, 10);
				cout << "Distance taken: " << tar_dist << endl;
			}
			//reads++;
			dist = "";
		}
	}
	// Disconnect from Arduino to free the serial port
	SP->~Serial();
	return 0;
}