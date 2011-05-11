/*
	ts_osx_server.c

	Copyright 2011 Michel Pollet <buserror@gmail.com>

 	This file is part of touchstream.

	touchstream is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	touchstream is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with touchstream.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "ts_defines.h"

#include <Carbon/Carbon.h>
#include <HIToolbox/CarbonEvents.h>
#include <mach/mach_port.h>
#include <mach/mach_interface.h>
#include <mach/mach_init.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/IOMessage.h>

#ifndef MAC_OS_X_VERSION_10_5
#define MAC_OS_X_VERSION_10_5
#endif

#include "ts_keymap.h"
#include "ts_verbose.h"

typedef struct ts_osx_server_t {
	ts_display_t display;

	CFMachPortRef			m_eventTapPort;
	CFRunLoopSourceRef		m_eventTapRLSR;

	double scrollSpeed, scrollSpeedFactor;

	int centerx, centery;
} ts_osx_server_t, *ts_osx_server_p;

#if __cplusplus
extern "C" {
#endif
    typedef int CGSConnectionID;
    CGError CGSSetConnectionProperty(CGSConnectionID cid, CGSConnectionID targetCID, CFStringRef key, CFTypeRef value);
    int _CGSDefaultConnection();
#if __cplusplus
}
#endif

static void
updateScrollSpeed(
		ts_osx_server_p d)
{
	CFPropertyListRef pref = CFPreferencesCopyValue(
							CFSTR("com.apple.scrollwheel.scaling") ,
							kCFPreferencesAnyApplication,
							kCFPreferencesCurrentUser,
							kCFPreferencesAnyHost);
	if (pref != NULL) {
		CFTypeID id = CFGetTypeID(pref);
		if (id == CFNumberGetTypeID()) {
			CFNumberRef value = (CFNumberRef)(pref);
			if (CFNumberGetValue(value, kCFNumberDoubleType, &d->scrollSpeed)) {
				if (d->scrollSpeed < 0.0)
					d->scrollSpeed = 0.0;
			}
		}
		CFRelease(pref);
	}
	d->scrollSpeedFactor = pow(10.0, d->scrollSpeed);
}

static SInt32
mapScrollWheelToSynergy(
		ts_osx_server_p d,
		SInt32 x)
{
	// return accelerated scrolling but not exponentially scaled as it is
	// on the mac.
	double w = (1.0 + d->scrollSpeed) * x / d->scrollSpeedFactor;
	return (SInt32)(32.0 * w);
}

static void
updateScreenShape(
		ts_osx_server_p d)
{

	// get info for each display
	CGDisplayCount displayCount = 0;

	if (CGGetActiveDisplayList(0, NULL, &displayCount) != CGDisplayNoErr) {
		return;
	}

	if (displayCount == 0) {
		return;
	}

	CGDirectDisplayID* displays = malloc(
	        sizeof(CGDirectDisplayID) * displayCount);
	if (displays == NULL) {
		return;
	}

	if (CGGetActiveDisplayList(displayCount, displays, &displayCount)
	        != CGDisplayNoErr) {
		free(displays);
		return;
	}

	// get smallest rect enclosing all display rects
	CGRect totalBounds = CGRectZero;
	for (CGDisplayCount i = 0; i < displayCount; ++i) {
		CGRect bounds = CGDisplayBounds(displays[i]);
		totalBounds = CGRectUnion(totalBounds, bounds);
	}

	int m_x, m_y, m_w, m_h;
	// get shape of default screen
	m_x = (SInt32) totalBounds.origin.x;
	m_y = (SInt32) totalBounds.origin.y;
	m_w = (SInt32) totalBounds.size.width;
	m_h = (SInt32) totalBounds.size.height;

	// get center of default screen
	CGDirectDisplayID main = CGMainDisplayID();
	CGRect rect = CGDisplayBounds(main);
	d->centerx = (rect.origin.x + rect.size.width) / 2;
	d->centery = (rect.origin.y + rect.size.height) / 2;

	free(displays);

	V2("%s: %d,%d %dx%d on %u %s\n", __func__, m_x, m_y, m_w, m_h,
	        displayCount, (displayCount == 1) ? "display" : "displays");
	d->display.bounds.x = m_x;
	d->display.bounds.y = m_y;
	d->display.bounds.w = m_w;
	d->display.bounds.h = m_h;
}

static void
displayReconfigurationCallback(
		CGDirectDisplayID displayID,
		CGDisplayChangeSummaryFlags flags,
		void * inUserData)
{
	ts_osx_server_p d = inUserData;

	CGDisplayChangeSummaryFlags mask = kCGDisplayMovedFlag |
		kCGDisplaySetModeFlag | kCGDisplayAddFlag | kCGDisplayRemoveFlag |
		kCGDisplayEnabledFlag | kCGDisplayDisabledFlag |
		kCGDisplayMirrorFlag | kCGDisplayUnMirrorFlag |
		kCGDisplayDesktopShapeChangedFlag;

	//	LOG((CLOG_DEBUG1 "event: display was reconfigured: %x %x %x", flags, mask, flags & mask));

	if (!(flags & mask)) { /* Something actually did change */

		V2("%s: screen unchanged\n", __func__);
		return;
		//screen->updateScreenShape(displayID, flags);
	}
	V2("%s: screen changed shape; refreshing dimensions\n", __func__);

	updateScreenShape(d);
}

uint32_t
mapKeyFromEvent(
		uint32_t * maskOut,
		CGEventRef event);

static bool
onKey(
		ts_osx_server_p  d,
		CGEventRef event)
{
	CGEventType eventKind = CGEventGetType(event);

	uint32_t key = mapKeyFromEvent(NULL, event);

	int down = eventKind == kCGEventKeyDown;
	if (eventKind == kCGEventFlagsChanged) {
	      CGEventFlags new = CGEventGetFlags(event);
	      switch (key) {
	    	  case kKeyShift_L:
	    	  case kKeyShift_R:
	   			  down = new & kCGEventFlagMaskShift;
	    		  break;
	    	  case kKeyControl_L:
	    	  case kKeyControl_R:
	   			  down = new & kCGEventFlagMaskControl;
	    		  break;
	    	  case kKeyCapsLock:
	   			  down = new & kCGEventFlagMaskAlphaShift;
	    		  break;
	    	  case kKeyMeta_L:
	    	  case kKeyMeta_R:
	   			  down = new & kCGEventFlagMaskCommand;
	   			  key = kKeyControl_L;
	    		  break;
	    	  case kKeyAlt_L:
	    	  case kKeyAlt_R:
	   			  down = new & kCGEventFlagMaskAlternate;
	    		  break;
	    		  break;
	      }
	}

	V3("%s key %04x (%c) %s\n", __func__, key,
			(key >= ' ' && key < 127) ? key : '.',
					down ? "down" : "up");
	ts_display_key(ts_master_get_active(d->display.master), key, down);

	return true;
}

static CGEventRef
handleCGInputEvent(
		CGEventTapProxy proxy,
		CGEventType type,
		CGEventRef event,
		void* refcon)
{
	ts_osx_server_p  d = refcon;
	CGPoint pos;

	static const int ts_button[] = {0, 2, 1};
	switch(type) {
		case kCGEventLeftMouseDown:
		case kCGEventRightMouseDown:
		case kCGEventOtherMouseDown: {
			int b = CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber);
			ts_display_button(
					ts_master_get_active(d->display.master),
					ts_button[b], 1);
			UInt32 modifiers;
			MouseTrackingResult res;
			Point pt;
			TrackMouseLocationWithOptions(NULL, 0, 0, &pt, &modifiers, &res);
		}	break;
		case kCGEventLeftMouseUp:
		case kCGEventRightMouseUp:
		case kCGEventOtherMouseUp: {
			int b = CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber);
			ts_display_button(
					ts_master_get_active(d->display.master),
					ts_button[b], 0);
		}	break;
		case kCGEventMouseMoved:
		case kCGEventLeftMouseDragged:
		case kCGEventRightMouseDragged:
		case kCGEventOtherMouseDragged: {
			pos = CGEventGetLocation(event);
			ts_master_mouse_move(
					d->display.master,
					pos.x - d->display.mousex,
					pos.y - d->display.mousey);

			if (!d->display.active) {
				pos.x = d->centerx;
				pos.y = d->centery;
				CGWarpMouseCursorPosition(pos);
				d->display.mousex = pos.x;
				d->display.mousey = pos.y;
			} else {
				d->display.master->mousex = pos.x;
				d->display.master->mousey = pos.y;
			}

			// The system ignores our cursor-centering calls if
			// we don't return the event. This should be harmless,
			// but might register as slight movement to other apps
			// on the system. It hasn't been a problem before, though.
			return event;
		}	break;
		case kCGEventScrollWheel: {
			int y = CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis2);
			int x = CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1);
			y = mapScrollWheelToSynergy(d, y);
			x = mapScrollWheelToSynergy(d, x);
		//	printf("wheel %3d %3d\n", y, x);
			ts_display_wheel(
					ts_master_get_active(d->display.master),
					0, y, x);
		}	break;
		case kCGEventKeyDown:
		case kCGEventKeyUp:
		case kCGEventFlagsChanged:
			onKey(d, event);
			break;
		case kCGEventTapDisabledByTimeout:
			// Re-enable our event-tap
			CGEventTapEnable(d->m_eventTapPort, true);
			V1("Quartz Event tap was disabled by timeout. Re-enabling.\n");
			break;
		case kCGEventTapDisabledByUserInput:
			V1("Quartz Event tap was disabled by user input!\n");
			break;
		case NX_NULLEVENT:
			break;
		case NX_SYSDEFINED:
			// Unknown, forward it
			return event;
			break;
		case NX_NUMPROCS:
			break;
		default:
			V1("Unknown Quartz Event type: 0x%02x\n", type);
	}
	return d->display.active ? event : NULL;
}

static void
driver_enter(
		ts_display_p display )
{
	ts_osx_server_p d = (ts_osx_server_p)display;
	updateScrollSpeed(d);

//	ts_osx_server_p d = (ts_osx_server_p)display;
	CGSetLocalEventsSuppressionInterval(0.0f);

	CGPoint pos;
	pos.x = display->mousex;
	pos.y = display->mousey;
	CGWarpMouseCursorPosition(pos);

}

static void
driver_leave(
		ts_display_p display)
{
//	ts_osx_server_p d = (ts_osx_server_p)display;
	/*
	 * this is needed, otherwise the "delta" coordinates we get
	 * just don't work properly
	 */
	CGSetLocalEventsSuppressionInterval(0.000001);
}

static void
driver_init(
		ts_display_p display)
{
	ts_osx_server_p d = (ts_osx_server_p)display;

	CGDisplayRegisterReconfigurationCallback(
			displayReconfigurationCallback,
			d  /* refcon */);

	updateScreenShape(d);
	d->display.active = 1;

	updateScrollSpeed(d);
	d->m_eventTapPort=CGEventTapCreate(kCGHIDEventTap, kCGHIDEventTap, 0,
									kCGEventMaskForAllEvents,
									handleCGInputEvent,
									d /* refcon */);
	if (!d->m_eventTapPort) {
		printf("Failed to create quartz event tap.\n");
		exit(1);
	}
	d->m_eventTapRLSR = CFMachPortCreateRunLoopSource(
			kCFAllocatorDefault,
			d->m_eventTapPort,
			0);

	if (!d->m_eventTapRLSR) {
		printf("Failed to create a CFRunLoopSourceRef for the quartz event tap.\n");
		exit(1);
	}
	CFRunLoopAddSource(CFRunLoopGetMain(),
			d->m_eventTapRLSR, kCFRunLoopDefaultMode);

	V2("Event handlers installed\n");
}

static void
driver_run(
		ts_display_p display)
{
	CFRunLoopRun();
}

void
osx_driver_getclipboard(
	struct ts_display_t *display,
	struct ts_display_t *to);
void
osx_driver_setclipboard(
		struct ts_display_t *d,
		ts_clipboard_p clipboard);


static ts_display_driver_t ts_osx_server_driver = {
		.init = driver_init,
		.run = driver_run,
		.enter = driver_enter,
		.leave = driver_leave,
		.getclipboard = osx_driver_getclipboard,
		.setclipboard = osx_driver_setclipboard,
};

static ts_display_p
ts_osx_server_main_create(
		struct ts_mux_t * mux,
		ts_master_p master,
		char * param)
{
	ts_osx_server_p res = malloc(sizeof(ts_osx_server_t));
	memset(res, 0, sizeof(ts_osx_server_t));

	char host[64];
	gethostname(host, sizeof(host));
	char * dot = strchr(host, '.');
	if (dot)
		*dot = 0;

	ts_display_init(&res->display, master, &ts_osx_server_driver, host, NULL);
	ts_master_display_add(master, &res->display);

	return &res->display;
}

extern ts_platform_create_callback_p ts_platform_create_server;

static void __osx_init() __attribute__((constructor));

static void __osx_init()
{
//	printf("%s\n",__func__);
	if (!AXAPIEnabled()) {
		fprintf(stderr, "%s need \"Enable access for assistive devices\"\n", __func__);
		exit(1);
	} else {
	//	printf("%s AXAPIEnabled\n", __func__);
	}
	ts_platform_create_server = ts_osx_server_main_create;
}

