#include "stdafx.h"

// Code snippet obtained from http://www.codeproject.com/Questions/136471/AVI-decompression

int example1(char * filename, LONG frame) {
	AVIFileInit();
	PAVIFILE pFile;
	AVIFileOpen(&pFile, filename, OF_READ, 0);
	PAVISTREAM gapavi;
	AVIFileGetStream(pFile, &gapavi, streamtypeVIDEO, 0);
	AVISTREAMINFO avis = {0};
	AVIStreamInfo(gapavi, &avis, sizeof(avis));

	BITMAPINFO *bi_in, bi_out;
	// Should be sizeof(*bi_in)
	long biSize = sizeof(bi_out);
	AVIStreamReadFormat(gapavi, 0, 0, &biSize);
	bi_in = (BITMAPINFO*)malloc(biSize+sizeof(bi_out.bmiColors));
	bi_in->bmiHeader.biSize = biSize;
	AVIStreamReadFormat(gapavi, 0, bi_in, &biSize);
	bi_out = *bi_in;

	HIC hic = 0;
	// Check if the data is already in a recognised format (i.e. Uncompressed RGB or YUY2)
	if(bi_in->bmiHeader.biCompression != 0 && bi_in->bmiHeader.biCompression != mmioFOURCC('Y','U','Y','2')){
		hic = ICDecompressOpen('CDIV', 0, &bi_in->bmiHeader, 0);

		biSize = ICDecompressGetFormat(hic, bi_in, &bi_out);

		// Check if the codec decompresses to YUY2 by default
		switch(bi_out.bmiHeader.biCompression){
		case mmioFOURCC('Y','U','Y','2'):
			bi_out.bmiHeader.biBitCount = 16;
			bi_out.bmiHeader.biSizeImage = 
				bi_out.bmiHeader.biWidth*bi_out.bmiHeader.biHeight*bi_out.bmiHeader.biBitCount/8;
			break;
		case 0:
			bi_out.bmiHeader.biBitCount = 24;
			bi_out.bmiHeader.biSizeImage =
				bi_out.bmiHeader.biWidth*bi_out.bmiHeader.biHeight*bi_out.bmiHeader.biBitCount/8;
			break;
		default:
			// Check if the codec is able to decompress to this format
			bi_out.bmiHeader.biCompression = mmioFOURCC('Y','U','Y','2');
			bi_out.bmiHeader.biBitCount = 16;
			bi_out.bmiHeader.biSizeImage =
				bi_out.bmiHeader.biWidth*bi_out.bmiHeader.biHeight*bi_out.bmiHeader.biBitCount/8;
			if(ICDecompressQuery(hic, &bi_in, &bi_out) != ICERR_OK)
				return 0;
		}
		ICDecompressBegin(hic,bi_in, &bi_out);
	}

	int in_buf_siz  = avis.dwSuggestedBufferSize;
	int out_buf_siz = bi_out.bmiHeader.biSizeImage;
	void *inbuf, *outbuf;
	if(hic)
		inbuf = malloc(max(out_buf_siz,in_buf_siz));

	// This is the buffer the pixels are going to be in
	unsigned char *AVIbuffer = (unsigned char *)malloc(out_buf_siz);
	if(hic)
		outbuf = AVIbuffer;
	else
		inbuf  = AVIbuffer;

	long size = 0;
	AVIStreamRead(gapavi, frame-1, 1, inbuf, in_buf_siz, &size, 0);

	// Decompress the data
	if(hic){
		bi_in->bmiHeader.biSizeImage = size;
		ICDecompress(hic, 0, &bi_in->bmiHeader, inbuf, &bi_out.bmiHeader, outbuf);
	}

	// At this point, I access AVIbuffer to find the pixel values of the current frame (be it in RGB or YUY2).

	return 1;
}