/* docklet.c
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

#include "core.h"

static EggTrayIcon *docklet= NULL; /*the docklet used to add the icon to the sys tray*/ 
static GtkWidget *box= NULL; /*the box to hold the icon, made static to make new mails smoother*/
static GtkTooltips *tooltip= NULL; /*the tooltip to display the number of messages*/
static int lock= 0; /*lock variable used so that only one account can be checked at a time*/

/*function to get the active account*/
static mail_details* get_active_account(void)
{
	mail_details *pcurrent_data= NULL;
	GSList *pcurrent= acclist;

	/*iterate through them all until it is found*/
	while(pcurrent!= NULL)
	{
		pcurrent_data= (mail_details *)pcurrent->data;

		if(pcurrent_data->active)
			return(pcurrent_data);
		
		pcurrent= g_slist_next(pcurrent);
	}
	return(NULL);
}

/*function to run the mail application when the icon is clicked*/
static int run_mailapp(char *mailapp)
{
	GError *spawn_error= NULL;
	char appstr[NAME_MAX* 2], acc_str[G_ASCII_DTOSTR_BUF_SIZE+ 1], *c= "\n";
	unsigned int i= 0, retval= 1, acount= 0;
	GSList *pcurrent= acclist;
	mail_details *pcurrent_data= NULL;
	gchar **args= NULL;

	memset(appstr, '\0', NAME_MAX+ 1);
	
	/*This is very ugly, but i think works*/
	/*find all '$', which means the arg and replace with the actual arg*/
	/*TODO if we were ever to port to other platforms we would need to do something here with G_DIR_SEPARATOR*/
	for(i= 0; i< strlen(mailapp); i++)
	{
		if(mailapp[i]== '$')
		{
			/*if the previous is not '\' convert, otherwise just add the '$'*/
			if((i== 0)|| ((i> 0)&& (mailapp[i- 1]!= '\\')))
			{
				/*it is a valid '$' so now we convert each account with new messages to a value*/
				acount= 0;
				while(pcurrent!= NULL)
				{
					pcurrent_data= (mail_details *)pcurrent->data;
					if(pcurrent_data->num_messages> 0)
					{
						/*this will add each one as a new arg, if you don't want that, tough shit, sorry*/
						g_snprintf(acc_str, sizeof(acc_str), "%s%d", (acount++)? c: "", pcurrent_data->id);
						g_strlcat(appstr, acc_str, sizeof(appstr)); 
					}
					pcurrent= g_slist_next(pcurrent);
				}
			}
			else
				appstr[strlen(appstr)]= '$';
		}
		else if(mailapp[i]== ' ')
		{
			/*it is a valid new arg, so count it and convert to 0x0D (non printable char)*/
			if((i== 0)|| ((i> 0)&& (mailapp[i- 1]!= '\\')&& (mailapp[i- 1]!= ' ')))
				g_strlcat(appstr, c, sizeof(appstr));
			/*it is a space in filename, so strcat normal*/
			else
				g_strlcat(appstr, " ", sizeof(appstr));
		}
		/*default is to just add the char*/
		else if(mailapp[i]!= '\\')
			appstr[strlen(appstr)]= *(mailapp+ i);
	}
	
	/*split the string into its args*/
	args= g_strsplit(appstr, c, 0);
	
	/*Run the mail application and report an error if it does not work*/
	if(!g_spawn_async(NULL, args, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &spawn_error))
	{
		error_and_log_no_exit("%s\n", spawn_error->message);
		run_error_dialog(spawn_error->message);
		if(spawn_error) g_error_free(spawn_error);
		retval= 0;
	}
	
	/*free the argument vector*/
	g_strfreev(args);
	return(retval);
}

/*function to destroy the icon*/
static void docklet_destroy(void)
{
	/*destroy the tooltip and box first*/
	if(tooltip)
	{
		gtk_object_destroy(GTK_OBJECT(tooltip));
		tooltip= NULL;
	}
	if(box)
	{
		gtk_widget_destroy(box);
		box= NULL;
	}
	/*destroy the widget then unref the docklet*/
	if(docklet)
	{
		gtk_widget_destroy(GTK_WIDGET(docklet)); 
		g_object_unref(G_OBJECT(docklet));
		docklet= NULL;
	}

}

/*function to read the messages by calling the plugin function*/
static void read_messages(mail_details *paccount)
{
	mtc_plugin_info *pitem= NULL;

	/*find the correct pluin to handle the click*/
	if((pitem= find_plugin(paccount->plgname))== NULL)
	{
		run_error_dialog(S_DOCKLET_ERR_FIND_PLUGIN_MSG, paccount->plgname, paccount->accname);
			
		/*this should not happen, so we exit (if it is found in the main mail read thread it should be found here)*/
		error_and_log(S_DOCKLET_ERR_FIND_PLUGIN, paccount->plgname);

	}
	/*call the plugin 'clicked' function to handle mail reading*/
	if(paccount->num_messages> 0)
		if((*pitem->clicked)(paccount, config.base_name)== 0)
			exit(EXIT_FAILURE);
			
}

/*function that is called when the icon is clicked*/
static void docklet_clicked(GtkWidget *button, GdkEventButton *event)
{	

	/*check if the mouse button that was pressed was either left double click or right single click*/
	if((event->type== GDK_2BUTTON_PRESS)||((event->type== GDK_BUTTON_PRESS)&&(event->button== 3)))
	{	
		GSList *pcurrent= acclist;
		mail_details *pcurrent_data= NULL;
		
		/*run mailapp if left double click*/
		if(event->type== GDK_2BUTTON_PRESS) 
			if(!run_mailapp(config.mail_program))
				return;
		
		/*Mark all accounts as read*/
		if(config.multiple)
		{
			while(pcurrent!= NULL)
			{
				pcurrent_data= (mail_details *)pcurrent->data;
				read_messages(pcurrent_data);
				pcurrent= g_slist_next(pcurrent);
			}
		}
		/*Only mark active account as read*/
		else
		{
			if((pcurrent_data= get_active_account())== NULL)
				return;

			read_messages(pcurrent_data);
		}
		
		/*destroy the icon if it exists*/
		if((pcurrent_data= get_active_account())!= NULL)
			pcurrent_data->active= 0;
		
		if(docklet!= NULL) docklet_destroy();

	}	
}

/*function to set the colour of the icon*/
void set_icon_colour(GdkPixbuf *pixbuf, char *colourstring)
{
	int width, height, rowstride, n_channels;
	guchar *pixels, *p;
	unsigned int r, g, b;
	int i= 0, j= 0;
	char shorthex[3];
	
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
}

/*function to set the 'number of messages' tooltip text*/
static void set_icon_text()
{
	
	GString *tipstring= NULL;
	char tmpstring[NAME_MAX+ 30];
	int first= 1;
	GSList *pcurrent= acclist;
	mail_details *pcurrent_data= NULL;
	
	memset(tmpstring, 0, NAME_MAX+ 30);
	
	/*if already a tooltip, destroy it*/
	if(tooltip)
	{
		gtk_object_destroy(GTK_OBJECT(tooltip));
		tooltip= NULL;
	}
	
	/*create tooltip and display it*/
	tooltip= gtk_tooltips_new();
	tipstring= g_string_new(NULL);

	/*create the summary of the accounts for the tooltip*/
	while(pcurrent!= NULL)
	{
		pcurrent_data= (mail_details *)pcurrent->data;
		if((config.multiple|| ((!config.multiple)&& pcurrent_data->active))&&
			(pcurrent_data->num_messages> 0))
		{
			/*if there are messages add it to summary*/
			g_snprintf(tmpstring, sizeof(tmpstring), (pcurrent_data->num_messages> 1)? S_DOCKLET_NEW_MESSAGES: S_DOCKLET_NEW_MESSAGE,
				pcurrent_data->accname, pcurrent_data->num_messages, (first)? "": "\n");
			
			/*insert at the start (to match list order)*/
			tipstring= g_string_prepend(tipstring, tmpstring);

			first= 0;
		}
		pcurrent= g_slist_next(pcurrent);
	}
	
	/*set the text*/	
	gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltip), box, tipstring->str, tipstring->str);
	
	g_string_free(tipstring, TRUE);
}

/*function to create the icon*/
static void docklet_create(mail_details *paccount)
{
	GtkWidget *icon;
	GdkPixbuf *unscaled;
	
	/*unreference the docklet if it is referenced*/
	if(docklet)
	{
		g_object_unref(G_OBJECT(docklet));
		docklet= NULL;
	}
	/*initialise widgets*/
	docklet= egg_tray_icon_new(PACKAGE);
	box= gtk_event_box_new();
	icon= gtk_image_new();

	/*set the signal for when icon is clicked*/
	g_signal_connect(G_OBJECT(box), "button-press-event", G_CALLBACK(docklet_clicked), NULL);

	/*add the icon to the container and show it*/
	gtk_container_add(GTK_CONTAINER(box), icon);
	gtk_container_add(GTK_CONTAINER(docklet), box);
	gtk_widget_show_all(GTK_WIDGET(docklet));
	
	/*reference the docklet*/
	g_object_ref(G_OBJECT(docklet));
		
	/*set the pixbuf to the icon (or small icon)*/
	if(config.icon_size== 16)
		unscaled= gdk_pixbuf_new_from_inline(-1, envelope_small, FALSE, NULL);
	else
		unscaled= gdk_pixbuf_new_from_inline(-1, envelope_white, FALSE, NULL);
		
	/*if it is valid*/
	if(unscaled)
	{
		GdkPixbuf *scaled;
		
		/*create the tooltip*/
		char *pcolour= (paccount)? paccount->icon: config.icon;
		pcolour++;
		
		/*scale the pixbuf and copy to new pixbuf*/
		scaled= gdk_pixbuf_scale_simple(unscaled, config.icon_size, config.icon_size, GDK_INTERP_BILINEAR);
		
		/*set the icon colour set the tooltip*/
		set_icon_colour(scaled, pcolour);
		gtk_image_set_from_pixbuf(GTK_IMAGE(icon), scaled);
		
		g_object_unref(unscaled);
		g_object_unref(scaled);
	}
	
}

/*Function to get the icon status
  will return none, multi, or the account depending on what is set*/
static int get_icon_status(void)
{
	/*we need to report different icon statuses*/
	GSList *pcurrent= acclist;
	mail_details *pcurrent_data= NULL;
	mail_details *plast_data= NULL;
	unsigned int multi= 0;
	
	while(pcurrent!= NULL)
	{
		pcurrent_data= (mail_details *)pcurrent->data;

		/*increment if multiple option is selected*/
		if(config.multiple&& (pcurrent_data->num_messages> 0))
			multi++;

		/*return multiple icon status if more than one*/
		if(multi> 1)
			return(ACTIVE_ICON_MULTI);
	
		/*set the last pointer to return if needed*/
		if(pcurrent_data->num_messages> 0)
			plast_data= &(*pcurrent_data);
		
		pcurrent= g_slist_next(pcurrent);
	}
	
	return((plast_data== NULL)? ACTIVE_ICON_NONE: (int)plast_data->id);
}

/*the main thread that is called to check the various mail accounts*/
gboolean mail_thread(gpointer data)
{
	/*if no thread is running*/
	if(!lock)
	{
		unsigned int errflag= 0;
		GString *err_msg= NULL;
		int retval= 0;
		mtc_plugin_info *pitem= NULL;
		GSList *pcurrent= acclist;
		mail_details *pcurrent_data= NULL;
	
		/*get the previous status of the icon*/
		int status= ACTIVE_ICON_NONE;
		int prev_status= get_icon_status();
	
		/*lock the thread*/
		lock= 1;

		/*go through each account*/
		while(pcurrent!= NULL)
		{	
			pcurrent_data= (mail_details *)pcurrent->data;

			/*initialise the array values to -1*/
			pcurrent_data->num_messages= -1;
			
			/*search for the plugin, if it is not found, report and error*/
			if((pitem= find_plugin(pcurrent_data->plgname))== NULL)
			{
				error_and_log_no_exit(S_DOCKLET_ERR_FIND_PLUGIN, pcurrent_data->plgname);
				run_error_dialog(S_DOCKLET_ERR_FIND_PLUGIN_MSG,
					pcurrent_data->plgname, pcurrent_data->accname);

				/*now go to the next account*/
				pcurrent= g_slist_next(pcurrent);
				continue;
			}

			/*use the plugin to check the mail and get number of messages*/
			retval= (*pitem->get_messages)
				(pcurrent_data, config.base_name, config.logfile, (config.net_debug)? MTC_DEBUG_MODE: 0);
			
			/*if there was a connection error*/
			if((retval== MTC_ERR_CONNECT)|| (pcurrent_data->num_messages== MTC_ERR_CONNECT))
			{
				if(err_msg== NULL)
					err_msg= g_string_new(NULL);
				
				err_msg= g_string_prepend(err_msg, "\n");
				err_msg= g_string_prepend(err_msg, pcurrent_data->hostname);
				errflag++;
			}
			/*if there was a bad error (i.e we must exit)*/
			if((retval== MTC_ERR_EXIT)|| (pcurrent_data->num_messages== MTC_ERR_EXIT))
			{
				/*extremely unlikely this will be allocated, but just to be safe*/
				if(err_msg!= NULL)
					g_string_free(err_msg, TRUE);

				/*message has (should have) already been reported by plugin, so exit*/
				exit(EXIT_FAILURE);
			}
			pcurrent= g_slist_next(pcurrent);
		}
		
		/*report if checking an account failed*/
		if(errflag)
		{
			run_error_dialog(S_DOCKLET_CONNECT_ERR, err_msg->str, PACKAGE);
			if(err_msg!= NULL)
				g_string_free(err_msg, TRUE);
			
		}
		status= get_icon_status();
			
		if(status!= prev_status)
		{
			/*we always destroy if the statuses are not equal*/
			if(docklet!= NULL) docklet_destroy();

			/*if there is an active icon, set it to 0*/
			if((pcurrent_data= get_active_account())!= NULL)
				pcurrent_data->active= 0;
			
			/*now add our icon*/
			/*add the multiple icon if it is set*/
			if(status== ACTIVE_ICON_MULTI)
				docklet_create(NULL);

			/*otherwise add the active one*/
			else if(status!= ACTIVE_ICON_NONE)
			{
				pcurrent_data= get_account(status);
				docklet_create(pcurrent_data);
				pcurrent_data->active= 1;
			}
		}
		/*this means that the same is active as the last*/
		else
		{
			/*no docklet exists, this almost certainly means the docklet was clicked*/
			if((status!= ACTIVE_ICON_NONE)&& (docklet== NULL))
			{
				/*if there is an active account, get rid of it*/
				if((pcurrent_data= get_active_account())!= NULL)
					pcurrent_data->active= 0;
				
				/*get/set the account*/
				pcurrent_data= get_account(status);
				docklet_create(pcurrent_data);
				pcurrent_data->active= 1;
			}

		}

		/*finally, set the text summary for the accounts*/
		if(docklet!= NULL)
			set_icon_text();
		
		/*unlock the thread*/
		lock=0;

	}
	
	return(TRUE);
}
