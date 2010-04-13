/**
 * @file llurldispatcher.cpp
 * @brief Central registry for all URL handlers
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"

#include "llurldispatcher.h"

// viewer includes
#include "llagent.h"			// teleportViaLocation()
#include "llcommandhandler.h"
#include "llfloaterurldisplay.h"
#include "llfloaterdirectory.h"
#include "llfloaterhtml.h"
#include "llfloaterworldmap.h"
#include "llfloaterhtmlhelp.h"
#include "llpanellogin.h"
#include "llstartup.h"			// gStartupState
#include "llurlsimstring.h"
#include "llweb.h"
#include "llworldmap.h"

// library includes
#include "llsd.h"

const std::string M7URL_SL_HELP_PREFIX		= "meta7://app.";
const std::string M7URL_SL_PREFIX			= "sl://";
const std::string M7URL_SECONDLIFE_PREFIX	= "meta7://";
const std::string M7URL_M7URL_PREFIX		= "http://m7url.com/meta7/";

const std::string M7URL_SL_HELP_PREFIX_2		= "secondlife://app.";
const std::string M7URL_SL_PREFIX_2			= "m7://";
const std::string M7URL_SECONDLIFE_PREFIX_2	= "secondlife://";
const std::string M7URL_M7URL_PREFIX_2		= "http://slurl.com/secondlife/";

const std::string M7URL_APP_TOKEN = "app/";

class LLURLDispatcherImpl
{
public:
	static bool isM7URL(const std::string& url);

	static bool isM7URLCommand(const std::string& url);

	static bool dispatch(const std::string& url,
						 LLWebBrowserCtrl* web,
						 bool trusted_browser);
		// returns true if handled or explicitly blocked.

	static bool dispatchRightClick(const std::string& url);

private:
	static bool dispatchCore(const std::string& url, 
							 bool right_mouse,
							 LLWebBrowserCtrl* web,
							 bool trusted_browser);
		// handles both left and right click

	static bool dispatchHelp(const std::string& url, bool right_mouse);
		// Handles sl://app.floater.html.help by showing Help floater.
		// Returns true if handled.

	static bool dispatchApp(const std::string& url,
							bool right_mouse,
							LLWebBrowserCtrl* web,
							bool trusted_browser);
		// Handles meta7:///app/agent/<agent_id>/about and similar
		// by showing panel in Search floater.
		// Returns true if handled or explicitly blocked.

	static bool dispatchRegion(const std::string& url, bool right_mouse);
		// handles meta7://Ahern/123/45/67/
		// Returns true if handled.

	static void regionHandleCallback(U64 handle, const std::string& url,
		const LLUUID& snapshot_id, bool teleport);
		// Called by LLWorldMap when a location has been resolved to a
	    // region name

	static void regionNameCallback(U64 handle, const std::string& url,
		const LLUUID& snapshot_id, bool teleport);
		// Called by LLWorldMap when a region name has been resolved to a
		// location in-world, used by places-panel display.

	static bool matchPrefix(const std::string& url, const std::string& prefix);
	
	static std::string stripProtocol(const std::string& url);

	friend class LLTeleportHandler;
};

// static
bool LLURLDispatcherImpl::isM7URL(const std::string& url)
{
	if (matchPrefix(url, M7URL_SL_HELP_PREFIX)) return true;
	if (matchPrefix(url, M7URL_SL_PREFIX)) return true;
	if (matchPrefix(url, M7URL_SECONDLIFE_PREFIX)) return true;
	if (matchPrefix(url, M7URL_M7URL_PREFIX)) return true;
	if (matchPrefix(url, M7URL_SL_HELP_PREFIX_2)) return true;
	if (matchPrefix(url, M7URL_SL_PREFIX_2)) return true;
	if (matchPrefix(url, M7URL_SECONDLIFE_PREFIX_2)) return true;
	if (matchPrefix(url, M7URL_M7URL_PREFIX_2)) return true;
	return false;
}

// static
bool LLURLDispatcherImpl::isM7URLCommand(const std::string& url)
{ 
	if (matchPrefix(url, M7URL_SL_PREFIX + M7URL_APP_TOKEN)
		|| matchPrefix(url, M7URL_SECONDLIFE_PREFIX + "/" + M7URL_APP_TOKEN)
		|| matchPrefix(url, M7URL_M7URL_PREFIX + M7URL_APP_TOKEN) 
		|| matchPrefix(url, M7URL_SL_PREFIX_2 + M7URL_APP_TOKEN)
		|| matchPrefix(url, M7URL_SECONDLIFE_PREFIX_2 + "/" + M7URL_APP_TOKEN)
		|| matchPrefix(url, M7URL_M7URL_PREFIX_2 + M7URL_APP_TOKEN) 
		)
	{
		return true;
	}
	return false;
}

// static
bool LLURLDispatcherImpl::dispatchCore(const std::string& url,
									   bool right_mouse,
									   LLWebBrowserCtrl* web,
									   bool trusted_browser)
{
	if (url.empty()) return false;
	if (dispatchHelp(url, right_mouse)) return true;
	if (dispatchApp(url, right_mouse, web, trusted_browser)) return true;
	if (dispatchRegion(url, right_mouse)) return true;

	/*
	// Inform the user we can't handle this
	std::map<std::string, std::string> args;
	args["M7URL"] = url;
	r;
	*/
	
	return false;
}

// static
bool LLURLDispatcherImpl::dispatch(const std::string& url,
								   LLWebBrowserCtrl* web,
								   bool trusted_browser)
{
	llinfos << "url: " << url << llendl;
	const bool right_click = false;
	return dispatchCore(url, right_click, web, trusted_browser);
}

// static
bool LLURLDispatcherImpl::dispatchRightClick(const std::string& url)
{
	llinfos << "url: " << url << llendl;
	const bool right_click = true;
	LLWebBrowserCtrl* web = NULL;
	const bool trusted_browser = false;
	return dispatchCore(url, right_click, web, trusted_browser);
}

// static
bool LLURLDispatcherImpl::dispatchHelp(const std::string& url, bool right_mouse)
{
#if LL_LIBXUL_ENABLED
	if (matchPrefix(url, M7URL_SL_HELP_PREFIX))
	{
		gViewerHtmlHelp.show();
		return true;
	}
#endif
	return false;
}

// static
bool LLURLDispatcherImpl::dispatchApp(const std::string& url, 
									  bool right_mouse,
									  LLWebBrowserCtrl* web,
									  bool trusted_browser)
{
	if (!isM7URL(url))
	{
		return false;
	}

	LLURI uri(url);
	LLSD pathArray = uri.pathArray();
	pathArray.erase(0); // erase "app"
	std::string cmd = pathArray.get(0);
	pathArray.erase(0); // erase "cmd"
	bool handled = LLCommandDispatcher::dispatch(
			cmd, pathArray, uri.queryMap(), web, trusted_browser);
	return handled;
}

// static
bool LLURLDispatcherImpl::dispatchRegion(const std::string& url, bool right_mouse)
{
	if (!isM7URL(url))
	{
		return false;
	}

	// Before we're logged in, need to update the startup screen
	// to tell the user where they are going.
	if (LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)
	{
		// Parse it and stash in globals, it will be dispatched in
		// STATE_CLEANUP.
		LLURLSimString::setString(url);
		// We're at the login screen, so make sure user can see
		// the login location box to know where they are going.
		
		LLPanelLogin::refreshLocation( true );
		return true;
	}

	std::string sim_string = stripProtocol(url);
	std::string region_name;
	S32 x = 128;
	S32 y = 128;
	S32 z = 0;
	LLURLSimString::parse(sim_string, &region_name, &x, &y, &z);

	LLFloaterURLDisplay* url_displayp = LLFloaterURLDisplay::getInstance(LLSD());
	url_displayp->setName(region_name);

	// Request a region handle by name
	LLWorldMap::getInstance()->sendNamedRegionRequest(region_name,
									  LLURLDispatcherImpl::regionNameCallback,
									  url,
									  false);	// don't teleport
	return true;
}

/*static*/
void LLURLDispatcherImpl::regionNameCallback(U64 region_handle, const std::string& url, const LLUUID& snapshot_id, bool teleport)
{
	std::string sim_string = stripProtocol(url);
	std::string region_name;
	S32 x = 128;
	S32 y = 128;
	S32 z = 0;
	LLURLSimString::parse(sim_string, &region_name, &x, &y, &z);

	LLVector3 local_pos;
	local_pos.mV[VX] = (F32)x;
	local_pos.mV[VY] = (F32)y;
	local_pos.mV[VZ] = (F32)z;

	
	// determine whether the point is in this region
	if ((x >= 0) && (x < REGION_WIDTH_UNITS) &&
		(y >= 0) && (y < REGION_WIDTH_UNITS))
	{
		// if so, we're done
		regionHandleCallback(region_handle, url, snapshot_id, teleport);
	}

	else
	{
		// otherwise find the new region from the location
		
		// add the position to get the new region
		LLVector3d global_pos = from_region_handle(region_handle) + LLVector3d(local_pos);

		U64 new_region_handle = to_region_handle(global_pos);
		LLWorldMap::getInstance()->sendHandleRegionRequest(new_region_handle,
										   LLURLDispatcherImpl::regionHandleCallback,
										   url, teleport);
	}
}

/* static */
void LLURLDispatcherImpl::regionHandleCallback(U64 region_handle, const std::string& url, const LLUUID& snapshot_id, bool teleport)
{
	std::string sim_string = stripProtocol(url);
	std::string region_name;
	S32 x = 128;
	S32 y = 128;
	S32 z = 0;
	LLURLSimString::parse(sim_string, &region_name, &x, &y, &z);

	// remap x and y to local coordinates
	S32 local_x = x % REGION_WIDTH_UNITS;
	S32 local_y = y % REGION_WIDTH_UNITS;
	if (local_x < 0)
		local_x += REGION_WIDTH_UNITS;
	if (local_y < 0)
		local_y += REGION_WIDTH_UNITS;
	
	LLVector3 local_pos;
	local_pos.mV[VX] = (F32)local_x;
	local_pos.mV[VY] = (F32)local_y;
	local_pos.mV[VZ] = (F32)z;


	
	if (teleport)
	{
		LLVector3d global_pos = from_region_handle(region_handle);
		global_pos += LLVector3d(local_pos);
		gAgent.teleportViaLocation(global_pos);
		if(gFloaterWorldMap)
		{
			gFloaterWorldMap->trackLocation(global_pos);
		}
	}
	else
	{
		// display informational floater, allow user to click teleport btn
		LLFloaterURLDisplay* url_displayp = LLFloaterURLDisplay::getInstance(LLSD());


		url_displayp->displayParcelInfo(region_handle, local_pos);
		if(snapshot_id.notNull())
		{
			url_displayp->setSnapshotDisplay(snapshot_id);
		}
		std::string locationString = llformat("%s %d, %d, %d", region_name.c_str(), x, y, z);
		url_displayp->setLocationString(locationString);
	}
}

// static
bool LLURLDispatcherImpl::matchPrefix(const std::string& url, const std::string& prefix)
{
	std::string test_prefix = url.substr(0, prefix.length());
	LLStringUtil::toLower(test_prefix);
	return test_prefix == prefix;
}

// static
std::string LLURLDispatcherImpl::stripProtocol(const std::string& url)
{
	std::string stripped = url;
	if (matchPrefix(stripped, M7URL_SL_HELP_PREFIX))
	{
		stripped.erase(0, M7URL_SL_HELP_PREFIX.length());
	}
	else if (matchPrefix(stripped, M7URL_SL_PREFIX))
	{
		stripped.erase(0, M7URL_SL_PREFIX.length());
	}
	else if (matchPrefix(stripped, M7URL_SECONDLIFE_PREFIX))
	{
		stripped.erase(0, M7URL_SECONDLIFE_PREFIX.length());
	}
	else if (matchPrefix(stripped, M7URL_M7URL_PREFIX))
	{
		stripped.erase(0, M7URL_M7URL_PREFIX.length());
	}
	else if (matchPrefix(stripped, M7URL_SL_PREFIX_2))
	{
		stripped.erase(0, M7URL_SL_PREFIX_2.length());
	}
	else if (matchPrefix(stripped, M7URL_SECONDLIFE_PREFIX_2))
	{
		stripped.erase(0, M7URL_SECONDLIFE_PREFIX_2.length());
	}
	else if (matchPrefix(stripped, M7URL_M7URL_PREFIX_2))
	{
		stripped.erase(0, M7URL_M7URL_PREFIX_2.length());
	}
	return stripped;
}

//---------------------------------------------------------------------------
// Teleportation links are handled here because they are tightly coupled
// to URL parsing and sim-fragment parsing
class LLTeleportHandler : public LLCommandHandler
{
public:
	// Teleport requests *must* come from a trusted browser
	// inside the app, otherwise a malicious web page could
	// cause a constant teleport loop.  JC
	LLTeleportHandler() : LLCommandHandler("teleport", true) { }

	bool handle(const LLSD& tokens, const LLSD& query_map,
				LLWebBrowserCtrl* web)
	{
		// construct a "normal" M7URL, resolve the region to
		// a global position, and teleport to it
		if (tokens.size() < 1) return false;

		// Region names may be %20 escaped.
		std::string region_name = LLURLSimString::unescapeRegionName(tokens[0]);

		// build meta7://De%20Haro/123/45/67 for use in callback
		std::string url = M7URL_SECONDLIFE_PREFIX;
		for (int i = 0; i < tokens.size(); ++i)
		{
			url += tokens[i].asString() + "/";
		}
		LLWorldMap::getInstance()->sendNamedRegionRequest(region_name,
			LLURLDispatcherImpl::regionHandleCallback,
			url,
			true);	// teleport
		return true;
	}
};
LLTeleportHandler gTeleportHandler;

//---------------------------------------------------------------------------

// static
bool LLURLDispatcher::isM7URL(const std::string& url)
{
	return LLURLDispatcherImpl::isM7URL(url);
}

// static
bool LLURLDispatcher::isM7URLCommand(const std::string& url)
{
	return LLURLDispatcherImpl::isM7URLCommand(url);
}

// static
bool LLURLDispatcher::dispatch(const std::string& url,
							   LLWebBrowserCtrl* web,
							   bool trusted_browser)
{
	return LLURLDispatcherImpl::dispatch(url, web, trusted_browser);
}

// static
bool LLURLDispatcher::dispatchRightClick(const std::string& url)
{
	return LLURLDispatcherImpl::dispatchRightClick(url);
}

// static
bool LLURLDispatcher::dispatchFromTextEditor(const std::string& url)
{
	// *NOTE: Text editors are considered sources of trusted URLs
	// in order to make objectim and avatar profile links in chat
	// history work.  While a malicious resident could chat an app
	// M7URL, the receiving resident will see it and must affirmatively
	// click on it.
	// *TODO: Make this trust model more refined.  JC
	const bool trusted_browser = true;
	LLWebBrowserCtrl* web = NULL;
	return LLURLDispatcherImpl::dispatch(url, web, trusted_browser);
}

// static
std::string LLURLDispatcher::buildM7URL(const std::string& regionname,
										S32 x, S32 y, S32 z)
{
	std::string m7url = M7URL_M7URL_PREFIX + regionname + llformat("/%d/%d/%d",x,y,z); 
	m7url = LLWeb::escapeURL( m7url );
	return m7url;
}
