#ifndef LZ4_H_
#define LZ4_H_

#include <sdw.h>

class CLz4
{
public:
	static u32 GetCompressBoundSize(u32 a_uUncompressedSize);
	static bool Uncompress(const u8* a_pCompressed, u32 a_uCompressedSize, u8* a_pUncompressed, u32& a_uUncompressedSize);
	static bool Compress(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize);
private:
	CLz4();
};

#endif	// LZ4_H_
