/* Copyright (c) 2010 Discrete Dreamscape All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. Neither the name Discrete Dreamscape nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY DISCRETE DREAMSCAPE AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DISCRETE DREAMSCAPE OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "llviewerprecompiledheaders.h"
#include "desktopnotifierlinux.h"

#include "notify.h"

const char* ICON_128 = "emerald_icon128.BMP";
const char* ICON_512 = "emerald_icon.BMP";
const gint NOTIFICATION_TIMEOUT = 5;

static const char* icon_wholename;

void Find_BMP_Resource(const char *basename)
{
	const int PATH_BUFFER_SIZE=1000;
	char path_buffer[PATH_BUFFER_SIZE];	/* Flawfinder: ignore */
	
	// Figure out where our BMP is living on the disk
	snprintf(path_buffer, PATH_BUFFER_SIZE-1, "%s%sres-sdl%s%s",	
		 gDirUtilp->getAppRODataDir().c_str(),
		 gDirUtilp->getDirDelimiter().c_str(),
		 gDirUtilp->getDirDelimiter().c_str(),
		 basename);
	path_buffer[PATH_BUFFER_SIZE-1] = '\0';
	
	icon_wholename = path_buffer;
}

DesktopNotifierLinux::DesktopNotifierLinux()
{
	if (notify_init("Emerald Viewer")) {
	    LL_INFOS("DesktopNotifierLinux") << "Linux desktop notifications initialized." << LL_ENDL;
	    // Find the name of our notification server. I kinda don't expect it to change after the start of the program.
	    gchar* name = NULL;
	    notify_get_server_info(&name, NULL, NULL, NULL);
	    if (!strncmp("notification-daemon", name, 19)) {
    	    // We're talking to notification-daemon, and I don't feel like scaling. Use a premade 128x128 icon.
	        Find_BMP_Resource(ICON_128);
	    } else {
	        // Talking to NotifyOSD or something else. Try the 512x512 icon and let it scale on its own.
	        Find_BMP_Resource(ICON_512);
	    }
	} else {
	    LL_WARNS("DesktopNotifierLinux") << "Linux desktop notifications FAILED to initialize." << LL_ENDL;
	}
}

void DesktopNotifierLinux::showNotification(const std::string& notification_title, const std::string& notification_message, const std::string& notification_type)
{
    NotifyNotification* notification = notify_notification_new(
        (gchar*)notification_title.c_str(),
        (gchar*)notification_message.c_str(),
        icon_wholename,
        NULL
    
    );
    notify_notification_set_category(notification, (gchar*)notification_type.c_str());
    notify_notification_set_timeout(notification, NOTIFICATION_TIMEOUT); // NotifyOSD ignores this.

    if (notify_notification_show(notification, NULL)) {
        LL_INFOS("DesktopNotifierLinux") << "Linux desktop notification type " << notification_type << "sent." << LL_ENDL;
    } else {
        LL_WARNS("DesktopNotifierLinux") << "Linux desktop notification FAILED to send." << LL_ENDL;
    }
}

bool DesktopNotifierLinux::isUsable()
{
	return notify_is_initted();
}

void DesktopNotifierLinux::registerApplication(const std::string& application, const std::set<std::string>& notificationTypes)
{
	// Do nothing for now.
}