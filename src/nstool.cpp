#include "nstool.h"
#include "nso.h"
#include "romfs.h"

CNsTool::SOption CNsTool::s_Option[] =
{
	{ nullptr, 0, USTR("action:") },
	{ USTR("extract"), USTR('x'), USTR("extract the target file") },
	{ USTR("create"), USTR('c'), USTR("create the target file") },
	{ USTR("uncompress"), USTR('u'), USTR("uncompress the target file") },
	{ USTR("compress"), USTR('z'), USTR("compress the target file") },
	{ USTR("sample"), 0, USTR("show the samples") },
	{ USTR("help"), USTR('h'), USTR("show this help") },
	{ nullptr, 0, USTR("\ncommon:") },
	{ USTR("type"), USTR('t'), USTR("[nso|romfs]\n\t\tthe type of the file, optional") },
	{ USTR("file"), USTR('f'), USTR("the target file, required") },
	{ USTR("verbose"), USTR('v'), USTR("show the info") },
	{ USTR("2016"), 0, USTR("AuthoringTool 2016 mode, garbage") },
	{ nullptr, 0, USTR(" extract/create:") },
	{ nullptr, 0, USTR("  nso:") },
	{ USTR("header"), 0, USTR("the header file of the target file") },
	{ nullptr, 0, USTR(" uncompress/compress:") },
	{ USTR("compress-type"), 0, USTR("[nso-lz4]\n\t\tthe type of the compress") },
	{ USTR("compress-out"), 0, USTR("the output file of uncompressed or compressed") },
	{ nullptr, 0, USTR("\nnso:") },
	{ nullptr, 0, USTR(" extract/create:") },
	{ USTR("nso-dir"), 0, USTR("the nso dir for the nso file") },
	{ nullptr, 0, USTR("\nromfs:") },
	{ nullptr, 0, USTR(" create:") },
	{ USTR("romfs"), 0, USTR("the reference romfs file") },
	{ nullptr, 0, USTR("  extract:") },
	{ USTR("romfs-dir"), 0, USTR("the romfs dir for the romfs file") },
	{ nullptr, 0, nullptr }
};

CNsTool::CNsTool()
	: m_eAction(kActionNone)
	, m_eFileType(kFileTypeUnknown)
	, m_bVerbose(false)
	, m_b2016(false)
	, m_eCompressType(kCompressTypeNone)
{
}

CNsTool::~CNsTool()
{
}

int CNsTool::ParseOptions(int a_nArgc, UChar* a_pArgv[])
{
	if (a_nArgc <= 1)
	{
		return 1;
	}
	for (int i = 1; i < a_nArgc; i++)
	{
		int nArgpc = static_cast<int>(UCslen(a_pArgv[i]));
		if (nArgpc == 0)
		{
			continue;
		}
		int nIndex = i;
		if (a_pArgv[i][0] != USTR('-'))
		{
			UPrintf(USTR("ERROR: illegal option\n\n"));
			return 1;
		}
		else if (nArgpc > 1 && a_pArgv[i][1] != USTR('-'))
		{
			for (int j = 1; j < nArgpc; j++)
			{
				switch (parseOptions(a_pArgv[i][j], nIndex, a_nArgc, a_pArgv))
				{
				case kParseOptionReturnSuccess:
					break;
				case kParseOptionReturnIllegalOption:
					UPrintf(USTR("ERROR: illegal option\n\n"));
					return 1;
				case kParseOptionReturnNoArgument:
					UPrintf(USTR("ERROR: no argument\n\n"));
					return 1;
				case kParseOptionReturnUnknownArgument:
					UPrintf(USTR("ERROR: unknown argument \"%") PRIUS USTR("\"\n\n"), m_sMessage.c_str());
					return 1;
				case kParseOptionReturnOptionConflict:
					UPrintf(USTR("ERROR: option conflict\n\n"));
					return 1;
				}
			}
		}
		else if (nArgpc > 2 && a_pArgv[i][1] == USTR('-'))
		{
			switch (parseOptions(a_pArgv[i] + 2, nIndex, a_nArgc, a_pArgv))
			{
			case kParseOptionReturnSuccess:
				break;
			case kParseOptionReturnIllegalOption:
				UPrintf(USTR("ERROR: illegal option\n\n"));
				return 1;
			case kParseOptionReturnNoArgument:
				UPrintf(USTR("ERROR: no argument\n\n"));
				return 1;
			case kParseOptionReturnUnknownArgument:
				UPrintf(USTR("ERROR: unknown argument \"%") PRIUS USTR("\"\n\n"), m_sMessage.c_str());
				return 1;
			case kParseOptionReturnOptionConflict:
				UPrintf(USTR("ERROR: option conflict\n\n"));
				return 1;
			}
		}
		i = nIndex;
	}
	return 0;
}

int CNsTool::CheckOptions()
{
	if (m_eAction == kActionNone)
	{
		UPrintf(USTR("ERROR: nothing to do\n\n"));
		return 1;
	}
	if (m_eAction != kActionSample && m_eAction != kActionHelp && m_sFileName.empty())
	{
		UPrintf(USTR("ERROR: no --file option\n\n"));
		return 1;
	}
	if (m_eAction == kActionExtract)
	{
		if (!checkFileType())
		{
			UPrintf(USTR("ERROR: %") PRIUS USTR("\n\n"), m_sMessage.c_str());
			return 1;
		}
		switch (m_eFileType)
		{
		case kFileTypeNso:
			if (m_sHeaderFileName.empty() && m_sNsoDirName.empty())
			{
				UPrintf(USTR("ERROR: nothing to be extract\n\n"));
				return 1;
			}
			break;
		case kFileTypeRomFs:
			if (m_sRomFsDirName.empty())
			{
				UPrintf(USTR("ERROR: no --romfs-dir option\n\n"));
				return 1;
			}
			break;
		default:
			break;
		}
	}
	if (m_eAction == kActionCreate)
	{
		if (m_eFileType == kFileTypeUnknown)
		{
			UPrintf(USTR("ERROR: no --type option\n\n"));
			return 1;
		}
		else
		{
			if (m_eFileType == kFileTypeNso)
			{
				if (m_sHeaderFileName.empty())
				{
					UPrintf(USTR("ERROR: no --header option\n\n"));
					return 1;
				}
			}
			switch (m_eFileType)
			{
			case kFileTypeNso:
				if (m_sNsoDirName.empty())
				{
					UPrintf(USTR("ERROR: no --nso-dir option\n\n"));
					return 1;
				}
				break;
			case kFileTypeRomFs:
				if (m_sRomFsDirName.empty())
				{
					UPrintf(USTR("ERROR: no --romfs-dir option\n\n"));
					return 1;
				}
				break;
			default:
				break;
			}
		}
	}
	if (m_eAction == kActionUncompress || m_eAction == kActionCompress)
	{
		if (m_eCompressType == kCompressTypeNone)
		{
			UPrintf(USTR("ERROR: no --compress-type option\n\n"));
			return 1;
		}
		else if (m_eCompressType == kCompressTypeNsoLz4)
		{
			m_eFileType = kFileTypeNso;
			if (!checkFileType())
			{
				UPrintf(USTR("ERROR: %") PRIUS USTR("\n\n"), m_sMessage.c_str());
				return 1;
			}
		}
		if (m_sCompressOutFileName.empty())
		{
			m_sCompressOutFileName = m_sFileName;
		}
	}
	return 0;
}

int CNsTool::Help()
{
	UPrintf(USTR("nstool %") PRIUS USTR(" by dnasdw\n\n"), AToU(NSTOOL_VERSION).c_str());
	UPrintf(USTR("usage: nstool [option...] [option]...\n\n"));
	UPrintf(USTR("option:\n"));
	SOption* pOption = s_Option;
	while (pOption->Name != nullptr || pOption->Doc != nullptr)
	{
		if (pOption->Name != nullptr)
		{
			UPrintf(USTR("  "));
			if (pOption->Key != 0)
			{
				UPrintf(USTR("-%c,"), pOption->Key);
			}
			else
			{
				UPrintf(USTR("   "));
			}
			UPrintf(USTR(" --%-8") PRIUS, pOption->Name);
			if (UCslen(pOption->Name) >= 8 && pOption->Doc != nullptr)
			{
				UPrintf(USTR("\n%16") PRIUS, USTR(""));
			}
		}
		if (pOption->Doc != nullptr)
		{
			UPrintf(USTR("%") PRIUS, pOption->Doc);
		}
		UPrintf(USTR("\n"));
		pOption++;
	}
	return 0;
}

int CNsTool::Action()
{
	if (m_eAction == kActionExtract)
	{
		if (!extractFile())
		{
			UPrintf(USTR("ERROR: extract file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionCreate)
	{
		if (!createFile())
		{
			UPrintf(USTR("ERROR: create file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionUncompress)
	{
		if (!uncompressFile())
		{
			UPrintf(USTR("ERROR: uncompress file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionCompress)
	{
		if (!compressFile())
		{
			UPrintf(USTR("ERROR: compress file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionSample)
	{
		return sample();
	}
	if (m_eAction == kActionHelp)
	{
		return Help();
	}
	return 0;
}

CNsTool::EParseOptionReturn CNsTool::parseOptions(const UChar* a_pName, int& a_nIndex, int a_nArgc, UChar* a_pArgv[])
{
	if (UCscmp(a_pName, USTR("extract")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionExtract;
		}
		else if (m_eAction != kActionExtract && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("create")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionCreate;
		}
		else if (m_eAction != kActionCreate && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("uncompress")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionUncompress;
		}
		else if (m_eAction != kActionUncompress && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("compress")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionCompress;
		}
		else if (m_eAction != kActionCompress && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("sample")) == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionSample;
		}
		else if (m_eAction != kActionSample && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (UCscmp(a_pName, USTR("help")) == 0)
	{
		m_eAction = kActionHelp;
	}
	else if (UCscmp(a_pName, USTR("type")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UChar* pType = a_pArgv[++a_nIndex];
		if (UCscmp(pType, USTR("nso")) == 0)
		{
			m_eFileType = kFileTypeNso;
		}
		else if (UCscmp(pType, USTR("romfs")) == 0)
		{
			m_eFileType = kFileTypeRomFs;
		}
		else
		{
			m_sMessage = pType;
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (UCscmp(a_pName, USTR("file")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sFileName = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("verbose")) == 0)
	{
		m_bVerbose = true;
	}
	else if (UCscmp(a_pName, USTR("2016")) == 0)
	{
		m_b2016 = true;
	}
	else if (UCscmp(a_pName, USTR("header")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sHeaderFileName = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("compress-type")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UChar* pType = a_pArgv[++a_nIndex];
		if (UCscmp(pType, USTR("nso-lz4")) == 0)
		{
			m_eCompressType = kCompressTypeNsoLz4;
		}
		else
		{
			m_sMessage = pType;
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (UCscmp(a_pName, USTR("compress-out")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sCompressOutFileName = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("romfs")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sRomFsFileName = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("nso-dir")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sNsoDirName = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("romfs-dir")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sRomFsDirName = a_pArgv[++a_nIndex];
	}
	return kParseOptionReturnSuccess;
}

CNsTool::EParseOptionReturn CNsTool::parseOptions(int a_nKey, int& a_nIndex, int m_nArgc, UChar* a_pArgv[])
{
	for (SOption* pOption = s_Option; pOption->Name != nullptr || pOption->Key != 0 || pOption->Doc != nullptr; pOption++)
	{
		if (pOption->Key == a_nKey)
		{
			return parseOptions(pOption->Name, a_nIndex, m_nArgc, a_pArgv);
		}
	}
	return kParseOptionReturnIllegalOption;
}

bool CNsTool::checkFileType()
{
	if (m_eFileType == kFileTypeUnknown)
	{
		if (CNso::IsNsoFile(m_sFileName))
		{
			m_eFileType = kFileTypeNso;
		}
		else if (CRomFs::IsRomFsFile(m_sFileName))
		{
			m_eFileType = kFileTypeRomFs;
		}
		else
		{
			m_sMessage = USTR("unknown file type");
			return false;
		}
	}
	else
	{
		bool bMatch = false;
		switch (m_eFileType)
		{
		case kFileTypeNso:
			bMatch = CNso::IsNsoFile(m_sFileName);
			break;
		case kFileTypeRomFs:
			bMatch = CRomFs::IsRomFsFile(m_sFileName);
			break;
		default:
			break;
		}
		if (!bMatch)
		{
			m_sMessage = USTR("the file type is mismatch");
			return false;
		}
	}
	return true;
}

bool CNsTool::extractFile()
{
	bool bResult = false;
	switch (m_eFileType)
	{
	case kFileTypeNso:
		{
			CNso nso;
			nso.SetFileName(m_sFileName);
			nso.SetVerbose(m_bVerbose);
			nso.SetHeaderFileName(m_sHeaderFileName);
			nso.SetNsoDirName(m_sNsoDirName);
			bResult = nso.ExtractFile();
		}
		break;
	case kFileTypeRomFs:
		{
			CRomFs romFs;
			romFs.SetFileName(m_sFileName);
			romFs.SetVerbose(m_bVerbose);
			romFs.SetRomFsDirName(m_sRomFsDirName);
			bResult = romFs.ExtractFile();
		}
		break;
	default:
		break;
	}
	return bResult;
}

bool CNsTool::createFile()
{
	bool bResult = false;
	switch (m_eFileType)
	{
	case kFileTypeNso:
		{
			CNso nso;
			nso.SetFileName(m_sFileName);
			nso.SetVerbose(m_bVerbose);
			nso.SetHeaderFileName(m_sHeaderFileName);
			nso.SetNsoDirName(m_sNsoDirName);
			bResult = nso.CreateFile();
		}
		break;
	case kFileTypeRomFs:
		{
			CRomFs romFs;
			romFs.SetFileName(m_sFileName);
			romFs.SetVerbose(m_bVerbose);
			romFs.Set2016(m_b2016);
			romFs.SetRomFsDirName(m_sRomFsDirName);
			romFs.SetRomFsFileName(m_sRomFsFileName);
			bResult = romFs.CreateFile();
		}
		break;
	default:
		break;
	}
	return bResult;
}

bool CNsTool::uncompressFile()
{
	bool bResult = false;
	if (m_eCompressType == kCompressTypeNsoLz4)
	{
		CNso nso;
		nso.SetFileName(m_sFileName);
		nso.SetVerbose(m_bVerbose);
		nso.SetCompressOutFileName(m_sCompressOutFileName);
		bResult = nso.UncompressFile();
	}
	return bResult;
}

bool CNsTool::compressFile()
{
	bool bResult = false;
	if (m_eCompressType == kCompressTypeNsoLz4)
	{
		CNso nso;
		nso.SetFileName(m_sFileName);
		nso.SetVerbose(m_bVerbose);
		nso.SetCompressOutFileName(m_sCompressOutFileName);
		bResult = nso.CompressFile();
	}
	return bResult;
}

int CNsTool::sample()
{
	UPrintf(USTR("sample:\n"));
	UPrintf(USTR("# extract nso\n"));
	UPrintf(USTR("nstool -xvtf nso main --header mainheader.bin --nso-dir main.dir\n\n"));
	UPrintf(USTR("# extract romfs\n"));
	UPrintf(USTR("nstool -xvtf romfs romfs.bin --romfs-dir romfs\n\n"));
	UPrintf(USTR("# create nso\n"));
	UPrintf(USTR("nstool -cvtf nso main --header mainheader.bin --nso-dir main.dir\n\n"));
	UPrintf(USTR("# create romfs without reference\n"));
	UPrintf(USTR("nstool -cvtf romfs romfs.bin --romfs-dir romfs\n\n"));
	UPrintf(USTR("# create romfs with reference\n"));
	UPrintf(USTR("nstool -cvtf romfs romfs.bin --romfs-dir romfs --romfs original_romfs.bin\n\n"));
	UPrintf(USTR("# uncompress nso with LZ4\n"));
	UPrintf(USTR("nstool -uvf main --compress-type nso-lz4 --compress-out main.unz\n\n"));
	UPrintf(USTR("# compress nso with LZ4\n"));
	UPrintf(USTR("nstool -zvf main.unz --compress-type nso-lz4 --compress-out main\n\n"));
	return 0;
}

int UMain(int argc, UChar* argv[])
{
	CNsTool tool;
	if (tool.ParseOptions(argc, argv) != 0)
	{
		return tool.Help();
	}
	if (tool.CheckOptions() != 0)
	{
		return 1;
	}
	return tool.Action();
}
