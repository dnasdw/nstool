#ifndef NSTOOL_H_
#define NSTOOL_H_

#include <sdw.h>

class CNsTool
{
public:
	enum EParseOptionReturn
	{
		kParseOptionReturnSuccess,
		kParseOptionReturnIllegalOption,
		kParseOptionReturnNoArgument,
		kParseOptionReturnUnknownArgument,
		kParseOptionReturnOptionConflict
	};
	enum EAction
	{
		kActionNone,
		kActionExtract,
		kActionCreate,
		kActionUncompress,
		kActionCompress,
		kActionSample,
		kActionHelp
	};
	enum EFileType
	{
		kFileTypeUnknown,
		kFileTypeNso,
		kFileTypeRomFs
	};
	enum ECompressType
	{
		kCompressTypeNone,
		kCompressTypeNsoLz4,
	};
	struct SOption
	{
		const UChar* Name;
		int Key;
		const UChar* Doc;
	};
	CNsTool();
	~CNsTool();
	int ParseOptions(int a_nArgc, UChar* a_pArgv[]);
	int CheckOptions();
	int Help();
	int Action();
	static SOption s_Option[];
private:
	EParseOptionReturn parseOptions(const UChar* a_pName, int& a_nIndex, int a_nArgc, UChar* a_pArgv[]);
	EParseOptionReturn parseOptions(int a_nKey, int& a_nIndex, int a_nArgc, UChar* a_pArgv[]);
	bool checkFileType();
	bool extractFile();
	bool createFile();
	bool uncompressFile();
	bool compressFile();
	int sample();
	EAction m_eAction;
	EFileType m_eFileType;
	UString m_sFileName;
	bool m_bVerbose;
	bool m_b2016;
	UString m_sHeaderFileName;
	ECompressType m_eCompressType;
	UString m_sCompressOutFileName;
	UString m_sRomFsFileName;
	UString m_sNsoDirName;
	UString m_sRomFsDirName;
	UString m_sMessage;
};

#endif	// NSTOOL_H_
