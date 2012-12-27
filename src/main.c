#include "main.h"
#include "render.h"
#include "colors.h"
#include<string.h>
#include<stdio.h>
#include<stdlib.h>

GPid pid=0;
GIOChannel *chout=NULL, *cherr=NULL;
static GMainLoop *main_loop = NULL;
static int function=0;
static int upgradable=0;
static int upgradeCount=0;
#define STDBUFFER_SIZE 512

gboolean out_watch(GIOChannel *source, GIOCondition condition, gpointer data)
{
	if(condition & G_IO_IN)
	{
		GString *str = g_string_new("");
		g_io_channel_read_line_string(source, str, NULL, NULL);

		switch(function)
		{
			case 1:++upgradable;
				break;
			case 2:
				if(!strncmp(str->str,"Upgrading",9) || !strncmp(str->str,"Downloading",11) || !strncmp(str->str,"Configuring",11))
				{
					++upgradeCount;
					
					int w = (596*upgradeCount)/upgradable;
					if(w>596) w=596;
					RenderBox(62,152,w+62,178,FILL,bgra[white]);
					RenderBox(62,182,658,208,FILL,bgra[gray]);
					RenderString(str->str,67,202,590,LEFT,VERY_SMALL,bgra[blue]);
				}
				break;
		}
		g_string_free(str, TRUE);
	}
	else return FALSE;
	return TRUE;
}

gboolean err_watch(GIOChannel *source, GIOCondition condition, gpointer data)
{
	if(condition & G_IO_IN)
	{
		GString *str = g_string_new("");
		g_io_channel_read_line_string(source, str, NULL, NULL);
		gDebug(LOG_ERR,ROUTINE_NAME,str->str);
		g_string_free(str, TRUE);
	}
	else return FALSE;
	return TRUE;
}

void child_watch(GPid pid, gint status, gpointer data)
{
	g_io_channel_unref(chout);
	g_io_channel_unref(cherr);
	g_spawn_close_pid(pid);
	g_main_loop_quit(main_loop);
	gDebug(LOG_INFO,ROUTINE_NAME,"done...");
}

void action(gchar **arg)
{
	int outfd;
	int errfd;

	if (g_spawn_async_with_pipes(NULL, arg, NULL, (GSpawnFlags)(G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD), NULL, NULL, &pid, NULL, &outfd, &errfd, NULL))
	{
		chout = g_io_channel_unix_new(outfd);
		cherr = g_io_channel_unix_new(errfd);
		g_io_channel_set_flags(chout, G_IO_FLAG_NONBLOCK, NULL);
		g_io_channel_set_flags(cherr, G_IO_FLAG_NONBLOCK, NULL);
			
		g_io_add_watch_full(chout, G_PRIORITY_LOW, (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_PRI | G_IO_NVAL), out_watch, NULL, NULL);
		g_io_add_watch_full(cherr, G_PRIORITY_LOW, (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_PRI | G_IO_NVAL), err_watch, NULL, NULL);
		
		g_io_channel_set_close_on_unref(chout, TRUE);
		g_io_channel_set_close_on_unref(cherr, TRUE);

		g_child_watch_add_full(G_PRIORITY_LOW, pid, child_watch, NULL, NULL);
	}
	else gDebug(LOG_ERR,ROUTINE_NAME,"create pipe");
	
	if(pid>0)
	{
		main_loop = g_main_loop_new(NULL, FALSE);
		g_main_loop_run(main_loop);
	}
}

void setcolors(unsigned short *pcolormap, int offset, int number, int bpp)
{
	int i,l,r,g,b;
	int j = offset;

	for (i = 0; i < number; i++)
	{
		r = (pcolormap[i] << 12 & 0xF000) >> 8;
		g = (pcolormap[i] << 8 & 0xF000) >> 8;
		b = (pcolormap[i] << 4 & 0xF000) >> 8;

		bgra[j][4]=j;
		bgra[j][2]=r;
		bgra[j][1]=g;
		bgra[j][0]=b;

		// fill color lines for faster boxdrawing
		for (l = 0; l < 720; l++)
		{
			colorline[j][(l*bpp)+3]=0xFF;
			colorline[j][(l*bpp)+2]=r;
			colorline[j][(l*bpp)+1]=g;
			colorline[j][(l*bpp)+0]=b;
		}
		j++;
	}
}


int main(int argc,char **argv)
{
	
	//system("cp -a /var/lib/opkg_old/* /var/lib/opkg/"); //for testing only
	system("/usr/bin/showiframe /usr/share/dreambox-bootlogo/backdrop.mvi");

	GString *tstr = g_string_new("");
	gint iarg;
	gchar **arg;
	int bpp=4;
	
	int fb=open("/dev/fb/0", O_RDWR);
	if(fb<1)
	{
		fb=open("/dev/fb0", O_RDWR);
		if(fb<1)
		{
			gDebug(LOG_ERR,ROUTINE_NAME,"Framebuffer failed");
			return;
		}
	}

	if(ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
	{
		gDebug(LOG_ERR,ROUTINE_NAME,"FBIOGET_FSCREENINFO fehlgeschlagen");
		return;
	}

	if(ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
	{
		gDebug(LOG_ERR,ROUTINE_NAME,"FBIOGET_VSCREENINFO fehlgeschlagen");
		return;
	}
	
	var_screeninfo.xres=var_screeninfo.xres_virtual=720;
	var_screeninfo.yres=var_screeninfo.yres_virtual=576;
	var_screeninfo.bits_per_pixel=8*bpp;

	if(ioctl(fb, FBIOPUT_VSCREENINFO, &var_screeninfo) == -1)
	{
		gDebug(LOG_ERR,ROUTINE_NAME,"FBIOPUT_VSCREENINFO");
		return;
	}

	if(ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
	{
		gDebug(LOG_ERR,ROUTINE_NAME,"FBIOGET_FSCREENINFO");
		return;
	}
	
	setcolors((unsigned short *)defaultcolors, 0, SIZECOLTABLE, bpp);
	
	if(!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0)))
	{
		gDebug(LOG_ERR,ROUTINE_NAME,"mapping");
		return;
	}

	if((error = FT_Init_FreeType(&library)))
	{
		gDebug(LOG_ERR,ROUTINE_NAME,"FT_Init_FreeType 0x%.2X", error);
		munmap(lfb, fix_screeninfo.smem_len);
		return;
	}

	if((error = FTC_Manager_New(library, 1, 2, 0, &MyFaceRequester, NULL, &manager)))
	{
		gDebug(LOG_ERR,ROUTINE_NAME,"FTC_Manager_New 0x%.2X", error);
		FT_Done_FreeType(library);
		munmap(lfb, fix_screeninfo.smem_len);
		return;
	}

	if((error = FTC_SBitCache_New(manager, &cache)))
	{
		gDebug(LOG_ERR,ROUTINE_NAME,"FTC_SBitCache_New 0x%.2X", error);
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		munmap(lfb, fix_screeninfo.smem_len);
		return;
	}

	if((error = FTC_Manager_Lookup_Face(manager, FONT, &face)))
	{
		gDebug(LOG_ERR,ROUTINE_NAME,"FTC_Manager_Lookup_Face 0x%.2X", error);
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		munmap(lfb, fix_screeninfo.smem_len);
		exit(-1);
	}
	else	desc.face_id = FONT;

	use_kerning = FT_HAS_KERNING(face);
	desc.flags = FT_LOAD_MONOCHROME;

	//Backbuffer initialisieren
	if(!(lbb = malloc(var_screeninfo.xres*fix_screeninfo.line_length*bpp)))
	{
		gDebug(LOG_ERR,ROUTINE_NAME,"malloc");
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		munmap(lfb, fix_screeninfo.smem_len);
		exit(-1);
	}

	// Screen leeren
	memset(lbb, 0, var_screeninfo.yres*fix_screeninfo.line_length);
	memcpy(lfb, lbb, var_screeninfo.yres*fix_screeninfo.line_length);
	
	RenderBox(60,100,660,150,GRID,bgra[white]);
	RenderBox(62,102,658,148,FILL,bgra[gray]);
	
	RenderBox(60,150,660,180,GRID,bgra[white]);
	RenderBox(62,152,658,178,FILL,bgra[gray]);

	RenderBox(60,180,660,210,GRID,bgra[white]);
	RenderBox(62,182,658,208,FILL,bgra[gray]);

	RenderString("Please wait, searching for updates...",67,135,590,LEFT,SMALL,bgra[blue]);
	
	function=1;
	iarg=0;
	arg = g_new(gchar *, 100);
	arg[iarg++] = (gchar*)"opkg";
	arg[iarg++] = (gchar*)"list-upgradable";
	arg[iarg++] = NULL;
	gDebug(LOG_INFO,ROUTINE_NAME,"Executing action:%s",arg);
	action(arg);
	g_free(arg);

	gDebug(LOG_INFO,ROUTINE_NAME,"%d updates found",upgradable);
	g_string_printf(tstr, "%d updates found",upgradable);
	RenderBox(62,102,658,148,FILL,bgra[gray]);
	RenderString(tstr->str,67,135,590,LEFT,SMALL,bgra[blue]);

	sleep(2);

	if(upgradable)
	{
		upgradable = upgradable*3;//fuer Upgrading, Downloading, Configuring //COMMENT
		//upgradable = upgradable*2;//fuer Upgrading, Downloading
	
		RenderBox(62,102,658,148,FILL,bgra[gray]);
		RenderString("Please wait, installing updates...",67,135,590,LEFT,SMALL,bgra[blue]);
	
		function=2;
		iarg=0;
		arg = g_new(gchar *, 100);
		arg[iarg++] = (gchar*)"opkg";
		arg[iarg++] = (gchar*)"--force-maintainer";
		arg[iarg++] = (gchar*)"upgrade";
		arg[iarg++] = NULL;
		action(arg);
		g_free(arg);
	
		//printf("%d\tupgradable\n",upgradable);
		//printf("%d\tcounter\n",upgradeCount);

		RenderBox(62,102,658,148,FILL,bgra[gray]);
		RenderString("done...",67,135,590,LEFT,SMALL,bgra[blue]);
		RenderBox(62,182,658,208,FILL,bgra[gray]);
		sleep(2);
	}
//--------------------------------------------------------------  OPKGFB EXTRA COMMANDS ------------------------------------
	// Screen leeren
	memset(lbb, 0, var_screeninfo.yres*fix_screeninfo.line_length);
	memcpy(lfb, lbb, var_screeninfo.yres*fix_screeninfo.line_length);
	RenderBox(60,100,660,150,GRID,bgra[white]);
	RenderBox(62,102,658,148,FILL,bgra[gray]);
	RenderBox(60,150,660,180,GRID,bgra[white]);
	RenderBox(62,152,658,178,FILL,bgra[gray]);
	RenderBox(60,180,660,210,GRID,bgra[white]);
	RenderBox(62,182,658,208,FILL,bgra[gray]);
	RenderString("Searching for additional commands...",67,135,590,LEFT,SMALL,bgra[blue]);

	FILE *fp = fopen("/tmp/opkgfb_cmds","r");
  if( fp == NULL ) {
		printf("No extra opkgfb commands found");
  }
	char line[1024];
	while(fgets(line, 1024, fp) != NULL)	{
		memset(lbb, 0, var_screeninfo.yres*fix_screeninfo.line_length);
		memcpy(lfb, lbb, var_screeninfo.yres*fix_screeninfo.line_length);
		RenderBox(60,100,660,150,GRID,bgra[white]);
		RenderBox(62,102,658,148,FILL,bgra[gray]);
		RenderBox(60,150,660,180,GRID,bgra[white]);
		RenderBox(62,152,658,178,FILL,bgra[gray]);
		RenderBox(60,180,660,210,GRID,bgra[white]);
		RenderBox(62,182,658,208,FILL,bgra[gray]);

		if (strlen(line) && line[strlen(line)-1] == '\r') {
			line[strlen(line)-1] = 0;
		}
		if (strlen(line) && line[strlen(line)-1] == '\n') {
			line[strlen(line)-1] = 0;
		}
		char command[STDBUFFER_SIZE];
		if(strstr(line, "install")) {
			RenderString(line,67,135,590,LEFT,SMALL,bgra[blue]);
			sprintf(command, "opkg %s",line);
			system(command);
		}
		if(strstr(line, "remove")) {
			RenderString(command,67,135,590,LEFT,SMALL,bgra[blue]);
			sprintf(command, "opkg --autoremove %s",line);
			system(command);
		}
		sleep(2);
	}
	fclose(fp);
//--------------------------------------------------------------  OPKGFB EXTRA COMMANDS ------------------------------------

	FTC_Manager_Done(manager);
	FT_Done_FreeType(library);

	memset(lbb, 0, var_screeninfo.yres*fix_screeninfo.line_length);
	memcpy(lfb, lbb, var_screeninfo.yres*fix_screeninfo.line_length);
	munmap(lfb, fix_screeninfo.smem_len);
	close(fb);
	
	free(lbb);
	g_string_free(tstr, TRUE);
	//system("cp -a /var/lib/opkg_old/* /var/lib/opkg/"); //for testing only
	exit(0);
}

