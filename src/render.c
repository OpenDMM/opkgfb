#include "render.h"

FT_Error MyFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface)
{
	FT_Error result = FT_New_Face(library, face_id, 0, aface);
	if(!result) gDebug(LOG_INFO,ROUTINE_NAME,"Font '%s' loaded", (char*)face_id);
	else        gDebug(LOG_ERR,ROUTINE_NAME,"Font '%s' not loaded", (char*)face_id);
	return result;
}

int RenderChar(FT_ULong currentchar, int sx, int sy, int ex, unsigned char *color)
{

	int bpp=4;
	int row, pitch, bit, x = 0, y = 0;
	FT_UInt glyphindex;
	FT_Vector kerning;
	FT_Error error;
	int tmpcolor;

	if(!(glyphindex = FT_Get_Char_Index(face, currentchar)))
	{
		gDebug(LOG_ERR,ROUTINE_NAME,"FT_Get_Char_Index %x \"%c\"", (int)currentchar,(int)currentchar);
		return 0;
	}

	FTC_Node anode;
	if((error = FTC_SBitCache_Lookup(cache, &desc, glyphindex, &sbit, &anode)))
	{
		gDebug(LOG_ERR,ROUTINE_NAME,"FTC_SBitCache_Lookup %x \"%c\"", (int)currentchar,(int)currentchar);
		return 0;
	}
	if(use_kerning)
	{
		FT_Get_Kerning(face, prev_glyphindex, glyphindex, ft_kerning_default, &kerning);
		prev_glyphindex = glyphindex;
		kerning.x >>= 6;
	}
	else
		kerning.x = 0;

	if(sx + sbit->xadvance >= ex)
		return -1;
	for(row = 0; row < sbit->height; row++)
	{
		for(pitch = 0; pitch < sbit->pitch; pitch++)
		{
			for(bit = 7; bit >= 0; bit--)
			{
				if(pitch*8 + 7-bit >= sbit->width) break; /* render needed bits only */
				if((sbit->buffer[row * sbit->pitch + pitch]) & 1<<bit)
				{
					memcpy(lfb + sx*bpp + (sbit->left + kerning.x + x)*bpp + fix_screeninfo.line_length*(sy - sbit->top + y), color, bpp);
				}
				x++;
			}
		}
		x = 0;
		y++;
	}
	return sbit->xadvance + kerning.x;
}

int GetStringLen(const char *string, int size)
{
	int stringlen = 0;

	switch (size)
	{
		case VERY_SMALL: desc.width = desc.height = FONTHEIGHT_VERY_SMALL; break;
		case SMALL     : desc.width = desc.height = FONTHEIGHT_SMALL     ; break;
		case BIG       : desc.width = desc.height = FONTHEIGHT_BIG       ; break;
	}
	prev_glyphindex = 0;
	while(*string != '\0')
	{
		stringlen += RenderChar(*string, -1, -1, -1, "");
		string++;
	}
	return stringlen;
}


void RenderString(const char *string, int sx, int sy, int maxwidth, int layout, int size, unsigned char *color)
{
	int stringlen, ex, charwidth;
	switch (size)
	{
		case VERY_SMALL: desc.width = desc.height = FONTHEIGHT_VERY_SMALL; break;
		case SMALL     : desc.width = desc.height = FONTHEIGHT_SMALL     ; break;
		case BIG       : desc.width = desc.height = FONTHEIGHT_BIG       ; break;
	}
	if(layout != LEFT)
	{
		stringlen = GetStringLen(string, size);
		switch(layout)
		{
			case CENTER:	if(stringlen < maxwidth) sx += (maxwidth - stringlen)/2;
					break;
			case RIGHT:	if(stringlen < maxwidth) sx += maxwidth - stringlen;
					break;
		}
	}
	prev_glyphindex = 0;
	ex = sx + maxwidth;
	while(*string != '\0' && *string != '\n')
	{
		if((charwidth = RenderChar(*string, sx, sy, ex, color)) == -1) return; // string > maxwidth
		sx += charwidth;
		string++;
	}
}

void RenderBox(int sx, int sy, int ex, int ey, int mode, unsigned char *color)
{
	int bpp=4;
	int loop;
	int tx;
	if(mode == FILL)
	{
		for(; sy <= ey; sy++)
			memcpy(lfb + sx*bpp + fix_screeninfo.line_length*sy,colorline[color[bpp]],bpp*(ex-sx+1));
	}
	else
	{
		for(loop = sx; loop <= ex; loop++)
		{
			memcpy(lfb + loop*bpp + fix_screeninfo.line_length*sy, colorline[color[bpp]], bpp);
			memcpy(lfb + loop*bpp + fix_screeninfo.line_length*(sy+1), colorline[color[bpp]], bpp);
			memcpy(lfb + loop*bpp + fix_screeninfo.line_length*(ey-1), colorline[color[bpp]], bpp);
			memcpy(lfb + loop*bpp + fix_screeninfo.line_length*(sy+2), colorline[color[bpp]], bpp);
			memcpy(lfb + loop*bpp + fix_screeninfo.line_length*(ey-2), colorline[color[bpp]], bpp);
			memcpy(lfb + loop*bpp + fix_screeninfo.line_length*ey, colorline[color[bpp]], bpp);
		}
		for(loop = sy; loop <= ey; loop++)
		{
			memcpy(lfb + sx*bpp + fix_screeninfo.line_length*loop, colorline[color[bpp]], bpp);
			memcpy(lfb + (sx+1)*bpp + fix_screeninfo.line_length*loop, colorline[color[bpp]], bpp);
			memcpy(lfb + (ex-1)*bpp + fix_screeninfo.line_length*loop, colorline[color[bpp]], bpp);
			memcpy(lfb + (sx+2)*bpp + fix_screeninfo.line_length*loop, colorline[color[bpp]], bpp);
			memcpy(lfb + (ex-2)*bpp + fix_screeninfo.line_length*loop, colorline[color[bpp]], bpp);
			memcpy(lfb + ex*bpp + fix_screeninfo.line_length*loop, colorline[color[bpp]], bpp);
		}
	}
}

void gDebug(int what, const char *where, const char* str, ...)
{
	char buf[1024], print_buf[1024];
	va_list ap;
	va_start(ap, str);
	vsnprintf(buf, 1023, str, ap);
	va_end(ap);

	switch(what)
	{
		case LOG_ERR:	snprintf(print_buf, sizeof(print_buf), "[%s] ERROR <%s>",where, buf);	break;
		default:	snprintf(print_buf, sizeof(print_buf), "[%s] %s",where, buf);	break;
	}

	printf("%s\n",print_buf);
	syslog(LOG_DAEMON | what, print_buf);
}
