#include "nso.h"
#include "lz4.h"
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

void CNso::SetHeaderFileName(const UString& a_sHeaderFileName)
{
	m_sHeaderFileName = a_sHeaderFileName;
}

void CNso::SetNsoDirName(const UString& a_sNsoDirName)
{
	m_sNsoDirName = a_sNsoDirName;
}

void CNso::SetCompressOutFileName(const UString& a_sCompressOutFileName)
{
	m_sCompressOutFileName = a_sCompressOutFileName;
}

bool CNso::ExtractFile()
{
	bool bResult = true;
	FILE* fp = UFopen(m_sFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	Fseek(fp, 0, SEEK_END);
	u32 uNsoSize = static_cast<u32>(Ftell(fp));
	if (uNsoSize < sizeof(NsoHeader))
	{
		fclose(fp);
		UPrintf(USTR("ERROR: nso is too short\n\n"));
		return false;
	}
	Fseek(fp, 0, SEEK_SET);
	m_vNso.resize(uNsoSize);
	fread(&*m_vNso.begin(), 1, m_vNso.size(), fp);
	fclose(fp);
	if (!m_sNsoDirName.empty())
	{
		if (!UMakeDir(m_sNsoDirName.c_str()))
		{
			return false;
		}
	}
	if (!extractHeader())
	{
		bResult = false;
	}
	if (!m_sNsoDirName.empty())
	{
		if (!extractModuleName())
		{
			bResult = false;
		}
		if (!extractCode())
		{
			bResult = false;
		}
	}
	return bResult;
}

bool CNso::CreateFile()
{
	bool bResult = true;
	if (!createHeader())
	{
		return false;
	}
	if (!createModuleName())
	{
		bResult = false;
	}
	if (!createCode())
	{
		bResult = false;
	}
	FILE* fp = UFopen(m_sFileName.c_str(), USTR("wb"));
	if (fp == nullptr)
	{
		return false;
	}
	fwrite(&*m_vNso.begin(), 1, m_vNso.size(), fp);
	fclose(fp);
	return bResult;
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
	if (uNsoSize < sizeof(NsoHeader))
	{
		fclose(fp);
		UPrintf(USTR("ERROR: nso is too short\n\n"));
		return false;
	}
	Fseek(fp, 0, SEEK_SET);
	m_vNso.resize(uNsoSize);
	fread(&*m_vNso.begin(), 1, m_vNso.size(), fp);
	fclose(fp);
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	uNsoSize = sizeof(NsoHeader) + pCompressedNsoHeader->ModuleNameSize + pCompressedNsoHeader->TextSize + pCompressedNsoHeader->RoSize + pCompressedNsoHeader->DataSize;
	vector<u8> vNso(uNsoSize);
	u32 uUncompressedOffset = 0;
	if (!uncompressHeader(vNso, uUncompressedOffset))
	{
		bResult = false;
	}
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
	return bResult;
}

bool CNso::CompressFile()
{
	bool bResult = true;
	FILE* fp = UFopen(m_sFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	Fseek(fp, 0, SEEK_END);
	u32 uNsoSize = static_cast<u32>(Ftell(fp));
	if (uNsoSize < sizeof(NsoHeader))
	{
		fclose(fp);
		UPrintf(USTR("ERROR: nso is too short\n\n"));
		return false;
	}
	Fseek(fp, 0, SEEK_SET);
	m_vNso.resize(uNsoSize);
	fread(&*m_vNso.begin(), 1, m_vNso.size(), fp);
	fclose(fp);
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	if (CLz4::GetCompressBoundSize(pUncompressedNsoHeader->TextSize) == 0 || CLz4::GetCompressBoundSize(pUncompressedNsoHeader->RoSize) == 0 || CLz4::GetCompressBoundSize(pUncompressedNsoHeader->DataSize) == 0)
	{
		UPrintf(USTR("ERROR: get compress bound error\n\n"));
		return false;
	}
	uNsoSize = sizeof(NsoHeader) + pUncompressedNsoHeader->ModuleNameSize + CLz4::GetCompressBoundSize(pUncompressedNsoHeader->TextSize) + CLz4::GetCompressBoundSize(pUncompressedNsoHeader->RoSize) + CLz4::GetCompressBoundSize(pUncompressedNsoHeader->DataSize);
	vector<u8> vNso(uNsoSize);
	u32 uCompressedOffset = 0;
	if (!compressHeader(vNso, uCompressedOffset))
	{
		return false;
	}
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

bool CNso::extractHeader()
{
	return extractFile(m_sHeaderFileName, 0, static_cast<u32>(sizeof(NsoHeader)), USTR("nso header"));
}

bool CNso::extractModuleName()
{
	NsoHeader* pNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	UString sModuleNameFileName = m_sNsoDirName + USTR("/module_name.bin");
	return extractFile(sModuleNameFileName, pNsoHeader->ModuleNameOffset, pNsoHeader->ModuleNameSize, USTR("module name"));
}

bool CNso::extractFile(const UString& a_sFileName, u32 a_uOffset, u32 a_uSize, const UChar* a_pType)
{
	bool bResult = true;
	if (!a_sFileName.empty())
	{
		FILE* fp = UFopen(a_sFileName.c_str(), USTR("wb"));
		if (fp == nullptr)
		{
			bResult = false;
		}
		else
		{
			if (m_bVerbose)
			{
				UPrintf(USTR("save: %") PRIUS USTR("\n"), a_sFileName.c_str());
			}
			fwrite(&*m_vNso.begin() + a_uOffset, 1, a_uSize, fp);
			fclose(fp);
		}
	}
	else if (m_bVerbose)
	{
		UPrintf(USTR("INFO: %") PRIUS USTR(" is not extract\n"), a_pType);
	}
	return bResult;
}

bool CNso::extractCode()
{
	bool bResult = true;
	NsoHeader* pNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	u32 uCodeSize = pNsoHeader->DataMemoryOffset + pNsoHeader->DataSize;
	vector<u8> vCode(uCodeSize);
	bool bCompressed = (pNsoHeader->Flags & TextCompress) != 0;
	if (!uncompress(m_vNso, pNsoHeader->TextFileOffset, pNsoHeader->TextFileSize, vCode, pNsoHeader->TextMemoryOffset, pNsoHeader->TextSize, bCompressed, false, nullptr))
	{
		bResult = false;
	}
	bCompressed = (pNsoHeader->Flags & RoCompress) != 0;
	if (!uncompress(m_vNso, pNsoHeader->RoFileOffset, pNsoHeader->RoFileSize, vCode, pNsoHeader->RoMemoryOffset, pNsoHeader->RoSize, bCompressed, false, nullptr))
	{
		bResult = false;
	}
	bCompressed = (pNsoHeader->Flags & DataCompress) != 0;
	if (!uncompress(m_vNso, pNsoHeader->DataFileOffset, pNsoHeader->DataFileSize, vCode, pNsoHeader->DataMemoryOffset, pNsoHeader->DataSize, bCompressed, false, nullptr))
	{
		bResult = false;
	}
	UString sCodeFileName = m_sNsoDirName + USTR("/code.bin");
	FILE* fp = UFopen(sCodeFileName.c_str(), USTR("wb"));
	if (fp == nullptr)
	{
		return false;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("save: %") PRIUS USTR("\n"), sCodeFileName.c_str());
	}
	if (!vCode.empty())
	{
		fwrite(&*vCode.begin(), 1, vCode.size(), fp);
	}
	fclose(fp);
	return bResult;
}

bool CNso::createHeader()
{
	FILE* fp = UFopen(m_sHeaderFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("load: %") PRIUS USTR("\n"), m_sHeaderFileName.c_str());
	}
	Fseek(fp, 0, SEEK_END);
	u32 uFileSize = static_cast<u32>(Ftell(fp));
	if (uFileSize < sizeof(NsoHeader))
	{
		fclose(fp);
		UPrintf(USTR("ERROR: nso header is too short\n\n"));
		return false;
	}
	Fseek(fp, 0, SEEK_SET);
	m_vNso.resize(sizeof(NsoHeader));
	fread(&*m_vNso.begin(), sizeof(NsoHeader), 1, fp);
	fclose(fp);
	return true;
}

bool CNso::createModuleName()
{
	NsoHeader* pNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	pNsoHeader->ModuleNameOffset = static_cast<u32>(m_vNso.size());
	UString sModuleNameFileName = m_sNsoDirName + USTR("/module_name.bin");
	FILE* fp = UFopen(sModuleNameFileName.c_str(), USTR("rb"));
	if (fp != nullptr)
	{
		if (m_bVerbose)
		{
			UPrintf(USTR("load: %") PRIUS USTR("\n"), sModuleNameFileName.c_str());
		}
		Fseek(fp, 0, SEEK_END);
		pNsoHeader->ModuleNameSize = static_cast<u32>(Ftell(fp));
		Fseek(fp, 0, SEEK_SET);
		m_vNso.resize(pNsoHeader->ModuleNameOffset + pNsoHeader->ModuleNameSize);
		pNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
		fread(&*m_vNso.begin() + pNsoHeader->ModuleNameOffset, 1, pNsoHeader->ModuleNameSize, fp);
		fclose(fp);
		return true;
	}
	else
	{
		pNsoHeader->ModuleNameSize = 1;
		m_vNso.resize(pNsoHeader->ModuleNameOffset + pNsoHeader->ModuleNameSize, 0);
		return false;
	}
}

bool CNso::createCode()
{
	UString sCodeFileName = m_sNsoDirName + USTR("/code.bin");
	FILE* fp = UFopen(sCodeFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("load: %") PRIUS USTR("\n"), sCodeFileName.c_str());
	}
	Fseek(fp, 0, SEEK_END);
	u32 uCodeSize = static_cast<u32>(Ftell(fp));
	Fseek(fp, 0, SEEK_SET);
	vector<u8> vCode(uCodeSize);
	if (!vCode.empty())
	{
		fread(&*vCode.begin(), 1, vCode.size(), fp);
	}
	fclose(fp);
	NsoHeader* pNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	u32 uNsoSize = static_cast<u32>(m_vNso.size());
	bool bCompress = (pNsoHeader->Flags & TextCompress) != 0;
	uNsoSize += bCompress ? CLz4::GetCompressBoundSize(pNsoHeader->TextSize) : pNsoHeader->TextSize;
	bCompress = (pNsoHeader->Flags & RoCompress) != 0;
	uNsoSize += bCompress ? CLz4::GetCompressBoundSize(pNsoHeader->RoSize) : pNsoHeader->RoSize;
	bCompress = (pNsoHeader->Flags & DataCompress) != 0;
	uNsoSize += bCompress ? CLz4::GetCompressBoundSize(pNsoHeader->DataSize) : pNsoHeader->DataSize;
	m_vNso.resize(uNsoSize);
	pNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	pNsoHeader->TextFileOffset = pNsoHeader->ModuleNameOffset + pNsoHeader->ModuleNameSize;
	bCompress = (pNsoHeader->Flags & TextCompress) != 0;
	bool bHash = (pNsoHeader->Flags & TextHash) != 0;
	u32 uCompressedSize = bCompress ? CLz4::GetCompressBoundSize(pNsoHeader->TextSize) : pNsoHeader->TextSize;
	if (!compress(vCode, pNsoHeader->TextMemoryOffset, pNsoHeader->TextSize, m_vNso, pNsoHeader->TextFileOffset, uCompressedSize, bCompress, bHash, pNsoHeader->TextHash, m_bVerbose))
	{
		return false;
	}
	pNsoHeader->TextFileSize = uCompressedSize;
	pNsoHeader->RoFileOffset = pNsoHeader->TextFileOffset + pNsoHeader->TextFileSize;
	bCompress = (pNsoHeader->Flags & RoCompress) != 0;
	bHash = (pNsoHeader->Flags & RoHash) != 0;
	uCompressedSize = bCompress ? CLz4::GetCompressBoundSize(pNsoHeader->RoSize) : pNsoHeader->RoSize;
	if (!compress(vCode, pNsoHeader->RoMemoryOffset, pNsoHeader->RoSize, m_vNso, pNsoHeader->RoFileOffset, uCompressedSize, bCompress, bHash, pNsoHeader->RoHash, m_bVerbose))
	{
		return false;
	}
	pNsoHeader->RoFileSize = uCompressedSize;
	pNsoHeader->DataFileOffset = pNsoHeader->RoFileOffset + pNsoHeader->RoFileSize;
	bCompress = (pNsoHeader->Flags & DataCompress) != 0;
	bHash = (pNsoHeader->Flags & DataHash) != 0;
	uCompressedSize = bCompress ? CLz4::GetCompressBoundSize(pNsoHeader->DataSize) : pNsoHeader->DataSize;
	if (!compress(vCode, pNsoHeader->DataMemoryOffset, pNsoHeader->DataSize, m_vNso, pNsoHeader->DataFileOffset, uCompressedSize, bCompress, bHash, pNsoHeader->DataHash, m_bVerbose))
	{
		return false;
	}
	pNsoHeader->DataFileSize = uCompressedSize;
	uNsoSize = pNsoHeader->DataFileOffset + pNsoHeader->DataFileSize;
	m_vNso.resize(uNsoSize);
	return true;
}

bool CNso::uncompressHeader(vector<u8>& a_vNso, u32& a_uUncompressedOffset)
{
	if (!uncompress(m_vNso, 0, sizeof(NsoHeader), a_vNso, a_uUncompressedOffset, sizeof(NsoHeader), false, false, nullptr))
	{
		return false;
	}
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	pUncompressedNsoHeader->Flags &= ~(TextCompress | RoCompress | DataCompress);
	a_uUncompressedOffset += sizeof(NsoHeader);
	return true;
}

bool CNso::uncompressModuleName(vector<u8>& a_vNso, u32& a_uUncompressedOffset)
{
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	pUncompressedNsoHeader->ModuleNameOffset = a_uUncompressedOffset;
	if (!uncompress(m_vNso, pCompressedNsoHeader->ModuleNameOffset, pCompressedNsoHeader->ModuleNameSize, a_vNso, pUncompressedNsoHeader->ModuleNameOffset, pUncompressedNsoHeader->ModuleNameSize, false, false, nullptr))
	{
		return false;
	}
	a_uUncompressedOffset += pUncompressedNsoHeader->ModuleNameSize;
	return true;
}

bool CNso::uncompressText(vector<u8>& a_vNso, u32& a_uUncompressedOffset)
{
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	bool bCompressed = (pCompressedNsoHeader->Flags & TextCompress) != 0;
	bool bHashed = (pCompressedNsoHeader->Flags & TextHash) != 0;
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	pUncompressedNsoHeader->TextFileOffset = a_uUncompressedOffset;
	pUncompressedNsoHeader->TextFileSize = pCompressedNsoHeader->TextSize;
	if (!uncompress(m_vNso, pCompressedNsoHeader->TextFileOffset, pCompressedNsoHeader->TextFileSize, a_vNso, pUncompressedNsoHeader->TextFileOffset, pUncompressedNsoHeader->TextFileSize, bCompressed, bHashed, pUncompressedNsoHeader->TextHash))
	{
		return false;
	}
	a_uUncompressedOffset += pUncompressedNsoHeader->TextFileSize;
	return true;
}

bool CNso::uncompressRo(vector<u8>& a_vNso, u32& a_uUncompressedOffset)
{
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	bool bCompressed = (pCompressedNsoHeader->Flags & RoCompress) != 0;
	bool bHashed = (pCompressedNsoHeader->Flags & RoHash) != 0;
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	pUncompressedNsoHeader->RoFileOffset = a_uUncompressedOffset;
	pUncompressedNsoHeader->RoFileSize = pCompressedNsoHeader->RoSize;
	if (!uncompress(m_vNso, pCompressedNsoHeader->RoFileOffset, pCompressedNsoHeader->RoFileSize, a_vNso, pUncompressedNsoHeader->RoFileOffset, pUncompressedNsoHeader->RoFileSize, bCompressed, bHashed, pUncompressedNsoHeader->RoHash))
	{
		return false;
	}
	a_uUncompressedOffset += pUncompressedNsoHeader->RoFileSize;
	return true;
}

bool CNso::uncompressData(vector<u8>& a_vNso, u32& a_uUncompressedOffset)
{
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	bool bCompressed = (pCompressedNsoHeader->Flags & DataCompress) != 0;
	bool bHashed = (pCompressedNsoHeader->Flags & DataHash) != 0;
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	pUncompressedNsoHeader->DataFileOffset = a_uUncompressedOffset;
	pUncompressedNsoHeader->DataFileSize = pCompressedNsoHeader->DataSize;
	if (!uncompress(m_vNso, pCompressedNsoHeader->DataFileOffset, pCompressedNsoHeader->DataFileSize, a_vNso, pUncompressedNsoHeader->DataFileOffset, pUncompressedNsoHeader->DataFileSize, bCompressed, bHashed, pUncompressedNsoHeader->DataHash))
	{
		return false;
	}
	a_uUncompressedOffset += pUncompressedNsoHeader->DataFileSize;
	return true;
}

bool CNso::compressHeader(vector<u8>& a_vNso, u32& a_uCompressedOffset)
{
	u32 uCompressedSize = sizeof(NsoHeader);
	if (!compress(m_vNso, 0, sizeof(NsoHeader), a_vNso, a_uCompressedOffset, uCompressedSize, false, false, nullptr, m_bVerbose))
	{
		return false;
	}
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	pCompressedNsoHeader->Flags |= TextCompress | RoCompress | DataCompress;
	a_uCompressedOffset += sizeof(NsoHeader);
	return true;
}

bool CNso::compressModuleName(vector<u8>& a_vNso, u32& a_uCompressedOffset)
{
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	pCompressedNsoHeader->ModuleNameOffset = a_uCompressedOffset;
	u32 uCompressedSize = pCompressedNsoHeader->ModuleNameSize;
	if (!compress(m_vNso, pUncompressedNsoHeader->ModuleNameOffset, pUncompressedNsoHeader->ModuleNameSize, a_vNso, pCompressedNsoHeader->ModuleNameOffset, uCompressedSize, false, false, nullptr, m_bVerbose))
	{
		return false;
	}
	pCompressedNsoHeader->ModuleNameSize = uCompressedSize;
	a_uCompressedOffset += pCompressedNsoHeader->ModuleNameSize;
	return true;
}

bool CNso::compressText(vector<u8>& a_vNso, u32& a_uCompressedOffset)
{
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	bool bCompressed = (pUncompressedNsoHeader->Flags & TextCompress) != 0;
	bool bHashed = (pUncompressedNsoHeader->Flags & TextHash) != 0;
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	pCompressedNsoHeader->TextFileOffset = a_uCompressedOffset;
	u32 uCompressedSize = CLz4::GetCompressBoundSize(pUncompressedNsoHeader->TextSize);
	if (!compress(m_vNso, pUncompressedNsoHeader->TextFileOffset, pUncompressedNsoHeader->TextFileSize, a_vNso, pCompressedNsoHeader->TextFileOffset, uCompressedSize, !bCompressed, !bCompressed && bHashed, !bCompressed ? pCompressedNsoHeader->TextHash : nullptr, m_bVerbose))
	{
		return false;
	}
	pCompressedNsoHeader->TextFileSize = uCompressedSize;
	a_uCompressedOffset += pCompressedNsoHeader->TextFileSize;
	return true;
}

bool CNso::compressRo(vector<u8>& a_vNso, u32& a_uCompressedOffset)
{
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	bool bCompressed = (pUncompressedNsoHeader->Flags & RoCompress) != 0;
	bool bHashed = (pUncompressedNsoHeader->Flags & RoHash) != 0;
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	pCompressedNsoHeader->RoFileOffset = a_uCompressedOffset;
	u32 uCompressedSize = CLz4::GetCompressBoundSize(pUncompressedNsoHeader->RoSize);
	if (!compress(m_vNso, pUncompressedNsoHeader->RoFileOffset, pUncompressedNsoHeader->RoFileSize, a_vNso, pCompressedNsoHeader->RoFileOffset, uCompressedSize, !bCompressed, !bCompressed && bHashed, !bCompressed ? pCompressedNsoHeader->RoHash : nullptr, m_bVerbose))
	{
		return false;
	}
	pCompressedNsoHeader->RoFileSize = uCompressedSize;
	a_uCompressedOffset += pCompressedNsoHeader->RoFileSize;
	return true;
}

bool CNso::compressData(vector<u8>& a_vNso, u32& a_uCompressedOffset)
{
	NsoHeader* pUncompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*m_vNso.begin());
	bool bCompressed = (pUncompressedNsoHeader->Flags & DataCompress) != 0;
	bool bHashed = (pUncompressedNsoHeader->Flags & DataHash) != 0;
	NsoHeader* pCompressedNsoHeader = reinterpret_cast<NsoHeader*>(&*a_vNso.begin());
	pCompressedNsoHeader->DataFileOffset = a_uCompressedOffset;
	u32 uCompressedSize = CLz4::GetCompressBoundSize(pUncompressedNsoHeader->DataSize);
	if (!compress(m_vNso, pUncompressedNsoHeader->DataFileOffset, pUncompressedNsoHeader->DataFileSize, a_vNso, pCompressedNsoHeader->DataFileOffset, uCompressedSize, !bCompressed, !bCompressed && bHashed, !bCompressed ? pCompressedNsoHeader->DataHash : nullptr, m_bVerbose))
	{
		return false;
	}
	pCompressedNsoHeader->DataFileSize = uCompressedSize;
	a_uCompressedOffset += pCompressedNsoHeader->DataFileSize;
	return true;
}

bool CNso::uncompress(const vector<u8>& a_vCompressed, u32 a_uCompressedOffset, u32 a_uCompressedSize, vector<u8>& a_vUncompressed, u32 a_uUncompressedOffset, u32 a_uUncompressedSize, bool a_bUncompress, bool a_bHash, u8* a_pHash)
{
	static const u8 uCompressedTemp = 0;
	static u8 uUncompressedTemp = 0;
	const u8* pCompressed = &uCompressedTemp;
	if (!a_vCompressed.empty())
	{
		pCompressed = &*a_vCompressed.begin() + a_uCompressedOffset;
	}
	else if (a_uCompressedOffset != 0 || a_uCompressedSize != 0)
	{
		return false;
	}
	u8* pUncompressed = &uUncompressedTemp;
	if (!a_vUncompressed.empty())
	{
		pUncompressed = &*a_vUncompressed.begin() + a_uUncompressedOffset;
	}
	else if (a_uUncompressedOffset != 0 || a_uUncompressedSize != 0)
	{
		return false;
	}
	if (a_bUncompress)
	{
		u32 uUncompressedSize = a_uUncompressedSize;
		if (!CLz4::Uncompress(pCompressed, a_uCompressedSize, pUncompressed, uUncompressedSize) || uUncompressedSize != a_uUncompressedSize)
		{
			UPrintf(USTR("ERROR: uncompress error\n\n"));
			return false;
		}
	}
	else
	{
		if (a_uUncompressedSize != a_uCompressedSize)
		{
			return false;
		}
		memcpy(pUncompressed, pCompressed, a_uUncompressedSize);
	}
	if (a_pHash != nullptr)
	{
		if (a_pHash)
		{
			SHA256(pUncompressed, a_uUncompressedSize, a_pHash);
		}
		else
		{
			memset(a_pHash, 0, SHA256_DIGEST_LENGTH);
		}
	}
	return true;
}

bool CNso::compress(const vector<u8>& a_vUncompressed, u32 a_uUncompressedOffset, u32 a_uUncompressedSize, vector<u8>& a_vCompressed, u32 a_uCompressedOffset, u32& a_uCompressedSize, bool a_bCompress, bool a_bHash, u8* a_pHash, bool a_bVerbose)
{
	static const u8 uUncompressedTemp = 0;
	static u8 uCompressedTemp = 0;
	const u8* pUncompressed = &uUncompressedTemp;
	if (!a_vUncompressed.empty())
	{
		pUncompressed = &*a_vUncompressed.begin() + a_uUncompressedOffset;
	}
	else if (a_uUncompressedOffset != 0 || a_uUncompressedSize != 0)
	{
		return false;
	}
	u8* pCompressed = &uCompressedTemp;
	if (!a_vCompressed.empty())
	{
		pCompressed = &*a_vCompressed.begin() + a_uCompressedOffset;
	}
	else if (a_uCompressedOffset != 0 || a_uCompressedSize != 0)
	{
		return false;
	}
	if (a_pHash != nullptr)
	{
		if (a_bHash)
		{
			SHA256(pUncompressed, a_uUncompressedSize, a_pHash);
		}
		else
		{
			memset(a_pHash, 0, SHA256_DIGEST_LENGTH);
		}
	}
	if (a_bCompress)
	{
		static bool c_bInfo = false;
		if (sizeof(void*) == 4 && a_bVerbose && !c_bInfo)
		{
			UPrintf(USTR("INFO: x86-64 is recommended for LZ4 compress\n"));
			c_bInfo = true;
		}
		if (!CLz4::Compress(pUncompressed, a_uUncompressedSize, pCompressed, a_uCompressedSize))
		{
			UPrintf(USTR("ERROR: compress error\n\n"));
			return false;
		}
	}
	else
	{
		a_uCompressedSize = a_uUncompressedSize;
		memcpy(pCompressed, pUncompressed, a_uUncompressedSize);
	}
	return true;
}
