#ifndef _AVIUtils_H
#define _AVIUtils_H

#include "stdafx.h"

// Function prototypes from AVIDump
void getBitmapInfo(PAVISTREAM gapavi);
int errMsg(const char *fmt, ...);
char *getWindowsErrorCode(HRESULT hr);
char *getErrorCode(HRESULT hr);
void printFourCcCode(DWORD code, char *suffix);
void printFileInfo(PAVIFILE pFile);
void printFileInfo(AVIFILEINFO aviInfo);
void printStreamInfo(PAVISTREAM pStream);
void printStreamInfo(AVISTREAMINFO avis);
void printBmiHeaderInfo(char *prefix, BITMAPINFOHEADER header);
void printAvailableDecompressors(LPBITMAPINFO pBmi);
void printAudioInfo(PAVIFILE pFile);

#endif
