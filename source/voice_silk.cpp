#include "voice_silk.h"

#include <stdlib.h>

// Output bitrate.
static const int kOutputBitrateBps = 32 * 1024;

// Opus supports 8000, 12000, 16000, 24000, and 48000 sampling rates.
static const int kOpusSamplingRate = 8000;

// Opus supports frame sizes of 2.5, 5, 10, 20, 40 and 60 ms.
static const int kFrameSizeMs = 20;

// Number of raw samples per frame.
static const int kFrameSamples = kOpusSamplingRate * kFrameSizeMs / 1000;

static const int kBytesPerSample = 2;
static const int kEncodedFrameSize = kOutputBitrateBps / 400;

Voice_Silk::~Voice_Silk()
{
	TermStates();
}

bool Voice_Silk::Init(int quality)
{
	m_Quality = quality;
	m_nRawBytes = kFrameSamples * kBytesPerSample;
	m_nRawSamples = m_nRawBytes >> 1;
	m_nEncodedBytes = kEncodedFrameSize;

	m_EncoderControl.API_sampleRate = kOpusSamplingRate;
	m_EncoderControl.maxInternalSampleRate = kOpusSamplingRate;
	m_EncoderControl.packetSize = kFrameSizeMs;
	m_EncoderControl.bitRate = kOutputBitrateBps;

	m_DecoderControl.API_sampleRate = kOpusSamplingRate;

	return InitStates();
}

void Voice_Silk::EncodeFrame(const char *pUncompressedBytes, char *pCompressed)
{
	short nBytesOut = m_nEncodedBytes;

	SKP_Silk_SDK_Encode(m_EncoderState, &m_EncoderControl, (const short *)pUncompressedBytes, m_nRawBytes / 2, (unsigned char *)pCompressed, &nBytesOut);
}

void Voice_Silk::DecodeFrame(const char *pCompressed, char *pDecompressedBytes)
{
	short nSamplesOut = m_nRawBytes / 2;

	SKP_Silk_SDK_Decode(m_DecoderState, &m_DecoderControl, 0, (const unsigned char *)pCompressed, m_nEncodedBytes, (short *)pDecompressedBytes, &nSamplesOut);
}

bool Voice_Silk::InitStates()
{
	int encoderSize;
	int decoderSize;

	SKP_Silk_SDK_Get_Encoder_Size(&encoderSize);
	SKP_Silk_SDK_Get_Decoder_Size(&decoderSize);

	m_EncoderState = malloc(encoderSize);
	m_DecoderState = malloc(decoderSize);

	if (m_EncoderState && m_DecoderState)
	{
		int retEnc = SKP_Silk_SDK_InitEncoder(m_EncoderState, &m_EncoderControl);
		int retDec = SKP_Silk_SDK_InitDecoder(m_DecoderState);

		return retEnc == SKP_SILK_NO_ERROR && retDec == SKP_SILK_NO_ERROR;
	}

	return false;
}

void Voice_Silk::ResetState()
{
	SKP_Silk_SDK_InitEncoder(m_EncoderState, &m_EncoderControl);
	SKP_Silk_SDK_InitDecoder(m_DecoderState);
}

void Voice_Silk::TermStates()
{
	if (m_EncoderState)
	{
		free(m_EncoderState);
		m_EncoderState = NULL;
	}

	if (m_DecoderState)
	{
		free(m_DecoderState);
		m_DecoderState = NULL;
	}
}