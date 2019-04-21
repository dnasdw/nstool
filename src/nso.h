#ifndef NSO_H_
#define NSO_H_

#include <sdw.h>

#include SDW_MSC_PUSH_PACKED
struct NsoHeader
{
	u32 Signature;
	u32 Version;
	u32 Reserved1;
	u32 Flags;
	u32 TextFileOffset;
	u32 TextMemoryOffset;
	u32 TextSize;
	u32 ModuleNameOffset;
	u32 RoFileOffset;
	u32 RoMemoryOffset;
	u32 RoSize;
	u32 ModuleNameSize;
	u32 DataFileOffset;
	u32 DataMemoryOffset;
	u32 DataSize;
	u32 BssSize;
	u8 ModuleId[32];
	u32 TextFileSize;
	u32 RoFileSize;
	u32 DataFileSize;
	u8 Reserved2[4];
	u32 EmbededOffset;
	u32 EmbededSize;
	u8 Reserved3[40];
	u8 TextHash[32];
	u8 RoHash[32];
	u8 DataHash[32];
} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

class CNso
{
public:
	enum NsoHeaderFlags
	{
		TextCompress = 1,
		RoCompress = 2,
		DataCompress = 4,
		TextHash = 8,
		RoHash = 16,
		DataHash = 32
	};
	CNso();
	~CNso();
	void SetFileName(const UString& a_sFileName);
	void SetVerbose(bool a_bVerbose);
	void SetHeaderFileName(const UString& a_sHeaderFileName);
	void SetNsoDirName(const UString& a_sNsoDirName);
	void SetCompressOutFileName(const UString& a_sCompressOutFileName);
	bool ExtractFile();
	bool CreateFile();
	bool UncompressFile();
	bool CompressFile();
	static bool IsNsoFile(const UString& a_sFileName);
	static const u32 s_uSignature;
private:
	bool extractHeader();
	bool extractModuleName();
	bool extractFile(const UString& a_sFileName, u32 a_uOffset, u32 a_uSize, const UChar* a_pType);
	bool extractCode();
	bool createHeader();
	bool createModuleName();
	bool createCode();
	bool uncompressHeader(vector<u8>& a_vNso, u32& a_uUncompressedOffset);
	bool uncompressModuleName(vector<u8>& a_vNso, u32& a_uUncompressedOffset);
	bool uncompressText(vector<u8>& a_vNso, u32& a_uUncompressedOffset);
	bool uncompressRo(vector<u8>& a_vNso, u32& a_uUncompressedOffset);
	bool uncompressData(vector<u8>& a_vNso, u32& a_uUncompressedOffset);
	bool compressHeader(vector<u8>& a_vNso, u32& a_uCompressedOffset);
	bool compressModuleName(vector<u8>& a_vNso, u32& a_uCompressedOffset);
	bool compressText(vector<u8>& a_vNso, u32& a_uCompressedOffset);
	bool compressRo(vector<u8>& a_vNso, u32& a_uCompressedOffset);
	bool compressData(vector<u8>& a_vNso, u32& a_uCompressedOffset);
	static bool uncompress(const vector<u8>& a_vCompressed, u32 a_uCompressedOffset, u32 a_uCompressedSize, vector<u8>& a_vUncompressed, u32 a_uUncompressedOffset, u32 a_uUncompressedSize, bool a_bUncompress, bool a_bHash, u8* a_pHash);
	static bool compress(const vector<u8>& a_vUncompressed, u32 a_uUncompressedOffset, u32 a_uUncompressedSize, vector<u8>& a_vCompressed, u32 a_uCompressedOffset, u32& a_uCompressedSize, bool a_bCompress, bool a_bHash, u8* a_pHash, bool a_bVerbose);
	UString m_sFileName;
	bool m_bVerbose;
	UString m_sHeaderFileName;
	UString m_sNsoDirName;
	UString m_sCompressOutFileName;
	vector<u8> m_vNso;
};

#endif	// NSO_H_
