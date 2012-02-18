// Copy.cpp : Defines the copyStream method.
// Uses AVIFileCreateStream for an open output file
// Loops over frames writing the output stream (in the output file)
// Handles keyframes individually
// Gets AVIERR_MEMORY for audio streams at about frame 5000000
// Seems to do video ok

#include "stdafx.h"

#define DEBUG_CRT 1

#if DEBUG_CRT
#  define _CRTDBG_MAP_ALLOC
#  include <stdlib.h>
#  include <crtdbg.h>
#endif

HRESULT getBufferSizes(PAVISTREAM pStream, LONG *sizeMin, LONG *sizeMax, LONG *nErrors);


HRESULT copyStream(PAVIFILE pFile2, PAVISTREAM pStream1) {
	if(!pFile2) {
		errMsg("PAVIFILE input is null"); 
		return AVIERR_BADPARAM; 
	}
	if(!pStream1) {
		errMsg("PAVISTREAM input is null"); 
		return AVIERR_BADPARAM; 
	}

	HRESULT hrReturnVal = AVIERR_OK;
	HRESULT hr;
	PAVISTREAM pStream2;

	// Get the format
	char *format1 = NULL;
	LONG formatSize1 = 0;
	hr = AVIStreamReadFormat(pStream1, 0, NULL, &formatSize1);
	if(hr != AVIERR_OK) {
		errMsg("Cannot read format size [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		hrReturnVal = hr;
		goto CLEANUP;
	}
	format1 = new char[formatSize1];
	hr = AVIStreamReadFormat(pStream1, 0,
		format1, &formatSize1);
	if(hr != AVIERR_OK) {
		errMsg("Cannot read format [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		hrReturnVal = hr;
		goto CLEANUP;
	}

	// Get the stream info from the input stream
	AVISTREAMINFO streamInfo1;
	ZeroMemory(&streamInfo1, sizeof(streamInfo1));
	hr=AVIStreamInfo(pStream1,&streamInfo1,sizeof(streamInfo1)); 
	if(hr != 0) { 
		errMsg("Cannot read stream info [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		hrReturnVal = hr;
		goto CLEANUP;
	}

	// Create the output stream
	hr = AVIFileCreateStream(pFile2, &pStream2, &streamInfo1);
	if(hr != AVIERR_OK) {
		errMsg("Cannot create stream [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		hrReturnVal = hr;
		goto CLEANUP;
	}

	// Set the format
	hr = AVIStreamSetFormat(pStream2, 0, format1, formatSize1);
	if(hr != AVIERR_OK) {
		errMsg("Cannot set format [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		hrReturnVal = hr;
		goto CLEANUP;
	}

#if DEBUG
	printf("formatSize1=%d\n", formatSize1);
	printf("BITMAPINFOHEADER=%d PCMWAVEFORMAT=%d WAVEFORMAT=%d WAVEFORMATEX=%d\n",
		sizeof(BITMAPINFOHEADER), sizeof(PCMWAVEFORMAT),
		sizeof(WAVEFORMAT), sizeof(WAVEFORMATEX));
#endif

#if 1
	// Get the buffer sizes
	LONG sizeMin;
	LONG sizeMax;
	LONG nSizeErrors;
	getBufferSizes(pStream1, &sizeMin, &sizeMax, &nSizeErrors);
	printf("  Required buffer sizes for input stream\n");
	printf("  dwSuggestedBufferSize=%d sizeMin=%d sizeMax=%d nSizeErrors=%d\n",
		streamInfo1.dwSuggestedBufferSize, sizeMin, sizeMax, nSizeErrors);
#endif

#if DEBUG_CRT
	OutputDebugString("\ncopyStream: before loop:\n");
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
	_CrtMemState s1;
	_CrtMemCheckpoint( &s1 );
	_CrtMemDumpStatistics( &s1 );
#endif

	// Loop over the frames
	char *pBuf1 = NULL;
	LONG bufSize1 = 0;
	LONG nFrames = AVIStreamStart(pStream1);
	// Set the start pBuf1
	DWORD startFrame = streamInfo1.dwStart;
	DWORD nKeyFrames = 0;
	DWORD nFramesProcessed = 0;
	DWORD maxErrors = 10;
	DWORD nErrors = 0;
	LONG frameOut = AVIStreamStart(pStream1);
	for(LONG i = AVIStreamStart(pStream1); i < AVIStreamEnd(pStream1); i++) {
		// Find the size for bufSize1
		hr = AVIStreamRead(pStream1, i, 1, NULL, 0, &bufSize1, NULL);
		if(hr != AVIERR_OK && hr != AVIERR_ERROR) {
			//if(++nErrors >= maxErrors) {
			//	goto ERRORS;
			//}
			//errMsg("Error reading stream size at frame %d [Error 0x%08x %s]",
			//	i, hr, getErrorCode(hr)); 
		}
		// Allocate space
		pBuf1 = new char[bufSize1];
		// Get the buffer
		hr = AVIStreamRead(pStream1, i, 1, pBuf1, bufSize1, NULL, NULL);
		if(hr != AVIERR_OK) {
			//if(++nErrors >= maxErrors) {
			//	goto ERRORS;
			//}
			//errMsg("Error reading stream at frame %d [Error 0x%08x %s]",
			//	i, hr, getErrorCode(hr)); 
		}

		// Write to the output stream
		if(AVIStreamIsKeyFrame(pStream1, i)) {
			nKeyFrames++;
			hr = AVIStreamWrite(pStream2, frameOut, 1, pBuf1, bufSize1,
				AVIIF_KEYFRAME, NULL, NULL);
		} else  {
			hr = AVIStreamWrite(pStream2, frameOut, 1, pBuf1, bufSize1,
				0, NULL, NULL);
		}
		if(hr != AVIERR_OK) {
#if DEBUG_CRT
			// Dump memory on first error
			if(nErrors < 1) {
				OutputDebugString("\ncopyStream: error:\n");
				_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
				_CrtMemState s2;
				_CrtMemCheckpoint( &s2 );
				_CrtMemDumpStatistics( &s2 );
			}
#endif
			if(++nErrors >= maxErrors) {
				goto ERRORS;
			}
			errMsg("Error writing stream at frame %d [Error 0x%08x %s]",
				i, hr, getErrorCode(hr)); 
		} else {
			frameOut++;
		}

		delete [] pBuf1;
		pBuf1 = NULL;
		nFrames++;
		nFramesProcessed++;
	}
	goto CLEANUP;

ERRORS:
	printf("More than %d errors, aborting\n", maxErrors);
	hrReturnVal = hr;

CLEANUP:
	printf("\n");
	printf("  AVIStreamStart=%d AVIStreamEnd=%d\n",
		AVIStreamStart(pStream1), AVIStreamEnd(pStream1));
	printf("  dwStart=%d dwLength=%d\n",
		streamInfo1.dwStart, streamInfo1.dwLength);
	printf("  %d frames processed, %d frames written, %d key frames\n",
		nFramesProcessed, frameOut - AVIStreamStart(pStream1), nKeyFrames);
	printf("  %d frame errors\n", nErrors);

#if DEBUG_CRT
	OutputDebugString("\ncopyStream: after loop:\n");
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
	_CrtMemState s3;
	_CrtMemCheckpoint( &s3 );
	_CrtMemDumpStatistics( &s3 );

	OutputDebugString("\ncopyStream: difference before/after loop:\n");
	_CrtMemState s4;
	_CrtMemDifference( &s4, &s1, &s3);
	_CrtMemDumpStatistics( &s4 );
#endif


#if 0
	// DEBUG
	printf("\nBefore AVIStreamRelease:\n");
	if(pStream2) {
		printStreamInfo(pStream2);
	}
#endif

	AVIStreamRelease(pStream2);

#if 0
	// DEBUG
	printf("\nAfter AVIStreamRelease:\n");
	if(pStream2) {
		printStreamInfo(pStream2);
	}
#endif
#if 0
	printf("\nAfter AVIStreamRelease:\n");
	if(pStream2) {
		printStreamInfo(pStream2);
	}
#endif

	if(format1) delete [] format1;
	if(pBuf1) delete [] pBuf1;

	return hrReturnVal;
}
