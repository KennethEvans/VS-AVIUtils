// Convert.cpp : Defines the convert video method.

#include "stdafx.h"

HRESULT convertVideo(PAVIFILE pFile2, PAVISTREAM pStreamIn) {
	if(!pStreamIn) {
		errMsg("PAVISTREAM input is null"); 
		return AVIERR_BADPARAM; 
	}

	HRESULT hrReturnVal = AVIERR_OK;
	HRESULT hr;
	DWORD res;
	HIC hic;
	BITMAPINFO bmi;
	long biSize = sizeof(bmi);
	ZeroMemory(&bmi, biSize);
	hr = AVIStreamReadFormat(pStreamIn, 0, &bmi, &biSize);
	if(hr != AVIERR_OK) {
		errMsg("Cannot get BITMAPINFO [Error 0x%08x %s]", hr, getErrorCode(hr)); 
		return hr; 
	}

	// Get the stream info from the input stream
	AVISTREAMINFO streamInfoOut;
	ZeroMemory(&streamInfoOut, sizeof(streamInfoOut));
	hr=AVIStreamInfo(pStreamIn,&streamInfoOut,sizeof(streamInfoOut)); 
	if(hr != 0) { 
		errMsg("Cannot read stream info [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		return hr; 
	}


	// Determine compression
	BITMAPINFOHEADER header = bmi.bmiHeader;
	printBmiHeaderInfo("    ", header);
	DWORD biCompression = header.biCompression;
	printf("  Compression: ");
	printFourCcCode(biCompression, "\n");

	// Open the decompressor
	hic = ICDecompressOpen(ICTYPE_VIDEO, biCompression, &header, NULL);
	if(hic) {
		printf("  ICDecompressOpen successful\n");
	} else {
		printf("  ICDecompressOpen unsuccessful\n");
		goto CLEANUP;
	}

	// Get the output format
	BITMAPINFO bmiOut;
	res = ICDecompressGetFormat(hic, &bmi, &bmiOut);
	if(res != ICERR_OK) {
		printf("Default output format not available\n");
		goto CLEANUP;
	} else {
		printf("  Default output format:\n");
		printBmiHeaderInfo("    ", bmiOut.bmiHeader);
	}

	// Loop over the frames
	char *frame = NULL;
	LONG frameSize = 0;
	LONG nFrames = AVIStreamStart(pStreamIn);
	// Set the start frame
	DWORD startFrame = streamInfoOut.dwStart;
	DWORD nKeyFrames = 0;
	DWORD nFramesProcessed = 0;
	DWORD maxErrors = 10;
	DWORD nErrors = 0;
	LONG minFrameSize = 0x7fffffff;  // TODO do this better
	LONG maxFrameSize = 0;
	LONG frameOut = AVIStreamStart(pStreamIn);
	for(int i = AVIStreamStart(pStreamIn); i < AVIStreamEnd(pStreamIn); i++) {
		// Find the frameSize
		hr = AVIStreamRead(pStreamIn, i, 1, NULL, 0, &frameSize, NULL);
		if(hr != AVIERR_OK && hr != AVIERR_ERROR) {
			if(++nErrors >= maxErrors) {
				goto ERRORS;
			}
			errMsg("Error reading stream size at frame %d [Error 0x%08x %s]",
				i, hr, getErrorCode(hr)); 
		}
		if(frameSize > maxFrameSize) maxFrameSize = frameSize;
		if(frameSize < minFrameSize) minFrameSize = frameSize;
		// Allocate space
		frame = new char[frameSize];
		// Get the frame
		hr = AVIStreamRead(pStreamIn, i, 1, frame, frameSize, NULL, NULL);
		if(hr != AVIERR_OK) {
			if(++nErrors >= maxErrors) {
				goto ERRORS;
			}
			errMsg("Error reading stream at frame %d [Error 0x%08x %s]",
				i, hr, getErrorCode(hr)); 
		}
	}

ERRORS:
	printf("More than %d errors, aborting\n", maxErrors);
	hrReturnVal = hr;

CLEANUP:
	ICClose(hic);
	return AVIERR_OK;
}
