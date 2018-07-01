#ifndef ROMFS_H_
#define ROMFS_H_

#include <sdw.h>

#include SDW_MSC_PUSH_PACKED
struct SRomFsMetaInfoSection
{
	u64 Offset;
	u64 Size;
} SDW_GNUC_PACKED;

struct SRomFsMetaInfo
{
	u64 Size;
	SRomFsMetaInfoSection Section[4];
	u64 DataOffset;
} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

class CRomFs
{
public:
	enum ESectionType
	{
		kSectionTypeDirHash,
		kSectionTypeDir,
		kSectionTypeFileHash,
		kSectionTypeFile
	};
	enum EExtractState
	{
		kExtractStateBegin,
		kExtractStateChildDir,
		kExtractStateSiblingDir,
		kExtractStateEnd
	};
	struct SCommonDirEntry
	{
		n32 ParentDirOffset;
		n32 SiblingDirOffset;
		n32 ChildDirOffset;
		n32 ChildFileOffset;
		n32 PrevDirOffset;
		n32 NameSize;
	};
	struct SCommonFileEntry
	{
		n32 ParentDirOffset;
		n32 SiblingFileOffset;
		union
		{
			n64 FileOffset;
			u64 RemapIgnoreLevel;
		};
		n64 FileSize;
		n32 PrevFileOffset;
		n32 NameSize;
	};
	union UCommonEntry
	{
		SCommonDirEntry Dir;
		SCommonFileEntry File;
	};
	struct SExtractStackElement
	{
		bool IsDir;
		n32 EntryOffset;
		UCommonEntry Entry;
		UString EntryName;
		UString Prefix;
		EExtractState ExtractState;
	};
	struct SEntry
	{
		bool BucketCountValid;
		UString Path;
		U16String EntryName;
		string EntryNameUTF8;
		int EntryNameSize;
		n32 EntryOffset;
		n32 BucketIndex;
		UCommonEntry Entry;
	};
	struct SCreateStackElement
	{
		int EntryOffset;
		vector<int> ChildOffset;
		int ChildIndex;
	};
	CRomFs();
	~CRomFs();
	void SetFileName(const UString& a_pFileName);
	void SetVerbose(bool a_bVerbose);
	void Set2016(bool a_b2016);
	void SetRomFsDirName(const UString& a_sRomFsDirName);
	void SetRomFsFileName(const UString& a_sRomFsFileName);
	bool ExtractFile();
	bool CreateFile();
	static bool IsRomFsFile(const UString& a_sFileName);
	static const n64 s_nDataOffset;
	static const n32 s_nInvalidOffset;
	static const int s_nEntryNameAlignment;
	static const n64 s_nFileSizeAlignment;
	static const n64 s_nDirHashAlignment;
private:
	void pushExtractStackElement(bool a_bIsDir, n32 a_nEntryOffset, const UString& a_sPrefix);
	bool extractDirEntry();
	bool extractFileEntry();
	void readEntry(SExtractStackElement& a_Element);
	void setupCreate();
	void buildIgnoreList();
	void pushDirEntry(const UString& a_sEntryName, n32 a_nParentDirOffset);
	bool pushFileEntry(const UString& a_sEntryName, n32 a_nParentDirOffset);
	void pushCreateStackElement(int a_nEntryOffset);
	bool createEntryList();
	bool matchInIgnoreList(const UString& a_sPath) const;
	u32 getRemapIgnoreLevel(const UString& a_sPath) const;
	void removeEmptyDirEntry();
	void removeDirEntry(int a_nIndex);
	void subDirOffset(n32& a_nOffset, int a_nIndex);
	void sortCreateEntry();
	void createHash();
	u32 computeBucketCount(u32 a_uEntries);
	void redirectOffset();
	void redirectOffset(n32& a_nOffset, bool a_bIsDir, bool a_bIsOrdered);
	void remap();
	bool travelFile();
	void travelDirEntry();
	void travelFileEntry();
	void createMetaInfo();
	bool updateData();
	void writeData(const void* a_pSrc, n64 a_nOffset, n64 a_nSize);
	void writeData(const void* a_pSrc, n64 a_nSize);
	bool writeDataFromFile(const UString& a_sPath, n64 a_nOffset, n64 a_nSize);
	static u32 hash(n32 a_nParentOffset, string& a_sEntryName);
	UString m_sFileName;
	bool m_bVerbose;
	bool m_b2016;
	UString m_sRomFsDirName;
	UString m_sRomFsFileName;
	FILE* m_fpRomFs;
	SRomFsMetaInfo m_RomFsMetaInfo;
	stack<SExtractStackElement> m_sExtractStack;
	vector<URegex> m_vIgnoreList;
	vector<URegex> m_vRemapIgnoreList;
	vector<SEntry> m_vCreateDir;
	vector<SEntry> m_vCreateFile;
	vector<SEntry*> m_vOrderedCreateDir;
	vector<SEntry*> m_vOrderedCreateFile;
	stack<SCreateStackElement> m_sCreateStack;
	vector<n32> m_vDirBucket;
	vector<n32> m_vFileBucket;
	map<UString, SCommonFileEntry> m_mTravelInfo;
};

#endif	// ROMFS_H_
