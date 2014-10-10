#ifndef __RENDER_H__
#define __RENDER_H__

#include <sys/syslog.h>
#include <linux/fb.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_CACHE_SMALL_BITMAPS_H

#define FONT		"/usr/share/fonts/tuxtxt.ttf"
#define ROUTINE_NAME	"opkgfb"

FT_Face face;
FTC_SBitCache cache;
FTC_ImageTypeRec desc;
FTC_SBit sbit;
FT_Bool use_kerning;
FT_UInt prev_glyphindex;
FT_Error error;
FT_Library library;
FTC_Manager manager;

enum
{
	black=0,
	red,
	green,
	yellow,
	blue,
	magenta=5,
	cyan,
	white,
	gray,
	transp,
	SIZECOLTABLE
};
unsigned char colorline[SIZECOLTABLE][720*4];

unsigned char *lfb,*lbb;

struct fb_fix_screeninfo fix_screeninfo;
struct fb_var_screeninfo var_screeninfo;


#define FONTHEIGHT_VERY_SMALL 16
#define FONTHEIGHT_SMALL      22
#define FONTHEIGHT_BIG        30
enum {FILL, GRID, LEFT=10, CENTER, RIGHT, VERY_SMALL=20, SMALL, BIG};

FT_Error MyFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface);
int RenderChar(FT_ULong currentchar, int sx, int sy, int ex, unsigned char *color);
void RenderString(const char *string, int sx, int sy, int maxwidth, int layout, int size, unsigned char *color);
void RenderBox(int sx, int sy, int ex, int ey, int mode, unsigned char *color);
void gDebug(int what, const char *where, const char* str, ...);

#endif
