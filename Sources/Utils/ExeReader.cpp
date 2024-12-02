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
// Windows

// I hate you Graph!!!

#if IS_WINDOWS


//----------------------------------------------------------------------------------------------------------------------
// resources

class Resource {

 public:

	Resource() { clear(); }
	Resource( const Resource & other ) = delete;

	Resource( Resource && other )
	{
		_hResInfo = other._hResInfo;    other._hResInfo = nullptr;
		_hResource = other._hResource;  other._hResource = nullptr;
		_lpData = other._lpData;        other._lpData = nullptr;
		_dwSize = other._dwSize;        other._dwSize = 0;
	}

	~Resource() { free(); }

	bool load( const QString & filePath, HMODULE hExeModule, LPWSTR lpType );
	void free();

	bool isLoaded() const   { return _lpData != nullptr; }
	operator bool() const   { return isLoaded(); }

	auto handle() const     { return _hResource; }
	auto data() const       { return _lpData; }
	auto size() const       { return _dwSize; }

 private:

	void clear()
	{
		_hResInfo = nullptr;
		_hResource = nullptr;
		_lpData = nullptr;
		_dwSize = 0;
	}

	HRSRC _hResInfo;
	HGLOBAL _hResource;
	const void * _lpData;
	DWORD _dwSize;

};

bool Resource::load( const QString & filePath, HMODULE hExeModule, LPWSTR lpType )
{
	if (isLoaded())
	{
		free();
	}

	HRSRC hResInfo = FindResource( hExeModule, MAKEINTRESOURCE(1), lpType );
	if (hResInfo == nullptr)
	{
		// this resource is optional, some exe files don't have it
		auto lastError = GetLastError();
		logDebug("ExeReader") << "Cannot find resource "<<lpType<<" in "<<filePath<<", FindResource() failed with error "<<lastError;
		return false;
	}

	HGLOBAL hResource = LoadResource( hExeModule, hResInfo );
	if (hResource == nullptr)  // careful: it's nullptr, not INVALID_HANDLE_VALUE
	{
		auto lastError = GetLastError();
		logRuntimeError("ExeReader") << "Cannot load resource "<<lpType<<" from "<<filePath<<", LoadResource() failed with error "<<lastError;
		return false;
	}

	void * lpData = LockResource( hResource );
	DWORD dwSize = SizeofResource( hExeModule, hResInfo );
	if (lpData == nullptr || dwSize == 0)
	{
		auto lastError = GetLastError();
		logRuntimeError("ExeReader") << "Cannot read resource "<<lpType<<" from "<<filePath<<", LockResource() failed with error "<<lastError;
		FreeResource( hResource );
		return false;
	}

	// either initialize all or leave all null
	_hResInfo = hResInfo;
	_hResource = hResource;
	_lpData = lpData;
	_dwSize = dwSize;
	return true;
}

void Resource::free()
{
	if (isLoaded())
	{
		FreeResource( _hResource );
		clear();
	}
}

struct LangInfo
{
	WORD language;
	WORD codePage;
};


//----------------------------------------------------------------------------------------------------------------------
// logging helper

class LoggingExeReader : protected LoggingComponent {

 public:

	LoggingExeReader( QString filePath ) : LoggingComponent("ExeReader"), _filePath( std::move(filePath) ) {}

	UncertainExeVersionInfo readVersionInfo();

 private:

	Resource getResource( HMODULE hExeModule, LPWSTR lpType );

	VS_FIXEDFILEINFO * getFixedVersionInfo( const Resource & res );
	span< LangInfo > getLangInfo( const Resource & res );
	QString getVerInfoValue( const Resource & res, const LangInfo & langInfo, const TCHAR * valueName );
	void extractVersionInfo( const Resource & res, ExeVersionInfo & verInfo );

	static QString QStr( const TCHAR * tstr )  { return QString::fromWCharArray( tstr ); }

 private:

	QString _filePath;

};


//----------------------------------------------------------------------------------------------------------------------
// version info extraction

Resource LoggingExeReader::getResource( HMODULE hExeModule, LPWSTR lpType )
{
	Resource res;
	res.load( _filePath, hExeModule, lpType );
	return res;
}

VS_FIXEDFILEINFO * LoggingExeReader::getFixedVersionInfo( const Resource & res )
{
	VS_FIXEDFILEINFO * verInfo;
	UINT verInfoSize;
	if (!VerQueryValue( res.data(), TEXT("\\"), (LPVOID*)&verInfo, &verInfoSize ))
	{
		auto lastError = GetLastError();
		logRuntimeError() << "Cannot read fixed version info of "<<_filePath<<", VerQueryValue(\"\\\") failed with error "<<lastError;
		return nullptr;
	}
	else if (verInfo == nullptr || verInfoSize < sizeof(VS_FIXEDFILEINFO))
	{
		logRuntimeError() << "Cannot read fixed version info of "<<_filePath<<", VerQueryValue(\"\\\") returned "<<verInfo<<','<<verInfoSize;
		return nullptr;
	}
	else if (verInfo->dwSignature != 0xFEEF04BD)
	{
		logRuntimeError() << "Cannot read fixed version info of "<<_filePath<<", VerQueryValue(\"\\\") returned invalid signature: "<<QStringLiteral("%1").arg( verInfo->dwSignature, 0, 16 );
		return nullptr;
	}
	return verInfo;
}

span< LangInfo > LoggingExeReader::getLangInfo( const Resource & res )
{
	LangInfo * lpTranslate;
	UINT cbTranslate = 0;
	if (!VerQueryValue( res.data(), TEXT("\\VarFileInfo\\Translation"), (LPVOID*)&lpTranslate, &cbTranslate ))
	{
		auto lastError = GetLastError();
		logRuntimeError() << "Cannot read language info of "<<_filePath<<", VerQueryValue(\"\\VarFileInfo\\Translation\") failed with error "<<lastError;
		return { nullptr, 0 };
	}
	else if (lpTranslate == nullptr || cbTranslate < sizeof(LangInfo))
	{
		logRuntimeError() << "Cannot read language info of "<<_filePath<<", VerQueryValue(\"\\VarFileInfo\\Translation\") returned "<<lpTranslate<<','<<cbTranslate;
		return { nullptr, 0 };
	}
	return { lpTranslate, cbTranslate / sizeof(LangInfo) };
}

QString LoggingExeReader::getVerInfoValue( const Resource & res, const LangInfo & langInfo, const TCHAR * valueName )
{
	TCHAR subBlock[128];
	HRESULT hr = StringCchPrintf(
		subBlock, 128, TEXT("\\StringFileInfo\\%04x%04x\\%s"), langInfo.language, langInfo.codePage, valueName
	);
	if (FAILED(hr))
	{
		logRuntimeError() << "StringCchPrintf() failed, valueName = " << QStr(valueName);
		return {};
	}

	LPVOID lpBuffer;
	UINT cchLen;  // number of characters
	if (!VerQueryValue( res.data(), subBlock, &lpBuffer, &cchLen ))
	{
		auto lastError = GetLastError();
		logRuntimeError() << "Cannot read file info value "<<QStr(valueName)<<" of "<<_filePath<<", VerQueryValue("<<QStr(subBlock)<<") failed with error "<<lastError;
		return {};
	}
	else if (lpBuffer == nullptr)
	{
		logRuntimeError() << "Cannot read file info value "<<QStr(valueName)<<" of "<<_filePath<<", VerQueryValue("<<QStr(subBlock)<<") returned nullptr";
		return {};
	}
	else if (cchLen == 0)
	{
		logInfo() << "Cannot read file info value "<<QStr(valueName)<<" of "<<_filePath<<", VerQueryValue("<<QStr(subBlock)<<") returned empty string";
		return {};
	}

	return QString::fromWCharArray( reinterpret_cast< const wchar_t * >( lpBuffer ), int( cchLen ) - 1 );
}

void LoggingExeReader::extractVersionInfo( const Resource & res, ExeVersionInfo & verInfo )
{
	VS_FIXEDFILEINFO * fixedVerInfo = getFixedVersionInfo( res );
	if (fixedVerInfo)
	{
		verInfo.version.major = (fixedVerInfo->dwFileVersionMS >> 16) & 0xffff;
		verInfo.version.minor = (fixedVerInfo->dwFileVersionMS >>  0) & 0xffff;
		verInfo.version.patch = (fixedVerInfo->dwFileVersionLS >> 16) & 0xffff;
		verInfo.version.build = (fixedVerInfo->dwFileVersionLS >>  0) & 0xffff;
	}

	auto languages = getLangInfo( res );
	if (!languages.empty())
	{
		auto & selectedLanguage = languages[0];  // 0 should be english most of the time
		verInfo.appName = getVerInfoValue( res, selectedLanguage, TEXT("ProductName") );
		verInfo.description = getVerInfoValue( res, selectedLanguage, TEXT("FileDescription") );
	}
}

UncertainExeVersionInfo LoggingExeReader::readVersionInfo()
{
	UncertainExeVersionInfo verInfo;

	// this can take up to 1 second sometimes, whyyy?! antivirus?
	HMODULE hExeModule = LoadLibraryEx( _filePath.toStdWString().c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE );
	if (!hExeModule)
	{
		auto lastError = GetLastError();
		logRuntimeError() << "Cannot open "<<_filePath<<", LoadLibraryEx() failed with error "<<lastError;
		verInfo.status = ReadStatus::CantOpen;
		return verInfo;
	}
	auto moduleGuard = autoClosable( hExeModule, FreeLibrary );

	Resource resource = getResource( hExeModule, RT_VERSION );
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
// public API

UncertainExeVersionInfo readExeVersionInfo( [[maybe_unused]] const QString & filePath )
{
 #if IS_WINDOWS
	LoggingExeReader exeReader( filePath );
	return exeReader.readVersionInfo();
 #else
	return { {}, ReadStatus::NotSupported };
 #endif
}

FileInfoCache< ExeVersionInfo > g_cachedExeInfo( readExeVersionInfo );


//----------------------------------------------------------------------------------------------------------------------
// serialization

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
