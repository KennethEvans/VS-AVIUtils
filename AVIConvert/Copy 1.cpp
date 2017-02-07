// Copy.cpp : Defines the copyStream1 method.
// Uses AVIFileCreateStream for an open output file
// Writes all frames at once to the output stream (in the output file)
// Either specifies AVIIF_KEYFRAME or not (currently doesn't)
// No AVIERR_MEMORY for audio streams at about frame 5000000,
//    but frames above that are not in the output file afterward
// Seems to do video ok, but frames are blank in KwikMedia (either keyframe option)

#include "stdafx.h"

HRESULT getBufferSizes(PAVISTREAM pStream, LONG *sizeMin, LONG *sizeMax, LONG *nErrors);


HRESULT copyStream1(PAVIFILE pFile2, PAVISTREAM pStream1) {
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

#if 1
	// Get the buffer sizes
	LONG sizeMin;
	LONG sizeMax;
	LONG nSizeErrors;
	getBufferSizes(pStream1, &sizeMin, &sizeMax, &nSizeErrors);
	printf("  Required buffer sizes for input stream\n");
	printf("  sizeMin=%d sizeMax=%d nSizeErrors=%d\n",
		sizeMin, sizeMax, nSizeErrors);
#endif

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
	// Reset the suggested buffer size
	streamInfo1.dwSuggestedBufferSize =
		streamInfo1.dwSampleSize * streamInfo1.dwLength;
	printf("  Resetting dwSuggestedBufferSize=%d\n",
		streamInfo1.dwSuggestedBufferSize);

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

	// Copy at one time
	char *pBuf1 = NULL;
	LONG bufSize1 = 0;
	DWORD nKeyFrames = 0;
	DWORD nErrors = 0;
	LONG frameOut = AVIStreamStart(pStream1);
	hr = AVIStreamRead(pStream1, streamInfo1.dwStart, streamInfo1.dwLength,
		NULL, 0, &bufSize1, NULL);
	if(hr != AVIERR_OK && hr != AVIERR_ERROR) {
		nErrors++;
		errMsg("Error reading stream size [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
	}
	pBuf1 = new char[bufSize1];
	hr = AVIStreamRead(pStream1,streamInfo1.dwStart, streamInfo1.dwLength,
		pBuf1, bufSize1, NULL, NULL);
	if(hr != AVIERR_OK) {
		nErrors++;
		errMsg("Error reading stream [Error 0x%08x %s]",
			hr, getErrorCode(hr));
	}
	hr = AVIStreamWrite(pStream2, streamInfo1.dwStart, streamInfo1.dwLength,
		pBuf1, bufSize1, 0, NULL, NULL);
		//pBuf1, bufSize1, AVIIF_KEYFRAME, NULL, NULL);
	if(hr != AVIERR_OK && hr != AVIERR_ERROR) {
		nErrors++;
		errMsg("Error writing stream [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
	}
	goto CLEANUP;

CLEANUP:
	printf("\n");
	printf("  bufSize1=%d\n", bufSize1);
	printf("  AVIStreamStart=%d AVIStreamEnd=%d\n",
		AVIStreamStart(pStream1), AVIStreamEnd(pStream1));
	printf("  dwStart=%d dwLength=%d\n",
		streamInfo1.dwStart, streamInfo1.dwLength);
	printf("  %d frames processed, %d frames written, %d key frames\n",
		streamInfo1.dwLength, streamInfo1.dwLength, nKeyFrames);
	printf("  %d frame errors\n", nErrors);

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
