// ConvertVideo.cpp : Defines the convert video method.

#include "stdafx.h"

HRESULT decompressVideo(PAVISTREAM pStream1, PAVISTREAM pStream2) {
	if(!pStream1) {
		errMsg("decompressVideo: stream 1 is null"); 
		return AVIERR_BADPARAM; 
	}
	if(!pStream2) {
		errMsg("decompressVideo: stream 2 is null"); 
		return AVIERR_BADPARAM; 
	}

	HRESULT hrReturnVal = AVIERR_OK;
	HRESULT hr;
	DWORD res;
	HIC hic = NULL;
	BITMAPINFO bmi1, bmi2;
	BITMAPINFOHEADER bmih1, bmih2;
	char *pBuf1 = NULL;
	char *pBuf2 = NULL;

	long biSize = sizeof(bmi1);
	ZeroMemory(&bmi1, biSize);
	hr = AVIStreamReadFormat(pStream1, 0, &bmi1, &biSize);
	if(hr != AVIERR_OK) {
		errMsg("Cannot get BITMAPINFO [Error 0x%08x %s]", hr, getErrorCode(hr)); 
		return hr; 
	}

	// Get the stream info from the input stream
	AVISTREAMINFO streamInfo1;
	ZeroMemory(&streamInfo1, sizeof(streamInfo1));
	hr=AVIStreamInfo(pStream1,&streamInfo1,sizeof(streamInfo1)); 
	if(hr != 0) { 
		errMsg("Cannot read stream info 1 [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		return hr; 
	}

	// Determine compression
	bmih1 = bmi1.bmiHeader;
	printBmiHeaderInfo("    ", bmih1);
	DWORD biCompression = bmih1.biCompression;
	printf("  Compression: ");
	printFourCcCode(biCompression, "\n");

	// Open the decompressor
	hic = ICDecompressOpen(ICTYPE_VIDEO, biCompression, &bmih1, NULL);
	if(hic) {
		printf("  ICDecompressOpen successful\n");
	} else {
		printf("  ICDecompressOpen unsuccessful\n");
		goto CLEANUP;
	}

	// Get the output format
	res = ICDecompressGetFormat(hic, &bmi1, &bmi2);
	if(res != ICERR_OK) {
		printf("Default output format not available\n");
		goto CLEANUP;
	}
	bmih2 = bmi2.bmiHeader;
	printf("  Default output format:\n");
	printBmiHeaderInfo("    ", bmih2);

#if 1
	// Get the buffer sizes for the input
	LONG sizeMin;
	LONG sizeMax;
	LONG nSizeErrors;
	getBufferSizes(pStream1, &sizeMin, &sizeMax, &nSizeErrors);
	printf("  Required buffer sizes for input stream\n");
	printf("  dwSuggestedBufferSize=%d sizeMin=%d sizeMax=%d nSizeErrors=%d\n",
		streamInfo1.dwSuggestedBufferSize, sizeMin, sizeMax, nSizeErrors);
#endif

	// Loop over the frames
	printf("  Processing %d frames starting at frame %d...\n",
		streamInfo1.dwLength, AVIStreamStart(pStream1));
	// The input buffer will be allocated with a determined size for each frame
	LONG bufSize1 = 0;
	// The output buffer is the size of the image
	LONG bufSize2 = bmih2.biSizeImage;
	pBuf2 = new char[bufSize2];
	LONG nFrames = AVIStreamStart(pStream1);
	// Set the start pBuf1
	DWORD nKeyFrames = 0;
	DWORD nFramesProcessed = 0;
	DWORD maxErrors = 10;
	DWORD nErrors = 0;
	LONG frameOut = AVIStreamStart(pStream1);
	for(int i = AVIStreamStart(pStream1); i < AVIStreamEnd(pStream1); i++) {
		// Find bufSize1
		hr = AVIStreamRead(pStream1, i, 1, NULL, 0, &bufSize1, NULL);
		if(hr != AVIERR_OK && hr != AVIERR_ERROR) {
			if(++nErrors >= maxErrors) {
				goto ERRORS;
			}
			errMsg("Error reading stream size at frame %d [Error 0x%08x %s]",
				i, hr, getErrorCode(hr)); 
		}
		// Allocate space
		pBuf1 = new char[bufSize1];
#if 0
		if(bufSize1 != streamInfo1.dwSuggestedBufferSize) {
			printf("Non-sandard frame size %d at frame %d\n", bufSize1, i);
		}
#endif

		// Get the frame
		hr = AVIStreamRead(pStream1, i, 1, pBuf1, bufSize1, NULL, NULL);
		if(hr != AVIERR_OK) {
			if(++nErrors >= maxErrors) {
				goto ERRORS;
			}
			errMsg("Error reading stream at frame %d [Error 0x%08x %s]",
				i, hr, getErrorCode(hr)); 
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

	if(hic) ICClose(hic);
	hic = NULL;
	if(pBuf1) delete [] pBuf1;
	pBuf1 = NULL;
	if(pBuf2) delete [] pBuf2;
	pBuf2 = NULL;

	return AVIERR_OK;
}
