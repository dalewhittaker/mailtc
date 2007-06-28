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

#include <gtk/gtktreeselection.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtktable.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkcellrenderer.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkcolorsel.h>
#include <gtk/gtkcolorseldialog.h>

#include "configdlg.h"
#include "filefunc.h"
#include "filterdlg.h"
#include "plugfunc.h"

enum { ACCOUNT_COLUMN= 0, PROTOCOL_COLUMN, N_COLUMNS };

/*A helper struct to make added to tables easier*/
typedef struct _dlgtable
{
    GtkWidget *widget;
    gint left;
    gint right;
    gint top;
    gint bottom;
    gint nrows;
    gint ncols;
    gint rowsadded;
    gint colsadded;
} dlgtable;

/*widget variables used for most functions*/
static GtkWidget *accname_entry,
    *hostname_entry,
    *port_entry,
    *username_entry,
    *password_entry,
    *mailprog_entry,
    *config_iconcolour_button,
    *delay_spin,
    *account_listbox,
    *cicon_label,
    *protocol_combo,
    *iconsize_checkbox,
#ifdef MTC_NOTMINIMAL
    *filter_checkbox,
    *filter_button,
    *nmailcmd_entry,
#endif /*MTC_NOTMINIMAL*/
    *multi_accounts_checkbox,
#ifdef MTC_EXPERIMENTAL
    *summary_checkbox,
    *summary_button,
#endif
    *cerr_combo,
    *cerr_spin,
    *cerr_label2;

static dlgtable cicon_table, dicon_table;
static GtkTreeViewColumn *listbox_account_column, *listbox_protocol_column;
static GtkCellRenderer *renderer;
static GtkListStore *store;
static GtkTreeSelection *selection;

/*add a widget to the table, auto-resizing if needed*/
static dlgtable *tbl_add(dlgtable *ptable, GtkWidget *pwidget, GtkAttachOptions xoptions, GtkAttachOptions yoptions)
{
    gboolean resize= FALSE;
    
    /*calculate where to add to the table, and resize if needed*/
    if(ptable->bottom> ptable->nrows)
    {
        ptable->nrows++;
        resize= TRUE;
    }
    if(ptable->right> ptable->ncols)
    {
        ptable->ncols++;
        resize= TRUE;
    }
    if(resize)
        gtk_table_resize(GTK_TABLE(ptable->widget), ptable->nrows, ptable->ncols);

    /*add to the table*/
    if(xoptions&& yoptions)
    {
        gtk_table_attach(GTK_TABLE(ptable->widget), pwidget, ptable->left, ptable->right,
                ptable->top, ptable->bottom, xoptions, yoptions, 0, 0);
    }
    else
    {
        gtk_table_attach_defaults(GTK_TABLE(ptable->widget), pwidget, ptable->left,
                                    ptable->right, ptable->top, ptable->bottom);
        /*g_print("defaults: %d, %d, %d, %d\n", 
                                    ptable->left, ptable->right, ptable->top, ptable->bottom);*/
    }
    return(ptable);
}

/*clear a table so it is initialised*/
static dlgtable *tbl_clear(dlgtable *ptable)
{
    ptable->left= -1;
    ptable->right= 0;
    ptable->top= -1;
    ptable->bottom= 0;
    return(ptable);
}

/*helper to initialise a table*/
static dlgtable *tbl_init(dlgtable *ptable, gint rows, gint cols)
{
    ptable->widget= gtk_table_new(rows, cols, FALSE);
    ptable->nrows= rows;
    ptable->ncols= cols;

    return(tbl_clear(ptable));
}

/*adds a widget to a new colum at the end of an existing row*/
static dlgtable *tbl_addcol(dlgtable *ptable, GtkWidget *pwidget, gint gap, gint span, GtkAttachOptions xoptions, GtkAttachOptions yoptions)
{
    ptable->left= ptable->left+ gap+ 1;
    ptable->right= ptable->right+ gap+ span;
    return(tbl_add(ptable, pwidget, xoptions, yoptions));
}

/*add a widget to a new row*/
static dlgtable *tbl_addcol_new(dlgtable *ptable, GtkWidget *pwidget, gint gap, gint span, GtkAttachOptions xoptions, GtkAttachOptions yoptions)
{
    ptable->left= -1;
    ptable->right= 0;
    ptable->top++;
    ptable->bottom++;
    return(tbl_addcol(ptable, pwidget, gap, span, xoptions, yoptions));
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
	gchar c= *text;
	g_signal_handlers_block_by_func(G_OBJECT(editable), G_CALLBACK(insert_text_handler), data);

	/*if the char is a digit add it to the entry*/
	if((c>= '0')&& (c<= '9'))
		gtk_editable_insert_text(editable, text, length, position);

	/*unblock it again*/
	g_signal_handlers_unblock_by_func(G_OBJECT(editable), G_CALLBACK(insert_text_handler), data);
	g_signal_stop_emission_by_name(G_OBJECT(editable), "insert_text");
}

/*function to save the mail account details to details and password file*/
static gboolean acc_save(int profile)
{
	gboolean retval= FALSE;
    gint empty= 0;
	gchar msg[10];
	GtkTreeIter iter;
	GSList *pcurrent= NULL;
	mtc_account *pcurrent_data= NULL;
	mtc_plugin *pitem= NULL;
    gchar *pbasename= NULL;

	if((pcurrent_data= get_account(profile))== NULL)
		err_exit(S_CONFIGDLG_ERR_GET_ACCOUNT_INFO, profile);
	
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
        err_dlg(GTK_MESSAGE_ERROR, S_CONFIGDLG_DETAILS_INCOMPLETE, msg);
		return FALSE;
	}

	/*copy the mail details to the structure*/
	g_strlcpy(pcurrent_data->accname, gtk_entry_get_text(GTK_ENTRY(accname_entry)), sizeof(pcurrent_data->accname));
	g_strlcpy(pcurrent_data->hostname, gtk_entry_get_text(GTK_ENTRY(hostname_entry)), sizeof(pcurrent_data->hostname));
    pcurrent_data->port= (gint)g_ascii_strtod(gtk_entry_get_text(GTK_ENTRY(port_entry)), NULL);
	g_strlcpy(pcurrent_data->username, gtk_entry_get_text(GTK_ENTRY(username_entry)), sizeof(pcurrent_data->username));
	g_strlcpy(pcurrent_data->password, gtk_entry_get_text(GTK_ENTRY(password_entry)), sizeof(pcurrent_data->password));
	
	/*get the relevant item depending on the active combo item*/
	if((pitem= g_slist_nth_data(plglist, gtk_combo_box_get_active(GTK_COMBO_BOX(protocol_combo))))== NULL)
		err_exit(S_CONFIGDLG_ERR_GET_ACTIVE_PLUGIN);

    pbasename= plg_name(pitem);
	g_strlcpy(pcurrent_data->plgname, pbasename, sizeof(pcurrent_data->plgname));
    g_free(pbasename);

#ifdef MTC_NOTMINIMAL
	/*get the filter setting*/
	pcurrent_data->runfilter= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(filter_checkbox));
#endif /*MTC_NOTMINIMAL*/

	/*write the details to the file*/
	retval= cfg_write();
	
	/*try to get the listbox iterator and add a row if there is no iterator*/
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter)) 
	{
  		gtk_list_store_append(store, &iter);
		if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
			err_exit(S_CONFIGDLG_ERR_LISTBOX_ITER);
	}

	/*clear the listbox*/
	gtk_list_store_clear(store);
	pcurrent= acclist;
	
	/*for each mail account read the details and add it to the listbox*/
	while(pcurrent!= NULL)
	{
		gtk_list_store_prepend(store, &iter);
		pcurrent_data= (mtc_account *)pcurrent->data;

		/*find the plugin and update the list*/
		if((pitem= plg_find(pcurrent_data->plgname))== NULL)
			err_dlg(GTK_MESSAGE_WARNING, S_CONFIGDLG_FIND_PLUGIN_MSG, pcurrent_data->plgname, pcurrent_data->accname);

		gtk_list_store_set(store, &iter, ACCOUNT_COLUMN, pcurrent_data->accname, PROTOCOL_COLUMN, (pitem!= NULL)? pitem->name: S_CONFIGDLG_ERR_FIND_PLUGIN, -1);
  		pcurrent= g_slist_next(pcurrent);
	}

	return(retval);
}

/*function to save the config options to config file*/
static gboolean cfg_save(void)
{
	gboolean retval= FALSE;
	gint active;

    if((active= gtk_combo_box_get_active(GTK_COMBO_BOX(cerr_combo)))< 0)
        active= 0;
	
    /*get the config information from the widgets*/
#ifdef MTC_NOTMINIMAL
	config.icon_size= ((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(iconsize_checkbox))== TRUE)? 16: 24); 
#endif /*MTC_NOTMINIMAL*/
	config.multiple= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(multi_accounts_checkbox)); 
	config.check_delay= (guint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(delay_spin)); 
	g_strlcpy(config.mail_program, gtk_entry_get_text(GTK_ENTRY(mailprog_entry)), sizeof(config.mail_program));
#ifdef MTC_EXPERIMENTAL
    config.run_summary= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(summary_checkbox)); 
#endif
    config.err_freq= (active< 2)? active: (gint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(cerr_spin));

#ifdef MTC_NOTMINIMAL
    g_strlcpy(config.nmailcmd, gtk_entry_get_text(GTK_ENTRY(nmailcmd_entry)), sizeof(config.nmailcmd));
#endif /*MTC_NOTMINIMAL*/

	/*write the config info to the file*/
	retval= cfg_write();
	
	return(retval);
}

/*signal called when close button of config dialog is pressed*/
static void close_button_pressed(void)
{
	/*display message to tell user to run mailtc, then dialog destroy widget*/
    err_dlg(GTK_MESSAGE_INFO, S_CONFIGDLG_READYTORUN, PACKAGE);
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
	accdlg_run(count, 1);
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
			err_exit(S_CONFIGDLG_ERR_LISTBOX_ITER);
	
		/*loop through listbox entries to see which is selected and run the details dialog for the account*/
		gtk_tree_model_foreach(model, get_current_selection, &count);
		accdlg_run(count- 1, 0);
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
			err_exit(S_CONFIGDLG_ERR_LISTBOX_ITER);
	
		/*loop through listbox entries to see which is selected*/
		gtk_tree_model_foreach(model, get_current_selection, &count);
		
		/*get the full count of rows*/
		gtk_tree_model_foreach(model, count_rows, &fullcount);
		
		/*Remove the account files/linked list*/
		remove_account(count- 1); 
	
        /*re-write the config*/
        cfg_write();

        /*remove the UIDL file*/
		rm_mtc_file(UIDL_FILE, count, fullcount);
		/*rm_mtc_file(DETAILS_FILE, count, fullcount);
		rm_mtc_file(FILTER_FILE, count, fullcount);*/
		
		/*remove from listbox*/
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter1);
	}
}

/*signal called when the icon colour button of config dialog is pressed*/
static void iconcolour_button_pressed(GtkWidget *widget, gchar *scolour)
{
	GtkWidget *colour_dialog;
	GtkWidget *coloursel;
	GdkColor colour;
	gint response;
	guint r, g, b;
	
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
/*although possibly this and function below could be merged*/
static void details_iconcolour_button_pressed(GtkWidget *widget, gpointer user_data)
{
    mtc_icon *picon= (mtc_icon *)user_data;
	iconcolour_button_pressed(widget, picon->colour);
	
	/*remove the icon from the dialog and read it with new colour*/
	gtk_container_remove(GTK_CONTAINER(dicon_table.widget), picon->image);
	
    picon= pixbuf_create(picon);
	if(!picon->image)
		err_exit(S_CONFIGDLG_ERR_CREATE_PIXBUF);

    tbl_clear(&dicon_table);
    tbl_addcol_new(&dicon_table, picon->image, 0, 1, 0, 0);
	gtk_widget_show(picon->image);

}
	
/*signal called when config iconcolour button is pressed*/
static void config_iconcolour_button_pressed(GtkWidget *widget, gpointer user_data)
{
    mtc_icon *picon= (mtc_icon *)user_data;
	iconcolour_button_pressed(widget, picon->colour);
	
	/*remove the icon from the dialog and read it with new colour*/
	gtk_container_remove(GTK_CONTAINER(cicon_table.widget), picon->image);
	
    picon= pixbuf_create(picon);
	if(!picon->image)
		err_exit(S_CONFIGDLG_ERR_CREATE_PIXBUF);
	
    tbl_clear(&cicon_table);
    tbl_addcol_new(&cicon_table, picon->image, 0, 1, 0, 0);
	gtk_widget_show(picon->image);

}

/*signal called when plugin information button is pressed*/
static void plginfo_button_pressed(GtkWidget *widget)
{
	mtc_plugin *pitem= NULL;

	/*get the relevant item depending on the active combo item*/
	if((pitem= g_slist_nth_data(plglist, gtk_combo_box_get_active(GTK_COMBO_BOX(protocol_combo))))== NULL)
		err_exit(S_CONFIGDLG_ERR_GET_ACTIVE_PLUGIN);
	
	/*set the port to the default port for the specified plugin*/
    err_dlg(GTK_MESSAGE_INFO, S_CONFIGDLG_DISPLAY_PLG_INFO, pitem->name, pitem->author, pitem->desc);

}

/*signal called when multi checkbox is pressed*/
static void multi_checkbox_pressed(GtkWidget *widget, gpointer user_data)
{
    mtc_icon *picon= (mtc_icon *)user_data;
	gtk_widget_set_sensitive(GTK_WIDGET(cicon_label), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
	gtk_widget_set_sensitive(GTK_WIDGET(picon->image), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
	gtk_widget_set_sensitive(GTK_WIDGET(config_iconcolour_button), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

#ifdef MTC_NOTMINIMAL
/*signal called when filter checkbox is pressed*/
static void filter_checkbox_pressed(GtkWidget *widget)
{
	gtk_widget_set_sensitive(filter_button, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

/*signal called when filter button is pressed*/
static void filter_button_pressed(GtkWidget *widget, gpointer data)
{
	mtc_account *pcurrent= NULL;
	gint *p_count= (gint*)data;

	if((pcurrent= get_account(*p_count))== NULL)
		err_exit(S_CONFIGDLG_ERR_GET_ACCOUNT_INFO, *p_count);		

	filterdlg_run(pcurrent);
}
#endif /*MTC_NOTMINIMAL*/

#ifdef MTC_EXPERIMENTAL
/*signal called when summary checkbox is pressed*/
static void summary_checkbox_pressed(GtkWidget *widget)
{
	gtk_widget_set_sensitive(summary_button, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

/*signal called when summary button is pressed*/
static void summary_button_pressed(GtkWidget *widget)
{
    GtkStyle *pstyle;
    GString *sfont;
	
    pstyle= gtk_widget_get_style(widget);
    sfont= g_string_new(NULL);
    g_string_printf(sfont, "%s %d", pango_font_description_get_family(pstyle->font_desc), pango_font_description_get_size(pstyle->font_desc)/ PANGO_SCALE);       
	sumcfgdlg_run(sfont->str);
    
    g_string_free(sfont, TRUE);
}
#endif

/*signal called when protocol combo box changes*/
static void protocol_combo_changed(GtkComboBox *entry)
{
	mtc_plugin *pitem= NULL;
	gchar port_str[G_ASCII_DTOSTR_BUF_SIZE];

	/*get the relevant item depending on the active combo item*/
	if((pitem= g_slist_nth_data(plglist, gtk_combo_box_get_active(GTK_COMBO_BOX(entry))))== NULL)
		err_exit(S_CONFIGDLG_ERR_GET_ACTIVE_PLUGIN);
	
	/*set the port to the default port for the specified plugin*/
	g_ascii_dtostr(port_str, G_ASCII_DTOSTR_BUF_SIZE, (gdouble)pitem->default_port);
	gtk_entry_set_text(GTK_ENTRY(port_entry), port_str); 

#ifdef MTC_NOTMINIMAL
	/*Now enable/disable the filter widgets depending on plugin flags*/
	gtk_widget_set_sensitive(filter_button, (pitem->flags& MTC_ENABLE_FILTERS));
	gtk_widget_set_sensitive(filter_checkbox, (pitem->flags& MTC_ENABLE_FILTERS));
#endif /*MTC_NOTMINIMAL*/
}

/*signal called when connection error combo box changes*/
static void cerr_combo_changed(GtkComboBox *entry)
{
	gint active;
    
    active= gtk_combo_box_get_active(GTK_COMBO_BOX(entry));
	if(active> 1)
    {
        gtk_widget_show(cerr_spin);
        gtk_widget_show(cerr_label2);
    }
    else
    {
        gtk_widget_hide(cerr_spin);
        gtk_widget_hide(cerr_label2);
    }
}

/*function to set the protocol default text*/
static gboolean set_combo_text(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer userdata)
{
	gchar *string;
	gtk_tree_model_get(model, iter, 0, &string, -1);
	
	/*compare the value from the details file with the current value
	 *and set it if it matches*/
	if(g_ascii_strcasecmp((gchar *)userdata, string)== 0)
	{	
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(protocol_combo), iter);
		g_free(string);
		return TRUE;
	}
	
	g_free(string);
	return FALSE;
}

/*display the details dialog*/
gboolean accdlg_run(gint profile, gint newaccount)
{
	GtkWidget *dialog, *iconcolour_button, *plginfo_button;
	GtkWidget *accname_label, *hostname_label, *port_label, *username_label, *password_label, *protocol_label, *icon_label;
	dlgtable main_table;
    GtkWidget *v_box_details;
	gchar port_str[G_ASCII_DTOSTR_BUF_SIZE];
	gint result= 0;
    gboolean saved= FALSE;
	mtc_account *pcurrent= NULL;
	GSList *plgcurrent= plglist;
	mtc_plugin *pitem= NULL;
	mtc_icon *picon= NULL;

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
	gtk_entry_set_max_length(GTK_ENTRY(port_entry), G_ASCII_DTOSTR_BUF_SIZE);
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

#ifdef MTC_NOTMINIMAL
	/*setup filter stuff*/
	filter_checkbox= gtk_check_button_new_with_label(S_CONFIGDLG_ENABLEFILTERS);
  	g_signal_connect(G_OBJECT(filter_checkbox), "clicked", G_CALLBACK(filter_checkbox_pressed), NULL);
	filter_button= gtk_button_new_with_label(S_CONFIGDLG_CONFIGFILTERS);
  	g_signal_connect(G_OBJECT(filter_button), "clicked", G_CALLBACK(filter_button_pressed), &profile);
#endif /*MTC_NOTMINIMAL*/

	/*add the plugin protocol names to the combo box*/
	while(plgcurrent!= NULL)
	{
		pitem= (mtc_plugin *)plgcurrent->data;
		gtk_combo_box_append_text(GTK_COMBO_BOX(protocol_combo), pitem->name);
		plgcurrent= g_slist_next(plgcurrent);
	}

	/*setup the icon colour info*/
	icon_label= gtk_label_new(S_CONFIGDLG_ICON_COLOUR);
	
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
	    g_ascii_dtostr(port_str, G_ASCII_DTOSTR_BUF_SIZE, (gdouble)pitem->default_port);
		gtk_entry_set_text(GTK_ENTRY(port_entry), port_str);
		gtk_entry_set_text(GTK_ENTRY(username_entry), pcurrent->username);
		gtk_entry_set_text(GTK_ENTRY(password_entry), pcurrent->password);

		/*set the combobox to first entry and get the iterator*/
		model= gtk_combo_box_get_model(GTK_COMBO_BOX(protocol_combo));
		if(!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(protocol_combo), &iter))
				err_exit(S_CONFIGDLG_ERR_COMBO_ITER);
				
		/*loop through listbox entries to see which is selected and run the details dialog for the account*/
		/*if not found for whatever reason, default to the first in the list*/
		if((pitem= plg_find(pcurrent->plgname))== NULL)
			pitem= plglist->data;

		gtk_tree_model_foreach(model, set_combo_text, (gchar *)pitem->name);
		/*gtk_tree_model_foreach(model, set_combo_text, pcurrent->plgname);*/

#ifdef MTC_NOTMINIMAL
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filter_checkbox), pcurrent->runfilter);
#endif /*MTC_NOTMINIMAL*/	
    }
	/*set the default pop port if details could not be read*/
	else
	{
		/*set the initial combo stuff*/
		protocol_combo_changed(GTK_COMBO_BOX(protocol_combo));
	
		/*set the default port value*/
		acclist= create_account();
		pcurrent= get_account(profile);
 	}

    picon= &pcurrent->icon;
  	g_signal_connect(G_OBJECT(iconcolour_button), "clicked", G_CALLBACK(details_iconcolour_button_pressed), picon);
	
#ifdef MTC_NOTMINIMAL
	gtk_widget_set_sensitive(filter_button, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(filter_checkbox)));
#endif /*MTC_NOTMINIMAL*/
    tbl_init(&main_table, 8, 3);

	gtk_table_set_col_spacings(GTK_TABLE(main_table.widget), 10);
	gtk_table_set_row_spacings(GTK_TABLE(main_table.widget), 10);
    tbl_addcol_new(&main_table, accname_label, 0, 1, 0, 0);
    tbl_addcol(&main_table, accname_entry, 1, 1, 0, 0);
    tbl_addcol_new(&main_table, hostname_label, 0, 1, 0, 0);
    tbl_addcol(&main_table, hostname_entry, 1, 1, 0, 0);
	tbl_addcol_new(&main_table, port_label, 0, 1, 0, 0);
    tbl_addcol(&main_table, port_entry, 1, 1, 0, 0);
	tbl_addcol_new(&main_table, username_label, 0, 1, 0, 0);
    tbl_addcol(&main_table, username_entry, 1, 1, 0, 0);
	tbl_addcol_new(&main_table, password_label, 0, 1, 0, 0);
    tbl_addcol(&main_table, password_entry, 1, 1, 0, 0);
	tbl_addcol_new(&main_table, protocol_label, 0, 1, 0, 0);
    tbl_addcol(&main_table, protocol_combo, 1, 1, 0, 0);
    tbl_addcol_new(&main_table, plginfo_button, 2, 1, 0, 0);
	
    tbl_init(&dicon_table, 1, 3);
	
    gtk_table_set_col_spacings(GTK_TABLE(dicon_table.widget), 5);
    tbl_addcol_new(&dicon_table, picon->image, 0, 1, GTK_FILL| GTK_EXPAND| GTK_SHRINK, GTK_FILL| GTK_EXPAND| GTK_SHRINK);
    tbl_addcol(&dicon_table, iconcolour_button, 1, 1, GTK_FILL| GTK_SHRINK, GTK_FILL| GTK_EXPAND| GTK_SHRINK);
    tbl_addcol_new(&main_table, icon_label, 0, 1, 0, 0);
    tbl_addcol(&main_table, dicon_table.widget, 1, 1, 0, 0);
#ifdef MTC_NOTMINIMAL
    tbl_addcol_new(&main_table, filter_checkbox, 0, 1, 0, 0);
    tbl_addcol(&main_table, filter_button, 1, 1, GTK_SHRINK| GTK_FILL, GTK_FILL| GTK_EXPAND| GTK_SHRINK);
#endif /*MTC_NOTMINIMAL*/
    gtk_container_set_border_width(GTK_CONTAINER(main_table.widget), 10);
	
	v_box_details= gtk_vbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(v_box_details), main_table.widget, FALSE, FALSE, 10);
	
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
				saved= acc_save(profile);
			break;
			/*if Cancel set saved to 1 so that the dialog will exit*/
			case GTK_RESPONSE_REJECT:
			{
				/*we need to check if a filter file was created and remove it if it was*/
				if(newaccount)
				{
					gchar filterfile[NAME_MAX+ 1];
					memset(filterfile, '\0', sizeof(filterfile));

					mtc_file(filterfile, FILTER_FILE, profile);

					if((IS_FILE(filterfile))&& (g_remove(filterfile)== -1))
						err_exit(S_CONFIGDLG_ERR_REMOVE_FILE, filterfile);
				
					/*we also must remove the account from the linked list*/
					remove_account(profile);
				}
			}
			default:
				saved= TRUE;
		}
	}

    /*we MUST remove the image from the table before destroying, otherwise it will also be destroyed*/
    gtk_container_remove(GTK_CONTAINER(dicon_table.widget), picon->image);
	
    /*destroy the dialog now that it is finished*/
	gtk_widget_destroy(dialog);

	return TRUE;
}

/*display the config notebook dialog*/
GtkWidget *cfgdlg_run(GtkWidget *dialog)
{
	GtkWidget *notebook;
	GtkWidget *notebook_title[3];
	GtkWidget *h_box_page2[2], *v_box_array[4];
	GtkWidget *mailprog_label, *delay_label, *cerr_label;
#ifdef MTC_NOTMINIMAL
    GtkWidget *nmailcmd_label;
#endif /*MTC_NOTMINIMAL*/
	GtkWidget *treescroll;
	GtkAdjustment *delay_adj, *cerr_adj;
	GtkWidget *add_button, *remove_button, *edit_button, *close_button, *save_button;
	GtkTooltips *multi_tooltip;
	gchar window_title[30];
	dlgtable main_table[2];
	GtkWidget *cerr_box;
    GSList *pcurrent= NULL;
	mtc_account *pcurrent_data= NULL;
	GtkTreeIter iter;
	mtc_plugin *pitem= NULL;
    mtc_icon *picon= NULL;

	/*set the config window title*/
	g_snprintf(window_title, 30, S_CONFIGDLG_CONFIG_TITLE, PACKAGE);
	
	/*setup the notebook*/
	notebook= gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
	notebook_title[0]= gtk_label_new(S_CONFIGDLG_TAB_GENERAL);
	notebook_title[1]= gtk_label_new("Display");
	notebook_title[2]= gtk_label_new(S_CONFIGDLG_TAB_ACCOUNTS);
  
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
	picon= &config.icon;
	
    /*set the icon button to open the colour dialog*/
	config_iconcolour_button= gtk_button_new_with_label(S_CONFIGDLG_SETICONCOLOUR);
	gtk_widget_set_sensitive(GTK_WIDGET(config_iconcolour_button), FALSE);
  	g_signal_connect(G_OBJECT(config_iconcolour_button), "clicked", G_CALLBACK(config_iconcolour_button_pressed), picon);
	
  	g_signal_connect(G_OBJECT(multi_accounts_checkbox), "clicked", G_CALLBACK(multi_checkbox_pressed), picon);

#ifdef MTC_EXPERIMENTAL
    /*setup summary stuff*/
	summary_checkbox= gtk_check_button_new_with_label(S_CONFIGDLG_ENABLE_SUMMARY);
  	g_signal_connect(G_OBJECT(summary_checkbox), "clicked", G_CALLBACK(summary_checkbox_pressed), NULL);
    summary_button= gtk_button_new_with_label(S_CONFIGDLG_SUMOPTS);
	gtk_widget_set_sensitive(GTK_WIDGET(summary_button), FALSE);
  	g_signal_connect(G_OBJECT(summary_button), "clicked", G_CALLBACK(summary_button_pressed), NULL);
 #endif

    /*setup connection error stuff*/
    cerr_label= gtk_label_new(S_CONFIGDLG_SHOW_NET_ERRDLG);
	gtk_misc_set_alignment(GTK_MISC(cerr_label), 0, 0.5);
	cerr_combo= gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(cerr_combo), S_CONFIGDLG_SHOW_NEVER);
    gtk_combo_box_append_text(GTK_COMBO_BOX(cerr_combo), S_CONFIGDLG_SHOW_ALWAYS);
    gtk_combo_box_append_text(GTK_COMBO_BOX(cerr_combo), S_CONFIGDLG_SHOW_EVERY);
    cerr_adj= (GtkAdjustment *)gtk_adjustment_new(2.0, 2.0, 5.0, 1.0, 1.0, 1.0);
	cerr_spin= gtk_spin_button_new(cerr_adj, 1.0, 0);
    cerr_label2= gtk_label_new(S_CONFIGDLG_FAILED_CONNECTIONS);
	g_signal_connect(G_OBJECT(GTK_COMBO_BOX(cerr_combo)), "changed", G_CALLBACK(cerr_combo_changed), NULL);
   
#ifdef MTC_NOTMINIMAL
    /*run command on new mail stuff*/
	nmailcmd_entry= gtk_entry_new();
    nmailcmd_label= gtk_label_new(S_CONFIGDLG_RUN_NEWMAIL_CMD);
	gtk_misc_set_alignment(GTK_MISC(nmailcmd_label), 0, 0.5);
	gtk_entry_set_max_length(GTK_ENTRY(nmailcmd_entry), NAME_MAX+ 1);
#endif /*MTC_NOTMINIMAL*/

	/*set the config details*/
    /*TODO moved to main.c*/
	/*if(!cfg_read())
        err_dlg(GTK_MESSAGE_WARNING, "Error reading config");*/

	gtk_widget_set_sensitive(GTK_WIDGET(picon->image), FALSE);
	gtk_entry_set_text(GTK_ENTRY(mailprog_entry), config.mail_program);

#ifdef MTC_NOTMINIMAL
	gtk_entry_set_text(GTK_ENTRY(nmailcmd_entry), config.nmailcmd);
#endif /*MTC_NOTMINIMAL*/
    
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(delay_spin), (gdouble)config.check_delay);
#ifdef MTC_NOTMINIMAL
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(iconsize_checkbox), (config.icon_size<= 16)? TRUE: FALSE);
#endif /*MTC_NOTMINIMAL*/
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(multi_accounts_checkbox), config.multiple);
#ifdef MTC_EXPERIMENTAL
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(summary_checkbox), config.run_summary);
#endif
    gtk_combo_box_set_active(GTK_COMBO_BOX(cerr_combo), (config.err_freq< 2)? config.err_freq: 2);
    if(config.err_freq> 1)
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(cerr_spin), (gdouble)config.err_freq);
	
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
  		err_exit(S_CONFIGDLG_ERR_COMBO_ITER);
	gtk_list_store_clear(store);

	/*create the scroll for the listbox*/
	treescroll= gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(treescroll), account_listbox);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(treescroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(treescroll), GTK_SHADOW_IN);

	/*add the accounts to the list*/
	/*read_accounts();*/
	pcurrent= acclist;

	while(pcurrent!= NULL)
	{
		gtk_list_store_prepend(store, &iter);
		pcurrent_data= (mtc_account *)pcurrent->data;

		/*find the plugin and add it to the listbox*/
		/*Message box here if we cannot find the plugin*/
		if((pitem= plg_find(pcurrent_data->plgname))== NULL)
			err_dlg(GTK_MESSAGE_WARNING, S_CONFIGDLG_FIND_PLUGIN_MSG, pcurrent_data->plgname, pcurrent_data->accname);
		
		/*add to the list, if we cannot find it, report that it is not found*/
		gtk_list_store_set(store, &iter, ACCOUNT_COLUMN, pcurrent_data->accname, PROTOCOL_COLUMN, (pitem!= NULL)? pitem->name: S_CONFIGDLG_ERR_FIND_PLUGIN, -1);
  		pcurrent= g_slist_next(pcurrent);
	}
  	g_object_unref(G_OBJECT(store));
  
	/*setup the listbox buttons here and the functions called when clicked*/
	edit_button= gtk_button_new_from_stock(GTK_STOCK_PROPERTIES);
	add_button= gtk_button_new_from_stock(GTK_STOCK_ADD);
	remove_button= gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	g_signal_connect(G_OBJECT(edit_button), "clicked", G_CALLBACK(edit_button_pressed), NULL);
	g_signal_connect(G_OBJECT(add_button), "clicked", G_CALLBACK(add_button_pressed), NULL);
	g_signal_connect(G_OBJECT(remove_button), "clicked", G_CALLBACK(remove_button_pressed), NULL);
	
    /*tab 1 widgets*/
    tbl_init(&main_table[0], 4, 3);
    gtk_table_set_col_spacings(GTK_TABLE(main_table[0].widget), 10);
	gtk_table_set_row_spacings(GTK_TABLE(main_table[0].widget), 10);
    tbl_addcol_new(&main_table[0], delay_label, 0, 1, 0, 0);
    tbl_addcol(&main_table[0], delay_spin, 1, 1, 0, 0);
    tbl_addcol_new(&main_table[0], mailprog_label, 0, 1, 0, 0);
    tbl_addcol(&main_table[0], mailprog_entry, 1, 1, 0, 0);
    tbl_addcol_new(&main_table[0], cerr_label, 0, 2, 0, 0);
    
    cerr_box= gtk_hbox_new(FALSE, 10);
    gtk_box_pack_start(GTK_BOX(cerr_box), cerr_combo, FALSE, FALSE, 20);
    gtk_box_pack_start(GTK_BOX(cerr_box), cerr_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cerr_box), cerr_label2, FALSE, FALSE, 0);
    tbl_addcol_new(&main_table[0], cerr_box, 0, 3, 0, 0);
    gtk_container_set_border_width(GTK_CONTAINER(main_table[0].widget), 10);
    
#ifdef MTC_NOTMINIMAL
    tbl_addcol_new(&main_table[0], nmailcmd_label, 0, 1, 0, 0);
    tbl_addcol(&main_table[0], nmailcmd_entry, 1, 1, 0, 0);
#endif /*MTC_NOTMINIMAL*/
	
    /*tab 2 widgets*/
    tbl_init(&main_table[1], 3, 3);
	gtk_table_set_col_spacings(GTK_TABLE(main_table[1].widget), 10);
	gtk_table_set_row_spacings(GTK_TABLE(main_table[1].widget), 10);
    tbl_addcol_new(&main_table[1], multi_accounts_checkbox, 0, 3, 0, 0);
    
    tbl_init(&cicon_table, 1, 3);
    tbl_addcol_new(&cicon_table, picon->image, 0, 1, GTK_FILL| GTK_EXPAND| GTK_SHRINK, GTK_FILL| GTK_EXPAND| GTK_SHRINK);
    tbl_addcol(&cicon_table, config_iconcolour_button, 1, 1, GTK_FILL| GTK_SHRINK, GTK_FILL| GTK_EXPAND| GTK_SHRINK);
    tbl_addcol_new(&main_table[1], cicon_label, 0, 1, 0, 0);
    tbl_addcol(&main_table[1], cicon_table.widget, 1, 1, 0, 0);
#ifdef MTC_EXPERIMENTAL
    tbl_addcol_new(&main_table[1], summary_checkbox, 0, 1, 0, 0);
    tbl_addcol(&main_table[1], summary_button, 1, 1, 0, 0);
#endif
#ifdef MTC_NOTMINIMAL
    tbl_addcol_new(&main_table[1], iconsize_checkbox, 0, 1, 0, 0);
#endif /*MTC_NOTMINIMAL*/
    gtk_container_set_border_width(GTK_CONTAINER(main_table[1].widget), 10);
	
    v_box_array[0]= gtk_vbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(v_box_array[0]), main_table[0].widget, FALSE, FALSE, 10);
	
	v_box_array[1]= gtk_vbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(v_box_array[1]), main_table[1].widget, FALSE, FALSE, 10);
	
    h_box_page2[0]= gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(h_box_page2[0]), treescroll, TRUE, TRUE, 10);
	h_box_page2[1]= gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(h_box_page2[1]), add_button, TRUE, TRUE, 10);
	gtk_box_pack_start(GTK_BOX(h_box_page2[1]), remove_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(h_box_page2[1]), edit_button, TRUE, TRUE, 10);
	v_box_array[2]= gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(v_box_array[2]), h_box_page2[0], TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(v_box_array[2]), h_box_page2[1], FALSE, FALSE, 5);
  
	/*create the notebook pages*/
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), v_box_array[0], notebook_title[0]);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), v_box_array[1], notebook_title[1]);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), v_box_array[2], notebook_title[2]);
	v_box_array[3]= gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(v_box_array[3]), notebook, FALSE, FALSE, 10);
  
	/*create the dialog and the buttons*/
	dialog= gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), window_title);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), v_box_array[3]);
	
    /*create the buttons for the dialog and call their functions when pressed*/
	save_button= gtk_button_new_from_stock(GTK_STOCK_SAVE);
	g_signal_connect(G_OBJECT(save_button), "clicked", G_CALLBACK(cfg_save), NULL);
    close_button= gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(close_button), "clicked", G_CALLBACK(close_button_pressed), NULL);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), save_button, TRUE, TRUE, 0);			  
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), close_button, TRUE, TRUE, 0);			  
	g_signal_connect(G_OBJECT(dialog), "delete_event", G_CALLBACK(delete_event), NULL);
	gtk_widget_show_all(dialog);
	
    /*hide the two widgets initially*/
    if(config.err_freq< 2)
    {
        gtk_widget_hide(cerr_spin);
        gtk_widget_hide(cerr_label2);
    }
	/*if(cicon)
		g_object_unref(cicon);*/
	return dialog;
}


