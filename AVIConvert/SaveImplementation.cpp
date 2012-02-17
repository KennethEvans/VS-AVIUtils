// TestCompression.cpp : Experiment with compression.

// CopyImplementation.cpp : Provides the implementation of AVIConvert that uses
// AVIFileCreateStream, AVIStreamRead, and AVIStreamWrite

#include "stdafx.h"
#include <string.h>
#include <math.h>

#define PATH_MAX _MAX_PATH
#define DEBUG 0

// Set this to save just one output file rather than breaking them up
// so the sizes are less than 1 GB
#define SAVE_SINGLE_FILE 0

// The maximum time interval in ms per file
// 2 min = 20*60*1000 = 1200000
#define MAX_TIME 1200000

// Global variables
extern char aviFileName1[PATH_MAX];
extern char aviFileName2[PATH_MAX];
extern int aviFileSpecified1;
extern int aviFileSpecified2;

// Function prototypes;
void printStreamParameters(char* prefix, int iFile, int iStream, PAVISTREAM pStream);

int SaveImplementation() {
	int retVal = 0;
	HRESULT hr;
	BOOL libOpened = FALSE;
	PAVIFILE pFile1 = NULL; 
	AVIFILEINFO fileInfo1;
	PAVISTREAM *pStreams1 = NULL;
	PAVISTREAM pStream = NULL;
	LPAVICOMPRESSOPTIONS *pOpts1 = NULL;
	char **ppFileNames2 = NULL;
	PAVISTREAM **ppStreams2 = NULL;
	int i, n;
	int nStreams=0;

	// Initialize library
	AVIFileInit();
	libOpened=TRUE;

	// Check file size before opening the source file
	int fileLen1 = getFileSize(aviFileName1);

	// Open source file
	hr = AVIFileOpen(&pFile1, aviFileName1, OF_SHARE_DENY_WRITE, 0L); 
	if(hr != AVIERR_OK) { 
		errMsg("Unable to open %s\n  [Error 0x%08x %s]",
			aviFileName1, hr, getErrorCode(hr)); 
		goto ABORT; 
	}

	// Print warning if the file size is < 1 GB
	//if(fileLen1 >= 1024*1024*1024) {
	//	errMsg("\n*** Warning: Input file size [%.2f GB] is 1 GB or greater\n"
	//		"This may cause problems", fileLen1/1024./1024./1024.);
	//}

	// Get source info
	hr = AVIFileInfo(pFile1,&fileInfo1,sizeof(fileInfo1));
	if(hr != AVIERR_OK) { 
		errMsg("Unable to get info from %s\n  [Error 0x%08x %s]",
			aviFileName1, hr, getErrorCode(hr)); 
		goto ABORT; 
	}
	nStreams=fileInfo1.dwStreams;

	// Determine the number of output files required
	// Length of file in ms
	double timeLen = 1000. * (double)fileInfo1.dwLength *
		(double)fileInfo1.dwScale /(double)fileInfo1.dwRate;
	double maxTime = 0;
	int nFiles2 = 0;
	while(maxTime <= timeLen) {
		nFiles2++;
		maxTime += MAX_TIME;
	}
	printf("\nThe input file has a length of %.2f min\n", timeLen / 60000.);
	printf("There will be %d output file(s) of maximum length %.1f min\n",
		nFiles2, MAX_TIME / 60000.);

	// Allocate the files and PAVISTREAM's arrays
	ppFileNames2 = new char*[nFiles2];
	ppStreams2 = new PAVISTREAM*[nFiles2];

	// Create the names
	int len = strlen(aviFileName2) + 1;
	char *prefix = new char[len];
	strcpy(prefix, aviFileName2);
	// Find the last dot
	char *pExtension = strrchr(prefix, '.');
	if(pExtension != NULL) {
		// Prefix will end at the last dot
		*pExtension = '\0';
		// pExtension should point to "avi" or whatever else is the extension
		pExtension++;
	}

	// Create the files and output streams arrays
	int idWidth = (int)(log10((double)nFiles2) + 1.5);
	for(n = 0; n < nFiles2; n++) {
		ppStreams2[n] = new PAVISTREAM;
		ppFileNames2[n] = new char[len + idWidth + 1];
		// Calculate the members of the array
		sprintf(ppFileNames2[n], "%s.%0*d.%s", prefix, idWidth, n,
			pExtension?pExtension : "");
		printf("  %s\n", ppFileNames2[n]);
	}
	if(prefix) delete [] prefix;

	// Print the fileInfo
	printf("\n");
	printFileInfo(fileInfo1);

	// Allocate the input streams
	if(nStreams <= 0) {
		errMsg("No streams in %s",aviFileName1); 
		goto ABORT;
	}
	pStreams1=new PAVISTREAM[nStreams];
	if(!pStreams1) {
		errMsg("Cannot allocate streams array"); 
		goto ABORT;
	}

	// Allocate the opts
	pOpts1 = new LPAVICOMPRESSOPTIONS[nStreams];
	if(!pStreams1) {
		errMsg("Cannot allocate streams array"); 
		goto ABORT;
	}

	// Loop over streams
	AVISTREAMINFO streamInfo;
	for (i = 0; i < nStreams; i++) {
		// Get the stream
		printf("\nStream %d\n", i);
		pStreams1[i] = NULL; 
		hr=AVIFileGetStream(pFile1, &pStreams1[i], 0L, i) ;
		if(hr != AVIERR_OK || pStreams1[i] == NULL) {
			errMsg("Cannot open stream %d [Error 0x%08x %s]",
				i, hr, getErrorCode(hr)); 
			continue; 
		}

		// Get stream info
		hr = AVIStreamInfo(pStreams1[i], &streamInfo, sizeof(streamInfo)); 
		if(hr != AVIERR_OK) { 
			errMsg("Unable to get stream info for stream %d [Error 0x%08x %s]",
				i, hr, getErrorCode(hr)); 
			goto ABORT; 
		}
		printStreamInfo(streamInfo);

		// Create the AVICOMPRESSOPTIONS
		pOpts1[i] = new  AVICOMPRESSOPTIONS;
		ZeroMemory(pOpts1[i], sizeof(AVICOMPRESSOPTIONS));
		(*pOpts1[i]).fccType = streamInfo.fccType;

		// Print AVICOMPRESSOPTIONS
		printf("\n  AVICOMPRESSOPTIONS\n");
		printCompressOptions(*pOpts1[i]);

		// Create the output streams as slices of the input streams
		printf("\nCalculating slices for stream %d...\n", i);
		PAVISTREAM pStream = pStreams1[i];
		PAVISTREAM pEditStream = NULL;
		hr = CreateEditableStream(&pEditStream, pStream);
		if(hr != AVIERR_OK) { 
			errMsg("Unable to get editable stream for stream %d"
				" [Error 0x%08x %s]",
				i, hr, getErrorCode(hr)); 
			goto ABORT; 
		}
		PAVISTREAM pEditStreamCopy = NULL;
		LONG lPos = 0;
		LONG streamMax = AVIStreamEnd(pStream) - 1;
		LONG sliceMin = AVIStreamStart(pStream);
		LONG sliceMax = 0;
		LONG sliceLength = 0;
		LONG sliceMinOut, sliceLengthOut;
		for(n = 0; n < nFiles2; n++) {
			ppStreams2[n][i] = NULL;
			sliceMax = AVIStreamTimeToSample(pStream, (n + 1) * MAX_TIME);
			if(sliceMax > streamMax) sliceMax = streamMax;
			sliceLength = sliceMax - sliceMin + 1;
			sliceMinOut = sliceMin;
			sliceLengthOut = sliceLength;
			hr = EditStreamCopy(pEditStream, &sliceMinOut, &sliceLengthOut,
				&pEditStreamCopy);
			if(hr != AVIERR_OK) { 
				errMsg("Unable to get slice for stream %d file %d"
					" [Error 0x%08x %s]",
					i, n, hr, getErrorCode(hr)); 
				goto ABORT; 
			}
			if(sliceMinOut != sliceMin || sliceLengthOut != sliceLength) {
				errMsg("Error getting slice for stream %d file %d:\n"
					"  streamMax=%d, sliceMin=%d sliceMinOut=%d,"
					" sliceLength=%d sliceLengthOut=%d",
					i, n, streamMax, sliceMin, sliceMinOut,
					sliceLength, sliceLengthOut);
				goto ABORT; 
			}
			ppStreams2[n][i] = pEditStreamCopy;
			sliceMin = sliceMax + 1;
			printStreamParameters("pEditStream     ", n, i, pEditStream);
			printStreamParameters("pEditStreamCopy ", n, i, pEditStreamCopy);
			//AVIStreamRelease(pEditStreamCopy);
		}
		AVIStreamRelease(pEditStream);
#if 1
		printf("\nStream Parameters\n");
		for(n = 0; n < nFiles2; n++) {
			printStreamParameters("", n, i, ppStreams2[n][i]);
		}
#endif
		//// Release the streams for all the files
		//// Do this here so we can use them for debugging
		//for(n = 0; n < nFiles2; n++) {
		//	AVIStreamRelease(ppStreams2[n][i]);
		//}
	}

	// Save the file(s)
#if SAVE_SINGLE_FILE
	printf("\nSaving single file...\n");
	hr = AVISaveV(aviFileName2, NULL, NULL, nStreams, pStreams1, pOpts1);
	if(hr != AVIERR_OK) {
		errMsg("Error saving file [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		goto ABORT; 
	} else {
		printf("\nSaved %s\n", aviFileName2);
	}
#else
	printf("\nSaving file(s)...\n");
	for(n = 0; n < nFiles2; n++) {
		printf("\nSaving %s...\n", ppFileNames2[n]);
#if 0
		for(i = 0; i < nStreams; i++) {
			printf("\nAVICOMPRESSOPTIONS for stream %d\n", i);
			printCompressOptions(*pOpts1[i]);
			printf("\n");
		}
#endif
#if 1
		for(i = 0; i < nStreams; i++) {
			printf("\nAVISTREAMINFO for stream %d\n", i);
			printStreamInfo(ppStreams2[n][i]);
			printf("\n");
		}
#endif

		hr = AVISaveV(ppFileNames2[n], NULL, NULL, nStreams, ppStreams2[n],
			pOpts1);
		if(hr != AVIERR_OK) {
			errMsg("Error saving file [Error 0x%08x %s]",
				hr, getErrorCode(hr)); 
			goto ABORT; 
		}
		int fileLen2 = getFileSize(ppFileNames2[n]);
		printf("  Size is %.2f GB\n", fileLen2 /1024./1024./ 1024.);
	}
#endif

#if 0
	// Print AVICOMPRESSOPTIONS
	printf("\nAVICOMPRESSOPTIONS after\n");
	printCompressOptions(opts);
#endif

	goto CLEANUP;

ABORT:
	retVal = 1;

CLEANUP:
	if(pFile1) AVIFileRelease(pFile1);
	AVIFileExit();
	// Free space
	if(pStreams1) {
		delete [] pStreams1;
		pStreams1 = NULL;
	}
	if(pOpts1) {
		for(i = 0; i < nStreams; i++) {
			if(pOpts1[i]) delete pOpts1[i];
			pOpts1[i] = NULL;
		}
		delete [] pOpts1;
		pOpts1 = NULL;
	}
	if(ppFileNames2) {
		for(n = 0; n < nFiles2; n++) {
			if(ppFileNames2[n]) delete ppFileNames2[n];
			ppFileNames2[n] = NULL;
		}
		delete [] ppFileNames2;
		ppFileNames2 = NULL;
	}
	if(ppStreams2) {
		for(n = 0; n < nFiles2; n++) {
			if(ppStreams2[n]) delete ppStreams2[n];
			ppStreams2[n] = NULL;
		}
		delete [] ppStreams2;
		ppStreams2 = NULL;
	}

	printf("\nAll done\n");

	return retVal;
}

void printStreamParameters(char* prefix, int iFile, int iStream, PAVISTREAM pStream) {
	printf(prefix);
	printf("%2d %2d ", iFile, iFile);
	if(pStream == NULL) {
		printf("NULL\n");
	} else {
		LONG start = AVIStreamStart(pStream);
		LONG end = AVIStreamEnd(pStream);
		LONG length = AVIStreamLength(pStream);
		printf("0x%08x start=%d end=%d length=%d\n", pStream, start, end, length);
	}
}
