#include "stdafx.h"

#define APP_OK 0
#define APP_ERROR 1

#define PATH_MAX _MAX_PATH

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

void printFileInfo(AVIFILEINFO aviInfo) {
	printf("MaxBytesPerSec: %d\n",aviInfo.dwMaxBytesPerSec);
	printf("Flags:\n");
	if(aviInfo.dwFlags & AVIFILEINFO_HASINDEX)
		printf("  HASINDEX\n");
	if(aviInfo.dwFlags & AVIFILEINFO_MUSTUSEINDEX)
		printf("  MUSTUSEINDEX\n");
	if(aviInfo.dwFlags & AVIFILEINFO_ISINTERLEAVED)
		printf("  ISINTERLEAVED\n");
	if(aviInfo.dwFlags & AVIFILEINFO_WASCAPTUREFILE)
		printf("  WASCAPTUREFILE\n");
	if(aviInfo.dwFlags & AVIFILEINFO_COPYRIGHTED)
		printf("  COPYRIGHTED\n");
	int nStreams=aviInfo.dwStreams;
	printf("Streams: %d\n",aviInfo.dwStreams);
	printf("SuggestedBufferSize: %d bytes\n",
		aviInfo.dwSuggestedBufferSize);
	printf("Width: %d\n",aviInfo.dwWidth);
	printf("Height: %d\n",aviInfo.dwHeight);
	printf("Scale: %d\n",aviInfo.dwScale);
	printf("Rate: %d\n",aviInfo.dwRate);
	double sps=(double)aviInfo.dwRate/(double)aviInfo.dwScale;
	printf("SamplesPerSec: %g\n",sps);
	double length=(sps != 0.0)?(double)aviInfo.dwLength/sps:0.0;
	printf("Length: %d (%g sec, %g min)\n",aviInfo.dwLength,
		length,length/60.0);
	printf("EditCount: %d\n",aviInfo.dwEditCount);
	printf("FileType: %s\n",aviInfo.szFileType);
	printf("\n");
}

void printStreamInfo(AVISTREAMINFO avis) {
	if (avis.fccType == streamtypeVIDEO) { 
		printf("  VIDEO\n");
	} else if (avis.fccType == streamtypeAUDIO) { 
		printf("  AUDIO\n");
	}  else if (avis.fccType == streamtypeMIDI) {  
		printf("  MIDI\n");
	}  else if (avis.fccType == streamtypeTEXT) {  
		printf("  TEXT\n");
	} else {
		printf("  Unknown fccType [%d]\n",avis.fccType);
	}
	printf("  Priority: %hd\n",avis.wPriority);
	printf("  Flags:\n");
	if(avis.dwFlags & AVISTREAMINFO_DISABLED)
		printf("    DISABLED\n");
	if(avis.dwFlags & AVISTREAMINFO_FORMATCHANGES)
		printf("    FORMATCHANGES\n");
	printf("  SuggestedBufferSize: %d bytes\n",avis.dwSuggestedBufferSize);
	printf("  Quality: %d [0-10,000]\n",avis.dwQuality);
	printf("  Left=%d Right=%d Top=%d Bottom=%d\n",
		avis.rcFrame.left,avis.rcFrame.right,
		avis.rcFrame.top,avis.rcFrame.bottom);
	printf("  Width=%d Height=%d\n",
		avis.rcFrame.right-avis.rcFrame.left,
		avis.rcFrame.bottom-avis.rcFrame.top);
	printf("  Start: %d\n",avis.dwStart);
	printf("  InitialFrames: %d\n",avis.dwInitialFrames);
	printf("  SampleSize: %d\n",avis.dwSampleSize);
	printf("  Scale: %d\n",avis.dwScale);
	printf("  Rate: %d\n",avis.dwRate);
	double sps=(double)avis.dwRate/(double)avis.dwScale;
	printf("    SamplesPerSec: %g\n",sps);
	double length=(sps != 0.0)?(double)avis.dwLength/sps:0.0;
	printf("  Length: %d (%g sec, %g min)\n",avis.dwLength,
		length,length/60.0);
	printf("  EditCount: %d\n",avis.dwEditCount);
	printf("  FormatChangeCount: %d\n",avis.dwFormatChangeCount);
	printf("  Name: %s\n",avis.szName);
}

void printBmiHeaderInfo(char *prefix, BITMAPINFOHEADER header) {
	printf(prefix);
	printf("biSize: %d\n",header.biSize);
	printf(prefix);
	printf("biWidth=%d biHeight=%d\n", header.biWidth, header.biHeight);
	printf(prefix);
	printf("biPlanes=%hd biBitCount=%hd\n", header.biPlanes, header.biBitCount);
	DWORD biCompression = header.biCompression;
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
	printf("biSizeImage: %d\n",header.biSizeImage);
	printf(prefix);
	printf("biXPelsPerMeter=%d biYPelsPerMeter=%d\n",
		header.biXPelsPerMeter, header.biYPelsPerMeter);
	printf(prefix);
	printf("biClrUsed=%hd biClrImportant=%hd\n",
		header.biClrUsed, header.biClrImportant);
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
