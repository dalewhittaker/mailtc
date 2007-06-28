/* common.c
 * Copyright (C) 2006 Dale Whittaker <dayul@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdlib.h> /*exit*/
#include <time.h> /*asctime etc.c*/
#include <gtk/gtkimage.h>

#include "common.h"
#include "envelope_large.h"

#ifdef MTC_NOTMINIMAL
#include "envelope_small.h"
#endif /*MTC_NOTMINIMAL*/

/*function to get the date and time*/
gchar *str_time(void)
{
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo= localtime(&rawtime);

	return(asctime(timeinfo));
}

/*run an error dialog reporting error*/
gboolean err_dlg(GtkMessageType type, gchar *errmsg, ...)
{
	GtkWidget *dialog;
	va_list list;
    gsize msglen= 0;
    gchar *err_string= NULL;

    va_start(list, errmsg);
    msglen= g_printf_string_upper_bound(errmsg, list)+ 1;
    va_end(list);

    err_string= (gchar *)g_malloc0(msglen);
	
	/*create a va_list and display it as a dialog*/
    va_start(list, errmsg); 
	g_vsnprintf(err_string, msglen, errmsg, list);
	dialog= gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, type, GTK_BUTTONS_OK, err_string);
	
    /*set the title*/
    gtk_window_set_title(GTK_WINDOW(dialog), PACKAGE);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	va_end(list);
	
    g_free(err_string);

	return TRUE;
}

/*output to stderr*/
static void err_stderr(gchar *errmsg, va_list args)
{
	g_vfprintf(stderr, errmsg, args);
	fflush(stderr);
}

/*output to logfile*/
static void err_log(gchar *errmsg, va_list args)
{
	gchar *ptimestring;
	
	ptimestring= str_time();
	g_strchomp(ptimestring);
	
	if(config.logfile!= NULL)
	{
		g_fprintf(config.logfile, "%s: ", ptimestring);
		g_vfprintf(config.logfile, errmsg, args);
	
		fflush(config.logfile);
	}
}

/*function to report error, log it, and then exit*/
gboolean err_exit(gchar *errmsg, ...)
{
	/*create va_list of arguments*/
	va_list list;
	
	va_start(list, errmsg); 
	err_stderr(errmsg, list);
	va_end(list);

    /*NOTE 64-bit crashes unless va_list is reset
     *which is why this is not cleaner than this
     *someday this will be tidyed*/
    va_start(list, errmsg); 
	err_log(errmsg, list);
	va_end(list);

	exit(EXIT_FAILURE);

	return FALSE; /*shouldnt really ever happen*/
}

/*function to report error and log*/
gboolean err_noexit(gchar *errmsg, ...)
{
	/*create va_list of arguments*/
	va_list list;
	
	va_start(list, errmsg); 
	err_stderr(errmsg, list);
	va_end(list);

    /*NOTE 64-bit crashes unless va_list is reset
     *which is why this is not cleaner than this
     *someday this will be tidyed*/
    va_start(list, errmsg); 
	err_log(errmsg, list);
	va_end(list);

	return TRUE;
}

/*function to set the colour of the icon*/
static GdkPixbuf *iconcolour_set(GdkPixbuf *pixbuf, gchar *colourstring)
{
	gint width, height, rowstride, n_channels;
	guchar *pixels, *p;
	guint r, g, b;
	gint i= 0, j= 0;
	gchar shorthex[3];
	
	/*copy each RGB value to separate integers for setting the colour later*/
	/*don't really like using sscanf but i can't find how to do otherwise*/
	g_strlcpy(shorthex, colourstring, sizeof(shorthex)); 
	sscanf(shorthex, "%x", &r);
	g_strlcpy(shorthex, colourstring+ 2, sizeof(shorthex));
	sscanf(shorthex, "%x", &g);
	g_strlcpy(shorthex, colourstring+ 4, sizeof(shorthex));
	sscanf(shorthex, "%x", &b);

	/*check icon details are valid*/
	n_channels= gdk_pixbuf_get_n_channels(pixbuf);
	g_assert(gdk_pixbuf_get_colorspace(pixbuf)== GDK_COLORSPACE_RGB);
	g_assert(gdk_pixbuf_get_bits_per_sample(pixbuf)== 8);
	g_assert(gdk_pixbuf_get_has_alpha(pixbuf));
	g_assert(n_channels== 4);
		
	/*get width and height of icon*/
	width= gdk_pixbuf_get_width(pixbuf);
	height= gdk_pixbuf_get_height(pixbuf);

	/*get rowstride and pixels of icon*/
	rowstride= gdk_pixbuf_get_rowstride(pixbuf);
	pixels= gdk_pixbuf_get_pixels(pixbuf);
		
	/*for each column of icon*/
	for(i=0; i< width; i++)
	{
		/*for each row of icon*/
		for(j= 0; j< height; j++)
		{
			/*set p to the current pixel value (rgba)*/
			p= pixels+ i* rowstride+ j* n_channels;

			/*if pixel is white set the rgb for it and set transparency off*/
			if((p[0]== 0xff)&& (p[1]== 0xff)&& (p[2]== 0xff)&& (p[3]== 0xff))
			{
				p[0]= r;
				p[1]= g;
				p[2]= b;
				p[3]= 0xff;
			}
		}
	}
    return(pixbuf);
}

/*function to create the pixbuf used for the icon*/
mtc_icon *pixbuf_create(mtc_icon *picon)
{
	GdkPixbuf *unscaled;
    const guint8 *penvelope;
    gint iconsize;
    gchar *pcolour= NULL;
#ifdef MTC_EGGTRAYICON
    GdkPixbuf *scaled;
#endif /*MTC_EGGTRAYICON*/

#ifdef MTC_NOTMINIMAL
    if(config.isdlg|| config.icon_size!= 16)
    {
        penvelope= envelope_large;
        iconsize= 24;
    }
    else
    {
        penvelope= envelope_small;
        iconsize= 16;
	}
#else
    penvelope= envelope_large;
    iconsize= 24;
#endif /*MTC_NOTMINIMAL*/

	/*get the pixbuf from the icon file*/
	unscaled= gdk_pixbuf_new_from_inline(-1, penvelope, FALSE, NULL);
        
	if(!unscaled)
		return(NULL);

#ifdef MTC_EGGTRAYICON
	/*if it is valid scale it and set the colour*/
	scaled= gdk_pixbuf_scale_simple(unscaled, iconsize, iconsize, GDK_INTERP_BILINEAR);
    pcolour= picon->colour;
	scaled= iconcolour_set(scaled, pcolour+ 1);
	gtk_image_set_from_pixbuf(GTK_IMAGE(picon->image), scaled);
	g_object_unref(G_OBJECT(scaled));
#else
	picon->pixbuf= gdk_pixbuf_scale_simple(unscaled, iconsize, iconsize, GDK_INTERP_BILINEAR);
    pcolour= picon->colour;
	picon->pixbuf= iconcolour_set(picon->pixbuf, pcolour+ 1);
	gtk_image_set_from_pixbuf(GTK_IMAGE(picon->image), picon->pixbuf);
#endif /*MTC_EGGTRAYICON*/

    /*cleanup*/
	g_object_unref(G_OBJECT(unscaled));

	return(picon);
}

/*creates an icon*/
mtc_icon *icon_create(mtc_icon *picon)
{
    picon->image= gtk_image_new();
	picon= pixbuf_create(picon);
    if(!picon|| !picon->image) 
		err_exit(S_FILEFUNC_ERR_CREATE_PIXBUF);
    
    /*ref count the image, be sure to unref it when we leave*/
    g_object_ref(G_OBJECT(picon->image));

    return(picon);
}
