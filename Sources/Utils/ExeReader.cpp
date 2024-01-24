//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: executable file parsing and information extraction
//======================================================================================================================

#include "ExeReader.hpp"

#include "LangUtils.hpp"  // atScopeEndDo
#include "ContainerUtils.hpp"  // span
#include "JsonUtils.hpp"
#include "ErrorHandling.hpp"

#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QStringBuilder>

#if IS_WINDOWS
	#include <windows.h>
	#include <strsafe.h>
#endif


namespace os {


//======================================================================================================================
//  Windows

#if IS_WINDOWS

// I hate you Graph!!!


//----------------------------------------------------------------------------------------------------------------------
//  resources

class Resource {

 public:

	Resource() : hResInfo( nullptr ), hResource( nullptr ), lpData( nullptr ), dwSize( 0 ) {}
	Resource( const Resource & other ) = delete;

	Resource( Resource && other )
	{
		hResInfo = other.hResInfo;    other.hResInfo = nullptr;
		hResource = other.hResource;  other.hResource = nullptr;
		lpData = other.lpData;        other.lpData = nullptr;
		dwSize = other.dwSize;        other.dwSize = 0;
	}

	~Resource()
	{
		if (hResource)
			FreeResource( hResource );
	}

	operator bool() const   { return lpData != nullptr; }

	auto handle() const     { return hResource; }
	auto data() const       { return reinterpret_cast< const uint8_t * >( lpData ); }
	auto size() const       { return dwSize; }

 private:

	friend Resource getResource( const QString & filePath, HMODULE hExeModule, LPWSTR lpType );

	HRSRC hResInfo;
	HGLOBAL hResource;
	const void * lpData;
	DWORD dwSize;

};

Resource getResource( const QString & filePath, HMODULE hExeModule, LPWSTR lpType )
{
	Resource res;

	res.hResInfo = FindResource( hExeModule, MAKEINTRESOURCE(1), lpType );
	if (res.hResInfo == nullptr)
	{
		// this resource is optional, some exe files don't have it
		auto lastError = GetLastError();
		logDebug("ExeReader") << "Cannot find resource "<<lpType<<" in "<<filePath<<", FindResource() failed with error "<<lastError;
		return {};
	}

	res.hResource = LoadResource( hExeModule, res.hResInfo );
	if (res.hResource == nullptr)  // careful: it's nullptr, not INVALID_HANDLE_VALUE
	{
		auto lastError = GetLastError();
		logRuntimeError("ExeReader") << "Cannot load resource "<<lpType<<" from "<<filePath<<", LoadResource() failed with error "<<lastError;
		return {};
	}

	res.lpData = LockResource( res.hResource );
	res.dwSize = SizeofResource( hExeModule, res.hResInfo );
	if (res.lpData == nullptr || res.dwSize == 0)
	{
		auto lastError = GetLastError();
		logRuntimeError("ExeReader") << "Cannot read resource "<<lpType<<" from "<<filePath<<", LockResource() failed with error "<<lastError;
		return {};
	}

	return res;
}


//----------------------------------------------------------------------------------------------------------------------
//  version info extraction

static VS_FIXEDFILEINFO * getRawVersionInfo( const void * resData )
{
	VS_FIXEDFILEINFO * verInfo;
	UINT verInfoSize;
	if (!VerQueryValue( resData, L"\\", (LPVOID*)&verInfo, &verInfoSize ))
	{
		auto lastError = GetLastError();
		logRuntimeError("ExeReader") << "Cannot read version info, VerQueryValue(\"\\\") failed with error "<<lastError;
		return nullptr;
	}
	else if (verInfo == nullptr || verInfoSize < sizeof(VS_FIXEDFILEINFO))
	{
		logRuntimeError("ExeReader") << "Cannot read version info, VerQueryValue(\"\\\") returned "<<verInfo<<','<<verInfoSize;
		return nullptr;
	}
	else if (verInfo->dwSignature != 0xFEEF04BD)
	{
		logRuntimeError("ExeReader") << QStringLiteral("Cannot read version info, VerQueryValue() returned invalid signature %1").arg( verInfo->dwSignature, 0, 16 );
		return nullptr;
	}
	return verInfo;
}

struct LangInfo
{
	WORD language;
	WORD codePage;
};

static span< LangInfo > getLangInfo( const void * resData )
{
	LangInfo * lpTranslate;
	UINT cbTranslate = 0;
	if (!VerQueryValue( resData, TEXT("\\VarFileInfo\\Translation"), (LPVOID*)&lpTranslate, &cbTranslate ))
	{
		auto lastError = GetLastError();
		logRuntimeError("ExeReader") << "Cannot read version info, VerQueryValue(\"\\VarFileInfo\\Translation\") failed with error "<<lastError;
		return { nullptr, 0 };
	}
	else if (lpTranslate == nullptr || cbTranslate < sizeof(LangInfo))
	{
		logRuntimeError("ExeReader") << "No language section in version info, VerQueryValue(\"\\VarFileInfo\\Translation\") returned "<<lpTranslate<<','<<cbTranslate;
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
		logRuntimeError("ExeReader") << "StringCchPrintf() failed";
		return {};
	}

	LPVOID lpBuffer;
	UINT cchLen;  // number of characters
	if (!VerQueryValue( resData, SubBlock, &lpBuffer, &cchLen ))
	{
		auto lastError = GetLastError();
		logRuntimeError("ExeReader") << "Cannot read version value, VerQueryValue("<<QString::fromWCharArray(SubBlock)<<") failed with error "<<lastError;
		return {};
	}
	else if (lpBuffer == nullptr)
	{
		logRuntimeError("ExeReader") << "Cannot read version value, VerQueryValue("<<QString::fromWCharArray(SubBlock)<<") returned nullptr";
		return {};
	}
	else if (cchLen == 0)
	{
		logDebug("ExeReader") << "Cannot read version value, VerQueryValue("<<QString::fromWCharArray(SubBlock)<<") returned empty string";
		return {};
	}

	return QString::fromWCharArray( reinterpret_cast< const wchar_t * >( lpBuffer ), int( cchLen ) - 1 );
}

static void extractVersionInfo( const Resource & res, ExeVersionInfo & verInfo )
{
	VS_FIXEDFILEINFO * rawVerInfo = getRawVersionInfo( res.data() );
	if (rawVerInfo)
	{
		verInfo.version.major = (rawVerInfo->dwFileVersionMS >> 16) & 0xffff;
		verInfo.version.minor = (rawVerInfo->dwFileVersionMS >>  0) & 0xffff;
		verInfo.version.patch = (rawVerInfo->dwFileVersionLS >> 16) & 0xffff;
		verInfo.version.build = (rawVerInfo->dwFileVersionLS >>  0) & 0xffff;
	}

	auto languages = getLangInfo( res.data() );
	if (!languages.empty())
	{
		verInfo.appName = getVerInfoValue( res.data(), languages[0], TEXT("ProductName") );
		verInfo.description = getVerInfoValue( res.data(), languages[0], TEXT("FileDescription") );
	}
}

// read PE file using LoadLibrary and FindResource,LoadResource flow
UncertainExeVersionInfo readVersionInfoUsingWinAPI( const QString & filePath )
{
	UncertainExeVersionInfo verInfo;

	// this can take up to 1 second sometimes, whyyy?! antivirus?
	HMODULE hExeModule = LoadLibraryEx( filePath.toStdWString().c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE );
	if (!hExeModule)
	{
		auto lastError = GetLastError();
		logRuntimeError("ExeReader") << "Cannot open "<<filePath<<", LoadLibraryEx() failed with error "<<lastError;
		verInfo.status = ReadStatus::CantOpen;
		return verInfo;
	}
	auto moduleGuard = autoClosable( hExeModule, FreeLibrary );

	Resource resource = getResource( filePath, hExeModule, RT_VERSION );
	if (!resource)
	{
		verInfo.status = ReadStatus::InfoNotPresent;
		return verInfo;
	}

	extractVersionInfo( resource, verInfo );

	verInfo.status = ReadStatus::Success;
	return verInfo;
}

#endif // IS_WINDOWS


//======================================================================================================================
//  public API

UncertainExeVersionInfo readExeVersionInfo( [[maybe_unused]] const QString & filePath )
{
 #if IS_WINDOWS
	return readVersionInfoUsingWinAPI( filePath );
 #else
	return { {}, ReadStatus::NotSupported };
 #endif
}


FileInfoCache< ExeVersionInfo > g_cachedExeInfo( readExeVersionInfo );


//----------------------------------------------------------------------------------------------------------------------
//  serialization

void ExeVersionInfo::serialize( QJsonObject & jsExeInfo ) const
{
	jsExeInfo["app_name"] = appName;
	jsExeInfo["description"] = description;
	jsExeInfo["version"] = version.toString();
}

void ExeVersionInfo::deserialize( const JsonObjectCtx & jsExeInfo )
{
	appName = jsExeInfo.getString("app_name");
	description = jsExeInfo.getString("description");
	version = Version( jsExeInfo.getString("version") );
}


} // namespace os
