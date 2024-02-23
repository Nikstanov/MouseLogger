#pragma warning(disable : 4996)

#include "Logger.h"

int curFile = 0;
std::vector<HFILE> files;
HHOOK hook;
POINT prevPoint;
ULONGLONG startTime = GetTickCount64();
DWORD fileSize = 0;
HANDLE hThread;
tm* ltm;
std::string DirName{ "logs" };
LPSTR bufFileName = new CHAR[100];
LogsReader* curLogsReader = new LogsReader((LPWSTR)nullptr);
std::map<int, std::string> mouseMessagesStrings;


HFILE createNewFile() {
	time_t now = time(0);
	tm* ltm = localtime(&now);
	std::string name{
		DirName + "\\" 
		+ std::to_string(ltm->tm_mday) + "."
		+ std::to_string(ltm->tm_mon + 1)+ "." 
		+ std::to_string(ltm->tm_year + 1900) + " "
		+ std::to_string(ltm->tm_hour) + "."
		+ std::to_string(ltm->tm_min)
		+ ".mouseLog"
	};
	//delete ltm;
	startTime = GetTickCount64();
	
	HANDLE file = CreateFileA(name.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD res = 0;
	std::string mes{ "ver " + std::to_string(VERSION) + "\n" };
	WriteFile(file, mes.c_str(), mes.length(), &res, NULL);
	mes = std::string{ "res " + std::to_string(GetSystemMetrics(SM_CXSCREEN)) + "," + std::to_string(GetSystemMetrics(SM_CYSCREEN))  + "\n"};
	WriteFile(file, mes.c_str(), mes.length(), &res, NULL);
	return (HFILE)file;
}

void changeDir(std::string newDir) {
	DirName = newDir;
}

std::string getDir() {
	if (DirName == "logs") {
		GetFinalPathNameByHandleA((HANDLE)files[0], bufFileName, 100, FILE_NAME_NORMALIZED);
		std::string dir = std::string{ bufFileName };
		return dir.substr(0, dir.find_last_of('\\'));
	}
	return DirName;
}

LogsReader* getCurrentLogsReader() {
	return curLogsReader;
}

DWORD WINAPI threadProc(LPVOID lpParam) {
	ULONGLONG time = GetTickCount64();
	while (true)
	{
		// Get mouse pos
		ULONGLONG tempTime = GetTickCount64();
		if ((tempTime - time) > GetFrequence()) {
			saveCursorPos();
			time = tempTime;
		}
		Sleep(GetFrequence());
	}
	return 0;
}

std::string pointToStrWithMessage(std::string mes, POINT pt) {
	ULONGLONG timestamp = GetTickCount64() - startTime;
	curLogsReader->addLog(mes, timestamp, pt);
	std::string message{ mes + " " + std::to_string(pt.x) + "," + std::to_string(pt.y) + " " + std::to_string(timestamp) + "\n"};
	return message;
}

//
//   FUNCTION: writeBufToFile(CHAR*, DWORD)
//
//   PURPOSE: Save buffer in current open file
//   RETURN: Writen bytes
//
DWORD writeBufToFile(const CHAR* buf, DWORD len) {
	DWORD res = 0;
	WriteFile((HANDLE)files[curFile], buf, len, &res, NULL);
	ULONGLONG curTime = GetTickCount64();
	fileSize += len;
	if (((curTime - curTime) >= GetMaxRecordsTime()) || (fileSize > GetMaxFilesSize())){
		curFile++;
		if (curFile >= GetFilesCount()) {
			curFile = 0;
		}
		std::string file_name{ DirName + "\\LogFile" + std::to_string(curFile) + ".mouseLog" };
		if (curFile <= files.size() - 1) {
			GetFinalPathNameByHandleA((HANDLE)files[curFile], bufFileName, 100, FILE_NAME_NORMALIZED);
			CloseHandle((HANDLE)files[curFile]);
			res = DeleteFileA(bufFileName + 4);
			files[curFile] = createNewFile();
		}
		else {
			files.push_back(createNewFile());
		}
		curTime = curTime;
		fileSize = len;
	}
	return res;
}


//
//   FUNCTION: MouseHook(int, WPARAM, LPARAM)
//
//   PURPOSE: Mouse hook function - get massage and write it in file
//   RETURN: Result code
//
LRESULT CALLBACK MouseHook(int nCode, WPARAM wParam, LPARAM lParam) {
	CHAR* buffer = new CHAR[50];
	if (nCode < 0) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}
	int offset = 0;
	std::string message;
	POINT pt = PMSLLHOOKSTRUCT(lParam)->pt;
	pt.x = pt.x / 2;
	pt.y = pt.y / 2;
	if (wParam != WM_MOUSEMOVE && mouseMessagesStrings.count(wParam)) {
		message = pointToStrWithMessage(mouseMessagesStrings[wParam], pt);
		writeBufToFile(message.c_str(), message.size());
	}
		
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

//
//   FUNCTION: initLogger()
//
//   PURPOSE: Initialize files and hooks
//   RETURN: Result code
//
int initLogger() {
	LogsReader::Init();
	mouseMessagesStrings[WM_LBUTTONDOWN] = std::string{ "LDWN" };
	mouseMessagesStrings[WM_RBUTTONDOWN] = std::string{ "RDWN" };
	mouseMessagesStrings[WM_LBUTTONUP] = std::string{ "LUP" };
	mouseMessagesStrings[WM_RBUTTONUP] = std::string{ "RUP" };
	mouseMessagesStrings[WM_MOUSEWHEEL] = std::string{ "WMV" };
	mouseMessagesStrings[WM_MOUSEMOVE] = std::string{ "MV" };

	GetCursorPos(&prevPoint);
	CreateDirectoryA("Logs", NULL);
	files.push_back(createNewFile());

	hook = SetWindowsHookExW(WH_MOUSE_LL, MouseHook, NULL, 0);
	hThread = CreateThread(
		NULL,
		0,
		threadProc,
		NULL,
		0,
		NULL);
	return 0;
}

//
//   FUNCTION: saveCursorPos()
//
//   PURPOSE: Get cursor pos and save it in file
//   RETURN: Result code
//
int saveCursorPos() {
	POINT point;
	GetCursorPos(&point);
	if ((prevPoint.x != point.x) || (prevPoint.y != point.y)) {
		std::string mes = pointToStrWithMessage(mouseMessagesStrings[WM_MOUSEMOVE], point);
		writeBufToFile(mes.c_str(), mes.size());
		prevPoint = point;
	}	
	return 0;
}


//
//   FUNCTION: closeLogger()
//
//   PURPOSE: Close all handlers and release memory
//   RETURN: Result code
//
int closeLogger() {
	CloseHandle(hThread);
	UnhookWindowsHookEx(hook);
	for (int i = 0; i < files.size(); i++) {
		CloseHandle((HANDLE)files[i]);
	}
	return 0;
}