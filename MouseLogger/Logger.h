#pragma once

#include "framework.h"
#include "Properties.h"
#include <string>
#include <vector>
#include <string>
#include <map>
#include "LogsReader.h"
#include "resource.h"

const double VERSION = 0.1;

std::string getDir();
void changeDir(std::string newDir);

LogsReader* getCurrentLogsReader();

//
//   FUNCTION: initLogger()
//
//   PURPOSE: Initialize files and hooks
//   RETURN: Result code
//
int initLogger();

//
//   FUNCTION: closeLogger()
//
//   PURPOSE: Close all handlers and release memory
//   RETURN: Result code
//
int closeLogger();

//
//   FUNCTION: closeLogger()
//
//   PURPOSE: Close all handlers
//   RETURN: Result code
//
int saveCursorPos();

/*
if (mouseMessagesStrings.count(wParam)) {
		pointToStrWithMessage(mouseMessagesStrings[wParam], pt);
		writeBufToFile(message.c_str(), message.size());
	}
*/