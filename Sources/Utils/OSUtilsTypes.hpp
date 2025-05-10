//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: types used by OSUtils.hpp, separated for less recompilation
//======================================================================================================================

#ifndef OS_UTILS_TYPES_INCLUDED
#define OS_UTILS_TYPES_INCLUDED


#include "Essential.hpp"

#include "ExeReaderTypes.hpp"  // UncertainExeVersionInfo

#include <QString>
#include <QStringList>


namespace os {


//----------------------------------------------------------------------------------------------------------------------
// installation properties

/// Type of sandbox environment an application might be installed in
enum class SandboxType
{
	None,
	Snap,
	Flatpak,
};
QString getSandboxName( SandboxType sandbox );

struct SandboxEnvInfo
{
	SandboxType type;   ///< sandbox environment type determined from path
	QString appName;   ///< name which the sandbox uses to identify the application
	QString homeDir;   ///< home directory reserved for this app (by default the app only has permissions to access this dir)
};

struct AppInfo
{
	QString exePath;             ///< path of the file from which the application info was constructed
	QString exeBaseName;         ///< executable file name without the file type suffix
	SandboxEnvInfo sandboxEnv;   ///< details related to the sandbox environment this app may be installed in
	UncertainExeVersionInfo versionInfo;  ///< version info extracted from the executable file
	QString displayName;         ///< display name of the application, suitable for identifying the app in the UI
	QString normalizedName;      ///< normalized application name suitable as a key to a map
};

struct ShellCommand
{
	QString executable;
	QStringList arguments;  ///< all command line arguments, including options to grant the extra permissions below
	QStringList extraPermissions;  ///< extra sandbox environment permissions needed to run this command (for displaying only)
};


//----------------------------------------------------------------------------------------------------------------------
// graphical environment

struct MonitorInfo
{
	QString name;
	int width;
	int height;
	bool isPrimary;
};


//----------------------------------------------------------------------------------------------------------------------
// miscellaneous

struct EnvVar
{
	QString name;
	QString value;
};


//----------------------------------------------------------------------------------------------------------------------


} // namespace os


#endif // OS_UTILS_TYPES_INCLUDED
