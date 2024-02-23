#include "framework.h"
#include <map>

#define POLLING_FREQUENCY "Polling_Frequency"
#define FILES_COUNT "Files_Count"
#define MAX_RECORDS_TIME "Records_Time"
#define MAX_FILES_SIZE	 "Max_Files_Size"
#define MAX_FILES_SIZE_NAME	 "Max_Files_Size_Name"
#define KEY_NAME "Mouse Logger"

HKEY hKey;

DWORD frequence = 20;
DWORD filesCount = 5;
DWORD maxRecordsTime = 1000*60*1;
DWORD maxFilesSize = 20;
DWORD maxFilesSizeInBytes = 20 * 1024 * 1024;
CHAR* maxFilesSizeType = new CHAR[2];
const CHAR* maxFilesSizeDef = "MB";

LSTATUS setDwordVal(const CHAR* name, DWORD* lpValue) {
	DWORD temp;
	temp = sizeof(DWORD);
	LRESULT lStatus = RegSetValueExA(hKey, name, 0, REG_DWORD, LPBYTE(lpValue), temp);
	return lStatus;
}

LSTATUS setStrVal(const CHAR* name, const CHAR* defaultValue, int def_len) {
	LRESULT lStatus = RegSetValueExA(hKey, name, 0, REG_SZ, LPBYTE(defaultValue), def_len);
	return lStatus;
}

LSTATUS getOrSetDefaultDWORD(const CHAR* name, DWORD* lpValue, DWORD defaultValue) {
	DWORD temp;
	temp = sizeof(DWORD);
	LRESULT lStatus = RegGetValueA(hKey, NULL, name, RRF_RT_DWORD, NULL, lpValue, &temp);
	if (lStatus != ERROR_SUCCESS) {
		if (lStatus == ERROR_FILE_NOT_FOUND) {
			lStatus = RegSetValueExA(hKey, name, 0, REG_DWORD, LPBYTE(&defaultValue), temp);
			*lpValue = defaultValue;
			if (lStatus != ERROR_SUCCESS) {
				return -1;
			}
		}
		else {
			return -1;
		}
	}
	return 0;
}

LSTATUS getOrSetDefaultString(const CHAR* name, CHAR* lpValue, DWORD* res_len, const CHAR* defaultValue, int def_len) {
	LRESULT lStatus = RegGetValueA(hKey, NULL, name, RRF_RT_REG_SZ, NULL, lpValue, res_len);
	if (lStatus != ERROR_SUCCESS) {
		if (lStatus == ERROR_FILE_NOT_FOUND) {
			lStatus = RegSetValueExA(hKey, name, 0, REG_SZ, LPBYTE(defaultValue), def_len);
			if (lStatus != ERROR_SUCCESS) {
				return -1;
			}
			return 1;
		}
		else {
			return -1;
		}
	}
	return 0;
}

int OpenProperties() {

	//Open key
	HKEY parentKey;
	LSTATUS lStatus = RegOpenCurrentUser(KEY_READ, &parentKey);
	if (lStatus == 0) {
		HKEY software;
		lStatus = RegOpenKeyExW(parentKey, L"Software", 0, KEY_CREATE_SUB_KEY | KEY_READ, &software);
		if (lStatus == 0) {
			RegCloseKey(parentKey);
			lStatus = RegOpenKeyExA(software, KEY_NAME, 0, KEY_ALL_ACCESS, &hKey);
			if (lStatus != ERROR_SUCCESS)
			{
				if (lStatus == ERROR_FILE_NOT_FOUND) {
					HKEY temp;
					DWORD res;
					lStatus = RegCreateKeyExA(software, KEY_NAME, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &temp, &res);
					if (lStatus != ERROR_SUCCESS) {
						return -1;
					}
				}
				else {
					return -1;
				}
			}
		}
		else {
			return -1;
		}
	}
	else {
		return -1;
	}

	// Get all properties from registry
	lStatus = getOrSetDefaultDWORD(POLLING_FREQUENCY, &frequence, 0);
	if (lStatus != ERROR_SUCCESS) {
		return -1;
	}
	lStatus = getOrSetDefaultDWORD(FILES_COUNT, &filesCount, filesCount);
	if (lStatus != ERROR_SUCCESS) {
		return -1;
	}
	lStatus = getOrSetDefaultDWORD(MAX_RECORDS_TIME, &maxRecordsTime, maxRecordsTime);
	if (lStatus != ERROR_SUCCESS) {
		return -1;
	}
	lStatus = getOrSetDefaultDWORD(MAX_FILES_SIZE, &maxFilesSize, maxFilesSize);
	if (lStatus != ERROR_SUCCESS) {
		return -1;
	}

	DWORD resSize;
	lStatus = getOrSetDefaultString(MAX_FILES_SIZE_NAME, maxFilesSizeType,&resSize, maxFilesSizeDef, 2);
	if ((lStatus != ERROR_SUCCESS) && (lStatus != 1)){
		return -1;
	}
	maxFilesSizeInBytes = maxFilesSize;
	if (lStatus != 1) {

		if (strcmp(maxFilesSizeType, "MB") == 0) {
			maxFilesSizeInBytes *= 1024 * 1024;
		}
		if (strcmp(maxFilesSizeType, "KB") == 0) {
			maxFilesSizeInBytes *= 1024;
		}
	}
	return 0;
}

DWORD GetFrequence() {
	return frequence;
}

DWORD GetMaxFilesSize() {
	return maxFilesSizeInBytes;
}


DWORD GetFilesCount() {
	return filesCount;
}

DWORD GetMaxRecordsTime() {
	return maxRecordsTime;
}

DWORD GetMaxFilesSizeEx() {
	return maxFilesSize;
}

CHAR* GetMaxFilesSizeType() {
	return maxFilesSizeType;
}

void SetMaxFilesSizeType(CHAR* type) {
	maxFilesSizeType = type;
}

void SetFrequence(DWORD val) {
	frequence = val;
}

void SetMaxFilesSize(DWORD val) {
	maxFilesSize = val;
}

void SetFilesCount(DWORD val) {
	filesCount = val;
}

void SetMaxRecordsTime(DWORD val) {
	maxRecordsTime = val;
}

int CloseProperies() {
	LRESULT lStatus = setDwordVal(POLLING_FREQUENCY, &frequence);
	if (lStatus != ERROR_SUCCESS) {
		return -1;
	}
	lStatus = setDwordVal(FILES_COUNT, &filesCount);
	if (lStatus != ERROR_SUCCESS) {
		return -1;
	}
	lStatus = setDwordVal(MAX_RECORDS_TIME, &maxRecordsTime);
	if (lStatus != ERROR_SUCCESS) {
		return -1;
	}
	lStatus = setDwordVal(MAX_FILES_SIZE, &maxFilesSize);
	if (lStatus != ERROR_SUCCESS) {
		return -1;
	}

	CHAR* res = new CHAR[2];
	DWORD resSize;
	lStatus = setStrVal(MAX_FILES_SIZE_NAME, maxFilesSizeDef, 2);
	if ((lStatus != ERROR_SUCCESS) && (lStatus != 1)) {
		return -1;
	}
	RegCloseKey(hKey);
	return 0;
}

