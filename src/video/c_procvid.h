#ifndef C_PROCVID_H
#define C_PROCVID_H

#include "../types.h"

// Processes & displays video
void showvideo(void);

u4 SwapMouseButtons(u4 buttons);

void processmouse1(void);
void processmouse2(void);

#ifdef __MSDOS__
// Outputs val at buf
void outputhex(u1* buf, u1 val);
#endif

void outputhex16(u2* buf, u1 val);

#ifdef __MSDOS__
// Outputs char glyph at buf
void outputchar(u1* buf, u1 glyph);
#endif

void outputchar16b(u2* buf, u1 glyph);

#ifdef __MSDOS__
void outputchar5x5(u1* buf, u1 glyph);
#endif

void outputchar16b5x5(u2* buf, u1 glyph);

// Outputs String from text to buf
void OutputGraphicString(u1* buf, char const* text);

void OutputGraphicString16b(u2* buf, char const* text);

#ifdef __MSDOS__
void OutputGraphicString5x5(u1* buf, char const* text);
#endif

void OutputGraphicString16b5x5(u2* buf, char const* text);

#ifdef __MSDOS__
void drawhline(u1* buf, u4 n, u1 colour);
#endif

void drawhline16b(u2* buf, u4 n, u2 colour);

#ifdef __MSDOS__
void drawvline(u1* buf, u4 n, u1 colour);
#endif

void drawvline16b(u2* buf, u4 n, u2 colour);

void DetermineNewest(void);

void GetPicture(void);

// draws a 10x10 filled box according to position x
#ifdef __MSDOS__
void drawfillboxsc(u4 x);
#endif
void drawfillboxsc16b(u4 x);

// draws a box according to position x and color colour
#ifdef __MSDOS__
void drawbox(u1 x, u1 colour);
#endif
void drawbox16b(u1 x, u2 colour);

#endif
