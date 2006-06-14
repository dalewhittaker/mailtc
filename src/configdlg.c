/* configdlg.c
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

/*widget variables used for most functions*/
static GtkWidget *accname_entry, *hostname_entry, *port_entry, *username_entry, *password_entry, *mailprog_entry, *dicon_table, *cicon_table, *cicon_label, *config_iconcolour_button;
static GtkWidget *delay_spin, *account_listbox, *dicon, *cicon, *protocol_combo, *iconsize_checkbox, *filter_checkbox, *filter_button, *multi_accounts_checkbox;
static GtkTreeViewColumn *listbox_account_column, *listbox_protocol_column;
static GtkCellRenderer *renderer;
static GtkListStore *store;
static GtkTreeSelection *selection;
static GdkPixbuf *scaled;

/*function to create the pixbuf used for the icon*/
static GtkWidget *create_pixbuf(GtkWidget *picon, char *pcolourstring)
{
	GdkPixbuf *unscaled;
	
	/*get the pixbuf from the icon file*/
	unscaled= gdk_pixbuf_new_from_inline(-1, envelope_white, FALSE, NULL);
	if(!unscaled)
		return 0;

	/*if it is valid scale it and set the colour*/
	scaled= gdk_pixbuf_scale_simple(unscaled, 24, 24, GDK_INTERP_BILINEAR);
	set_icon_colour(scaled, pcolourstring+ 1);
	gtk_image_set_from_pixbuf(GTK_IMAGE(picon), scaled);
	
	if(!picon)
		return(NULL);

	/*cleanup*/
	g_object_unref(unscaled);
	g_object_unref(scaled);

	return(picon);
}
	

/*function called when the dialog window is closed is pressed on the config dialog*/
static gboolean delete_event(void)
{
	/*quit the program*/
	gtk_main_quit();
	return FALSE;
}

/*signal which ensures that the user can only type numbers in the port text entry*/
static void insert_text_handler(GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer data)
{
	/*block all the text for the entry*/
	char c= *text;
	g_signal_handlers_block_by_func(G_OBJECT(editable), G_CALLBACK(insert_text_handler), data);

	/*if the char is a digit add it to the entry*/
	if((c>= '0')&&(c<= '9'))
		gtk_editable_insert_text(editable, text, length, position);

	/*unblock it again*/
	g_signal_handlers_unblock_by_func(G_OBJECT(editable), G_CALLBACK(insert_text_handler), data);
	g_signal_stop_emission_by_name(G_OBJECT(editable), "insert_text");
}

/*function to save the mail account details to details and password file*/
static int save_mail_details(int profile)
{
	int retval= 0, empty= 0;
	GtkWidget *msgdlg;
	char msg[10];
	GtkTreeIter iter;
	GSList *pcurrent= NULL;
	mail_details *pcurrent_data= NULL;
	mtc_plugin_info *pitem= NULL;

	if((pcurrent_data= get_account(profile))== NULL)
		error_and_log(S_CONFIGDLG_ERR_GET_ACCOUNT_INFO, profile);
	
	/*check that there are no empty values before saving the details*/
	if(g_ascii_strcasecmp(gtk_entry_get_text(GTK_ENTRY(password_entry)), "")== 0)
	{
		g_strlcpy(msg, S_CONFIGDLG_PASSWORD, 10);
		empty++;
	}
	if(g_ascii_strcasecmp(gtk_entry_get_text(GTK_ENTRY(username_entry)), "")== 0)
	{
		g_strlcpy(msg, S_CONFIGDLG_USERNAME, 10);
		empty++;
	}
	if(g_ascii_strcasecmp(gtk_entry_get_text(GTK_ENTRY(port_entry)), "")== 0)
	{	
		g_strlcpy(msg, S_CONFIGDLG_PORT, 10);
		empty++;
	}
	if(g_ascii_strcasecmp(gtk_entry_get_text(GTK_ENTRY(hostname_entry)), "")== 0)
	{
		g_strlcpy(msg, S_CONFIGDLG_HOSTNAME, 10);
		empty++;
	}
	if(g_ascii_strcasecmp(gtk_entry_get_text(GTK_ENTRY(accname_entry)), "")== 0)
	{
		g_strlcpy(msg, S_CONFIGDLG_ACCNAME, 10);
		empty++;
	}
	
	/*if there were any empty values tell user to enter it*/
	if(empty)
	{
		msgdlg= gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, S_CONFIGDLG_DETAILS_INCOMPLETE, msg);
		gtk_dialog_run(GTK_DIALOG(msgdlg));
		gtk_widget_destroy(msgdlg);
		return 0;
	}

	/*copy the mail details to the structure*/
	g_strlcpy(pcurrent_data->accname, gtk_entry_get_text(GTK_ENTRY(accname_entry)), NAME_MAX);
	g_strlcpy(pcurrent_data->hostname, gtk_entry_get_text(GTK_ENTRY(hostname_entry)), LOGIN_NAME_MAX+ HOST_NAME_MAX);
	g_strlcpy(pcurrent_data->port, gtk_entry_get_text(GTK_ENTRY(port_entry)), PORT_LEN);
	g_strlcpy(pcurrent_data->username, gtk_entry_get_text(GTK_ENTRY(username_entry)), LOGIN_NAME_MAX+ HOST_NAME_MAX);
	g_strlcpy(pcurrent_data->password, gtk_entry_get_text(GTK_ENTRY(password_entry)), PASSWORD_LEN);
	
	/*get the relevant item depending on the active combo item*/
	if((pitem= g_slist_nth_data(plglist, gtk_combo_box_get_active(GTK_COMBO_BOX(protocol_combo))))== NULL)
		error_and_log(S_CONFIGDLG_ERR_GET_ACTIVE_PLUGIN);

	g_strlcpy(pcurrent_data->plgname, g_path_get_basename(g_module_name(pitem->handle)), PROTOCOL_LEN);

	/*get the filter setting*/
	pcurrent_data->runfilter= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(filter_checkbox));

	/*write the details to the file*/
	retval= write_user_details(pcurrent_data);
	
	/*try to get the listbox iterator and add a row if there is no iterator*/
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter)) 
	{
  		gtk_list_store_append(store, &iter);
		if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
			error_and_log(S_CONFIGDLG_ERR_LISTBOX_ITER);
	}

	/*clear the listbox*/
	gtk_list_store_clear(store);
	pcurrent= acclist;
	
	/*for each mail account read the details and add it to the listbox*/
	while(pcurrent!= NULL)
	{
		gtk_list_store_prepend(store, &iter);
		pcurrent_data= (mail_details *)pcurrent->data;

		/*find the plugin and update the list*/
		if((pitem= find_plugin(pcurrent_data->plgname))== NULL)
			run_error_dialog(S_CONFIGDLG_FIND_PLUGIN_MSG, pcurrent_data->plgname, pcurrent_data->accname);

		gtk_list_store_set(store, &iter, ACCOUNT_COLUMN, pcurrent_data->accname, PROTOCOL_COLUMN, (pitem!= NULL)? pitem->name: S_CONFIGDLG_ERR_FIND_PLUGIN, -1);
  		pcurrent= g_slist_next(pcurrent);
	}

	return(retval);
}

/*function to save the config options to config file*/
static int save_config_details(void)
{
	char delaystring[G_ASCII_DTOSTR_BUF_SIZE];
	int retval= 0;
	
	/*get the config information from the widgets*/
	g_ascii_dtostr(delaystring, sizeof(delaystring), gtk_spin_button_get_value(GTK_SPIN_BUTTON(delay_spin))); 
	config.icon_size= ((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(iconsize_checkbox))== TRUE)? 16: 24); 
	config.multiple= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(multi_accounts_checkbox)); 
	g_strlcpy(config.check_delay, delaystring, 3); 
	g_strlcpy(config.mail_program, gtk_entry_get_text(GTK_ENTRY(mailprog_entry)), NAME_MAX);
  	
	/*write the config info to the file*/
	retval= write_config_file();
	
	return(retval);
}

/*signal called when close button of config dialog is pressed*/
static void close_button_pressed(void)
{
	/*display message to tell user to run mailtc, then dialog destroy widget*/
	GtkWidget *msgdlg= gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, S_CONFIGDLG_READYTORUN, PACKAGE);
	gtk_dialog_run(GTK_DIALOG(msgdlg));
	gtk_widget_destroy(msgdlg);
	gtk_main_quit();
}

/*iterator function used to get number of rows in listbox*/
static gboolean count_rows(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer userdata)
{
	/*assign the pointer to the data and increment it*/
	gint *p_count= (gint*)userdata;
	g_assert(p_count!= NULL);
	(*p_count)++;

	/*return FALSE to keep the function counting*/
	return(FALSE);
}

/*signal called when the add button of config dialog is pressed*/
static void add_button_pressed(void)
{
	GtkTreeModel *model;
	gint count= 0;

	/*get the listbox model and count the number of rows*/
	model= gtk_tree_view_get_model(GTK_TREE_VIEW(account_listbox));

	gtk_tree_model_foreach(model, count_rows, &count);

	/*otherwise run the details dialog*/
	run_details_dialog(count, 1);
}

/*iterator function used to get the current selection from the listbox*/
static gboolean get_current_selection(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer userdata)
{
	/*set the pointer to point to the data and then increment it*/
	gint *p_count= (gint*)userdata;
	g_assert(p_count!= NULL);
	(*p_count)++;
	
	/*return TRUE if it is selected to stop count, other wise carry on*/
	return(gtk_tree_selection_iter_is_selected(selection, iter));
}

/*signal called when the edit button of config dialog is pressed*/
static void edit_button_pressed(void)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gint count= 0;
	
	/*get the listbox model and selection*/
	model= gtk_tree_view_get_model(GTK_TREE_VIEW(account_listbox));
	selection= gtk_tree_view_get_selection(GTK_TREE_VIEW(account_listbox));

	/*test if any are selected*/
	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		/*get the iterator if a row is selected*/
		if(!gtk_tree_model_get_iter_first(model, &iter))
			error_and_log(S_CONFIGDLG_ERR_LISTBOX_ITER);
	
		/*loop through listbox entries to see which is selected and run the details dialog for the account*/
		gtk_tree_model_foreach(model, get_current_selection, &count);
		run_details_dialog(count- 1, 0);
	}
}

/*signal called when the remove button of config dialog is pressed*/
static void remove_button_pressed(void)
{
	GtkTreeModel *model;
	GtkTreeIter iter1, iter2;
	gint count= 0, fullcount=0;
	
	/*get the listbox model and selection*/
	model= gtk_tree_view_get_model(GTK_TREE_VIEW(account_listbox));
	selection= gtk_tree_view_get_selection(GTK_TREE_VIEW(account_listbox));

	/*test if any are selected*/
	if(gtk_tree_selection_get_selected(selection, &model, &iter1))
	{
		/*get the listbox iterator*/
		if(!gtk_tree_model_get_iter_first(model, &iter2))
			error_and_log(S_CONFIGDLG_ERR_LISTBOX_ITER);
	
		/*loop through listbox entries to see which is selected*/
		gtk_tree_model_foreach(model, get_current_selection, &count);
		
		/*get the full count of rows*/
		gtk_tree_model_foreach(model, count_rows, &fullcount);
		
		/*remove the details and password file*/
		remove_file(DETAILS_FILE, count, fullcount);
		remove_file(PASSWORD_FILE, count, fullcount);
		remove_file(UIDL_FILE, count, fullcount);
		remove_file(FILTER_FILE, count, fullcount);
		
		/*Remove the account files/linked list*/
		remove_account(count- 1); 
		
		/*remove from listbox*/
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter1);
	}
}

/*signal called when the icon colour button of config dialog is pressed*/
static void iconcolour_button_pressed(GtkWidget *widget, char *scolour)
{
	GtkWidget *colour_dialog;
	GtkWidget *coloursel;
	GdkColor colour;
	gint response;
	unsigned int r, g, b;
	
	/*create the colour selection dialog and remove the help button*/
	colour_dialog= gtk_color_selection_dialog_new(S_CONFIGDLG_BUTTON_ICONCOLOUR);
	coloursel= GTK_COLOR_SELECTION_DIALOG(colour_dialog)->colorsel;
	gtk_widget_destroy(GTK_COLOR_SELECTION_DIALOG(colour_dialog)->help_button);
	
	/*set the initial colour to the colour from the file*/
	gdk_color_parse(scolour, &colour);
	gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(coloursel), &colour);
	
	/*run the dialog and get the selected colour if ok was selected*/
	response= gtk_dialog_run(GTK_DIALOG(colour_dialog));
	
	if(response== GTK_RESPONSE_OK)
	{
		gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(coloursel), &colour);
		gtk_widget_destroy(colour_dialog);
	}
	else
	{
		gtk_widget_destroy(colour_dialog);
		return;
	}

	/*remove the extra RGB data and create the colour string*/
	r= colour.red/ 256;
	g= colour.green/ 256;
	b= colour.blue/ 256;
	g_snprintf(scolour, ICON_LEN, "#%02X%02X%02X", r, g, b);

}

/*signal called when details iconcolour button is pressed*/
static void details_iconcolour_button_pressed(GtkWidget *widget, char *pcolourstring)
{
	iconcolour_button_pressed(widget, pcolourstring);
	
	/*remove the icon from the dialog and read it with new colour*/
	gtk_container_remove(GTK_CONTAINER(dicon_table), dicon);
	
	if(!create_pixbuf(dicon, pcolourstring))
		error_and_log(S_CONFIGDLG_ERR_CREATE_PIXBUF);
  	gtk_table_attach_defaults(GTK_TABLE(dicon_table), dicon, 0, 1, 0, 1);
	gtk_widget_show(dicon);

}
	
/*signal called when config iconcolour button is pressed*/
static void config_iconcolour_button_pressed(GtkWidget *widget, char *pcolourstring)
{
	iconcolour_button_pressed(widget, pcolourstring);
	
	/*remove the icon from the dialog and read it with new colour*/
	gtk_container_remove(GTK_CONTAINER(cicon_table), cicon);
	
	if(!create_pixbuf(cicon, pcolourstring))
		error_and_log(S_CONFIGDLG_ERR_CREATE_PIXBUF);
	
  	gtk_table_attach_defaults(GTK_TABLE(cicon_table), cicon, 0, 1, 0, 1);
	gtk_widget_show(cicon);

}

/*signal called when plugin information button is pressed*/
static void plginfo_button_pressed(GtkWidget *widget)
{
	mtc_plugin_info *pitem= NULL;
	GtkWidget *dialog= NULL;

	/*get the relevant item depending on the active combo item*/
	if((pitem= g_slist_nth_data(plglist, gtk_combo_box_get_active(GTK_COMBO_BOX(protocol_combo))))== NULL)
		error_and_log(S_CONFIGDLG_ERR_GET_ACTIVE_PLUGIN);
	
	/*set the port to the default port for the specified plugin*/
	dialog= gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, 
			S_CONFIGDLG_DISPLAY_PLG_INFO, pitem->name, pitem->author, pitem->desc);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

/*signal called when filter checkbox is pressed*/
static void filter_checkbox_pressed(GtkWidget *widget)
{
	gtk_widget_set_sensitive(filter_button, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

/*signal called when filter checkbox is pressed*/
static void multi_checkbox_pressed(GtkWidget *widget)
{
	gtk_widget_set_sensitive(GTK_WIDGET(cicon_label), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
	gtk_widget_set_sensitive(GTK_WIDGET(cicon), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
	gtk_widget_set_sensitive(GTK_WIDGET(config_iconcolour_button), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

/*signal called when filter button is pressed*/
static void filter_button_pressed(GtkWidget *widget, gpointer data)
{
	mail_details *pcurrent= NULL;
	gint *p_count= (gint*)data;

	if((pcurrent= get_account(*p_count))== NULL)
		error_and_log(S_CONFIGDLG_ERR_GET_ACCOUNT_INFO, *p_count);		
	
	run_filter_dialog(pcurrent);
}

/*signal called when protocol combo box changes*/
static void protocol_combo_changed(GtkComboBox *entry)
{
	mtc_plugin_info *pitem= NULL;
	gchar port_str[G_ASCII_DTOSTR_BUF_SIZE];

	/*get the relevant item depending on the active combo item*/
	if((pitem= g_slist_nth_data(plglist, gtk_combo_box_get_active(GTK_COMBO_BOX(entry))))== NULL)
		error_and_log(S_CONFIGDLG_ERR_GET_ACTIVE_PLUGIN);
	
	/*set the port to the default port for the specified plugin*/
	g_ascii_dtostr(port_str, G_ASCII_DTOSTR_BUF_SIZE, (gdouble)pitem->default_port);
	gtk_entry_set_text(GTK_ENTRY(port_entry), port_str); 

	/*Now enable/disable the filter widgets depending on plugin flags*/
	gtk_widget_set_sensitive(filter_button, (pitem->flags& MTC_ENABLE_FILTERS));
	gtk_widget_set_sensitive(filter_checkbox, (pitem->flags& MTC_ENABLE_FILTERS));
}

/*function to set the protocol default text*/
static gboolean set_combo_text(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer userdata)
{
	gchar *string;
	gtk_tree_model_get(model, iter, 0, &string, -1);
	
	/*compare the value from the details file with the current value
	 *and set it if it matches*/
	if(g_ascii_strcasecmp((char *)userdata, string)== 0)
	{	
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(protocol_combo), iter);
		g_free(string);
		return TRUE;
	}
	
	g_free(string);
	return FALSE;
}

/*display the details dialog*/
int run_details_dialog(int profile, int newaccount)
{
	GtkWidget *dialog, *iconcolour_button, *plginfo_button;
	GtkWidget *accname_label, *hostname_label, *port_label, *username_label, *password_label, *protocol_label, *icon_label;
	GtkWidget *main_table, *v_box_details;
	gint result= 0, saved= 0;
	mail_details *pcurrent= NULL;
	GSList *plgcurrent= plglist;
	mtc_plugin_info *pitem= NULL;
	
	/*setup the account name info*/
	accname_entry= gtk_entry_new();
	accname_label= gtk_label_new(S_CONFIGDLG_DETAILS_ACNAME);
  	gtk_entry_set_max_length(GTK_ENTRY(accname_entry), NAME_MAX+ 1);
	
	/*setup the hostname info*/
	hostname_entry= gtk_entry_new();
	hostname_label= gtk_label_new(S_CONFIGDLG_DETAILS_SERVER);
  	gtk_entry_set_max_length(GTK_ENTRY(hostname_entry), LOGIN_NAME_MAX+ HOST_NAME_MAX+ 1);
	
	/*setup the port info*/
	port_entry= gtk_entry_new();
	port_label= gtk_label_new(S_CONFIGDLG_DETAILS_PORT);
	gtk_entry_set_max_length(GTK_ENTRY(port_entry), PORT_LEN);
	g_signal_connect(G_OBJECT(port_entry), "insert_text", G_CALLBACK(insert_text_handler), (gpointer)port_entry);
	
	/*setup the username info*/
	username_entry= gtk_entry_new();
	username_label= gtk_label_new(S_CONFIGDLG_DETAILS_USERNAME);
	gtk_entry_set_max_length(GTK_ENTRY(username_entry), LOGIN_NAME_MAX+ HOST_NAME_MAX+ 1);
  
	/*setup the password info*/
	password_entry= gtk_entry_new();
	password_label= gtk_label_new(S_CONFIGDLG_DETAILS_PASSWORD);
	gtk_entry_set_max_length(GTK_ENTRY(password_entry), PASSWORD_LEN);
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
	
	/*setup the protocol stuff*/
	protocol_label= gtk_label_new(S_CONFIGDLG_DETAILS_PROTOCOL);
	protocol_combo= gtk_combo_box_new_text();

	/*setup the plugin info stuff*/
	plginfo_button= gtk_button_new_with_label(S_CONFIGDLG_PLG_INFO_BUTTON);
  	g_signal_connect(G_OBJECT(plginfo_button), "clicked", G_CALLBACK(plginfo_button_pressed), NULL);

	/*setup filter stuff*/
	filter_checkbox= gtk_check_button_new_with_label(S_CONFIGDLG_ENABLEFILTERS);
  	g_signal_connect(G_OBJECT(filter_checkbox), "clicked", G_CALLBACK(filter_checkbox_pressed), NULL);
	filter_button= gtk_button_new_with_label(S_CONFIGDLG_CONFIGFILTERS);
  	g_signal_connect(G_OBJECT(filter_button), "clicked", G_CALLBACK(filter_button_pressed), &profile);

	/*add the plugin protocol names to the combo box*/
	while(plgcurrent!= NULL)
	{
		pitem= (mtc_plugin_info *)plgcurrent->data;
		gtk_combo_box_append_text(GTK_COMBO_BOX(protocol_combo), pitem->name);
		plgcurrent= g_slist_next(plgcurrent);
	}

	/*setup the icon colour info*/
	icon_label= gtk_label_new(S_CONFIGDLG_ICON_COLOUR);
	dicon= gtk_image_new();
	g_object_ref(dicon);
	
	/*set the icon button to open the colour dialog*/
	iconcolour_button= gtk_button_new_with_label(S_CONFIGDLG_SETICONCOLOUR);
 
 	/*set initial combo values*/
	gtk_combo_box_set_active(GTK_COMBO_BOX(protocol_combo), 0);
	g_signal_connect(G_OBJECT(GTK_COMBO_BOX(protocol_combo)), "changed", G_CALLBACK(protocol_combo_changed), NULL);
	
	/*read the details for the selected account and display it in the widgets*/
	if((pcurrent= get_account(profile))!= NULL)
	{
		GtkTreeModel *model;
		GtkTreeIter iter;
		
		/*set the inital combo stuff (must be done after account read, but before port change)*/
		protocol_combo_changed(GTK_COMBO_BOX(protocol_combo));
	
	gtk_entry_set_text(GTK_ENTRY(accname_entry), pcurrent->accname);
		gtk_entry_set_text(GTK_ENTRY(hostname_entry), pcurrent->hostname);
		gtk_entry_set_text(GTK_ENTRY(port_entry), pcurrent->port);
		gtk_entry_set_text(GTK_ENTRY(username_entry), pcurrent->username);
		gtk_entry_set_text(GTK_ENTRY(password_entry), pcurrent->password);

		/*set the combobox to first entry and get the iterator*/
		model= gtk_combo_box_get_model(GTK_COMBO_BOX(protocol_combo));
		if(!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(protocol_combo), &iter))
				error_and_log(S_CONFIGDLG_ERR_COMBO_ITER);
				
		/*loop through listbox entries to see which is selected and run the details dialog for the account*/
		/*if not found for whatever reason, default to the first in the list*/
		if((pitem= find_plugin(pcurrent->plgname))== NULL)
			pitem= plglist->data;

		gtk_tree_model_foreach(model, set_combo_text, (char *)pitem->name);
		/*gtk_tree_model_foreach(model, set_combo_text, pcurrent->plgname);*/
		
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filter_checkbox), pcurrent->runfilter);
	}
	/*set the default pop port if details could not be read*/
	else
	{
		/*set the initial combo stuff*/
		protocol_combo_changed(GTK_COMBO_BOX(protocol_combo));
	
		/*set the default port value*/
		acclist= create_account();
		pcurrent= get_account(profile);
		g_strlcpy(pcurrent->icon, "#FFFFFF", ICON_LEN);
 	}
  	g_signal_connect(G_OBJECT(iconcolour_button), "clicked", G_CALLBACK(details_iconcolour_button_pressed), pcurrent->icon);
	
	/*create the icon pixbuf (must be called after read_user_details*/
	if(!(dicon= create_pixbuf(dicon, pcurrent->icon)))
		error_and_log(S_CONFIGDLG_ERR_CREATE_PIXBUF);

	gtk_widget_set_sensitive(filter_button, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(filter_checkbox)));
	
	/*pack the stuff into the boxes*/
	main_table= gtk_table_new(9, 3, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(main_table), 10);
	gtk_table_set_row_spacings(GTK_TABLE(main_table), 10);
	gtk_table_attach_defaults(GTK_TABLE(main_table), accname_label, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(main_table), accname_entry, 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(main_table), hostname_label, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(main_table), hostname_entry, 2, 3, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(main_table), port_label, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(main_table), port_entry, 2, 3, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(main_table), username_label, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(main_table), username_entry, 2, 3, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(main_table), password_label, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(main_table), password_entry, 2, 3, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(main_table), protocol_label, 0, 1, 5, 6);
	gtk_table_attach_defaults(GTK_TABLE(main_table), protocol_combo, 2, 3, 5, 6);
	gtk_table_attach_defaults(GTK_TABLE(main_table), plginfo_button, 2, 3, 6, 7);

	dicon_table= gtk_table_new(1, 3, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(dicon_table), 5);
  	gtk_table_attach(GTK_TABLE(dicon_table), dicon, 0, 1, 0, 1, GTK_FILL| GTK_EXPAND| GTK_SHRINK, GTK_FILL| GTK_EXPAND| GTK_SHRINK, 0, 0);
  	gtk_table_attach(GTK_TABLE(dicon_table), iconcolour_button, 2, 3, 0, 1, GTK_FILL| GTK_SHRINK, GTK_FILL| GTK_EXPAND| GTK_SHRINK, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(main_table), icon_label, 0, 1, 7, 8);
	gtk_table_attach_defaults(GTK_TABLE(main_table), dicon_table, 2, 3, 7, 8);
	gtk_table_attach_defaults(GTK_TABLE(main_table), filter_checkbox, 0, 1, 8, 9);
  	gtk_table_attach(GTK_TABLE(main_table), filter_button, 2, 3, 8, 9, GTK_SHRINK| GTK_FILL, GTK_FILL| GTK_EXPAND| GTK_SHRINK, 0, 0);
	gtk_container_set_border_width(GTK_CONTAINER(main_table), 10);
	
	v_box_details= gtk_vbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(v_box_details), main_table, FALSE, FALSE, 10);
	
	/*create the details dialog*/
	dialog= gtk_dialog_new_with_buttons(S_CONFIGDLG_DETAILS_TITLE, NULL, GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), v_box_details);
	gtk_widget_show_all(v_box_details);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 50, 50);

	/*keep running dialog until details are saved (i.e all values entered)*/
	while(!saved)
	{
		result= gtk_dialog_run(GTK_DIALOG(dialog)); 
		switch(result)
		{
			/*if OK get the value of saved to check if details are saved correctly*/
			case GTK_RESPONSE_ACCEPT:
				saved= save_mail_details(profile);
			break;
			/*if Cancel set saved to 1 so that the dialog will exit*/
			case GTK_RESPONSE_REJECT:
			{
				/*we need to check if a filter file was created and remove it if it was*/
				if(newaccount)
				{
					char filterfile[NAME_MAX+ 1];
					memset(filterfile, '\0', NAME_MAX);

					get_account_file(filterfile, FILTER_FILE, profile);

					if((IS_FILE(filterfile))&& (remove(filterfile)== -1))
						error_and_log(S_CONFIGDLG_ERR_REMOVE_FILE, filterfile);
				
					/*we also must remove the account from the linked list*/
					remove_account(profile);
				}
			}
			default:
				saved= 1;
		}
	}

	if(dicon)
		g_object_unref(dicon);
	
	/*destroy the dialog now that it is finished*/
	gtk_widget_destroy(dialog);

	return 1;
}

/*display the config notebook dialog*/
GtkWidget *run_config_dialog(GtkWidget *dialog)
{
	GtkWidget *notebook;
	GtkWidget *notebook_title[2];
	GtkWidget *h_box_page2[2], *v_box_array[3];
	GtkWidget *mailprog_label, *delay_label;
	GtkWidget *table;
	GtkWidget *treescroll;
	GtkAdjustment *delay_adj;
	GtkWidget *add_button, *remove_button, *edit_button, *close_button, *save_button;
	GtkTooltips *multi_tooltip;
	gchar window_title[30];
	GtkWidget *main_table;
	GSList *pcurrent= NULL;
	mail_details *pcurrent_data= NULL;
	GtkTreeIter iter;
	mtc_plugin_info *pitem= NULL;

	/*set the config window title*/
	g_snprintf(window_title, 30, S_CONFIGDLG_CONFIG_TITLE, PACKAGE);
	
	/*setup the notebook*/
	notebook= gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
	notebook_title[0]= gtk_label_new(S_CONFIGDLG_TAB_GENERAL);
	notebook_title[1]= gtk_label_new(S_CONFIGDLG_TAB_ACCOUNTS);
  
	/*setup the delay info*/ 
	delay_label= gtk_label_new(S_CONFIGDLG_INTERVAL);
	gtk_misc_set_alignment(GTK_MISC(delay_label), 0, 0.5);
	delay_adj= (GtkAdjustment *)gtk_adjustment_new(1.0, 1.0, 60.0, 1.0, 5.0, 5.0);
	delay_spin= gtk_spin_button_new(delay_adj, 1.0, 0);

	/*setup the iconsize info*/
	iconsize_checkbox= gtk_check_button_new_with_label(S_CONFIGDLG_SMALLICON);
	
	/*set the option for multiple accounts*/
	multi_accounts_checkbox= gtk_check_button_new_with_label(S_CONFIGDLG_MULTIPLE_ACCOUNTS);
	multi_tooltip= gtk_tooltips_new();
	gtk_tooltips_set_tip(GTK_TOOLTIPS(multi_tooltip), multi_accounts_checkbox, S_CONFIGDLG_MULTI_TOOLTIP, S_CONFIGDLG_MULTI_TOOLTIP);
	
	/*setup the mail program info*/
	mailprog_entry= gtk_entry_new();
	mailprog_label= gtk_label_new(S_CONFIGDLG_MAILAPP);
	gtk_misc_set_alignment(GTK_MISC(mailprog_label), 0, 0.5);
	gtk_entry_set_max_length(GTK_ENTRY(mailprog_entry), NAME_MAX+ 1);

	/*setup the icon colour info*/
	cicon_label= gtk_label_new(S_CONFIGDLG_ICON_COLOUR_MULTIPLE);
	gtk_misc_set_alignment(GTK_MISC(cicon_label), 1, 0.5);
	gtk_widget_set_sensitive(GTK_WIDGET(cicon_label), FALSE);
	cicon= gtk_image_new();
	gtk_widget_set_sensitive(GTK_WIDGET(cicon), FALSE);
	g_object_ref(cicon);
	
	/*set the icon button to open the colour dialog*/
	config_iconcolour_button= gtk_button_new_with_label(S_CONFIGDLG_SETICONCOLOUR);
	gtk_widget_set_sensitive(GTK_WIDGET(config_iconcolour_button), FALSE);
  	g_signal_connect(G_OBJECT(config_iconcolour_button), "clicked", G_CALLBACK(config_iconcolour_button_pressed), config.icon);
	
  	g_signal_connect(G_OBJECT(multi_accounts_checkbox), "clicked", G_CALLBACK(multi_checkbox_pressed), NULL);
	
	/*set the config details*/
	read_config_file();
	gtk_entry_set_text(GTK_ENTRY(mailprog_entry), config.mail_program);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(delay_spin), g_ascii_strtod(config.check_delay, NULL));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(iconsize_checkbox), (config.icon_size<= 16)? TRUE: FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(multi_accounts_checkbox), config.multiple);
	
	/*create the save button for the dialog and call save function when pressed*/
	save_button= gtk_button_new_from_stock(GTK_STOCK_SAVE);
	g_signal_connect(G_OBJECT(save_button), "clicked", G_CALLBACK(save_config_details), NULL);
  
	/*setup the listbox here*/
	store= gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
	gtk_list_store_append(GTK_LIST_STORE(store), &iter);
	account_listbox= gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	renderer= gtk_cell_renderer_text_new();
	listbox_account_column= gtk_tree_view_column_new_with_attributes(S_CONFIGDLG_LTAB_ACCOUNT, renderer, "text", ACCOUNT_COLUMN, NULL);
	gtk_tree_view_column_set_min_width(listbox_account_column, 250);
	listbox_protocol_column= gtk_tree_view_column_new_with_attributes(S_CONFIGDLG_LTAB_PROTOCOL, renderer, "text", PROTOCOL_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(account_listbox), listbox_account_column);
	gtk_tree_view_append_column(GTK_TREE_VIEW(account_listbox), listbox_protocol_column);   
	selection= gtk_tree_view_get_selection(GTK_TREE_VIEW(account_listbox));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
  
	/*get the model for the listbox and clear it before*/
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
  		error_and_log(S_CONFIGDLG_ERR_COMBO_ITER);
	gtk_list_store_clear(store);

	/*create the scroll for the listbox*/
	treescroll= gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(treescroll), account_listbox);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(treescroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(treescroll), GTK_SHADOW_IN);

	/*add the accounts to the list*/
	read_accounts();
	pcurrent= acclist;

	while(pcurrent!= NULL)
	{
		gtk_list_store_prepend(store, &iter);
		pcurrent_data= (mail_details *)pcurrent->data;

		/*find the plugin and add it to the listbox*/
		/*Message box here if we cannot find the plugin*/
		if((pitem= find_plugin(pcurrent_data->plgname))== NULL)
			run_error_dialog(S_CONFIGDLG_FIND_PLUGIN_MSG, pcurrent_data->plgname, pcurrent_data->accname);
		
		/*add to the list, if we cannot find it, report that it is not found*/
		gtk_list_store_set(store, &iter, ACCOUNT_COLUMN, pcurrent_data->accname, PROTOCOL_COLUMN, (pitem!= NULL)? pitem->name: S_CONFIGDLG_ERR_FIND_PLUGIN, -1);
  		pcurrent= g_slist_next(pcurrent);
	}
  	g_object_unref(store);
  
	/*create the icon pixbuf*/
	if(!(cicon= create_pixbuf(cicon, config.icon)))
		error_and_log(S_CONFIGDLG_ERR_CREATE_PIXBUF);

	/*setup the listbox buttons here and the functions called when clicked*/
	edit_button= gtk_button_new_from_stock(GTK_STOCK_PROPERTIES);
	add_button= gtk_button_new_from_stock(GTK_STOCK_ADD);
	remove_button= gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	g_signal_connect(G_OBJECT(edit_button), "clicked", G_CALLBACK(edit_button_pressed), NULL);
	g_signal_connect(G_OBJECT(add_button), "clicked", G_CALLBACK(add_button_pressed), NULL);
	g_signal_connect(G_OBJECT(remove_button), "clicked", G_CALLBACK(remove_button_pressed), NULL);
	
	main_table= gtk_table_new(5, 3, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(main_table), 10);
	gtk_table_set_row_spacings(GTK_TABLE(main_table), 10);
	gtk_table_attach_defaults(GTK_TABLE(main_table), delay_label, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(main_table), delay_spin, 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(main_table), mailprog_label, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(main_table), mailprog_entry, 2, 3, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(main_table), multi_accounts_checkbox, 0, 3, 2, 3);
	table= gtk_table_new(1, 3, TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table), save_button, 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(main_table), iconsize_checkbox, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(main_table), table, 2, 3, 4, 5);
	
	cicon_table= gtk_table_new(1, 3, FALSE);
  	gtk_table_attach(GTK_TABLE(cicon_table), cicon, 0, 1, 0, 1, GTK_FILL| GTK_EXPAND| GTK_SHRINK, GTK_FILL| GTK_EXPAND| GTK_SHRINK, 0, 0);
  	gtk_table_attach(GTK_TABLE(cicon_table), config_iconcolour_button, 2, 3, 0, 1, GTK_FILL| GTK_SHRINK, GTK_FILL| GTK_EXPAND| GTK_SHRINK, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(main_table), cicon_label, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(main_table), cicon_table, 2, 3, 3, 4);
	gtk_container_set_border_width(GTK_CONTAINER(main_table), 10);
	
	v_box_array[0]= gtk_vbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(v_box_array[0]), main_table, FALSE, FALSE, 10);
	
	h_box_page2[0]= gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(h_box_page2[0]), treescroll, TRUE, TRUE, 10);
	h_box_page2[1]= gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(h_box_page2[1]), add_button, TRUE, TRUE, 10);
	gtk_box_pack_start(GTK_BOX(h_box_page2[1]), remove_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(h_box_page2[1]), edit_button, TRUE, TRUE, 10);
	v_box_array[1]= gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(v_box_array[1]), h_box_page2[0], TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(v_box_array[1]), h_box_page2[1], FALSE, FALSE, 5);
  
	/*create the notebook pages*/
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), v_box_array[0], notebook_title[0]);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), v_box_array[1], notebook_title[1]);
	v_box_array[2]= gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(v_box_array[2]), notebook, FALSE, FALSE, 10);
  
	/*create the dialog and the buttons*/
	dialog= gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), window_title);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), v_box_array[2]);
	close_button= gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(close_button), "clicked", G_CALLBACK(close_button_pressed), NULL);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), close_button, TRUE, TRUE, 0);			  
	g_signal_connect(G_OBJECT(dialog), "delete_event", G_CALLBACK(delete_event), NULL);
	gtk_widget_show_all(dialog);
	
	/*if(cicon)
		g_object_unref(cicon);*/
	return dialog;
}


