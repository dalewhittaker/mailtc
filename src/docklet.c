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

static gboolean lock= FALSE; /*lock variable used so that only one account can be checked at a time*/

/*gets the containers icon if there is any present*/
#ifdef MTC_EGGTRAYICON
static GtkWidget *docklet_icon(void)
{
    GList *pchild;
    mtc_trayicon *ptrayicon;

    ptrayicon= &config.trayicon;

    pchild= gtk_container_get_children(GTK_CONTAINER(ptrayicon->box));
    
    /*nothing there*/
    if(pchild== NULL|| pchild->data== NULL)
        return(NULL);
    else
    {
        GtkWidget *pimage;
        pimage= (GtkWidget *)pchild->data;
        return(pimage);
    }
}
#else
static mtc_icon *docklet_icon(gint active)
{
    /*no icon*/
    if(active== ACTIVE_ICON_NONE)
        return(NULL);

    /*multi icon*/
    if(active== ACTIVE_ICON_MULTI)
        return(&config.icon);

    else
    {
        mtc_account *paccount;
        
        paccount= get_account((guint)active);
        if(paccount== NULL)
            return(NULL);
        else
            return(&paccount->icon);
	}
}
#endif /*MTC_EGGTRAYICON*/

/*function to get the active account (i.e the one that is shown in the docklet)*/
static mtc_account* get_active_account(void)
{
	mtc_account *pcurrent_data= NULL;
	GSList *pcurrent= acclist;
#ifdef MTC_EGGTRAYICON
    GtkWidget *pdockimage= NULL;
    mtc_icon *picon= NULL;

    /*get the icon that is shown in the docklet if any*/
    pdockimage= docklet_icon();
    if(pdockimage== NULL)
        return(NULL);
#else
    mtc_trayicon *ptrayicon;

    ptrayicon= &config.trayicon;
#endif /*MTC_EGGTRAYICON*/

    /*iterate through them all until it is found*/
	while(pcurrent!= NULL)
	{
		pcurrent_data= (mtc_account *)pcurrent->data;
#ifdef MTC_EGGTRAYICON
        picon= (mtc_icon *)&pcurrent_data->icon;

        if((picon!= NULL)&& (picon->image== pdockimage))
			return(pcurrent_data);
#else
        if(pcurrent_data->id== (guint)ptrayicon->active)
            return(pcurrent_data);
#endif /*MTC_EGGTRAYICON*/

		pcurrent= g_slist_next(pcurrent);
	}
	return(NULL);
}

/*function to run the mail application when the icon is clicked*/
#ifdef MTC_NOTMINIMAL
static gboolean run_cmd(gchar *mailapp, gboolean newmail)
#else
static gboolean run_cmd(gchar *mailapp)
#endif /*MTC_NOTMINIMAL*/
{
	GError *spawn_error= NULL;
	gchar appstr[NAME_MAX* 2], acc_str[G_ASCII_DTOSTR_BUF_SIZE+ 1], *c= "\n";
	guint i= 0, acount= 0;
	GSList *pcurrent= acclist;
	mtc_account *pcurrent_data= NULL;
	gchar **args= NULL;
    gboolean retval= TRUE;

	memset(appstr, '\0', NAME_MAX+ 1);
	
	/*This is very ugly, but i think works*/
	/*find all '$', which means the arg and replace with the actual arg*/
	for(i= 0; i< strlen(mailapp); i++)
	{
		if(mailapp[i]== '$')
		{
			/*if the previous is not '\' convert, otherwise just add the '$'*/
			if((i== 0)|| ((i> 0)&& (mailapp[i- 1]!= PATH_DELIM)))
			{
				/*it is a valid '$' so now we convert each account with new messages to a value*/
				acount= 0;
				while(pcurrent!= NULL)
				{
					pcurrent_data= (mtc_account *)pcurrent->data;

#ifdef MTC_NOTMINIMAL
                    if((!newmail && pcurrent_data->msginfo.num_messages> 0)||
                        (newmail && pcurrent_data->msginfo.new_messages> 0))
#else
                    if(pcurrent_data->num_messages> 0)
#endif /*MTC_NOTMINIMAL*/
                    {
						/*this will add each one as a new arg, if you don't want that, tough shit, sorry*/
                        /*TODO all kinds of extra info could be made available here, maybe someday i'll add them*/
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
			if((i== 0)|| ((i> 0)&& (mailapp[i- 1]!= PATH_DELIM)&& (mailapp[i- 1]!= ' ')))
				g_strlcat(appstr, c, sizeof(appstr));
			/*it is a space in filename, so strcat normal*/
			else
				g_strlcat(appstr, " ", sizeof(appstr));
		}
		/*default is to just add the char*/
		else if(mailapp[i]!= PATH_DELIM)
			appstr[strlen(appstr)]= *(mailapp+ i);
	}
	
	/*split the string into its args*/
	args= g_strsplit(appstr, c, 0);
	
	/*Run the mail application and report an error if it does not work*/
	if(!g_spawn_async(NULL, args, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &spawn_error))
	{
		err_noexit("%s\n", spawn_error->message);
		err_dlg(spawn_error->message);
		if(spawn_error) g_error_free(spawn_error);
		retval= FALSE;
	}
	
	/*free the argument vector*/
	g_strfreev(args);
	return(retval);
}

/*function to destroy the icon*/
#ifdef MTC_EGGTRAYICON
static void docklet_remove(GtkWidget *pimage)
#else
static void docklet_remove()
#endif /*MTC_EGGTRAYICON*/
{
    mtc_trayicon *ptrayicon;
    
    ptrayicon= &config.trayicon;

#ifdef MTC_EXPERIMENTAL
    if(config.run_summary)
        sumdlg_hide();
#endif

    /*remove any existing widget
     *we don't need to check if it has children,
     *because this has already been done in mail_thread*/
#ifdef MTC_EGGTRAYICON
    gtk_container_remove(GTK_CONTAINER(ptrayicon->docklet), ptrayicon->box);
    
    /*destroy the widget then unref the docklet*/
	if(ptrayicon->docklet)
	{
		g_object_unref(G_OBJECT(ptrayicon->docklet));
        g_signal_handlers_disconnect_by_func(G_OBJECT(ptrayicon->docklet), G_CALLBACK(docklet_destroyed), NULL);
		gtk_widget_destroy(GTK_WIDGET(ptrayicon->docklet)); 
		ptrayicon->docklet= NULL;
	}

    gtk_container_remove(GTK_CONTAINER(ptrayicon->box), pimage);
#else
    if(gtk_status_icon_get_visible(ptrayicon->docklet))
        gtk_status_icon_set_visible(ptrayicon->docklet, FALSE);

    ptrayicon->active= ACTIVE_ICON_NONE;
#endif /*MTC_EGGTRAYICON*/

}

/*function to read the messages by calling the plugin function*/
static void docklet_read(mtc_account *paccount, gboolean exitflag)
{
	mtc_plugin *pitem= NULL;

	/*find the correct pluin to handle the click*/
	if((pitem= plg_find(paccount->plgname))== NULL)
	{
		err_dlg(S_DOCKLET_ERR_FIND_PLUGIN_MSG, paccount->plgname, paccount->accname);
		err_noexit(S_DOCKLET_ERR_FIND_PLUGIN, paccount->plgname);
		
		/*this should not happen for single mode; if it is active, it should exist*/
		if(exitflag)
			exit(EXIT_FAILURE);

	}

	/*call the plugin 'clicked' function to handle mail reading*/
#ifdef MTC_NOTMINIMAL
	if(paccount->msginfo.num_messages> 0)
#else
    if(paccount->num_messages> 0)
#endif /*MTC_NOTMINIMAL*/
		if((*pitem->clicked)(paccount)!= MTC_RETURN_TRUE)
			exit(EXIT_FAILURE);
			
}

/*generic click handler for the docklet*/
static void docklet_click_handler(gboolean leftclick)
{
	GSList *pcurrent= acclist;
	mtc_account *pcurrent_data= NULL;
#ifdef MTC_EGGTRAYICON
    GtkWidget *pdockimage= NULL;
#endif /*MTC_EGGTRAYICON*/

    /*run mailapp if left double click*/
	if(leftclick) 
#ifdef MTC_NOTMINIMAL
		if(*config.mail_program && !run_cmd(config.mail_program, FALSE))
#else
		if(*config.mail_program && !run_cmd(config.mail_program))
#endif /*MTC_NOTMINIMAL*/
				return;

	/*Mark all accounts as read*/
	if(config.multiple)
	{
		while(pcurrent!= NULL)
		{
			pcurrent_data= (mtc_account *)pcurrent->data;
			docklet_read(pcurrent_data, FALSE);
			pcurrent= g_slist_next(pcurrent);
		}
	}
	/*Only mark active account as read*/
	else
	{
		if((pcurrent_data= get_active_account())== NULL)
			return;
           
		docklet_read(pcurrent_data, TRUE);
	}
		
    /*remove the icon from the docklet*/
#ifdef MTC_EGGTRAYICON
    pdockimage= docklet_icon();
    if(pdockimage!= NULL)
        docklet_remove(pdockimage);
#else
    docklet_remove();
#endif /*MTC_EGGTRAYICON*/
	
}

#ifdef MTC_EGGTRAYICON
/*function that is called when the icon is clicked*/
void docklet_clicked(GtkWidget *button, GdkEventButton *event)
{	
    guint modifiers;

    modifiers= gtk_accelerator_get_default_mod_mask();

	/*check if the mouse button that was pressed was either left double click or right single click*/
	if((event->type== GDK_2BUTTON_PRESS)||
        ((event->type== GDK_BUTTON_PRESS)&& (event->button== 3))||
        ((event->type== GDK_BUTTON_PRESS)&& (event->button== 1)&& ((event->state& modifiers)== GDK_CONTROL_MASK)))
	{	
        /*handler to show summary dialog on left click plus CTRL*/
#ifdef MTC_EXPERIMENTAL
        if((event->type== GDK_BUTTON_PRESS)&& (event->button== 1)&& ((event->state& modifiers)== GDK_CONTROL_MASK))
        {
            if(config.run_summary)
                sumdlg_show();
            return;
        }
#endif
        /*call the handler to read, and run the mail app if it is a two button click*/
		docklet_click_handler((event->type== GDK_2BUTTON_PRESS));
	}	
}
#else
/*called when the GtkStatusIcon is right clicked*/
void docklet_rclicked(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
{
    docklet_click_handler(FALSE);
}

/*called when the GtkStatusIcon is left clicked*/
void docklet_lclicked(GtkStatusIcon *status_icon, gpointer user_data)
{
    docklet_click_handler(TRUE);
}
#endif /*MTC_EGGTRAYICON*/

/*function to set the 'number of messages' tooltip text*/
static void docklet_tooltip()
{
	GString *tipstring= NULL;
	gchar tmpstring[NAME_MAX+ 30];
	gboolean first= TRUE;
	GSList *pcurrent= acclist;
	mtc_account *pcurrent_data= NULL;
    mtc_trayicon *ptrayicon;
    mtc_account *pactive= NULL;
	gint nmsgs= 0;
    gboolean can_add;

    can_add= FALSE;
    ptrayicon= &config.trayicon;
	
    memset(tmpstring, 0, sizeof(tmpstring));

#ifdef MTC_EGGTRAYICON
    /*disable the tooltip while we create it*/
    gtk_tooltips_disable(ptrayicon->tooltip);
#endif /*MTC_EGGTRAYICON*/

	/*create tooltip and display it*/
	tipstring= g_string_new(NULL);

    /*get the active account*/
    pactive= get_active_account();

	/*create the summary of the accounts for the tooltip*/
	while(pcurrent!= NULL)
	{
		pcurrent_data= (mtc_account *)pcurrent->data;
#ifdef MTC_NOTMINIMAL
        nmsgs= pcurrent_data->msginfo.num_messages;
#else
        nmsgs= pcurrent_data->num_messages;
#endif /*MTC_NOTMINIMAL*/
        
        if(nmsgs> 0)
        {
            /*don't add if it is not the active account*/
            if((config.multiple)|| ((!config.multiple)&& (pcurrent_data== pactive)))
            {
                
        	    /*if there are messages add it to summary*/
			    g_snprintf(tmpstring, sizeof(tmpstring), (nmsgs> 1)? S_DOCKLET_NEW_MESSAGES: S_DOCKLET_NEW_MESSAGE,
				    pcurrent_data->accname, nmsgs, (first)? "": "\n");
			
			    /*insert at the start (to match list order)*/
			    tipstring= g_string_prepend(tipstring, tmpstring);

			    first= FALSE;
            }

        }
		pcurrent= g_slist_next(pcurrent);
	}
	
	/*set the text*/
#ifdef MTC_EGGTRAYICON
   if(tipstring->str)
	    gtk_tooltips_set_tip(GTK_TOOLTIPS(ptrayicon->tooltip), ptrayicon->box, tipstring->str, NULL);
	
    g_string_free(tipstring, TRUE);
	
    /*right now re-enable the tip*/
    gtk_tooltips_enable(ptrayicon->tooltip);
#else
    if(tipstring->str)
        gtk_status_icon_set_tooltip(ptrayicon->docklet, tipstring->str);
#endif /*MTC_EGGTRAYICON*/

}

/*function to create the icon*/
static void docklet_add(mtc_icon *picon)
{
    mtc_trayicon *ptrayicon;

    ptrayicon= &config.trayicon;
	
	/*add the icon to the container and show it*/
#ifdef MTC_EGGTRAYICON
   /*add the image to the event box*/
    gtk_container_add(GTK_CONTAINER(ptrayicon->box), picon->image);
    
    /*create the docklet and add the event box to it*/
    ptrayicon->docklet= egg_tray_icon_new(PACKAGE);
    
    /*add the destroy callback to reload icon in case the panel dies*/
    g_signal_connect(G_OBJECT(ptrayicon->docklet), "destroy", G_CALLBACK(docklet_destroyed), NULL);
	
    /*ref the box, as it will decrease ref count when removed*/
	gtk_container_add(GTK_CONTAINER(ptrayicon->docklet), ptrayicon->box);
	g_object_ref(G_OBJECT(ptrayicon->docklet));
	
    gtk_widget_show(GTK_WIDGET(picon->image));
	gtk_widget_show_all(GTK_WIDGET(ptrayicon->docklet));
#else
    gtk_status_icon_set_from_pixbuf(ptrayicon->docklet, picon->pixbuf);
    if(!gtk_status_icon_get_visible(ptrayicon->docklet))
        gtk_status_icon_set_visible(ptrayicon->docklet, TRUE);
#endif /*MTC_EGGTRAYICON*/
    
    /*if we want to show summary dialog, do it here*/
#ifdef MTC_EXPERIMENTAL
        if(config.run_summary)
            sumdlg_create();
#endif
/*	}*/
	
}

/*Function to get the icon status
  will return none, multi, or the account depending on what is set*/
#ifdef MTC_EGGTRAYICON
static mtc_icon *docklet_status(void)
#else
static gint docklet_status(void)
#endif
{
	/*we need to report different icon statuses*/
	GSList *pcurrent= acclist;
	mtc_account *pcurrent_data= NULL;
	mtc_account *plast_data= NULL;
	guint multi= 0;
    gint nmsgs= 0;
	
	while(pcurrent!= NULL)
	{
		pcurrent_data= (mtc_account *)pcurrent->data;
#ifdef MTC_NOTMINIMAL
        nmsgs= pcurrent_data->msginfo.num_messages;
#else
        nmsgs= pcurrent_data->num_messages;
#endif /*MTC_NOTMINIMAL*/

		/*increment if multiple option is selected*/
		if(config.multiple&& (nmsgs> 0))
			multi++;

		/*return multiple icon status if more than one*/
		if(multi> 1)
#ifdef MTC_EGGTRAYICON
            return(&config.icon);
#else
            return(ACTIVE_ICON_MULTI);
#endif /*MTC_EGGTRAYICON*/

		/*set the last pointer to return if needed*/
		if(nmsgs> 0)
			plast_data= &(*pcurrent_data);
		
		pcurrent= g_slist_next(pcurrent);
	}
    /*return NULL if no new icon, or the account if there is*/
#ifdef MTC_EGGTRAYICON
	return((plast_data== NULL)? NULL: &plast_data->icon);
#else
    return((plast_data== NULL)? ACTIVE_ICON_NONE: (gint)plast_data->id);
#endif /*MTC_EGGTRAYICON*/
}

/*callback function that is called if the panel dies*/
#ifdef MTC_EGGTRAYICON
void docklet_destroyed(GtkWidget *widget, gpointer data)
{
    GtkWidget *pdockimage= NULL;
    mtc_icon *picon= NULL;
    
    picon= docklet_status();
    
    /*The icon must be removed and then re-added*/
    pdockimage= docklet_icon();
    if(pdockimage!= NULL)
        docklet_remove(pdockimage);
 
    docklet_add(picon);
}
#endif /*MTC_EGGTRAYICON*/

/*the main thread that is called to check the various mail accounts*/
gboolean mail_thread(gpointer data)
{
	/*if no thread is running*/
	if(!lock)
	{
		guint errflag= 0;
		GString *err_msg= NULL;
		mtc_plugin *pitem= NULL;
		GSList *pcurrent= acclist;
		mtc_account *pcurrent_data= NULL;
	    mtc_error retval= MTC_RETURN_TRUE;
        gboolean newmail= FALSE;
        mtc_icon *picon= NULL;

#ifdef MTC_EGGTRAYICON
        GtkWidget *pdockimage= NULL;
#else
        gint status= ACTIVE_ICON_NONE;
        mtc_trayicon *ptrayicon;

        ptrayicon= &config.trayicon;
#endif /*MTC_EGGTRAYICON*/

		/*lock the thread*/
		lock= TRUE;

		/*go through each account*/
		while(pcurrent!= NULL)
		{	
			pcurrent_data= (mtc_account *)pcurrent->data;

			/*initialise the array values to -1*/
#ifdef MTC_NOTMINIMAL
			pcurrent_data->msginfo.num_messages= -1;
#else
            pcurrent_data->num_messages= -1;
#endif /*MTC_NOTMINIMAL*/
			
			/*search for the plugin, if it is not found, report and error*/
			if((pitem= plg_find(pcurrent_data->plgname))== NULL)
			{
				err_noexit(S_DOCKLET_ERR_FIND_PLUGIN, pcurrent_data->plgname);
				err_dlg(S_DOCKLET_ERR_FIND_PLUGIN_MSG,
					pcurrent_data->plgname, pcurrent_data->accname);

				/*now go to the next account*/
				pcurrent= g_slist_next(pcurrent);
				continue;
			}

			/*use the plugin to check the mail and get number of messages*/
			if((retval= (*pitem->get_messages)(pcurrent_data))!= MTC_RETURN_TRUE)
            {
#ifdef MTC_NOTMINIMAL
                pcurrent_data->msginfo.num_messages= -1;
			    pcurrent_data->msginfo.new_messages= -1;
#else
                pcurrent_data->num_messages= -1;
#endif /*MTC_NOTMINIMAL*/
            }

			/*if there was a connection error*/
			if(retval== MTC_ERR_CONNECT)
			{
                /*new connection error, so increment*/
                pcurrent_data->cerr++;
                err_noexit(S_DOCKLET_ERR_CONNECT, pcurrent_data->hostname, pcurrent_data->cerr);
                
                if(err_msg== NULL)
					err_msg= g_string_new(NULL);
				
			    /*add to the error string if it is time to report connection error, otherwise increment*/
                if((config.err_freq!= 0)&& (config.err_freq== 1|| config.err_freq<= pcurrent_data->cerr))
                {
                    err_msg= g_string_prepend(err_msg, "\n");
				    err_msg= g_string_prepend(err_msg, pcurrent_data->hostname);
                    pcurrent_data->cerr= 0;
				    errflag++;
                }
			}
			/*if there was a bad error (i.e we must exit)*/
			else if(retval== MTC_ERR_EXIT)
			{
				/*extremely unlikely this will be allocated, but just to be safe*/
				if(err_msg!= NULL)
					g_string_free(err_msg, TRUE);

				/*message has (should have) already been reported by plugin, so exit*/
				exit(EXIT_FAILURE);
			}
            /*no error, so reset the error count*/
            else
                pcurrent_data->cerr= 0;
            
            /*set the new mail flag*/
#ifdef MTC_NOTMINIMAL
            if(pcurrent_data->msginfo.new_messages> 0)
#else
            if(pcurrent_data->num_messages> 0)
#endif /*MTC_NOTMINIMAL*/
                newmail= TRUE;

			pcurrent= g_slist_next(pcurrent);
		}
		
		/*report if checking an account failed*/
		if(errflag)
            err_dlg(S_DOCKLET_CONNECT_ERR, err_msg->str, PACKAGE, config.dir, LOG_FILE);
        
        /*free the string if need be*/
        if(err_msg!= NULL)
			g_string_free(err_msg, TRUE);
      
#ifdef MTC_EGGTRAYICON
        /*retrieve the new icon, if any*/
        picon= docklet_status();

        /*get the current one that is in the container*/
        pdockimage= docklet_icon();
        
        /*remove any existing icon*/
        if((pdockimage!= NULL)&& ((picon== NULL)|| (picon->image!= pdockimage)))
            docklet_remove(pdockimage);
        
        /*now add our new one if needed*/
        if(picon!= NULL&& picon->image!= NULL)
        {
            if(picon->image!= pdockimage)
                docklet_add(picon);
        }

    	/*next, set the text summary for the accounts*/
        if((picon!= NULL)&& (picon->image!= NULL))
			docklet_tooltip();
#else
        /*work out what icon (if any) should be shown*/
        status= docklet_status();

        /*remove any existing icon*/
        if(status== ACTIVE_ICON_NONE|| status!= ptrayicon->active)
            docklet_remove();

        /*add the new icon if required*/
        if(status!= ACTIVE_ICON_NONE)
        {
            if(status!= ptrayicon->active)
            {
                picon= docklet_icon(status);
                if(picon!= NULL)
                    docklet_add(picon);
            }
            if(picon!= NULL)
                docklet_tooltip();
        }
        /*update the active one*/
        ptrayicon->active= status;

#endif /*MTC_EGGTRAYICON*/
        
        /*Summary dialog text buffer would be clear/reset here*/

#ifdef MTC_NOTMINIMAL
        /*finally, launch the app we want to run if new mails*/
        if(newmail&& *config.nmailcmd)
            run_cmd(config.nmailcmd, TRUE);
#endif /*MTC_NOTMINIMAL*/

		/*unlock the thread*/
		lock= FALSE;

	}
	
	return(TRUE);
}

