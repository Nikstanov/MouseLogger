#pragma once

#include "framework.h"

//
//  FUNCTION: OpenProperties()
//
//  PURPOSE: Open main key in registry and get program properties
//
int OpenProperties();

//
//  FUNCTION: GetFrequence()
//
//  PURPOSE: Return frequence value from properties
//
DWORD GetFrequence();

//
//  FUNCTION: GetFilesCount()
//
//  PURPOSE: Return files count from properties
//
DWORD GetFilesCount();

//
//  FUNCTION: GetMaxRecordsTime()
//
//  PURPOSE: Return max time of recording info in file from properties
//
DWORD GetMaxRecordsTime();

//
//  FUNCTION: GetMaxRecordsTime()
//
//  PURPOSE: Return max size of logs file from properties
//
DWORD GetMaxFilesSize();
DWORD GetMaxFilesSizeEx();
CHAR* GetMaxFilesSizeType();

int CloseProperies();

void SetMaxFilesSizeType(CHAR* type);
void SetFrequence(DWORD val);
void SetMaxFilesSize(DWORD val);
void SetFilesCount(DWORD val);
void SetMaxRecordsTime(DWORD val);


