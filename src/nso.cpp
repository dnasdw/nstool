#include "nso.h"
#include <lz4.h>
#include <openssl/sha.h>

const u32 CNso::s_uSignature = SDW_CONVERT_ENDIAN32('NSO0');

CNso::CNso()
	: m_bVerbose(false)
{
}

CNso::~CNso()
{
}

void CNso::SetFileName(const UString& a_sFileName)
{
	m_sFileName = a_sFileName;
}

void CNso::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

void CNso::SetCompressOutFileName(const UString& a_sCompressOutFileName)
{
	m_sCompressOutFileName = a_sCompressOutFileName;
}

bool CNso::UncompressFile()
{
	bool bResult = true;
	FILE* fp = UFopen(m_sFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	Fseek(fp, 0, SEEK_END);
	u32 uNsoSize = static_cast<u32>(Ftell(fp));
	Fseek(fp, 0, SEEK_SET);
	m_vNso.resize(uNsoSize);
	fread(&*m_vNso.begin(), 1, m_vNso.size(), fp);
	fclose(fp);
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	uNsoSize = sizeof(NsoHeader) + pCompressedNsoHeader->ModuleNameSize + pCompressedNsoHeader->TextSize + pCompressedNsoHeader->RoSize + pCompressedNsoHeader->DataSize;
	vector<u8> vNso(uNsoSize);
	if (!uncompressHeader(vNso))
	{
		bResult = false;
	}
	u32 uUncompressedOffset = sizeof(NsoHeader);
	if (!uncompressModuleName(vNso, uUncompressedOffset))
	{
		bResult = false;
	}
	if (!uncompressText(vNso, uUncompressedOffset))
	{
		bResult = false;
	}
	if (!uncompressRo(vNso, uUncompressedOffset))
	{
		bResult = false;
	}
	if (!uncompressData(vNso, uUncompressedOffset))
	{
		bResult = false;
	}
	fp = UFopen(m_sCompressOutFileName.c_str(), USTR("wb"));
	if (fp == nullptr)
	{
		return false;
	}
	fwrite(&*vNso.begin(), 1, vNso.size(), fp);
	fclose(fp);
	return true;
}

bool CNso::CompressFile()
{
	if (sizeof(void*) == 4)
	{
		UPrintf(USTR("INFO: x86-64 is recommended for LZ4 compress\n"));
	}
	bool bResult = true;
	FILE* fp = UFopen(m_sFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	Fseek(fp, 0, SEEK_END);
	u32 uNsoSize = static_cast<u32>(Ftell(fp));
	Fseek(fp, 0, SEEK_SET);
	m_vNso.resize(uNsoSize);
	fread(&*m_vNso.begin(), 1, m_vNso.size(), fp);
	fclose(fp);
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	if (LZ4_compressBound(pUncompressedNsoHeader->TextSize) == 0 || LZ4_compressBound(pUncompressedNsoHeader->RoSize) == 0 || LZ4_compressBound(pUncompressedNsoHeader->DataSize) == 0)
	{
		UPrintf(USTR("ERROR: get compress bound error\n\n"));
		return false;
	}
	uNsoSize = sizeof(NsoHeader) + pUncompressedNsoHeader->ModuleNameSize + LZ4_compressBound(pUncompressedNsoHeader->TextSize) + LZ4_compressBound(pUncompressedNsoHeader->RoSize) + LZ4_compressBound(pUncompressedNsoHeader->DataSize);
	vector<u8> vNso(uNsoSize);
	if (!compressHeader(vNso))
	{
		return false;
	}
	u32 uCompressedOffset = sizeof(NsoHeader);
	if (!compressModuleName(vNso, uCompressedOffset))
	{
		bResult = false;
	}
	if (!compressText(vNso, uCompressedOffset))
	{
		bResult = false;
	}
	if (!compressRo(vNso, uCompressedOffset))
	{
		bResult = false;
	}
	if (!compressData(vNso, uCompressedOffset))
	{
		bResult = false;
	}
	vNso.resize(uCompressedOffset);
	fp = UFopen(m_sCompressOutFileName.c_str(), USTR("wb"));
	if (fp == nullptr)
	{
		return false;
	}
	fwrite(&*vNso.begin(), 1, vNso.size(), fp);
	fclose(fp);
	return bResult;
}

bool CNso::IsNsoFile(const UString& a_sFileName)
{
	FILE* fp = UFopen(a_sFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	NsoHeader nsoHeader;
	fread(&nsoHeader, sizeof(nsoHeader), 1, fp);
	fclose(fp);
	return nsoHeader.Signature == s_uSignature;
}

bool CNso::uncompressHeader(vector<u8>& a_vNso)
{
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	memcpy(pUncompressedNsoHeader, &*m_vNso.begin(), sizeof(NsoHeader));
	pUncompressedNsoHeader->Flags &= ~(TextCompress | RoCompress | DataCompress);
	return true;
}

bool CNso::uncompressModuleName(vector<u8>& a_vNso, u32& a_uUncompressedOffset)
{
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	u32 uOffset = pUncompressedNsoHeader->ModuleNameOffset;
	u32 uCompressedSize = pUncompressedNsoHeader->ModuleNameSize;
	if (!uncompress(false, uOffset, uCompressedSize, pUncompressedNsoHeader->ModuleNameSize, nullptr, a_vNso, a_uUncompressedOffset))
	{
		return false;
	}
	pUncompressedNsoHeader->ModuleNameOffset = uOffset;
	return true;
}

bool CNso::uncompressText(vector<u8>& a_vNso, u32& a_uUncompressedOffset)
{
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	bool bCompressed = (pCompressedNsoHeader->Flags & TextCompress) != 0;
	bool bHashed = (pCompressedNsoHeader->Flags & TextHash) != 0;
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	u32 uOffset = pUncompressedNsoHeader->TextFileOffset;
	u32 uCompressedSize = pUncompressedNsoHeader->TextFileSize;
	if (!uncompress(bCompressed, uOffset, uCompressedSize, pUncompressedNsoHeader->TextSize, bHashed ? pUncompressedNsoHeader->TextHash : nullptr, a_vNso, a_uUncompressedOffset))
	{
		return false;
	}
	pUncompressedNsoHeader->TextFileOffset = uOffset;
	pUncompressedNsoHeader->TextFileSize = uCompressedSize;
	return true;
}

bool CNso::uncompressRo(vector<u8>& a_vNso, u32& a_uUncompressedOffset)
{
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	bool bCompressed = (pCompressedNsoHeader->Flags & RoCompress) != 0;
	bool bHashed = (pCompressedNsoHeader->Flags & RoHash) != 0;
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	u32 uOffset = pUncompressedNsoHeader->RoFileOffset;
	u32 uCompressedSize = pUncompressedNsoHeader->RoFileSize;
	if (!uncompress(bCompressed, uOffset, uCompressedSize, pUncompressedNsoHeader->RoSize, bHashed ? pUncompressedNsoHeader->RoHash : nullptr, a_vNso, a_uUncompressedOffset))
	{
		return false;
	}
	pUncompressedNsoHeader->RoFileOffset = uOffset;
	pUncompressedNsoHeader->RoFileSize = uCompressedSize;
	return true;
}

bool CNso::uncompressData(vector<u8>& a_vNso, u32& a_uUncompressedOffset)
{
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	bool bCompressed = (pCompressedNsoHeader->Flags & DataCompress) != 0;
	bool bHashed = (pCompressedNsoHeader->Flags & DataHash) != 0;
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	u32 uOffset = pUncompressedNsoHeader->DataFileOffset;
	u32 uCompressedSize = pUncompressedNsoHeader->DataFileSize;
	if (!uncompress(bCompressed, uOffset, uCompressedSize, pUncompressedNsoHeader->DataSize, bHashed ? pUncompressedNsoHeader->DataHash : nullptr, a_vNso, a_uUncompressedOffset))
	{
		return false;
	}
	pUncompressedNsoHeader->DataFileOffset = uOffset;
	pUncompressedNsoHeader->DataFileSize = uCompressedSize;
	return true;
}

bool CNso::uncompress(bool a_bUncompresse, u32& a_uOffset, u32& a_uCompressedSize, u32 a_uUncompressedSize, u8* a_pHash, vector<u8>& a_vNso, u32& a_uUncompressedOffset)
{
	bool bResult = true;
	u8* pCompressed = &*m_vNso.begin() + a_uOffset;
	u8* pUncompressed = &*a_vNso.begin() + a_uUncompressedOffset;
	if (a_bUncompresse)
	{
		n32 nUncompressedSize = a_uUncompressedSize;
		nUncompressedSize = LZ4_decompress_safe(reinterpret_cast<char*>(pCompressed), reinterpret_cast<char*>(pUncompressed), a_uCompressedSize, a_uUncompressedSize);
		if (nUncompressedSize < 0 || nUncompressedSize != a_uUncompressedSize)
		{
			bResult = false;
		}
		if (!bResult)
		{
			UPrintf(USTR("ERROR: uncompress error\n\n"));
		}
	}
	else
	{
		memcpy(pUncompressed, pCompressed, a_uUncompressedSize);
	}
	a_uOffset = a_uUncompressedOffset;
	a_uCompressedSize = a_uUncompressedSize;
	if (a_pHash != nullptr)
	{
		SHA256(pUncompressed, a_uUncompressedSize, a_pHash);
	}
	a_uUncompressedOffset += a_uUncompressedSize;
	return bResult;
}

bool CNso::compressHeader(vector<u8>& a_vNso)
{
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	memcpy(pCompressedNsoHeader, &*m_vNso.begin(), sizeof(NsoHeader));
	pCompressedNsoHeader->Flags |= TextCompress | RoCompress | DataCompress;
	return true;
}

bool CNso::compressModuleName(vector<u8>& a_vNso, u32& a_uCompressedOffset)
{
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	u32 uOffset = pCompressedNsoHeader->ModuleNameOffset;
	u32 uCompressedSize = pCompressedNsoHeader->ModuleNameSize;
	if (!compress(false, uOffset, uCompressedSize, pCompressedNsoHeader->ModuleNameSize, nullptr, a_vNso, a_uCompressedOffset))
	{
		return false;
	}
	pCompressedNsoHeader->ModuleNameOffset = uOffset;
	return true;
}

bool CNso::compressText(vector<u8>& a_vNso, u32& a_uCompressedOffset)
{
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	bool bCompressed = (pUncompressedNsoHeader->Flags & TextCompress) != 0;
	bool bHashed = (pUncompressedNsoHeader->Flags & TextHash) != 0;
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	u32 uOffset = pCompressedNsoHeader->TextFileOffset;
	u32 uCompressedSize = pCompressedNsoHeader->TextFileSize;
	if (!compress(!bCompressed, uOffset, uCompressedSize, pCompressedNsoHeader->TextSize, bHashed && !bCompressed ? pCompressedNsoHeader->TextHash : nullptr, a_vNso, a_uCompressedOffset))
	{
		return false;
	}
	pCompressedNsoHeader->TextFileOffset = uOffset;
	pCompressedNsoHeader->TextFileSize = uCompressedSize;
	return true;
}

bool CNso::compressRo(vector<u8>& a_vNso, u32& a_uCompressedOffset)
{
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	bool bCompressed = (pUncompressedNsoHeader->Flags & RoCompress) != 0;
	bool bHashed = (pUncompressedNsoHeader->Flags & RoHash) != 0;
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	u32 uOffset = pCompressedNsoHeader->RoFileOffset;
	u32 uCompressedSize = pCompressedNsoHeader->RoFileSize;
	if (!compress(!bCompressed, uOffset, uCompressedSize, pCompressedNsoHeader->RoSize, bHashed && !bCompressed ? pCompressedNsoHeader->RoHash : nullptr, a_vNso, a_uCompressedOffset))
	{
		return false;
	}
	pCompressedNsoHeader->RoFileOffset = uOffset;
	pCompressedNsoHeader->RoFileSize = uCompressedSize;
	return true;
}

bool CNso::compressData(vector<u8>& a_vNso, u32& a_uCompressedOffset)
{
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	bool bCompressed = (pUncompressedNsoHeader->Flags & DataCompress) != 0;
	bool bHashed = (pUncompressedNsoHeader->Flags & DataHash) != 0;
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	u32 uOffset = pCompressedNsoHeader->DataFileOffset;
	u32 uCompressedSize = pCompressedNsoHeader->DataFileSize;
	if (!compress(!bCompressed, uOffset, uCompressedSize, pCompressedNsoHeader->DataSize, bHashed && !bCompressed ? pCompressedNsoHeader->DataHash : nullptr, a_vNso, a_uCompressedOffset))
	{
		return false;
	}
	pCompressedNsoHeader->DataFileOffset = uOffset;
	pCompressedNsoHeader->DataFileSize = uCompressedSize;
	return true;
}

bool CNso::compress(bool a_bCompresse, u32& a_uOffset, u32& a_uCompressedSize, u32 a_uUncompressedSize, u8* a_pHash, vector<u8>& a_vNso, u32& a_uCompressedOffset)
{
	bool bResult = true;
	u8* pUncompressed = &*m_vNso.begin() + a_uOffset;
	u8* pCompressed = &*a_vNso.begin() + a_uCompressedOffset;
	if (a_pHash != nullptr)
	{
		SHA256(pUncompressed, a_uUncompressedSize, a_pHash);
	}
	if (a_bCompresse)
	{
		u32 uCompressedSize = LZ4_compressBound(a_uUncompressedSize);
		if (uCompressedSize == 0)
		{
			bResult = false;
		}
		if (bResult)
		{
			a_uCompressedSize = LZ4_compress_default(reinterpret_cast<char*>(pUncompressed), reinterpret_cast<char*>(pCompressed), a_uUncompressedSize, uCompressedSize);
			if (a_uCompressedSize == 0)
			{
				bResult = false;
			}
		}
		if (!bResult)
		{
			UPrintf(USTR("ERROR: compress error\n\n"));
		}
	}
	else
	{
		memcpy(pCompressed, pUncompressed, a_uCompressedSize);
	}
	a_uOffset = a_uCompressedOffset;
	a_uCompressedOffset += a_uCompressedSize;
	return bResult;
}
