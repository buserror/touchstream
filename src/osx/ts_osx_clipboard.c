/*
	ts_osx_clipboard.c

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

/*
 * This file implements copy and paste on OSX
 *
 * What it does first is use the normal ts_diplay callbacks, and schedule
 * an "immediate" timer on the main OSX CFRunLoop to ensure the clipboard
 * operations are run on the same thread as the normal even loop.
 *
 * This is not /strictly/ necessary but it also allows to return quicker to
 * the caller (who is probably a ts_mux too)
 *
 * The clipboard support only handles UTF8
 */
#include <Carbon/Carbon.h>
#include "ts_display.h"
#include "ts_master.h"
#include "ts_display_proxy.h"

static PasteboardRef clip = 0;
static ts_display_p loop_display;
static ts_clipboard_p loop_clipboard;
static ts_display_p loop_to = NULL;

static void
osx_driver_getclipboard_loop(
	struct ts_display_t *display,
	struct ts_display_t *to)
{
	if (!clip)
		PasteboardCreate(kPasteboardClipboard, &clip);
	if (!clip)
		return;

	ts_clipboard_clear(&display->clipboard);

	CFDataRef cfdata;
	OSStatus err = noErr;
	ItemCount nItems;
	uint32_t i;

	PasteboardSynchronize(clip);
	if ((err = PasteboardGetItemCount(clip, &nItems)) != noErr) {
		printf("apple pasteboard GetItemCount failed\n");
		return;
	}

	for (i = 1; i <= nItems; ++i) {
		PasteboardItemID itemID;
		CFArrayRef flavorTypeArray;
		CFIndex flavorCount;

		if ((err = PasteboardGetItemIdentifier(clip, i, &itemID)) != noErr) {
			printf("can't get pasteboard item identifier\n");
			return;
		}

		if ((err = PasteboardCopyItemFlavors(clip, itemID,
		        &flavorTypeArray)) != noErr) {
			printf("Can't copy pasteboard item flavors\n");
			return;
		}

		flavorCount = CFArrayGetCount(flavorTypeArray);
		CFIndex flavorIndex;
		for (flavorIndex = 0; flavorIndex < flavorCount; ++flavorIndex) {
			CFStringRef flavorType;
			flavorType = (CFStringRef) CFArrayGetValueAtIndex(flavorTypeArray,
			        flavorIndex);
			if (UTTypeConformsTo(flavorType, CFSTR("public.utf8-plain-text"))) {
				if ((err = PasteboardCopyItemFlavorData(clip, itemID,
				        CFSTR("public.utf8-plain-text"), &cfdata)) != noErr) {
					printf("apple pasteboard CopyItem failed\n");
					return;
				}
				CFIndex length = CFDataGetLength(cfdata);
				uint8_t * data = malloc(length + 1);

				CFDataGetBytes(cfdata, CFRangeMake(0, length), data);
				data[length] = 0;
				V1 ("%s DATA %d!! '%s'\n", __func__, (int)length, data);
				ts_clipboard_add(&display->clipboard, "text", data, length);

				CFRelease(cfdata);
			}
		}
		CFRelease(flavorTypeArray);
	}
	if (display->clipboard.flavorCount)
		ts_display_setclipboard(
				to,
				&display->clipboard);
}

void
osx_driver_setclipboard_loop(
		struct ts_display_t *d,
		ts_clipboard_p clipboard)
{
	if (!clip)
		PasteboardCreate(kPasteboardClipboard, &clip);

	if (!clipboard->flavorCount)
		return;

	for (int i = 0; i < clipboard->flavorCount; i++) {
		if (!strcmp(clipboard->flavor[i].name, "text")) {
			printf("%s adding %d bytes of %s\n", __func__,
					clipboard->flavor[i].size, clipboard->flavor[i].name);
			if (PasteboardClear(clip) != noErr) {
				printf("apple pasteboard clear failed");
				return;
			}
			PasteboardSyncFlags flags = PasteboardSynchronize(clip);
			if ((flags & kPasteboardModified) || !(flags & kPasteboardClientIsOwner)) {
				printf("apple pasteboard cannot assert ownership");
				return;
			}
			CFDataRef cfdata = CFDataCreate(kCFAllocatorDefault,
					(uint8_t*)clipboard->flavor[i].data,
					clipboard->flavor[i].size);

			if (cfdata == nil) {
				printf("apple pasteboard cfdatacreate failed");
				return;
			}
			if (PasteboardPutItemFlavor(clip, (PasteboardItemID) 1,
			        CFSTR("public.utf8-plain-text"), cfdata, 0) != noErr) {
				printf("apple pasteboard putitem failed");
				CFRelease(cfdata);
			}
			CFRelease(cfdata);
			return;
		}
	}
}

static void
osx_getclipboard_callback (
	CFRunLoopTimerRef timer,
	void *info)
{
	//printf("%s was called\n", __func__);
	osx_driver_getclipboard_loop(loop_display, loop_to);
}

static void
osx_setclipboard_callback (
	CFRunLoopTimerRef timer,
	void *info)
{
	//printf("%s was called\n", __func__);
	osx_driver_setclipboard_loop(loop_display, loop_clipboard);
}

void
osx_driver_getclipboard(
	struct ts_display_t *display,
	struct ts_display_t *to)
{
	//printf("%s called\n", __func__);
	loop_display = display;
	loop_clipboard = NULL;
	loop_to = to;

	CFRunLoopTimerRef timer = CFRunLoopTimerCreate(NULL, 0, 0, 0, 0, osx_getclipboard_callback, NULL);

	CFRunLoopAddTimer(CFRunLoopGetMain(), timer, kCFRunLoopCommonModes);
}

void
osx_driver_setclipboard(
		struct ts_display_t *display,
		ts_clipboard_p clipboard)
{
	//printf("%s called\n", __func__);
	loop_display = display;
	loop_clipboard = clipboard;
	loop_to = NULL;

	CFRunLoopTimerRef timer = CFRunLoopTimerCreate(NULL, 0, 0, 0, 0, osx_setclipboard_callback, NULL);

	CFRunLoopAddTimer(CFRunLoopGetMain(), timer, kCFRunLoopCommonModes);
}
