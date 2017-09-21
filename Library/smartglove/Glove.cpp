#include "Glove.h"

Glove::Glove(std::string _UUIDstr, int _ID)
{
	//set UUID and glove ID
	UUIDstr = _UUIDstr;
	ID = _ID;

	parseUUID(UUIDstr);
}

Glove::~Glove()
{
	//close the BLE connection
	CloseHandle(pHandle);
	//clear and free the vectors
	minValues.clear();
	maxValues.clear();
	stretchRaw.clear();
	imuRaw.clear();
}

void Glove::parseUUID(std::string _UUID)
{
	//first three digits indicate IMU
	//000 = no IMU, 006 = IMU
	if (_UUID.substr(1, 3) == "006") hasIMU = true;
	else hasIMU = false;
	//initialize IMUraw even if no IMU exists, just in case
	imuRaw = std::vector<unsigned short>(6, 0);
	//next three digits indicate number of sensors
	//005 = 5 sensors, 010 = 10 sensors
	sensorCount = std::stoi(_UUID.substr(4, 3));
	//initialize stretch sensor inputs accordingly
	stretchRaw = std::vector<unsigned short>(sensorCount, 0);
	minValues = std::vector<unsigned short>(sensorCount, USHRT_MAX);
	maxValues = std::vector<unsigned short>(sensorCount, 0);
	minAngles = std::vector<double>(15, 0);
	maxAngles = std::vector<double>(15, 1);
	stretch = std::vector<double>(sensorCount, 0);
	//next two digits are service number
	//remaining UUID data is meaningless
}

void Glove::setAngles(double* minimum, double* maximum)
{
	minAngles.clear();
	for (int i = 0; i < 15; i++)
	{
		minAngles.push_back(minimum[i]);
	}
	maxAngles.clear();
	for (int i = 0; i < 15; i++)
	{
		maxAngles.push_back(maximum[i]);
	}
}

void Glove::clearCalibration()
{
	//set values that are guaranteed to be overwritten
	for (int i = 0; i < sensorCount; i++)
	{
		minValues[i] = USHRT_MAX;
		maxValues[i] = 0;
	}
}