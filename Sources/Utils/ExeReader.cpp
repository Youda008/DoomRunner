//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: executable file parsing and information extraction
//======================================================================================================================

#include "ExeReader.hpp"

#include "LangUtils.hpp"  // atScopeEndDo

//#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QStringBuilder>

#if IS_WINDOWS
	#include <windows.h>
	#include <strsafe.h>
#endif


//======================================================================================================================
#if IS_WINDOWS

/*
template< typename Struct >
bool readStructAt( QFile & file, DWORD offset, Struct & dest )
{
	if (!file.seek( offset ))
	{
		return false;
	}
	if (file.read( reinterpret_cast<char*>(&dest), sizeof(dest) ) < qint64( sizeof(dest) ))
	{
		return false;
	}
	return true;
}

ExeInfo readExeInfo( const QString & filePath )
{
	ExeInfo exeInfo;

	QFile file( filePath );
	if (!file.open( QIODevice::ReadOnly ))
	{
		exeInfo.status = ExeReadStatus::CantOpen;
		return exeInfo;
	}

	IMAGE_DOS_HEADER dosHeader;
	if (!readStructAt( file, 0, dosHeader ))
	{
		exeInfo.status = ExeReadStatus::InvalidFormat;
		return exeInfo;
	}

	IMAGE_NT_HEADERS NTHeaders;
	if (!readStructAt( file, DWORD( dosHeader.e_lfanew ), NTHeaders ))
	{
		exeInfo.status = ExeReadStatus::InvalidFormat;
		return exeInfo;
	}

	IMAGE_DATA_DIRECTORY & resourceDirLocation = NTHeaders.OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_RESOURCE ];

	//IMAGE_RESOURCE_DIRECTORY;
	//IMAGE_RESOURCE_DIRECTORY_ENTRY;
	//IMAGE_RESOURCE_DATA_ENTRY;

	IMAGE_RESOURCE_DIRECTORY resourceDir;
	if (!readStructAt( file, resourceDirLocation.VirtualAddress, resourceDir ))
	{
		exeInfo.status = ExeReadStatus::InvalidFormat;
		return exeInfo;
	}

	// fuck this

	exeInfo.status = ExeReadStatus::Success;
	return exeInfo;
}
*/

static VS_FIXEDFILEINFO * getRawVersionInfo( const void * resData )
{
	VS_FIXEDFILEINFO * verInfo;
	UINT verInfoSize;
	if (!VerQueryValue( resData, L"\\", (LPVOID*)&verInfo, &verInfoSize ))
	{
		qDebug() << "Cannot read version info, VerQueryValue() failed with error" << GetLastError();
		return nullptr;
	}
	else if (verInfo == nullptr || verInfoSize < sizeof(VS_FIXEDFILEINFO))
	{
		qDebug() << "Cannot read version info, VerQueryValue() returned" << verInfo << verInfoSize;
		return nullptr;
	}
	else if (verInfo->dwSignature != 0xFEEF04BD)
	{
		qDebug() << QStringLiteral("Cannot read version info, VerQueryValue() returned invalid signature").arg( verInfo->dwSignature, 0, 16 );
		return nullptr;
	}
	return verInfo;
}

struct LangInfo
{
	WORD language;
	WORD codePage;
};

struct LangInfo_span
{
	LangInfo * data;
	size_t size;

	LangInfo * begin() const { return data; }
	LangInfo * end() const { return data + size; }
};

static LangInfo_span getLangInfo( const void * resData )
{
	LangInfo * lpTranslate;
	UINT cbTranslate = 0;
	if (!VerQueryValue( resData, TEXT("\\VarFileInfo\\Translation"), (LPVOID*)&lpTranslate, &cbTranslate ))
	{
		qDebug() << "Cannot read version info, VerQueryValue(\"\\VarFileInfo\\Translation\") failed";
		return { nullptr, 0 };
	}
	else if (lpTranslate == nullptr || cbTranslate < sizeof(LangInfo))
	{
		qDebug() << "No language section in version info, VerQueryValue(\"\\VarFileInfo\\Translation\") returned " << lpTranslate << cbTranslate;
		return { nullptr, 0 };
	}
	return { lpTranslate, cbTranslate / sizeof(LangInfo) };
}

static QString getVerInfoValue( const void * resData, const LangInfo & langInfo, const TCHAR * valueName )
{
	TCHAR SubBlock[128];
	HRESULT hr = StringCchPrintf(
		SubBlock, 128, TEXT("\\StringFileInfo\\%04x%04x\\%s"), langInfo.language, langInfo.codePage, valueName
	);
	if (FAILED(hr))
	{
		qDebug() << "StringCchPrintf failed";
		return {};
	}

	LPVOID lpBuffer;
	UINT cchLen;  // number of characters
	if (!VerQueryValue( resData, SubBlock, &lpBuffer, &cchLen ))
	{
		qDebug().nospace() << "Cannot read version value, VerQueryValue("<<QString::fromWCharArray(SubBlock)<<") failed";
		return {};
	}
	else if (lpBuffer == nullptr || cchLen == 0)
	{
		qDebug().nospace() << "Cannot read version value, VerQueryValue("<<QString::fromWCharArray(SubBlock)<<") returned" << lpBuffer << cchLen;
		return {};
	}

	return QString::fromWCharArray( (const wchar_t*)lpBuffer, int(cchLen)-1 );
}

std::optional< ExeVersionInfo > readExeVersionInfo( const QString & filePath )
{
	HMODULE hExeModule = LoadLibraryEx( filePath.toStdWString().c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE );
	if (!hExeModule)
	{
		qDebug().nospace() << "Cannot open "<<filePath<<", LoadLibraryEx() failed with error "<<GetLastError();
		return std::nullopt;
	}
	auto moduleGuard = atScopeEndDo( [&](){ FreeLibrary( hExeModule ); } );

	HRSRC hResInfo = FindResource( hExeModule, MAKEINTRESOURCE(1), RT_VERSION );
	if (!hResInfo)
	{
		qDebug().nospace() << "Cannot find resource RT_VERSION in "<<filePath<<", FindResource() failed with error "<<GetLastError();
		return std::nullopt;
	}

	HGLOBAL hResource = LoadResource( hExeModule, hResInfo );
	if (!hResource)
	{
		qDebug().nospace() << "Cannot load resource RT_VERSION from "<<filePath<<", LoadResource() failed with error "<<GetLastError();
		return std::nullopt;
	}
	auto resGuard = atScopeEndDo( [&](){ FreeResource( hResource ); } );

	const void * resData = LockResource( hResource );
	DWORD resSize = SizeofResource( hExeModule, hResInfo );
	if (!resData || resSize == 0)
	{
		qDebug().nospace() << "Cannot read resource RT_VERSION from "<<filePath<<", LockResource() failed with error "<<GetLastError();
		return std::nullopt;
	}

	ExeVersionInfo verInfo;

	VS_FIXEDFILEINFO * rawVerInfo = getRawVersionInfo( resData );
	if (!rawVerInfo)
	{
		return std::nullopt;
	}
	verInfo.v.major = (rawVerInfo->dwFileVersionMS >> 16) & 0xffff;
	verInfo.v.minor = (rawVerInfo->dwFileVersionMS >>  0) & 0xffff;
	verInfo.v.patch = (rawVerInfo->dwFileVersionLS >> 16) & 0xffff;
	verInfo.v.build = (rawVerInfo->dwFileVersionLS >>  0) & 0xffff;

	auto langInfo_span = getLangInfo( resData );
	if (!langInfo_span.data || langInfo_span.size == 0)
	{
		return std::nullopt;
	}
	verInfo.appName = getVerInfoValue( resData, langInfo_span.data[0], TEXT("ProductName") );
	verInfo.description = getVerInfoValue( resData, langInfo_span.data[0], TEXT("FileDescription") );

	return { std::move(verInfo) };
}

#endif // IS_WINDOWS
