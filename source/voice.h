#ifndef VOICE_H
#define VOICE_H

#define BYTES_PER_SAMPLE		2
#define MAX_FRAMEBUFFER_SAMPLES	2048

class Voice
{
public:
	Voice();
	virtual ~Voice() {}

	virtual bool Init(int quality = 4) { return true; }
	virtual void Release() { delete this; }

	virtual int Compress(const char *pUncompressedBytes, int nSamples, char *pCompressed, int maxCompressedBytes, bool bFinal = true);
	virtual int Decompress(const char *pCompressed, int compressedBytes, char *pUncompressed, int maxUncompressedBytes);

protected:
	virtual void DecodeFrame(const char *pCompressed, char *pDecompressedBytes) = 0;
	virtual void EncodeFrame(const char *pUncompressedBytes, char *pCompressed) = 0;

protected:
	int m_Quality;
	void *m_EncoderState;
	void *m_DecoderState;

	short m_EncodeBuffer[MAX_FRAMEBUFFER_SAMPLES];
	int m_nEncodeBufferSamples;
	int m_nRawBytes;
	int m_nRawSamples;
	int m_nEncodedBytes;
};

#endif // VOICE_H
