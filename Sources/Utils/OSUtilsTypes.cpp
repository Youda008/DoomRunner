//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: types used by OSUtils.hpp, separated for less recompilation
//======================================================================================================================

#include "OSUtilsTypes.hpp"


//----------------------------------------------------------------------------------------------------------------------

namespace os {


QString getSandboxName( SandboxType sandbox )
{
	switch (sandbox)
	{
		case SandboxType::Snap:    return "Snap";
		case SandboxType::Flatpak: return "Flatpak";
		default:                  return "<invalid>";
	}
}


} // namespace os
