#include "lz4.h"
#include <lz4.h>

u32 CLz4::GetCompressBoundSize(u32 a_uUncompressedSize)
{
	return LZ4_compressBound(a_uUncompressedSize);
}

bool CLz4::Uncompress(const u8* a_pCompressed, u32 a_uCompressedSize, u8* a_pUncompressed, u32& a_uUncompressedSize)
{
	bool bResult = true;
	n32 nUncompressedSize = LZ4_decompress_safe(reinterpret_cast<const char*>(a_pCompressed), reinterpret_cast<char*>(a_pUncompressed), a_uCompressedSize, a_uUncompressedSize);
	if (nUncompressedSize < 0)
	{
		bResult = false;
	}
	else
	{
		a_uUncompressedSize = nUncompressedSize;
	}
	return bResult;
}

bool CLz4::Compress(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize)
{
	bool bResult = true;
	u32 uCompressedSize = LZ4_compressBound(a_uUncompressedSize);
	if (uCompressedSize == 0)
	{
		bResult = false;
	}
	else
	{
		uCompressedSize = LZ4_compress_default(reinterpret_cast<const char*>(a_pUncompressed), reinterpret_cast<char*>(a_pCompressed), a_uUncompressedSize, a_uCompressedSize);
		if (uCompressedSize == 0)
		{
			bResult = false;
		}
		else
		{
			a_uCompressedSize = uCompressedSize;
		}
	}
	return bResult;
}

CLz4::CLz4()
{
}
