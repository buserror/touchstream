/*
	ts_osx_keymap.c

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


#include <Carbon/Carbon.h>
#include "ts_defines.h"
#include "ts_keymap.h"

/*
 * Most of this material is a rehash of synergy's COSXKeyState.cpp
 */
// Note that some virtual keys codes appear more than once.  The
// first instance of a virtual key code maps to the KeyID that we
// want to generate for that code.  The others are for mapping
// different KeyIDs to a single key code.
enum {
	s_shiftVK    = 56,
	s_controlVK  = 59,
	s_altVK      = 58,
	s_superVK    = 55,
	s_capsLockVK = 57,
	s_numLockVK  = 71,
	s_osxNumLock = 1 << 16,
};

static const uint32_t	s_controlKeys[255] = {
	// cursor keys.  if we don't do this we'll may still get these from
	// the keyboard resource but they may not correspond to the arrow
	// keys.
	[123] = kKeyLeft,
	[124] = kKeyRight,
	[126] = kKeyUp,
	[125] = kKeyDown,
	[115] = kKeyHome,
	[119] = kKeyEnd,
	[116] = kKeyPageUp,
	[121] = kKeyPageDown,
	[114] = kKeyInsert,

	// function keys
	[122] = kKeyF1,
	[120] = kKeyF2,
	[99] = kKeyF3,
	[118] = kKeyF4,
	[96] = kKeyF5,
	[97] = kKeyF6,
	[98] = kKeyF7,
	[100] = kKeyF8,
	[101] = kKeyF9,
	[109] = kKeyF10,
	[103] = kKeyF11,
	[111] = kKeyF12,
	[105] = kKeyF13,
	[107] = kKeyF14,
	[113] = kKeyF15,
	[106] = kKeyF16,

	[82] = kKeyKP_0,
	[83] = kKeyKP_1,
	[84] = kKeyKP_2,
	[85] = kKeyKP_3,
	[86] = kKeyKP_4,
	[87] = kKeyKP_5,
	[88] = kKeyKP_6,
	[89] = kKeyKP_7,
	[91] = kKeyKP_8,
	[92] = kKeyKP_9,
	[65] = kKeyKP_Decimal,
	[81] = kKeyKP_Equal,
	[67] = kKeyKP_Multiply,
	[69] = kKeyKP_Add,
	[75] = kKeyKP_Divide,
	[79] = kKeyKP_Subtract,
	[76] = kKeyKP_Enter,

	// virtual key 110 is fn+enter and i have no idea what that's supposed
	// to map to.  also the enter key with numlock on is a modifier but i
	// don't know which.

	// modifier keys.  OS X doesn't seem to support right handed versions
	// of modifier keys so we map them to the left handed versions.
	[s_shiftVK] = kKeyShift_L,
	[60] = kKeyShift_R,
	[s_controlVK] = kKeyControl_L,
	[62] = kKeyControl_R,
	[s_altVK] = kKeyAlt_L,
	[61] = kKeyAlt_R,

	[s_superVK] = kKeyMeta_L,
	[54] = kKeyMeta_R,

	// toggle modifiers
	[s_numLockVK] = kKeyNumLock,
	[s_capsLockVK] = kKeyCapsLock
};

uint32_t
unicharToKeyID(UniChar c)
{
	switch (c) {
	case 3:
		return kKeyKP_Enter;
	case 8:
		return kKeyBackSpace;
	case 9:
		return kKeyTab;
	case 13:
		return kKeyReturn;
	case 27:
		return kKeyEscape;
	case 127:
		return kKeyDelete;
	default:
		if (c < 32)
			return kKeyNone;
		return c;
	}
}

uint32_t keyDownMap[255];

uint32_t
mapKeyFromEvent(
		uint32_t * maskOut,
		CGEventRef event)
{
	// get virtual key
	UInt32 vkCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);

	// handle up events
	UInt32 eventKind = CGEventGetType(event);
	if (eventKind == kCGEventKeyUp) {
		// the id isn't used.  we just need the same button we used on
		// the key press.  note that we don't use or reset the dead key
		// state;  up events should not affect the dead key state.
		uint32_t res = keyDownMap[vkCode];
		keyDownMap[vkCode] = 0;
		return res ? res : vkCode;
	}

	// check for special keys
	if (s_controlKeys[vkCode]) {
	//	printf("%s mapped special key %3d to %4x\n", __func__, (int)vkCode, (int)s_controlKeys[vkCode]);
		return keyDownMap[vkCode] = s_controlKeys[vkCode];
	}

	// get keyboard info

	TISInputSourceRef currentKeyboardLayout = TISCopyCurrentKeyboardLayoutInputSource();

	if (currentKeyboardLayout == NULL) {
		printf("currentKeyboardLayout NULL!\n");
		return kKeyNone;
	}

	// choose action
	UInt16 action;
	if (eventKind == kCGEventKeyDown)
		action = kUCKeyActionDown;
	else if (CGEventGetIntegerValueField(event, kCGKeyboardEventAutorepeat) == 1)
		action = kUCKeyActionAutoKey;
	else
		return 0;

	// translate via uchr resource
	CFDataRef ref = (CFDataRef) TISGetInputSourceProperty(currentKeyboardLayout,
								kTISPropertyUnicodeKeyLayoutData);
	const UCKeyboardLayout* layout = (const UCKeyboardLayout*) CFDataGetBytePtr(ref);
	const int layoutValid = (layout != NULL);

	if (layoutValid) {
		static UInt32		m_deadKeyState = 0;
		UInt32 modifiers = 0;
		// translate key
		UniCharCount count;
		UniChar chars[2];
	//	LOG((CLOG_DEBUG2 "modifiers: %08x", modifiers & 0xffu));
		OSStatus status = UCKeyTranslate(layout,
							vkCode & 0xffu, action,
							(modifiers >> 8) & 0xffu,
							LMGetKbdType(), 0, &m_deadKeyState,
							sizeof(chars) / sizeof(chars[0]), &count, chars);

		// get the characters
		if (status == 0) {
			if (count != 0 || m_deadKeyState == 0) {
				m_deadKeyState = 0;
				return keyDownMap[vkCode] = unicharToKeyID(chars[0]);
			}
		}
	}

	return 0;
}
