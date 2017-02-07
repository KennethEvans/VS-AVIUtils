// TestCompression.cpp : Experiment with compression.

// CopyImplementation.cpp : Provides the implementation of AVIConvert that uses
// AVIFileCreateStream, AVIStreamRead, and AVIStreamWrite

#include "stdafx.h"
#include <string.h>
#include <math.h>

#define PATH_MAX _MAX_PATH
#define DEBUG 0

// Set this to trim the input file to eliminated frames for which
// ACIStreamFindNextKeyframe returns -1
#define TRIM_BAD_KEYFRAMES 1

// Set this to determine the method used for handling keyframes
// If neither is defined, keyframes are not handled
// In order of precedence
#define DO_KEYFRAMES1 0
#define DO_KEYFRAMES2 1

// Set this to save just one output file rather than breaking them up
// so the sizes are less than 1 GB
#define SAVE_SINGLE_FILE 0

// The maximum time interval in ms per file
// 2 min = 20*60*1000 = 1200000
#define MAX_TIME 1200000

// If the last keyframe is beyond this fraction of the length corresponding
// to MAX_TIME, then end the frame just before the last keyframe
#define KEYFRAME_FRACT .75

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
	double fileLen1 = getFileSize(aviFileName1);

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
		// Define this for convenience
		PAVISTREAM pStream = pStreams1[i];

		// Get stream info
		hr = AVIStreamInfo(pStream, &streamInfo, sizeof(streamInfo)); 
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

		// Get keyframe information
		printf("\n  Getting keyframes...\n");
		LONG nKeyFrames = 0;
		LONG firstKeyFrame = -1;
		LONG lastKeyFrame = -1;
		LONG nBadNearest = 0;
		LONG firstBadNearest = -1;
		LONG lastBadNearest = -1;
		LONG nearestKeyFrame = -1;
		for(LONG f = AVIStreamStart(pStream); f < AVIStreamEnd(pStream); f++) {
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
		printf("    nKeyframes=%d firstKeyFrame=%d lastKeyFrame=%d\n",
			nKeyFrames, firstKeyFrame, lastKeyFrame);
		printf("    nBadNearest=%d firstBadNearest=%d lastBadNearest=%d\n",
			nBadNearest, firstBadNearest, lastBadNearest);

		// Create the output streams as slices of the input streams
		printf("\n  Calculating slices for stream %d...\n", i);
		// Create an editable stream from the input stream
		PAVISTREAM pEditStream = NULL;
		hr = CreateEditableStream(&pEditStream, pStream);
		if(hr != AVIERR_OK) { 
			errMsg("Unable to get editable stream for stream %d"
				" [Error 0x%08x %s]",
				i, hr, getErrorCode(hr)); 
			goto ABORT; 
		}
		LONG streamMax = AVIStreamEnd(pEditStream) - 1;

#if TRIM_BAD_KEYFRAMES
		PAVISTREAM pEditStreamCut = NULL;
		LONG cutStart = firstBadNearest;
		LONG cutEnd = streamMax;
		LONG cutLength = cutEnd - cutStart + 1;
		hr = EditStreamCut(pEditStream, &cutStart, &cutLength,
			&pEditStreamCut);
		if(hr != AVIERR_OK) { 
			errMsg("Unable to trim frames for stream %d file %d"
				" [Error 0x%08x %s]",
				i, n, hr, getErrorCode(hr)); 
			goto ABORT; 
		} else if(pEditStreamCut == NULL) {
			printf("Error trimming streams from stream %d file %d\n"
				"  Stream holding cut is NULL\n", i, n);
			goto ABORT; 
		} else {
			printf("  Trimmed %d frames from %d to %d in split stream\n",
				cutLength, cutStart, cutEnd);
			streamMax = AVIStreamEnd(pEditStream) - 1;
			printf("    New streamMax=%d\n", streamMax);
			// We don't use the cut part, so release it now
			AVIStreamRelease(pEditStreamCut);
		}
#endif

		// This will be used as the pointer to the editable stream that
		// will be created for each file.
		PAVISTREAM pEditStreamFile = NULL;
		LONG lPos = 0;
		LONG sliceMin = AVIStreamStart(pEditStream);
		LONG sliceMax = 0;
		double sliceMaxLength = (double)MAX_TIME * (double)streamInfo.dwRate /
			(1000. * (double)streamInfo.dwScale);
		LONG sliceLength = 0;
		LONG sliceMinOut, sliceLengthOut;
#if DO_KEYFRAMES1
		LONG nearestKeyFrame;
#endif
		int doInsertKeyFrame;
		// Loop over files
		for(n = 0; n < nFiles2; n++) {
			doInsertKeyFrame = 0;

			// Check if the stream length still requires this file
			ppStreams2[n][i] = NULL;
			if(sliceMin > streamMax) {
				printf("  There is no longer a need for file %d\n", n);
				continue;
			}

			sliceMax = AVIStreamTimeToSample(pEditStream, (n + 1) * MAX_TIME);
			if(sliceMax > streamMax) sliceMax = streamMax;
#if DO_KEYFRAMES1
			// If there are keyframes and we are not at the end and we are not
			// just before a keyframe, then adjust
			nearestKeyFrame = AVIStreamNearestKeyFrame(pEditStream, sliceMax);
			if(nKeyFrames > 0 && sliceMax < streamMax &&
				!AVIStreamIsKeyFrame(pEditStream, sliceMax + 1) &&
				nearestKeyFrame >= 0)
			{
				// Handle the last frame
				if(nearestKeyFrame > KEYFRAME_FRACT * sliceMaxLength) {
					// We will end the slice here, otherwise go with it as is
					printf("Changing sliceEnd from %d to %d ("
						" before previous keyframe)\n",
						sliceMax, nearestKeyFrame - 1);
					sliceMax = nearestKeyFrame - 1;
				}
			}
			// Handle the first frame in the slice
			if(nKeyFrames > 0  && nearestKeyFrame >= 0) {
				if(!AVIStreamIsKeyFrame(pEditStream, sliceMin)) {
					// The first frame is not a key frame so make it one
					doInsertKeyFrame = 1;
					// Use the first one before the first frame
					nearestKeyFrame = AVIStreamNearestKeyFrame(pEditStream,
						sliceMin);
					if(nearestKeyFrame < 0) {
						printf("Beginning is not a kayframe and cannot"
							" find a keyframe to insert\n");
					} else {
						printf("Inserting the keyframe at %d to the beginning\n",
							nearestKeyFrame);
					}
				}
			}
#endif
			// Copy the slice to pEditStreamFile
			sliceLength = sliceMax - sliceMin + 1;
			sliceMinOut = sliceMin;
			sliceLengthOut = sliceLength;
			hr = EditStreamCopy(pEditStream, &sliceMinOut, &sliceLengthOut,
				&pEditStreamFile);
			if(hr != AVIERR_OK) { 
				errMsg("Unable to get slice for stream %d file %d"
					" [Error 0x%08x %s]",
					i, n, hr, getErrorCode(hr)); 
				goto ABORT; 
			}
			if(sliceMinOut != sliceMin || sliceLengthOut != sliceLength) {
				errMsg("Error getting slice for stream %d file %d:\n"
					"  streamMax=%d\n"
					"  sliceMin=%d sliceMinOut=%d\n"
					"  sliceLength=%d sliceLengthOut=%d",
					i, n, streamMax, sliceMin, sliceMinOut,
					sliceLength, sliceLengthOut);
				goto ABORT; 
			}
#if DO_KEYFRAMES1
			// Handle first frame is not a keyframe using method 1
			if(doInsertKeyFrame) {
				LONG start1 = nearestKeyFrame;
				LONG length1 = 1;
				// Put it at the beginning
				LONG start2 = 0;
				LONG length2 = 1;
				hr = EditStreamPaste(pEditStreamFile, &start2, &length2,
					pEditStream, start1, length1);
				if(hr != AVIERR_OK) { 
					errMsg("Unable to insert keyframe for stream %d file %d"
						" [Error 0x%08x %s]",
						i, n, hr, getErrorCode(hr)); 
					goto ABORT; 
				} else {
					printf("Added keyframe from original position %d"
						" to position %d in split stream\n",
						start1, start2);
				}
			}
#elif DO_KEYFRAMES2
			// Handle first frame is not a keyframe using method2
			// If the first frame is not a keyframe, insert frame 0
			if(!AVIStreamIsKeyFrame(pEditStream, sliceMin)) {
				// Frame 0
				LONG start1 = 0;
				LONG length1 = 1;
				// Put it at the beginning
				LONG start2 = 0;
				LONG length2 = 1;
				hr = EditStreamPaste(pEditStreamFile, &start2, &length2,
					pEditStream, start1, length1);
				if(hr != AVIERR_OK) { 
					errMsg("Unable to insert keyframe 0 for stream %d file %d"
						" [Error 0x%08x %s]",
						i, n, hr, getErrorCode(hr)); 
					goto ABORT; 
				} else {
					printf("  Added keyframe from original position %d"
						" to position %d in split stream\n",
						start1, start2);
				}
			}
#endif

			// Set the editable stream as the output stream
			ppStreams2[n][i] = pEditStreamFile;
			sliceMin = sliceMax + 1;
#if 1
			// DEBUG
			printf("\n  Slice for file %d:\n"
					"    sliceMax=%d streamMax=%d\n"
					"    sliceMin=%d sliceMinOut=%d\n"
					"    sliceLength=%d sliceLengthOut=%d\n"
					"    doInsertKeyFrame=%d  sliceMaxLength=%.2f\n", 
					n,
					sliceMax, streamMax, sliceMin, sliceMinOut,
					sliceLength, sliceLengthOut,
					doInsertKeyFrame, sliceMaxLength);
			printStreamParameters("    pEditStream     ", n, i, pEditStream);
			printStreamParameters("    pEditStreamFile ", n, i, pEditStreamFile);
#endif
		}
		// We are through with the editable stream created for this stream
		AVIStreamRelease(pEditStream);
		pEditStream = NULL;
#if 1
		printf("\n  Outout Stream Parameters for stream %d\n", i);
		for(n = 0; n < nFiles2; n++) {
			printStreamParameters("    ", n, i, ppStreams2[n][i]);
		}
#endif
		// Do not release the new streams here
		// AVIFileSaveV will not work
#if 0
		// Release the streams for all the files
		// Do this here so we can use them for debugging
		for(n = 0; n < nFiles2; n++) {
			AVIStreamRelease(ppStreams2[n][i]);
		}
#endif
	}

	// Save the file(s)
#if SAVE_SINGLE_FILE
	// Save as a single file without breaking into slices
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
	// Save the slices
	printf("\nSaving file(s)...\n");
	for(n = 0; n < nFiles2; n++) {
		printf("\nSaving %s...\n", ppFileNames2[n]);

		// Check if there are streams for this file
		int doSave = 1;
		for(i = 0; i < nStreams; i++) {
			if(ppFileNames2[n][i] == NULL) {
				doSave = 1;
				printf("  There is no data for stream %d\n", i);
			}
		}
		if(!doSave) {
			printf(" Not saving this file\n");
			continue;
		}
#if 0
		// Print the nAVICOMPRESSOPTIONS for debugging
		for(i = 0; i < nStreams; i++) {
			printf("\nAVICOMPRESSOPTIONS for stream %d\n", i);
			printCompressOptions(*pOpts1[i]);
			printf("\n");
		}
#endif
#if 0
		// Print the AVISTREAMINFO for debugging
		for(i = 0; i < nStreams; i++) {
			printf("\nAVISTREAMINFO for stream %d\n", i);
			printStreamInfo(ppStreams2[n][i]);
			printf("\n");
		}
#endif
		// Save the file using AVISaveV
		hr = AVISaveV(ppFileNames2[n], NULL, NULL, nStreams, ppStreams2[n],
			pOpts1);
		if(hr != AVIERR_OK) {
			errMsg("Error saving file [Error 0x%08x %s]",
				hr, getErrorCode(hr)); 
			goto ABORT; 
		}
		double fileLen2 = getFileSize(ppFileNames2[n]);
		if(fileLen2 == -1) {
			printf("  Error getting file size\n");
		} else {
			printf("  Size is %.2f GB\n", fileLen2 /1024./1024./ 1024.);
		}
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
			if(pOpts1[i]) delete [] pOpts1[i];
			pOpts1[i] = NULL;
		}
		delete [] pOpts1;
		pOpts1 = NULL;
	}
	if(ppFileNames2) {
		for(n = 0; n < nFiles2; n++) {
			if(ppFileNames2[n]) delete  [] ppFileNames2[n];
			ppFileNames2[n] = NULL;
		}
		delete [] ppFileNames2;
		ppFileNames2 = NULL;
	}
	if(ppStreams2) {
		for(n = 0; n < nFiles2; n++) {
			if(ppStreams2[n]) delete [] ppStreams2[n];
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
	// Note this is reversed from the arguments and the order of the array indices
	printf("stream=%d file=%d ", iStream, iFile);
	if(pStream == NULL) {
		printf("NULL\n");
	} else {
		LONG start = AVIStreamStart(pStream);
		LONG end = AVIStreamEnd(pStream);
		LONG length = AVIStreamLength(pStream);
		printf("0x%08x start=%d end=%d length=%d\n", pStream, start, end, length);
	}
}
