#pragma once

#include "framework.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <map>

enum LogsCommands {
	LUP, LDWN, RUP, RDWN, B1, B2, B3, B4, WMV, MV
};

struct Log {
	LogsCommands command;
	POINT point;
	ULONGLONG timestamp;
};

class LogsReader {
private:
	std::ifstream in;
	std::vector<Log> logs;
	SIZE size;
	bool updatedFlag;
	int** clickFrequencyMatrix = nullptr;

	bool getFileInfo();
	void getLogsFromFile();
public:
	static void Init();
	bool isUpdated();
	int getLogsSize();
	LogsReader(const char* fileName);
	LogsReader(LPWSTR fileName);
	~LogsReader();
	std::vector<Log> getLogs();
	SIZE getLoggedScreenSize();
	int** getClickFrequencyMatrix();
	void addLog(std::string mes, ULONGLONG timestamp, POINT point);
};