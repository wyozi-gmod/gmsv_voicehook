#include "voice.h"

#include <string.h>

// valve_minmax_on.h
#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

Voice::Voice()
{
	m_Quality = 0;
	m_EncoderState = NULL;
	m_DecoderState = NULL;

	m_nEncodeBufferSamples = 0;
	m_nRawBytes = 0;
	m_nRawSamples = 0;
	m_nEncodedBytes = 0;
}

int Voice::Compress(const char *pUncompressedBytes, int nSamples, char *pCompressed, int maxCompressedBytes, bool bFinal)
{
	const short *pUncompressed = (const short *)pUncompressedBytes;
	int nCompressedBytes = 0;

	while ((nSamples + m_nEncodeBufferSamples) >= m_nRawSamples && (maxCompressedBytes - nCompressedBytes) >= m_nEncodedBytes)
	{
		// Get the data block out.
		short samples[MAX_FRAMEBUFFER_SAMPLES];
		memcpy(samples, m_EncodeBuffer, m_nEncodeBufferSamples * BYTES_PER_SAMPLE);
		memcpy(&samples[m_nEncodeBufferSamples], pUncompressed, (m_nRawSamples - m_nEncodeBufferSamples) * BYTES_PER_SAMPLE);
		nSamples -= m_nRawSamples - m_nEncodeBufferSamples;
		pUncompressed += m_nRawSamples - m_nEncodeBufferSamples;
		m_nEncodeBufferSamples = 0;

		// Compress it.
		EncodeFrame((const char *)samples, &pCompressed[nCompressedBytes]);
		nCompressedBytes += m_nEncodedBytes;
	}

	// Store the remaining samples.
	int nNewSamples = min(nSamples, min(m_nRawSamples - m_nEncodeBufferSamples, m_nRawSamples));

	if (nNewSamples)
	{
		memcpy(&m_EncodeBuffer[m_nEncodeBufferSamples], &pUncompressed[nSamples - nNewSamples], nNewSamples * BYTES_PER_SAMPLE);
		m_nEncodeBufferSamples += nNewSamples;
	}

	// If it must get the last data, just pad with zeros..
	if (bFinal && m_nEncodeBufferSamples && (maxCompressedBytes - nCompressedBytes) >= m_nEncodedBytes)
	{
		memset(&m_EncodeBuffer[m_nEncodeBufferSamples], 0, (m_nRawSamples - m_nEncodeBufferSamples) * BYTES_PER_SAMPLE);
		EncodeFrame((const char *)m_EncodeBuffer, &pCompressed[nCompressedBytes]);
		nCompressedBytes += m_nEncodedBytes;
		m_nEncodeBufferSamples = 0;
	}

	return nCompressedBytes;
}

int Voice::Decompress(const char *pCompressed, int compressedBytes, char *pUncompressed, int maxUncompressedBytes)
{
	int nDecompressedBytes = 0;
	int curCompressedByte = 0;

	while ((compressedBytes - curCompressedByte) >= m_nEncodedBytes && (maxUncompressedBytes - nDecompressedBytes) >= m_nRawBytes)
	{
		DecodeFrame(&pCompressed[curCompressedByte], &pUncompressed[nDecompressedBytes]);
		curCompressedByte += m_nEncodedBytes;
		nDecompressedBytes += m_nRawBytes;
	}

	return nDecompressedBytes / BYTES_PER_SAMPLE;
}
