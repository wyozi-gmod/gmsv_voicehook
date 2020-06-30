#ifndef VOICE_SILK_H
#define VOICE_SILK_H

#include "voice.h"
#include <silk/SKP_Silk_SDK_API.h>

class Voice_Silk : public Voice
{
public:
	~Voice_Silk();

	bool Init(int quality = 4);

private:
	void DecodeFrame(const char *pCompressed, char *pDecompressedBytes);
	void EncodeFrame(const char *pUncompressedBytes, char *pCompressed);
	bool InitStates();
	void ResetState();
	void TermStates();

private:
	SKP_SILK_SDK_EncControlStruct m_EncoderControl;
	SKP_SILK_SDK_DecControlStruct m_DecoderControl;
};

#endif // VOICE_SILK_H
