#include "stdafx.h"

#include <sys/stat.h>

// Global variables
char errorString[1024];  // Danger fixed size

/// Get Windows error codes.  Doesn't work for AVIERR codes.
char *getWindowsErrorCode(HRESULT hr) {
	LPVOID lpMsgBuf;
	DWORD dw = GetLastError(); 

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_FROM_HMODULE |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		hr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );
	strcpy(errorString, (LPTSTR)&lpMsgBuf);

	LocalFree(lpMsgBuf);
	return errorString;
}

/// Get Windows error codes using GetLastError()
char *getWindowsLastErrorCode() {
	DWORD dw = GetLastError();
	return getWindowsErrorCode(dw);
}

/// Function to return the names of AVIERR codes.  (Built from Vfw.h).
char *getErrorCode(HRESULT hr) {
	if(hr == AVIERR_OK) {
		return "AVIERR_OK";
	} else if(hr == AVIERR_UNSUPPORTED) {
		return "AVIERR_UNSUPPORTED";
	} else if(hr == AVIERR_BADFORMAT) {
		return "AVIERR_BADFORMAT";
	} else if(hr == AVIERR_MEMORY) {
		return "AVIERR_MEMORY";
	} else if(hr == AVIERR_INTERNAL) {
		return "AVIERR_INTERNAL";
	} else if(hr == AVIERR_BADFLAGS) {
		return "AVIERR_BADFLAGS";
	} else if(hr == AVIERR_BADPARAM) {
		return "AVIERR_BADPARAM";
	} else if(hr == AVIERR_BADSIZE) {
		return "AVIERR_BADSIZE";
	} else if(hr == AVIERR_BADHANDLE) {
		return "AVIERR_BADHANDLE";
	} else if(hr == AVIERR_FILEREAD) {
		return "AVIERR_FILEREAD";
	} else if(hr == AVIERR_FILEWRITE) {
		return "AVIERR_FILEWRITE";
	} else if(hr == AVIERR_FILEOPEN) {
		return "AVIERR_FILEOPEN";
	} else if(hr == AVIERR_COMPRESSOR) {
		return "AVIERR_COMPRESSOR";
	} else if(hr == AVIERR_NOCOMPRESSOR) {
		return "AVIERR_NOCOMPRESSOR";
	} else if(hr == AVIERR_READONLY) {
		return "AVIERR_READONLY";
	} else if(hr == AVIERR_NODATA) {
		return "AVIERR_NODATA";
	} else if(hr == AVIERR_BUFFERTOOSMALL) {
		return "AVIERR_BUFFERTOOSMALL";
	} else if(hr == AVIERR_CANTCOMPRESS) {
		return "AVIERR_CANTCOMPRESS";
	} else if(hr == AVIERR_USERABORT) {
		return "AVIERR_USERABORT";
	} else if(hr == AVIERR_ERROR) {
		return "AVIERR_ERROR";
	} else {
		return "Unknown";
	}
}

int errMsg(const char *fmt, ...)
{
	va_list vargs;
	static char lstring[4096]; // Danger - fixed size

	va_start(vargs,fmt);
	(void)vsprintf(lstring,fmt,vargs);
	va_end(vargs);

	if(lstring[0] != '\0') {
		printf("%s\n",lstring);
	}

	return 0;
}

void printFileInfo(PAVIFILE pFile) {
	if(!pFile) {
		errMsg("PAVIFILE input is NULL\n");
		return;
	}
	AVIFILEINFO fileInfo;
	HRESULT hr = AVIFileInfo(pFile,&fileInfo,sizeof(fileInfo));
	if(hr != AVIERR_OK) { 
		errMsg("Unable to get file info [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		return; 
	}
	printFileInfo(fileInfo);
}

void printFileInfo(AVIFILEINFO fileInfo) {
	printf("MaxBytesPerSec: %d\n",fileInfo.dwMaxBytesPerSec);
	printf("Flags:\n");
	if(fileInfo.dwFlags & AVIFILEINFO_HASINDEX)
		printf("  HASINDEX\n");
	if(fileInfo.dwFlags & AVIFILEINFO_MUSTUSEINDEX)
		printf("  MUSTUSEINDEX\n");
	if(fileInfo.dwFlags & AVIFILEINFO_ISINTERLEAVED)
		printf("  ISINTERLEAVED\n");
	if(fileInfo.dwFlags & AVIFILEINFO_WASCAPTUREFILE)
		printf("  WASCAPTUREFILE\n");
	if(fileInfo.dwFlags & AVIFILEINFO_COPYRIGHTED)
		printf("  COPYRIGHTED\n");
	int nStreams=fileInfo.dwStreams;
	printf("Streams: %d\n",fileInfo.dwStreams);
	printf("SuggestedBufferSize: %d bytes\n",
		fileInfo.dwSuggestedBufferSize);
	printf("Width: %d\n",fileInfo.dwWidth);
	printf("Height: %d\n",fileInfo.dwHeight);
	printf("Scale: %d\n",fileInfo.dwScale);
	printf("Rate: %d\n",fileInfo.dwRate);
	double sps=(double)fileInfo.dwRate/(double)fileInfo.dwScale;
	printf("SamplesPerSec: %g\n",sps);
	double length=(sps != 0.0)?(double)fileInfo.dwLength/sps:0.0;
	printf("Length: %d (%g sec, %g min)\n",fileInfo.dwLength,
		length,length/60.0);
	printf("EditCount: %d\n",fileInfo.dwEditCount);
	printf("FileType: %s\n",fileInfo.szFileType);
}

void printStreamInfo(PAVISTREAM pStream) {
	if(!pStream) {
		errMsg("PAVIFILE input is NULL\n");
		return;
	}
	AVISTREAMINFO streamInfo;
	HRESULT hr=AVIStreamInfo(pStream, &streamInfo, sizeof(streamInfo)); 
	if(hr != AVIERR_OK) { 
		errMsg("Unable to get stream info [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		return; 
	}
	printStreamInfo(streamInfo);
}

void printStreamInfo(AVISTREAMINFO pStream) {
	if (pStream.fccType == streamtypeVIDEO) { 
		printf("  VIDEO\n");
	} else if (pStream.fccType == streamtypeAUDIO) { 
		printf("  AUDIO\n");
	}  else if (pStream.fccType == streamtypeMIDI) {  
		printf("  MIDI\n");
	}  else if (pStream.fccType == streamtypeTEXT) {  
		printf("  TEXT\n");
	} else {
		printf("  Unknown fccType [%d]\n",pStream.fccType);
	}
	printf("  Priority: %hd\n",pStream.wPriority);
	printf("  Flags:\n");
	if(pStream.dwFlags & AVISTREAMINFO_DISABLED)
		printf("    DISABLED\n");
	if(pStream.dwFlags & AVISTREAMINFO_FORMATCHANGES)
		printf("    FORMATCHANGES\n");
	printf("  SuggestedBufferSize: %d bytes\n",pStream.dwSuggestedBufferSize);
	printf("  Quality: %d [0-10,000]\n",pStream.dwQuality);
	printf("  Left=%d Right=%d Top=%d Bottom=%d\n",
		pStream.rcFrame.left,pStream.rcFrame.right,
		pStream.rcFrame.top,pStream.rcFrame.bottom);
	printf("  Width=%d Height=%d\n",
		pStream.rcFrame.right-pStream.rcFrame.left,
		pStream.rcFrame.bottom-pStream.rcFrame.top);
	printf("  Start: %d\n",pStream.dwStart);
	printf("  InitialFrames: %d\n",pStream.dwInitialFrames);
	printf("  SampleSize: %d\n",pStream.dwSampleSize);
	printf("  Scale: %d\n",pStream.dwScale);
	printf("  Rate: %d\n",pStream.dwRate);
	double sps=(double)pStream.dwRate/(double)pStream.dwScale;
	printf("    SamplesPerSec: %g\n",sps);
	double length=(sps != 0.0)?(double)pStream.dwLength/sps:0.0;
	printf("  Length: %d (%g sec, %g min)\n",pStream.dwLength,
		length,length/60.0);
	printf("  EditCount: %d\n",pStream.dwEditCount);
	printf("  FormatChangeCount: %d\n",pStream.dwFormatChangeCount);
	printf("  Name: %s\n",pStream.szName);
}


void printKeyFrameInfo(char *prefix, PAVISTREAM pStream) {
	// Get keyframe information
	LONG nKeyFrames = 0;
	LONG firstKeyFrame = -1;
	LONG lastKeyFrame = -1;
	// Keyframes that give -1 for AVIStreamNearestKeyFrame
	LONG nBadNearest = 0;
	LONG firstBadNearest = -1;
	LONG lastBadNearest = -1;
	LONG nearestKeyFrame = -1;
	LONG start = AVIStreamStart(pStream);
	LONG end = AVIStreamEnd(pStream) - 1;
	for(LONG f = start; f <= end; f++) {
		if(AVIStreamIsKeyFrame(pStream, f)) {
			nKeyFrames++;
			if(firstKeyFrame < 0)  firstKeyFrame = f;
			lastKeyFrame = f;
		}
		// Check if AVIStreamNearestKeyFrame is working
		nearestKeyFrame = AVIStreamNearestKeyFrame(pStream, f);
		if(nearestKeyFrame == -1) {
			nBadNearest++;
			if(firstBadNearest < 0)  firstBadNearest = f;
			lastBadNearest = f;
		}
	}
	printf("%sstartFrame=%d endFrame=%d length=%d\n",
		prefix, start, end, end - start + 1);
	printf("%snKeyframes=%d firstKeyFrame=%d lastKeyFrame=%d\n",
		prefix, nKeyFrames, firstKeyFrame, lastKeyFrame);
	printf("%snBadNearest=%d firstBadNearest=%d lastBadNearest=%d\n",
		prefix, nBadNearest, firstBadNearest, lastBadNearest);
}

void printBmiHeaderInfo(char *prefix, BITMAPINFOHEADER bih) {
	printf(prefix);
	printf("biSize: %d\n",bih.biSize);
	printf(prefix);
	printf("biWidth=%d biHeight=%d\n", bih.biWidth, bih.biHeight);
	printf(prefix);
	printf("biPlanes=%hd biBitCount=%hd\n", bih.biPlanes, bih.biBitCount);
	DWORD biCompression = bih.biCompression;
	if (biCompression == BI_RGB) { 
		printf(prefix);
		printf("biCompression=BI_RGB\n");
	} else 	if (biCompression == BI_RLE8) { 
		printf(prefix);
		printf("biCompression=BI_RLE8\n");
	} else 	if (biCompression == BI_RLE4) { 
		printf(prefix);
		printf("biCompression=BI_RLE4\n");
	} else 	if (biCompression == BI_BITFIELDS) { 
		printf(prefix);
		printf("biCompression=BI_BITFIELDS\n");
	} else 	if (biCompression == BI_JPEG) { 
		printf(prefix);
		printf("biCompression=BI_JPEG\n");
	} else 	if (biCompression == BI_PNG) { 
		printf(prefix);
		printf("biCompression=BI_PNG\n");
	} else {
		printf(prefix);
		printf("biCompression=");
		printFourCcCode(biCompression, "\n");
	}
	printf(prefix);
	printf("biSizeImage: %d\n",bih.biSizeImage);
	printf(prefix);
	printf("biXPelsPerMeter=%d biYPelsPerMeter=%d\n",
		bih.biXPelsPerMeter, bih.biYPelsPerMeter);
	printf(prefix);
	printf("biClrUsed=%hd biClrImportant=%hd\n",
		bih.biClrUsed, bih.biClrImportant);
}

void printCompressOptions(AVICOMPRESSOPTIONS opts) {
	if (opts.fccType == streamtypeVIDEO) { 
		printf("  VIDEO\n");
	} else if (opts.fccType == streamtypeAUDIO) { 
		printf("  AUDIO\n");
	}  else if (opts.fccType == streamtypeMIDI) {  
		printf("  MIDI\n");
	}  else if (opts.fccType == streamtypeTEXT) {  
		printf("  TEXT\n");
	} else {
		printf("  Unknown fccType [%d]\n",opts.fccType);
	}
	DWORD fccHandler = opts.fccHandler;
	if (fccHandler == BI_RGB) { 
		printf("  fccHandler=BI_RGB\n");
	} else 	if (fccHandler == BI_RLE8) { 
		printf("  fccHandler=BI_RLE8\n");
	} else 	if (fccHandler == BI_RLE4) { 
		printf("  fccHandler=BI_RLE4\n");
	} else 	if (fccHandler == BI_BITFIELDS) { 
		printf("  fccHandler=BI_BITFIELDS\n");
	} else 	if (fccHandler == BI_JPEG) { 
		printf("  fccHandler=BI_JPEG\n");
	} else 	if (fccHandler == BI_PNG) { 
		printf("  fccHandler=BI_PNG\n");
	} else {
		printf("  fccHandler=");
		printFourCcCode(fccHandler, "\n");
	}
	printf("  fccType: ");
	printFourCcCode(opts.fccType, "\n");
	printf("  dwKeyFrameEvery: %d\n",opts.dwKeyFrameEvery);
	printf("  dwQuality: %d\n",opts.dwQuality);
	printf("  dwBytesPerSecond: %d\n",opts.dwBytesPerSecond);
	DWORD dwFlags = opts.dwFlags;
	printf("  dwFlags: \n");
	if(dwFlags & AVICOMPRESSF_DATARATE) printf("  AVICOMPRESSF_DATARATE ");
	if(dwFlags & AVICOMPRESSF_INTERLEAVE) printf("  AVICOMPRESSF_INTERLEAVE ");
	if(dwFlags & AVICOMPRESSF_KEYFRAMES) printf("  AVICOMPRESSF_KEYFRAMES ");
	if(dwFlags & AVICOMPRESSF_VALID) printf("  AVICOMPRESSF_VALID ");
	if(opts.lpFormat == NULL) {
		printf("  lpFormat: NULL\n");
	} else {
		printf("  lpFormat: Non-NULL\n");
	}
	printf("  cbFormat: %d\n",opts.cbFormat);
	if(opts.lpParms == NULL) {
		printf("  lpParms: NULL\n");
	} else {
		printf("  lpParms: Non-NULL\n");
	}
	printf("  cbParms: %d\n",opts.cbParms);
	printf("  dwInterleaveEvery: %d\n",opts.dwInterleaveEvery);

	printf("  dwBytesPerSecond: %d\n",opts.dwBytesPerSecond);
	printf("  dwBytesPerSecond: %d\n",opts.dwBytesPerSecond);
}

void printFourCcCode(DWORD code, char *suffix) {
	printf("%c%c%c%c",
		(char)(code), (char)(code>>8),
		(char)(code>>16), (char)(code>>24));
	printf(suffix);
}

void printAvailableDecompressors(LPBITMAPINFO pBmi) {
	HIC hic;
	ICINFO icinfo, icinfo2;

	printf("      Available decompressors\n");
	for (int i=0; ICInfo(ICTYPE_VIDEO, i, &icinfo); i++) { 
		hic = ICOpen(icinfo.fccType, icinfo.fccHandler, ICMODE_QUERY); 
		if (hic) {
			// Apparently the szName and szDescription are not filled in
			ICGetInfo(hic, &icinfo2, sizeof(icinfo2));
			DWORD fccHandler = icinfo2.fccHandler;
			// Have to use %S since it is WCHAR array
			printf("        %2d ", i);
			printFourCcCode(fccHandler, " ");
			printf("%S: %S -> ",
				icinfo2.szName, icinfo2.szDescription);

			// Skip this compressor if it can't handle the format. 
			if (icinfo.fccType == ICTYPE_VIDEO && pBmi != NULL && 
				ICDecompressQuery(hic, pBmi, NULL) != ICERR_OK) { 
					printf("No\n");
					ICClose(hic);
					continue; 
			} else {
				printf("Yes\n");
				// Get the size of the default format
				BITMAPINFO bmiOut;
				DWORD res = ICDecompressGetFormat(hic, pBmi, &bmiOut);
				printf("          ");
				printFourCcCode(fccHandler, ": "); 
				if(res != ICERR_OK) {
					printf("Default output format not available\n");
				} else {
					printf("Default output format:\n");
					printBmiHeaderInfo("            ", bmiOut.bmiHeader);
				}
			}
			ICClose(hic); 
		} 
	} 
}

void printAudioInfo(PAVIFILE pFile) {
	PAVISTREAM *pStreams = new PAVISTREAM[1];

	// Find the first audio stream
	HRESULT hr=AVIFileGetStream(pFile, &pStreams[0], streamtypeAUDIO, 0) ;
	if(hr != AVIERR_OK || pStreams[0] == NULL) {
		errMsg("printAudioInfo: Cannot open an audio stream"); 
		return; 
	}

	printStreamInfo(pStreams[0]);

	// Cleanup
	delete [] pStreams;
	pStreams = NULL;
}

HRESULT getBufferSizes(PAVISTREAM pStream, LONG *sizeMin, LONG *sizeMax, LONG *nErrors) {
	HRESULT hr;
	HRESULT hrReturnVal = AVIERR_OK;

	*sizeMin = LONG_MAX;
	*sizeMax = 0;
	LONG size;
	*nErrors = 0;
	for(int i = AVIStreamStart(pStream); i < AVIStreamEnd(pStream); i++) {
		hr = AVIStreamSampleSize(pStream, i, &size);
		if(hr != AVIERR_OK) {
			*nErrors++;
			hrReturnVal = AVIERR_ERROR;
			continue;
		}
		if(size < *sizeMin) *sizeMin = size;
		if(size > *sizeMax) *sizeMax = size;
	}
	return hrReturnVal;
}

HRESULT getNStreams(PAVIFILE pFile, DWORD *nStreams) {
	if(pFile == NULL) {
		return AVIERR_BADPARAM;
	}

	// Get file info
	AVIFILEINFO fileInfo; 
	HRESULT hr=AVIFileInfo(pFile, &fileInfo, sizeof(fileInfo));
	if(hr != AVIERR_OK) {
		return hr;
	}
	*nStreams=fileInfo.dwStreams;
	return AVIERR_OK;
}

/// Get the length of the file as a double.  Returns -1 on error.
/// Avoids problems with LONG_MAX ~ 2 GB, ULONG_MAX ~ 4 GB, etc.
double getFileSize(char *fileName) {
	if(!fileName) {
		return -1;
	}

	HANDLE hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, 0, NULL);
	if(hFile == INVALID_HANDLE_VALUE) {
		return -1;
    }

	LARGE_INTEGER lpFileSize;
	BOOL bRetVal = GetFileSizeEx(hFile, &lpFileSize);
	CloseHandle(hFile);
	if(!bRetVal) {
		return -1;
	}

	return (double)lpFileSize.QuadPart;
}
