#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include "../c_init.h"
#include "../c_intrf.h"
#include "../cfg.h"
#include "../cpu/execute.h"
#include "../cpu/regs.h"
#include "../input.h"
#include "../link.h"
#include "../types.h"
#include "../video/procvid.h"
#include "../video/procvidc.h"
#include "../zmovie.h"
#include "../zpath.h"
#include "c_gui.h"
#include "c_guikeys.h"
#include "gui.h"
#include "guikeys.h"
#include "guiwindp.h"

#ifdef __MSDOS__
#	include <string.h>

#	include "../asm_call.h"
#	include "../dos/initvid.h"
#	include "../ui.h"
#	include "../video/copyvid.h"
#	include "guimisc.h"
#else
#	include "../video/ntsc.h"
#	include "guifuncs.h"
#endif

#ifdef __UNIXSDL__
#	include "../linux/sdllink.h"

#	ifdef __OPENGL__
#		include "../linux/gl_draw.h"
#	endif
#endif

#ifdef __WIN32__
#	include "../win/winlink.h"
#endif


#ifdef __UNIXSDL__
#	define IFKEY(key, a, b) if ((key) == (a) || (numlockptr != 1 && (key) == (b)))
#else
#	define IFKEY(key, a, b) if ((key) == (b))
#endif


u1 GUIDelayB;
u4 GUIkeydelay;

static u1 GUIJoyPadnk[8];
static u4 GUIfirstkey;
static u4 GUIlastkey;
static u4 GUInextkeydelay;


static void GUIqcheckkeys(u4 const p1)
{
	if (pressed[p1] == 1) GUIfirstkey = 1;
}


static void GUIqcheckkeys2(u4 const p1, u4 const p2)
{
	if (pressed[p1] == 1 && GUIJoyPadnk[p2] != 2) GUIfirstkey = 1;
}


static bool GUIgetprkeys(u4 const p1, u4 const p2, char* const al)
{
	if (pressed[p1] == 1)
	{
		GUInextkeydelay = 10;
	}
	else if (pressed[p1] != 2 || GUIfirstkey == 1 || GUIlastkey != p1)
	{
		return false;
	}
	GUIlastkey = p1;
	pressed[p1] = 2;
	if (GUIkeydelay == 0)
	{
		GUIkeydelay     = GUInextkeydelay;
		GUInextkeydelay = 2;
		*al             = p2;
	}
	return true;
}


static bool GUIgetprkeysb(u4 const p1, u4 const p2, char* const al)
{
	if (pressed[p1] != 1)
	{
		if (pressed[p1] == 0) GUIescpress = 0;
	}
	else if (GUIescpress != 1)
	{
		pressed[p1] = 2;
		*al = p2;
		return true;
	}
	return false;
}


static bool GUIgetprkeys2(u4 const p1, u4 const p2, u4 const p3, char* const al)
{
	if (pressed[p1] == 0)
	{
		GUIJoyPadnk[p3] = 0;
		return false;
	}
	else if (GUIJoyPadnk[p3] != 2 && pressed[p1] == 1)
	{
		GUInextkeydelay = 10;
	}
	else if (GUIfirstkey == 1 || GUIlastkey != p1)
	{
		return false;
	}
	GUIlastkey = p1;
	GUIJoyPadnk[p3] = 2;
	if (GUIkeydelay == 0)
	{
		GUIkeydelay     = GUInextkeydelay;
		GUIDelayB       = 5;
		GUInextkeydelay = 2;
		*al             = p2;
	}
	return true;
}


static u1 ToUpperASM(u1 dh)
{
	if ('a' <= dh && dh <= 'z') dh -= 'a' - 'A';
	return dh;
}


static void KeyTabInc(u4* const first, ...) // tab arrays
{
	va_list ap;
	va_start(ap, first);

	u4* a = first;
	for (;;)
	{
		u4 const eax = a[0] / a[1];
		a[0] = a[0] % a[1];
		u4* const b = va_arg(ap, u4*);
		(b ? b : first)[0] |= eax;
		if (eax == 1) break;
		if (eax < a[0]) ++a[0];
		if (a[0] != 0) break;
		if (!b) break;
		a = b;
	}

	va_end(ap);
}


static void GUIKeyCheckbox(u1* const p1, char const p2, char const dh)
{
	if (dh == p2) *p1 ^= 1;
}


static void GUIKeyButtonHole(u1* const p1, u1 const p2, char const p3, char const dh)
{
	if (dh == p3) *p1 = p2;
}


static char GUIInputBoxText(char* const* const p1, void (* const p2)(void), char const dh)
{
	if (dh == '\0') return dh;

	if (GUIInputBox == 0) return dh;

	char* const ecx = p1[GUIInputBox - 1];
	char*       eax = ecx;

#ifdef __WIN32__
	// Look for paste request
	if (ctrlptr && dh == 'V')
	{
		CBBuffer = ecx;
		CBLength = GUIInputLimit;
		PasteClipBoard();
		return '\0';
	}
#endif

	// Find end of string
	while (*eax != '\0') ++eax;

	if (dh == 8) // Backspace
	{
		if (eax != ecx) eax[-1] = '\0';
		return '\0';
	}

	if (dh == 13) // Enter
	{
		GUIInputBox = 0;
		p2();
		return '\0';
	}

	// check if we're at the end
	if (eax != ecx + GUIInputLimit) *eax = dh;
	return '\0';
}


static void GUIInputKeys(char dh)
{
	dh = ToUpperASM(dh);
	if (dh == 9)
	{
		KeyTabInc(GUIInputTabs, (u4*)0);
		GUIFreshInputSelect = 1;
	}
	GUIKeyCheckbox(&GameSpecificInput, 'G', dh);
	GUIKeyCheckbox(&AllowUDLR,         'A', dh);
	GUIKeyCheckbox(&Turbo30hz,         'T', dh);
	if (dh == 'U')
	{
		pl12s34 ^= 1;
		MultiTap = pl12s34 != 1 && (pl3contrl != 0 || pl4contrl != 0 || pl5contrl != 0);
	}
#ifdef __MSDOS__
	GUIKeyCheckbox(&SidewinderFix, 'S', dh);
	if (dh == 'J')
	{
		switch (cplayernum)
		{
			case 0: pl1p209 ^= 1; break;
			case 1: pl2p209 ^= 1; break;
			case 2: pl3p209 ^= 1; break;
			case 3: pl4p209 ^= 1; break;
			case 4: pl5p209 ^= 1; break;
		}
		SetDevice();
	}
#endif
}


static void GUIOptionKeys(char dh)
{
	if (dh == 9)
	{
		KeyTabInc(GUIOptionTabs, (u4*)0);
	}
	dh = ToUpperASM(dh);
	if (GUIOptionTabs[0] == 1)
	{ // Basic
		if (ShowMMXSupport == 1) GUIKeyCheckbox(&MMXSupport, 'M', dh);
		GUIKeyCheckbox(&Show224Lines,       'L', dh);
		GUIKeyCheckbox(&newengen,           'N', dh);
		GUIKeyCheckbox(&bgfixer,            'A', dh);
		GUIKeyCheckbox(&AutoPatch,          'I', dh);
		GUIKeyCheckbox(&DisplayInfo,        'R', dh);
		GUIKeyCheckbox(&RomInfo,            'G', dh);
#ifdef __WIN32__
		GUIKeyCheckbox(&PauseFocusChange,   'B', dh);
		GUIKeyCheckbox(&HighPriority,       'P', dh);
		CheckPriority();
#endif
		GUIKeyCheckbox(&DisableScreenSaver, 'D', dh);
#ifdef __WIN32__
		CheckScreenSaver();
#endif
	}

	if (GUIOptionTabs[0] == 2)
	{ // More
		GUIKeyCheckbox(&FPSAtStart,            'F', dh);
		GUIKeyCheckbox(&TimerEnable,           'C', dh);
		GUIKeyCheckbox(&TwelveHourClock,       'H', dh);
		GUIKeyCheckbox(&ClockBox,              'X', dh);
		GUIKeyCheckbox(&SmallMsgText,          'S', dh);
		GUIKeyCheckbox(&GUIEnableTransp,       'T', dh);
		GUIKeyButtonHole(&ScreenShotFormat, 0, 'B', dh);
#ifndef NO_PNG
		GUIKeyButtonHole(&ScreenShotFormat, 1, 'P', dh);
#endif
	}
}


#ifdef __MSDOS__
static void DOSClearScreenKey(void)
{
	asm_call(DOSClearScreen);
	memset(vidbufferofsb, 0, 288 * 128 * 4);
}
#endif


static void GUIVideoKeys(char dh, char const dl)
{
#ifndef __MSDOS__
	dh = GUIInputBoxText(GUICustomResTextPtr, SetCustomXY, dh);
#endif

	if (dh == 9)
	{
		if (NTSCFilter != 0 && GUINTVID[cvidmode] != 0)
		{
			KeyTabInc(GUIVideoTabs, GUIVntscTab, (u4*)0);
		}
		else
		{
			KeyTabInc(GUIVideoTabs, (u4*)0);
		}
	}

	dh = ToUpperASM(dh);
	if (GUIVideoTabs[0] == 1)
	{
		IFKEY(dl, 89, 71) // "Home"
		{
			GUIcurrentvideocursloc = 0;
			GUIcurrentvideoviewloc = 0;
		}

		IFKEY(dl, 95, 79) // "End"
		{
			u4 const eax = NumVideoModes - 1;
			GUIcurrentvideocursloc = eax;
			GUIcurrentvideoviewloc = eax - 19;
			// XXX probably should be 0x80000000 (below, too)
			if (GUIcurrentvideoviewloc & 0x8000000) GUIcurrentvideoviewloc = 0;
		}

		IFKEY(dl, 90, 72) // "Up"
		{
			if (GUIcurrentvideocursloc != 0)
			{
				if (GUIcurrentvideoviewloc == GUIcurrentvideocursloc) --GUIcurrentvideoviewloc;
				--GUIcurrentvideocursloc;
			}
		}

		IFKEY(dl, 80, 96) // "Down"
		{
			u4 const ebx = GUIcurrentvideocursloc + 1;
			if (ebx != NumVideoModes)
			{
				++GUIcurrentvideocursloc;
				if (ebx - 20 == GUIcurrentvideoviewloc) ++GUIcurrentvideoviewloc;
			}
		}

		IFKEY(dl, 73, 91) // "PageUp"
		{
			GUIcurrentvideoviewloc -= 20;
			GUIcurrentvideocursloc -= 20;
			if (GUIcurrentvideoviewloc & 0x8000000) GUIcurrentvideoviewloc = 0;
			if (GUIcurrentvideocursloc & 0x8000000) GUIcurrentvideocursloc = 0;
		}

		IFKEY(dl, 97, 81) // "PageDown"
		{
			GUIcurrentvideoviewloc += 20;
			GUIcurrentvideocursloc += 20;
			u4 ebx = NumVideoModes - 1;
			if (GUIcurrentvideocursloc >= ebx) GUIcurrentvideocursloc = ebx;
			ebx -= 19;
			if ((s4)GUIcurrentvideoviewloc >= (s4)ebx)
			{
				if (ebx & 0x8000000) ebx = 0;
				GUIcurrentvideoviewloc = ebx;
			}
		}

		if (dl == 13) GUICBHold = 4; // "Return"
	}

	if (GUIVideoTabs[0] == 2)
	{
#ifdef __MSDOS__
		if ((smallscreenon & 0xFF) != 1)
#endif
		{
#ifdef __MSDOS__
			if (ScreenScale != 1)
#endif
			{
				if (dh == 'I')
				{
					u4 const ebx = cvidmode;
#ifndef __MSDOS__
					if (GUIBIFIL[ebx] != 0)
					{
						BilinearFilter ^= 1;
						NTSCFilter      = 0;
#ifdef __WIN32__
						initDirectDraw();
#elif defined __OPENGL__
						initwinvideo();
#endif
						Clear2xSaIBuffer();
					}
					else
					{
#else
						if (GUIEAVID[ebx] != 0) goto interpolation;
#endif
#ifdef __WIN32__
						if (GUIDSIZE[ebx] != 0)
#else
						if (GUII2VID[ebx] != 0)
#endif
						{
#ifdef __MSDOS__
interpolation:
#endif
							antienab ^= 1;
							if (antienab != 0)
							{
								En2xSaI    = 0;
								hqFilter   = 0;
								NTSCFilter = 0;
							}
						}
#ifndef __MSDOS__
					}
#endif
				}

				if (dh == 'N')
				{
					if (GUINTVID[cvidmode] != 0)
					{
						NTSCFilter ^= 1;
						if (NTSCFilter != 0)
						{
							En2xSaI        = 0;
							hqFilter       = 0;
							scanlines      = 0;
							antienab       = 0;
#ifdef __OPENGL__
							BilinearFilter = 0;
#endif
#ifdef __WIN32__
							if (NTSCFilter != 0) Keep4_3Ratio = 1;
#endif
#ifndef __MSDOS__
							NTSCFilterInit();
#endif
						}
					}
				}

#ifdef __MSDOS__
				if (GUI2xVID[cvidmode] != 0)
#else
				if (GUIDSIZE[cvidmode] != 0)
#endif
				{
					switch (dh)
					{
						u1 al;
						case 'S': al = 1; goto yesfilter;
						case 'E': al = 2; goto yesfilter;
						case 'P': al = 3; goto yesfilter;
yesfilter:
						Clear2xSaIBuffer();
						hqFilter   = 0;
						scanlines  = 0;
						antienab   = 0;
						NTSCFilter = 0;
						En2xSaI = En2xSaI != al ? al : 0;
						return;
					}
				}

				if (dh == 'Q')
				{
#ifndef __MSDOS__
					if (GUIHQ2X[cvidmode] != 0 || GUIHQ3X[cvidmode] != 0 || GUIHQ4X[cvidmode] != 0)
#else
					if (GUIHQ2X[cvidmode] != 0)
#endif
					{
						Clear2xSaIBuffer();
						hqFilter ^= 1;
						if (hqFilter != 0)
						{
							scanlines  = 0;
							En2xSaI    = 0;
							antienab   = 0;
							NTSCFilter = 0;
						}
					}
				}

#ifndef __MSDOS__
				if (dh == 'X')
				{
					if (hqFilter != 0 && GUIHQ2X[cvidmode] != 0)
					{
						Clear2xSaIBuffer();
						GUIKeyButtonHole(&hqFilterlevel, 2, 'X', dh);
					}
				}

				if (dh == '3')
				{
					if (hqFilter != 0 && GUIHQ3X[cvidmode] != 0)
					{
						Clear2xSaIBuffer();
						GUIKeyButtonHole(&hqFilterlevel, 3, '3', dh);
					}
				}

				if (dh == '4')
				{
					if (hqFilter != 0 && GUIHQ4X[cvidmode] != 0)
					{
						Clear2xSaIBuffer();
						GUIKeyButtonHole(&hqFilterlevel, 4, '4', dh);
					}
				}
#endif
			}

#ifdef __MSDOS__
			if (GUISLVID[cvidmode] != 0)
#else
			if (GUIDSIZE[cvidmode] != 0)
#endif
			{
				GUIKeyButtonHole(&scanlines, 0, 'O', dh);
#ifdef __MSDOS__
				if (dh == 'O') goto needupdate;
#endif
				if (dh == 'F')
				{
					En2xSaI    = 0;
					hqFilter   = 0;
					NTSCFilter = 0;
					GUIKeyButtonHole(&scanlines, 1, 'F', dh);
#ifdef __MSDOS__
needupdate:
					asm_call(DOSClearScreen);
					if (cvidmode == 2 || cvidmode == 5) // modeQ
					{
						cbitmode = 1;
						asm_call(initvideo2);
						cbitmode = 0;
						GUISetPal();
					}
#endif
				}
			}

#ifdef __MSDOS__
			if (ScreenScale != 1)
#endif
			{
#ifdef __MSDOS__
				if (GUIHSVID[cvidmode] != 0)
#else
				if (GUIDSIZE[cvidmode] != 0)
#endif
				{
					if (dh == '5')
					{
						En2xSaI    = 0;
						hqFilter   = 0;
						NTSCFilter = 0;
#ifdef __MSDOS__
						asm_call(DOSClearScreen);
#endif
						GUIKeyButtonHole(&scanlines, 3, '5', dh);
					}

					if (dh == '5')
					{
						En2xSaI    = 0;
						hqFilter   = 0;
						NTSCFilter = 0;
#ifdef __MSDOS__
						asm_call(DOSClearScreen);
#endif
						GUIKeyButtonHole(&scanlines, 2, '2', dh);
					}
				}
			}
		}
		GUIKeyCheckbox(&GrayscaleMode, 'G', dh);

		if (dh == 'H')
		{
			if (GUIM7VID[cvidmode] != 0) Mode7HiRes16b ^= 1;
		}

#if !defined __UNIXSDL__ || defined __OPENGL__
		if (dh == 'V')
		{
#ifdef __UNIXSDL__
			if (allow_glvsync == 1 && GUIBIFIL[cvidmode] != 0)
#endif
			{
				vsyncon ^= 1;
#ifdef __WIN32__
				initDirectDraw();
				Clear2xSaIBuffer();
#elif defined __MSDOS__
				if (vsyncon != 0) Triplebufen = 0;
#elif defined __OPENGL__
				initwinvideo();
				Clear2xSaIBuffer();
#endif
			}
		}
#endif

#ifndef __UNIXSDL__
		if (dh == 'T')
		{
#ifdef __WIN32__
			if (GUIWFVID[cvidmode] != 0)
			{
				TripleBufferWin ^= 1;
				initDirectDraw();
				Clear2xSaIBuffer();
			}
#else
			if (GUITBVID[cvidmode] != 0)
			{
				Triplebufen ^= 1;
				if (Triplebufen != 0) vsyncon = 0;
			}
#endif
		}
#endif

#ifndef __MSDOS__
		if (dh == 'R')
		{
			if (GUIKEEP43[cvidmode] != 0)
			{
				Keep4_3Ratio ^= 1;
				initwinvideo();
				Clear2xSaIBuffer();
			}
		}
#else
		if (dh == 'M')
		{
			if (GUISSVID[cvidmode] != 0)
			{
				smallscreenon ^= 1;
				ScreenScale    = 0;
				antienab       = 0;
				En2xSaI        = 0;
				scanlines      = 0;
				DOSClearScreenKey();
			}
		}

		if (dh == 'C')
		{
			if (GUIWSVID[cvidmode] != 0)
			{
				ScreenScale  ^= 1;
				smallscreenon = 0;
				antienab      = 0;
				En2xSaI       = 0;
				DOSClearScreenKey();
			}
		}
#endif
	}

	if ((s4)GUIVntscTab >= 1)
	{
		GUIKeyCheckbox(&NTSCBlend, 'B', dh);
		GUIKeyCheckbox(&NTSCRef,   'R', dh);
	}
}


static void GUISoundKeys(char dh)
{
	dh = ToUpperASM(dh);
	if (dh == 'E')
	{ // Enable sound
		soundon ^= 1;
#ifdef __WIN32__
		reInitSound();
#endif
	}

	GUIKeyCheckbox(&StereoSound,   'S', dh);
	GUIKeyCheckbox(&RevStereo,     'V', dh);
	GUIKeyCheckbox(&Surround,      'M', dh);
	GUIKeyCheckbox(&SPCDisable,    'D', dh);
#ifdef __MSDOS__
	GUIKeyCheckbox(&Force8b,       'F', dh);
#endif
#ifdef __WIN32__
	GUIKeyCheckbox(&PrimaryBuffer, 'P', dh);
#endif

	if (dh == 'R')
	{ // Sampling Rate
		static u1 const sampratenext[] = { 1, 4, 5, 6, 2, 3, 0, 0 };
		SoundQuality = SoundQuality & 0xFFFFFF00 | sampratenext[SoundQuality & 0xFF];
	}

	GUIKeyButtonHole(&SoundInterpType, 0, 'N', dh);
	GUIKeyButtonHole(&SoundInterpType, 1, 'G', dh);
	GUIKeyButtonHole(&SoundInterpType, 2, 'C', dh);
	if (MMXSupport != 0) GUIKeyButtonHole(&SoundInterpType, 3, '8', dh);

	GUIKeyButtonHole(&LowPassFilterType, 0, 'O', dh);
	GUIKeyButtonHole(&LowPassFilterType, 1, 'I', dh);
	GUIKeyButtonHole(&LowPassFilterType, 2, 'Y', dh);
	if (MMXSupport != 0) GUIKeyButtonHole(&LowPassFilterType, 3, 'H', dh);
}


static void GUIGUIOptnsKeys(char dh)
{
	dh = ToUpperASM(dh);
	GUIKeyCheckbox(&mousewrap,   'M', dh);
	GUIKeyCheckbox(&mouseshad,   'S', dh);
	GUIKeyCheckbox(&esctomenu,   'G', dh);
	GUIKeyCheckbox(&savewinpos,  'W', dh);
	GUIKeyCheckbox(&GUIRClick,   'C', dh);
	GUIKeyCheckbox(&JoyPad1Move, 'U', dh);
	GUIKeyCheckbox(&newfont,     'O', dh);
	GUIKeyCheckbox(&lhguimouse,  '/', dh);

	if (dh == 'F')
	{
		FilteredGUI ^= 1;
#ifdef __MSDOS__
		asm_call(DOSClearScreen);
#else
		if (GUIBIFIL[cvidmode] != 0)
		{
#	ifdef __WIN32__
			initDirectDraw();
#	elif defined __OPENGL__
			initwinvideo();
#	endif
		}
#endif
		Clear2xSaIBuffer();
	}

	GUIKeyButtonHole(&GUIEffect, 0, 'E', dh);
	GUIKeyButtonHole(&GUIEffect, 1, 'N', dh);
	GUIKeyButtonHole(&GUIEffect, 2, 'A', dh);
	GUIKeyButtonHole(&GUIEffect, 3, 'B', dh);
	GUIKeyButtonHole(&GUIEffect, 4, 'R', dh);
	GUIKeyButtonHole(&GUIEffect, 5, 'K', dh);

#ifdef __WIN32__
	GUIKeyCheckbox(&MouseWheel,        'H', dh);
	GUIKeyCheckbox(&TrapMouseCursor,   'P', dh);
	GUIKeyCheckbox(&AlwaysOnTop,       'T', dh);
	GUIKeyCheckbox(&SaveMainWindowPos, 'V', dh);
	GUIKeyCheckbox(&AllowMultipleInst, 'L', dh);
#endif
}


static void GUIAboutKeys(char dh)
{
	dh = ToUpperASM(dh);
	GUIKeyCheckbox(&EEgg, 'E', dh);
}


static void GUIMovieKeys(char dh)
{
	dh = ToUpperASM(dh);
	dh = GUIInputBoxText(GUIMovieTextPtr, SetMovieForcedLength, dh);
	if (dh == 9)
	{
		if (MovieProcessing == 0) KeyTabInc(GUIMovieTabs, GUIDumpingTab, (u4*)0);
	}

	GUIKeyButtonHole((u1*)&CMovieExt, 'v', '0', dh); // XXX ugly cast
	GUIKeyButtonHole((u1*)&CMovieExt, '1', '1', dh); // XXX ugly cast
	GUIKeyButtonHole((u1*)&CMovieExt, '2', '2', dh); // XXX ugly cast
	GUIKeyButtonHole((u1*)&CMovieExt, '3', '3', dh); // XXX ugly cast
	GUIKeyButtonHole((u1*)&CMovieExt, '4', '4', dh); // XXX ugly cast
	GUIKeyButtonHole((u1*)&CMovieExt, '5', '5', dh); // XXX ugly cast
	GUIKeyButtonHole((u1*)&CMovieExt, '6', '6', dh); // XXX ugly cast
	GUIKeyButtonHole((u1*)&CMovieExt, '7', '7', dh); // XXX ugly cast
	GUIKeyButtonHole((u1*)&CMovieExt, '8', '8', dh); // XXX ugly cast
	GUIKeyButtonHole((u1*)&CMovieExt, '9', '9', dh); // XXX ugly cast

	if (GUIMovieTabs[0] == 1)
	{
		GUIKeyButtonHole(&MovieStartMethod, 0, 'N', dh);
		GUIKeyButtonHole(&MovieStartMethod, 1, 'P', dh);
		GUIKeyButtonHole(&MovieStartMethod, 2, 'R', dh);
		GUIKeyButtonHole(&MovieStartMethod, 3, 'S', dh);

		GUIKeyButtonHole(&MZTForceRTR, 0, 'M', dh);
		GUIKeyButtonHole(&MZTForceRTR, 1, 'C', dh);
		GUIKeyButtonHole(&MZTForceRTR, 2, 'B', dh);

		GUIKeyCheckbox(&MovieDisplayFrame, 'D', dh);
	}

	if (GUIDumpingTab[0] == 1)
	{
		GUIKeyButtonHole(&MovieVideoMode, 0, 'O', dh);
		GUIKeyButtonHole(&MovieVideoMode, 1, 'W', dh);
		GUIKeyButtonHole(&MovieVideoMode, 2, 'F', dh);
		GUIKeyButtonHole(&MovieVideoMode, 3, 'L', dh);
		GUIKeyButtonHole(&MovieVideoMode, 4, 'X', dh);
		GUIKeyButtonHole(&MovieVideoMode, 5, 'C', dh);

		GUIKeyCheckbox(&MovieAudio,         'A', dh);
		GUIKeyCheckbox(&MovieVideoAudio,    'V', dh);
		GUIKeyCheckbox(&MovieAudioCompress, 'M', dh);

		GUIKeyButtonHole(&MovieForcedLengthEnabled, 0, 'Z', dh);
		GUIKeyButtonHole(&MovieForcedLengthEnabled, 1, 'R', dh);
		GUIKeyButtonHole(&MovieForcedLengthEnabled, 2, 'U', dh);
	}
}


static void GUIComboKeys(char dh)
{
	// Calculate Position
	GUIComboPos = strlen(GUIComboTextH);

	switch (dh)
	{
		case '\b': // Backspace
			if (GUIComboPos != 0)
			{
				GUICCFlash                     = 0;
				GUIComboTextH[GUIComboPos - 1] = '\0';
			}
			break;

		case '\0':
		case '\r':
			break;

		default:
			if (GUIComboPos != 19)
			{
				GUICCFlash                 = 0;
				GUIComboTextH[GUIComboPos] = dh;
			}
			break;
	}
}


static void GUIAddonKeys(char dh)
{
	dh = ToUpperASM(dh);
	GUIKeyButtonHole(&device1, 0, 'G', dh);
	GUIKeyButtonHole(&device1, 1, 'M', dh);
	GUIKeyButtonHole(&device2, 0, 'A', dh);
	GUIKeyButtonHole(&device2, 1, 'O', dh);
	GUIKeyButtonHole(&device2, 2, 'S', dh);
	GUIKeyButtonHole(&device2, 3, '1', dh);
	GUIKeyButtonHole(&device2, 4, '2', dh);

	GUIKeyCheckbox(&mouse1lh, 'L', dh);
	GUIKeyCheckbox(&mouse2lh, 'E', dh);
}


static void GUIChipKeys(char dh)
{
	dh = ToUpperASM(dh);
	GUIKeyCheckbox(&nssdip1, '1', dh);
	GUIKeyCheckbox(&nssdip2, '2', dh);
	GUIKeyCheckbox(&nssdip3, '3', dh);
	GUIKeyCheckbox(&nssdip4, '4', dh);
	GUIKeyCheckbox(&nssdip5, '5', dh);
	GUIKeyCheckbox(&nssdip6, '6', dh);
}


static void GUIPathKeys(char dh)
{
	if (GUIPathTabs[0] == 1) dh = GUIInputBoxText(GUIPathsTab1Ptr, init_save_paths, dh);
	if (GUIPathTabs[0] == 2) dh = GUIInputBoxText(GUIPathsTab2Ptr, init_save_paths, dh);
	if (GUIPathTabs[0] == 3) dh = GUIInputBoxText(GUIPathsTab3Ptr, init_save_paths, dh);

	if (dh == 9) KeyTabInc(GUIPathTabs, (u4*)0);

	if (GUIPathTabs[0] == 1)
	{ // General
		GUIKeyButtonHole(&RelPathBase, 0, 'C', dh);
		GUIKeyButtonHole(&RelPathBase, 1, 'R', dh);
	}
}


static void GUISaveKeys(char dh)
{
	dh = ToUpperASM(dh);
	GUIKeyCheckbox(&AutoIncSaveSlot, 'I', dh);
	GUIKeyCheckbox(&nosaveSRAM,      'D', dh);
	GUIKeyCheckbox(&SRAMSave5Sec,    'C', dh);
	GUIKeyCheckbox(&LatestSave,      'S', dh);
	GUIKeyCheckbox(&AutoState,       'A', dh);
	GUIKeyCheckbox(&SRAMState,       'L', dh);
	GUIKeyCheckbox(&PauseLoad,       'P', dh);
	GUIKeyCheckbox(&PauseRewind,     'R', dh);
}


static void GUISpeedKeys(char dh)
{
	dh = ToUpperASM(dh);
	GUIKeyCheckbox(&FastFwdToggle, 'T', dh);

	if (dh == 'A') // Framerate Checkboxes
	{
		if (frameskip != 0) // 0 = autoframerate / 1-10 = frameskip 0-9
		{
			frameskip = 0;
		}
		else
		{
			FPSOn     = 0;
			frameskip = 1;
		}
	}
}


void GUIgetcurrentinput(void)
{
	char UseExtKey = '\0';
	char ch        = '\0';
	if (Check_Key() != 0)
	{
		char const al = Get_Key();
		ch = al;
		if (al == 0)
		{ // Extended key
			UseExtKey = Get_Key();
			ch = '\0';
		}
	}
	char dh = ch;
	GUIDelayB = 0;
	// Convert pressed to keys
	GUIfirstkey = 0;

#ifdef __UNIXSDL___
	GUIqcheckkeys(90); // UP
	GUIqcheckkeys(96); // DOWN
	GUIqcheckkeys(92); // LEFT
	GUIqcheckkeys(94); // RIGHT
	GUIqcheckkeys(89); // HOME
	GUIqcheckkeys(91); // PGUP
	GUIqcheckkeys(95); // END
	GUIqcheckkeys(97); // PGDOWN
#endif
	GUIqcheckkeys(72); // NUMPAD STUFF
	GUIqcheckkeys(80);
	GUIqcheckkeys(75);
	GUIqcheckkeys(77);
	GUIqcheckkeys(73);
	GUIqcheckkeys(81);
	GUIqcheckkeys(71);
	GUIqcheckkeys(79);
	GUIqcheckkeys(1);
	GUIqcheckkeys(0x1C);
#ifndef __MSDOS__
	GUIqcheckkeys(0xC8);
	GUIqcheckkeys(0xD0);
	GUIqcheckkeys(0xCB);
	GUIqcheckkeys(0xCD);
	GUIqcheckkeys(0xC9);
	GUIqcheckkeys(0xD1);
	GUIqcheckkeys(0xC7);
	GUIqcheckkeys(0xCF);
	GUIqcheckkeys(0x9C);
#endif

	if (JoyPad1Move != 0)
	{
		JoyRead();
		GUIqcheckkeys2(pl1upk,    0);
		GUIqcheckkeys2(pl1downk,  1);
		GUIqcheckkeys2(pl1leftk,  2);
		GUIqcheckkeys2(pl1rightk, 3);
		GUIqcheckkeys2(pl1Lk,     4);
		GUIqcheckkeys2(pl1Rk,     5);
		GUIqcheckkeys2(pl1Bk,     6);
		GUIqcheckkeys2(pl1Ak,     7);
	}

	char al = '\0';
#ifdef __UNIXSDL__
	if (GUIgetprkeys(90, 90, &al)) goto done; // UP
	if (GUIgetprkeys(96, 96, &al)) goto done; // DOWN
	if (GUIgetprkeys(92, 92, &al)) goto done; // LEFT
	if (GUIgetprkeys(94, 94, &al)) goto done; // RIGHT
	if (GUIgetprkeys(89, 89, &al)) goto done; // HOME
	if (GUIgetprkeys(91, 91, &al)) goto done; // PGUP
	if (GUIgetprkeys(95, 95, &al)) goto done; // END
	if (GUIgetprkeys(97, 97, &al)) goto done; // PGDOWN

	if (GUIgetprkeys(72, 72, &al)) goto done; // KP8
	if (GUIgetprkeys(80, 80, &al)) goto done; // KP2
	if (GUIgetprkeys(75, 75, &al)) goto done; // KP4
	if (GUIgetprkeys(77, 77, &al)) goto done; // KP6
	if (GUIgetprkeys(71, 71, &al)) goto done; // KP7
	if (GUIgetprkeys(79, 79, &al)) goto done; // KP9
	if (GUIgetprkeys(81, 81, &al)) goto done; // KP1
	if (GUIgetprkeys(73, 73, &al)) goto done; // KP3
#else
	if (GUIgetprkeys(72, 72, &al)) goto done;
	if (GUIgetprkeys(80, 80, &al)) goto done;
	if (GUIgetprkeys(75, 75, &al)) goto done;
	if (GUIgetprkeys(77, 77, &al)) goto done;
	if (GUIgetprkeys(71, 71, &al)) goto done;
	if (GUIgetprkeys(79, 79, &al)) goto done;
	if (GUIgetprkeys(81, 81, &al)) goto done;
	if (GUIgetprkeys(73, 73, &al)) goto done;
#endif
	if (GUIgetprkeysb(1, 27, &al)) goto done;
	if (GUIgetprkeys(0x1C, 13, &al)) goto done;

	if (UseExtKey == 72 || UseExtKey == 80) al = UseExtKey; // Extended key

#ifdef __UNIXSDL__
	if (GUIgetprkeys(90, 90, &al)) goto done; // UP
	if (GUIgetprkeys(96, 96, &al)) goto done; // DOWN
	if (GUIgetprkeys(92, 92, &al)) goto done; // LEFT
	if (GUIgetprkeys(94, 94, &al)) goto done; // RIGHT
	if (GUIgetprkeys(89, 89, &al)) goto done; // HOME
	if (GUIgetprkeys(91, 91, &al)) goto done; // PGUP
	if (GUIgetprkeys(95, 95, &al)) goto done; // END
	if (GUIgetprkeys(97, 97, &al)) goto done; // PGDOWN

	if (GUIgetprkeys(72, 72, &al)) goto done; // KP8
	if (GUIgetprkeys(80, 80, &al)) goto done; // KP2
	if (GUIgetprkeys(75, 75, &al)) goto done; // KP4
	if (GUIgetprkeys(77, 77, &al)) goto done; // KP6
	if (GUIgetprkeys(71, 71, &al)) goto done; // KP7
	if (GUIgetprkeys(79, 79, &al)) goto done; // KP9
	if (GUIgetprkeys(81, 81, &al)) goto done; // KP1
	if (GUIgetprkeys(73, 73, &al)) goto done; // KP3

	if (GUIgetprkeys(0x09C, 13, &al)) goto done; // ENTER
#endif
#ifdef __WIN32__
	//if (GUIgetprkeys(0xC8, 72, &al)) goto done;
	//if (GUIgetprkeys(0xD0, 80, &al)) goto done;
	if (GUIgetprkeys(0xCB, 75, &al)) goto done;
	if (GUIgetprkeys(0xCD, 77, &al)) goto done;
	if (GUIgetprkeys(0xC7, 71, &al)) goto done;
	if (GUIgetprkeys(0xCF, 79, &al)) goto done;
	if (GUIgetprkeys(0xD1, 81, &al)) goto done;
	if (GUIgetprkeys(0xC9, 73, &al)) goto done;
	if (GUIgetprkeys(0x9C, 13, &al)) goto done;
#endif

	if (JoyPad1Move != 0)
	{
#ifndef __WIN32__
		if (GUIgetprkeys2(pl1upk,    72, 0, &al)) goto done;
		if (GUIgetprkeys2(pl1downk,  80, 1, &al)) goto done;
#endif
		if (GUIgetprkeys2(pl1leftk,  75, 2, &al)) goto done;
		if (GUIgetprkeys2(pl1rightk, 77, 3, &al)) goto done;
		if (GUIgetprkeys2(pl1Lk,     73, 4, &al)) goto done;
		if (GUIgetprkeys2(pl1Rk,     81, 5, &al)) goto done;
		if (GUIgetprkeys2(pl1Bk,     27, 6, &al)) goto done;
		if (GUIgetprkeys2(pl1Ak,     13, 7, &al)) goto done;
	}

	if (dh == ' ') al = ' ';
	GUInextkeydelay = 10;
	GUIkeydelay     =  0;

done:
	if (GUIcmenupos == 0)
	{
		if (al == 27 && GUIwinptr == 0)
		{
			if (romloadskip == 0) GUIQuit = 2;
		}
		else
		{
			u4 const eax = GUIwinptr - 1;
			u4 const ebx = GUIwinorder[eax];
			if (al == 27 && (ebx != 13 || CheatWinMode <= 1))
			{ // Close window
				GUIwinorder[eax] = 0;
				GUIwinactiv[ebx] = 0;
				if (--GUIwinptr == 0) GUIcmenupos = GUIpmenupos;
			}
			else
			{
				eop* f;
				switch (ebx)
				{
					case  1: f = GUILoadKeys;        break;
					case  2: f = GUIStateSelKeys;    break;
					case  3: GUIInputKeys(dh);       return;
					case  4: GUIOptionKeys(dh);      return;
					case  5: GUIVideoKeys(dh, al);   return;
					case  6: GUISoundKeys(dh);       return;
					case  7: f = GUICheatKeys;       break;
					case 10: GUIGUIOptnsKeys(dh);    return;
					case 11: GUIAboutKeys(dh);       return;
					case 12: f = GUIResetKeys;       break;
					case 13: f = GUICheatSearchKeys; break;
					case 14: f = GUIStateKeys;       break;
					case 15: GUIMovieKeys(dh);       return;
					case 16: GUIComboKeys(dh);       return;
					case 17: GUIAddonKeys(dh);       return;
					case 18: GUIChipKeys(dh);        return;
					case 19: GUIPathKeys(dh);        return;
					case 20: GUISaveKeys(dh);        return;
					case 21: GUISpeedKeys(dh);       return;
					case  8: f = GUIGetInputLine;    break;
					default: return;
				}
				u4 eax = al;
				u4 edx = dh << 8 | al;
				asm volatile("call %A2" : "+a" (eax), "+d" (edx) : "g" (f) : "cc", "memory", "ecx", "ebx", "esi", "edi");
			}
		}
	}
	else
	{
		// Process menu
		dh = ToUpperASM(dh);

		switch (dh) // Main Menu Hotkeys
		{
			case 'X': GUIcmenupos = 0; GUIcrowpos = 0; break; // Close
			case 'R': GUIcmenupos = 1; GUIcrowpos = 0; break; // Recent
			case 'G': GUIcmenupos = 2; GUIcrowpos = 0; break; // Game
			case 'C': GUIcmenupos = 3; GUIcrowpos = 0; break; // Config
			case 'H': GUIcmenupos = 4; GUIcrowpos = 0; break; // Cheat
			case 'N': GUIcmenupos = 5; GUIcrowpos = 0; break; // Netplay
			case 'M': GUIcmenupos = 6; GUIcrowpos = 0; break; // Misc
		}

		if (GUIcmenupos == 1)
		{ // Recently Played Hotkeys
			switch (dh)
			{
				case '1': GUIcrowpos =  0; break;
				case '2': GUIcrowpos =  1; break;
				case '3': GUIcrowpos =  2; break;
				case '4': GUIcrowpos =  3; break;
				case '5': GUIcrowpos =  4; break;
				case '6': GUIcrowpos =  5; break;
				case '7': GUIcrowpos =  6; break;
				case '8': GUIcrowpos =  7; break;
				case '9': GUIcrowpos =  8; break;
				case '0': GUIcrowpos =  9; break;
				case 'F': GUIcrowpos = 11; break;
				case 'L': GUIcrowpos = 12; break;
			}
		}

		if (GUIcmenupos == 2)
		{ // Game Hotkeys
			switch (dh)
			{
				case 'L': GUIcrowpos = 0; break;
				case 'E': GUIcrowpos = 2; break;
				case 'S': GUIcrowpos = 4; break;
				case 'O': GUIcrowpos = 5; break;
				case 'P': GUIcrowpos = 6; break;
				case 'Q': GUIcrowpos = 8; break;
			}
		}

		if (GUIcmenupos == 3)
		{ // Config Hotkeys
			switch (dh)
			{
				case 'I': GUIcrowpos =  0; break;
				case 'D': GUIcrowpos =  2; break;
				case 'F': GUIcrowpos =  3; break;
				case 'O': GUIcrowpos =  5; break;
				case 'V': GUIcrowpos =  6; break;
				case 'S': GUIcrowpos =  7; break;
				case 'P': GUIcrowpos =  8; break;
				case 'A': GUIcrowpos =  9; break;
				case 'E': GUIcrowpos = 10; break;
			}
		}

		if (GUIcmenupos == 4)
		{ // Cheat Hotkeys
			switch (dh)
			{
				case 'A': GUIcrowpos = 0; break;
				case 'B': GUIcrowpos = 1; break;
				case 'S': GUIcrowpos = 2; break;
			}
		}

		if (GUIcmenupos == 6)
		{ // Misc Hotkeys
			switch (dh)
			{
				case 'K': GUIcrowpos = 0;
				case 'U': GUIcrowpos = 1;
				case 'O': GUIcrowpos = 2;
				case 'E': GUIcrowpos = 3;
				case 'S': GUIcrowpos = 4;
				case 'A': GUIcrowpos = 6;
			}
		}

		if (romloadskip == 0 && al == 27) GUIQuit = 2;

		if (al == 13) GUITryMenuItem();

		IFKEY(al, 92, 75)
		{
			GUIcrowpos  = 0;
			GUIcmenupos = GUIcmenupos > 1 ? GUIcmenupos - 1 : 6;
		}

		IFKEY(al, 94, 77)
		{
			GUIcrowpos  = 0;
			GUIcmenupos = GUIcmenupos != 6 ? GUIcmenupos + 1 : 1;
		}

		if (GUIcmenupos != 0)
		{
			IFKEY(al, 96, 80)
			{
				u4              eax = GUIcrowpos;
				u1 const* const ebx = GUICYLocPtr;
				if (ebx[eax + 1] == 2)
				{ // Bottom
					GUIcrowpos = 0;
				}
				else
				{
					do ++eax; while (ebx[eax + 1] == 0);
					GUIcrowpos = eax;
				}
				return;
			}

			IFKEY(al, 90, 72)
			{
				u4              eax = GUIcrowpos;
				u1 const* const ebx = GUICYLocPtr;
				if ((u1)eax == 0 || ebx[eax + 1] == 3)
				{ // Top
					GUIcrowpos = *ebx;
				}
				else
				{
					do --eax; while (ebx[eax + 1] == 0);
					GUIcrowpos = eax;
				}
				return;
			}
		}
	}
}