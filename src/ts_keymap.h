/*
	ts_keymap.h

	Copyright 2011 Michel Pollet <buserror@gmail.com>

 	This file is part of touchstream.

	touchstream is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	touchstream is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY, without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with touchstream.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __TS_KEYMAP_H___
#define __TS_KEYMAP_H___

enum {
	KeyModifierShift      = 0x0001,
	KeyModifierControl    = 0x0002,
	KeyModifierAlt        = 0x0004,
	KeyModifierMeta       = 0x0008,
	KeyModifierSuper      = 0x0010,
	KeyModifierAltGr      = 0x0020,
	KeyModifierCapsLock   = 0x1000,
	KeyModifierNumLock    = 0x2000,
	KeyModifierScrollLock = 0x4000,
};
//@}

//! @name Modifier key bits
//@{
enum {
	kKeyModifierBitNone       = 16,
	kKeyModifierBitShift      = 0,
	kKeyModifierBitControl    = 1,
	kKeyModifierBitAlt        = 2,
	kKeyModifierBitMeta       = 3,
	kKeyModifierBitSuper      = 4,
	kKeyModifierBitAltGr      = 5,
	kKeyModifierBitCapsLock   = 12,
	kKeyModifierBitNumLock    = 13,
	kKeyModifierBitScrollLock = 14,
	kKeyModifierNumBits       = 16,
};
//@}

//! @name Modifier key identifiers
//@{
enum {
	kKeyModifierIDNull     = 0,
	kKeyModifierIDShift    = 1,
	kKeyModifierIDControl  = 2,
	kKeyModifierIDAlt      = 3,
	kKeyModifierIDMeta     = 4,
	kKeyModifierIDSuper    = 5,
	kKeyModifierIDLast     = 6,
};
//@}

//! @name Key identifiers
//@{
// all identifiers except kKeyNone and those in 0xE000 to 0xE0FF
// inclusive are equal to the corresponding X11 keysym - 0x1000.
enum {
// no key
	kKeyNone		= 0x0000,

// TTY functions
	kKeyBackSpace	= 0xEF08,	/* back space, back char */
	kKeyTab			= 0xEF09,
	kKeyLinefeed	= 0xEF0A,	/* Linefeed, LF */
	kKeyClear		= 0xEF0B,
	kKeyReturn		= 0xEF0D,	/* Return, enter */
	kKeyPause		= 0xEF13,	/* Pause, hold */
	kKeyScrollLock	= 0xEF14,
	kKeySysReq		= 0xEF15,
	kKeyEscape		= 0xEF1B,
	kKeyHenkan		= 0xEF23,	/* Start/Stop Conversion */
	kKeyHangulKana	= 0xEF26,	/* Hangul, Kana */
	kKeyHiraganaKatakana = 0xEF27,	/* Hiragana/Katakana toggle */
	kKeyZenkaku		= 0xEF2A,	/* Zenkaku/Hankaku */
	kKeyHanjaKanzi	= 0xEF2A,	/* Hanja, Kanzi */
	kKeyDelete		= 0xEFFF,	/* Delete, rubout */

// cursor control
	kKeyHome		= 0xEF50,
	kKeyLeft		= 0xEF51,	/* Move left, left arrow */
	kKeyUp			= 0xEF52,	/* Move up, up arrow */
	kKeyRight		= 0xEF53,	/* Move right, right arrow */
	kKeyDown		= 0xEF54,	/* Move down, down arrow */
	kKeyPageUp		= 0xEF55,
	kKeyPageDown	= 0xEF56,
	kKeyEnd			= 0xEF57,	/* EOL */
	kKeyBegin		= 0xEF58,	/* BOL */

// misc functions
	kKeySelect		= 0xEF60,	/* Select, mark */
	kKeyPrint		= 0xEF61,
	kKeyExecute		= 0xEF62,	/* Execute, run, do */
	kKeyInsert		= 0xEF63,	/* Insert, insert here */
	kKeyUndo		= 0xEF65,	/* Undo, oops */
	kKeyRedo		= 0xEF66,	/* redo, again */
	kKeyMenu		= 0xEF67,
	kKeyFind		= 0xEF68,	/* Find, search */
	kKeyCancel		= 0xEF69,	/* Cancel, stop, abort, exit */
	kKeyHelp		= 0xEF6A,	/* Help */
	kKeyBreak		= 0xEF6B,
	kKeyAltGr	 	= 0xEF7E,	/* Character set switch */
	kKeyNumLock		= 0xEF7F,

// keypad
	kKeyKP_Space	= 0xEF80,	/* space */
	kKeyKP_Tab		= 0xEF89,
	kKeyKP_Enter	= 0xEF8D,	/* enter */
	kKeyKP_F1		= 0xEF91,	/* PF1, KP_A, ... */
	kKeyKP_F2		= 0xEF92,
	kKeyKP_F3		= 0xEF93,
	kKeyKP_F4		= 0xEF94,
	kKeyKP_Home		= 0xEF95,
	kKeyKP_Left		= 0xEF96,
	kKeyKP_Up		= 0xEF97,
	kKeyKP_Right	= 0xEF98,
	kKeyKP_Down		= 0xEF99,
	kKeyKP_PageUp	= 0xEF9A,
	kKeyKP_PageDown	= 0xEF9B,
	kKeyKP_End		= 0xEF9C,
	kKeyKP_Begin	= 0xEF9D,
	kKeyKP_Insert	= 0xEF9E,
	kKeyKP_Delete	= 0xEF9F,
	kKeyKP_Equal	= 0xEFBD,	/* equals */
	kKeyKP_Multiply	= 0xEFAA,
	kKeyKP_Add		= 0xEFAB,
	kKeyKP_Separator= 0xEFAC,	/* separator, often comma */
	kKeyKP_Subtract	= 0xEFAD,
	kKeyKP_Decimal	= 0xEFAE,
	kKeyKP_Divide	= 0xEFAF,
	kKeyKP_0		= 0xEFB0,
	kKeyKP_1		= 0xEFB1,
	kKeyKP_2		= 0xEFB2,
	kKeyKP_3		= 0xEFB3,
	kKeyKP_4		= 0xEFB4,
	kKeyKP_5		= 0xEFB5,
	kKeyKP_6		= 0xEFB6,
	kKeyKP_7		= 0xEFB7,
	kKeyKP_8		= 0xEFB8,
	kKeyKP_9		= 0xEFB9,

// function keys
	kKeyF1			= 0xEFBE,
	kKeyF2			= 0xEFBF,
	kKeyF3			= 0xEFC0,
	kKeyF4			= 0xEFC1,
	kKeyF5			= 0xEFC2,
	kKeyF6			= 0xEFC3,
	kKeyF7			= 0xEFC4,
	kKeyF8			= 0xEFC5,
	kKeyF9			= 0xEFC6,
	kKeyF10			= 0xEFC7,
	kKeyF11			= 0xEFC8,
	kKeyF12			= 0xEFC9,
	kKeyF13			= 0xEFCA,
	kKeyF14			= 0xEFCB,
	kKeyF15			= 0xEFCC,
	kKeyF16			= 0xEFCD,
	kKeyF17			= 0xEFCE,
	kKeyF18			= 0xEFCF,
	kKeyF19			= 0xEFD0,
	kKeyF20			= 0xEFD1,
	kKeyF21			= 0xEFD2,
	kKeyF22			= 0xEFD3,
	kKeyF23			= 0xEFD4,
	kKeyF24			= 0xEFD5,
	kKeyF25			= 0xEFD6,
	kKeyF26			= 0xEFD7,
	kKeyF27			= 0xEFD8,
	kKeyF28			= 0xEFD9,
	kKeyF29			= 0xEFDA,
	kKeyF30			= 0xEFDB,
	kKeyF31			= 0xEFDC,
	kKeyF32			= 0xEFDD,
	kKeyF33			= 0xEFDE,
	kKeyF34			= 0xEFDF,
	kKeyF35			= 0xEFE0,

// modifiers
	kKeyShift_L		= 0xEFE1,	/* Left shift */
	kKeyShift_R		= 0xEFE2,	/* Right shift */
	kKeyControl_L	= 0xEFE3,	/* Left control */
	kKeyControl_R	= 0xEFE4,	/* Right control */
	kKeyCapsLock	= 0xEFE5,	/* Caps lock */
	kKeyShiftLock	= 0xEFE6,	/* Shift lock */
	kKeyMeta_L		= 0xEFE7,	/* Left meta */
	kKeyMeta_R		= 0xEFE8,	/* Right meta */
	kKeyAlt_L		= 0xEFE9,	/* Left alt */
	kKeyAlt_R		= 0xEFEA,	/* Right alt */
	kKeySuper_L		= 0xEFEB,	/* Left super */
	kKeySuper_R		= 0xEFEC,	/* Right super */
	kKeyHyper_L		= 0xEFED,	/* Left hyper */
	kKeyHyper_R		= 0xEFEE,	/* Right hyper */

// multi-key character composition
	kKeyCompose			= 0xEF20,
	kKeyDeadGrave		= 0x0300,
	kKeyDeadAcute		= 0x0301,
	kKeyDeadCircumflex	= 0x0302,
	kKeyDeadTilde		= 0x0303,
	kKeyDeadMacron		= 0x0304,
	kKeyDeadBreve		= 0x0306,
	kKeyDeadAbovedot	= 0x0307,
	kKeyDeadDiaeresis	= 0x0308,
	kKeyDeadAbovering	= 0x030a,
	kKeyDeadDoubleacute	= 0x030b,
	kKeyDeadCaron		= 0x030c,
	kKeyDeadCedilla		= 0x0327,
	kKeyDeadOgonek		= 0x0328,

// more function and modifier keys
	kKeyLeftTab			= 0xEE20,

// update modifiers
	kKeySetModifiers	= 0xEE06,
	kKeyClearModifiers	= 0xEE07,

// group change
	kKeyNextGroup		= 0xEE08,
	kKeyPrevGroup		= 0xEE0A,

// extended keys
	kKeyEject			= 0xE001,
	kKeySleep			= 0xE05F,
	kKeyWWWBack			= 0xE0A6,
	kKeyWWWForward		= 0xE0A7,
	kKeyWWWRefresh		= 0xE0A8,
	kKeyWWWStop			= 0xE0A9,
	kKeyWWWSearch		= 0xE0AA,
	kKeyWWWFavorites	= 0xE0AB,
	kKeyWWWHome			= 0xE0AC,
	kKeyAudioMute		= 0xE0AD,
	kKeyAudioDown		= 0xE0AE,
	kKeyAudioUp			= 0xE0AF,
	kKeyAudioNext		= 0xE0B0,
	kKeyAudioPrev		= 0xE0B1,
	kKeyAudioStop		= 0xE0B2,
	kKeyAudioPlay		= 0xE0B3,
	kKeyAppMail			= 0xE0B4,
	kKeyAppMedia		= 0xE0B5,
	kKeyAppUser1		= 0xE0B6,
	kKeyAppUser2		= 0xE0B7,
};

#endif /* __TS_KEYMAP_H___ */
