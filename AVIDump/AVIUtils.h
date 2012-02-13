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
void printCompressOptions(AVICOMPRESSOPTIONS opts);
void printAvailableDecompressors(LPBITMAPINFO pBmi);
void printAudioInfo(PAVIFILE pFile);
HRESULT getBufferSizes(PAVISTREAM pStream, LONG *sizeMin, LONG *sizeMax,
					   LONG *nErrors);
HRESULT getNStreams(PAVIFILE pFile, DWORD *nStreams);

// Function prototypes from Copy
HRESULT copyStream(PAVIFILE pFile2, PAVISTREAM pStream1);
HRESULT copyStream1(PAVIFILE pFile2, PAVISTREAM pStream1);

// Function prototypes from ConvertVideo
HRESULT decompressVideo(PAVISTREAM pStream1, PAVISTREAM pStream2);

#endif
