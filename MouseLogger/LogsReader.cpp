#include "LogsReader.h"

const double VERSION = 0.1;
std::map<std::string, LogsCommands> LogsCommandsNames;

int LogsReader::getLogsSize()
{
	return logs.size();
}

SIZE LogsReader::getLoggedScreenSize() {
	return size;
}

void LogsReader::Init() {
	LogsCommandsNames["MV"] = LogsCommands::MV;
	LogsCommandsNames["LUP"] = LogsCommands::LUP;
	LogsCommandsNames["LDWN"] = LogsCommands::LDWN;
	LogsCommandsNames["RUP"] = LogsCommands::RUP;
	LogsCommandsNames["RWDN"] = LogsCommands::RDWN;
	LogsCommandsNames["WMV"] = LogsCommands::WMV;
	LogsCommandsNames["B1"] = LogsCommands::B1;
	LogsCommandsNames["B2"] = LogsCommands::B2;
	LogsCommandsNames["B3"] = LogsCommands::B3;
	LogsCommandsNames["B4"] = LogsCommands::B4;
}

LogsReader::LogsReader(const char* fileName){
	if (fileName != NULL) {
		in.open(fileName);
	}
	else {
		size.cx = GetSystemMetrics(SM_CXSCREEN);
		size.cy = GetSystemMetrics(SM_CYSCREEN);
	}
	if (!getFileInfo()) {
		return;
	}
	clickFrequencyMatrix = new int* [size.cx] {};
	for (unsigned i{}; i < size.cx; i++)
	{
		clickFrequencyMatrix[i] = new int[size.cy] {};
	}
	getLogs();
}

LogsReader::LogsReader(LPWSTR fileName) {
	if (fileName != NULL) {
		in.open(fileName);
	}
	else {
		size.cx = GetSystemMetrics(SM_CXSCREEN);
		size.cy = GetSystemMetrics(SM_CYSCREEN);
	}
	if (!getFileInfo()) {
		return;
	}
	clickFrequencyMatrix = new int* [size.cx] {};
	for (unsigned i{}; i < size.cx; i++)
	{
		clickFrequencyMatrix[i] = new int[size.cy] {};
	}
	getLogs();
}

bool LogsReader::isUpdated()
{
	bool temp = updatedFlag;
	if (temp) {
		updatedFlag = false;
	}
	return temp;
}

std::vector<Log> LogsReader::getLogs() {
	if (in.is_open()) {
		getLogsFromFile();
	}
	return logs;
}

bool LogsReader::getFileInfo()
{
	if (!in.is_open()) {
		return true;
	}
	std::string line;
	try {
		std::getline(in, line);
		if (line.substr(0, line.find(' ')).compare("ver") != 0) {
			return false;
		}
		line.erase(0, line.find(' ') + 1);
		double ver = std::stod(line);
		if (ver != VERSION) {
			return false;;
		}

		std::getline(in, line);
		if (line.substr(0, line.find(' ')).compare("res") != 0) {
			return false;
		}
		line.erase(0, line.find(' ') + 1);
		size.cx = std::stoi(line.substr(0, line.find(',')));
		line.erase(0, line.find(',') + 1);
		size.cy = std::stoi(line);
	}
	catch (...) {
		return false;
	}
}

void LogsReader::addLog(std::string mes, ULONGLONG timestamp, POINT point) {
	
	Log log;
	log.command = LogsCommandsNames[mes];
	if (log.command == NULL) {
		return;
	}
	if (log.command == LogsCommands::LDWN) {
		updatedFlag = true;
		clickFrequencyMatrix[point.x][point.y] += 1;
	}
	log.point = point;
	log.timestamp = timestamp;
	if (logs.size() > 2000) {
		logs.erase(logs.cbegin());
	}
	logs.push_back(log);
}

void LogsReader::getLogsFromFile() {
	std::string line;
	if (in.is_open()) {
		try {
			while (std::getline(in, line))
			{
				std::string com = line.substr(0, line.find(' '));
				line.erase(0, line.find(' ') + 1);
				POINT point;
				point.x = std::stoi(line.substr(0, line.find(',')));
				line.erase(0, line.find(',') + 1);
				point.y = std::stoi(line.substr(0, line.find(' ')));
				line.erase(0, line.find(' ') + 1);
				ULONGLONG timestamp = std::stoull(line.substr(0, line.find(' ')));
				addLog(com, timestamp, point);
			}
		}
		catch (...) {
			return;
		}
	}
}

int** LogsReader::getClickFrequencyMatrix() {
	return clickFrequencyMatrix;
}

LogsReader::~LogsReader() {
	if (in.is_open()) {
		in.close();
	}
	logs.clear();
	for (int i = 0; i < size.cx; i++)
	{
		delete[] clickFrequencyMatrix[i];
	}
}