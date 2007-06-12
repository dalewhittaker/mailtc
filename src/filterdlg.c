/* filterdlg.c
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
#include <gtk/gtkstock.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtktable.h>
#include <gtk/gtkradiobutton.h>

#include "filefunc.h"
#include "filterdlg.h"

/*TODO don't like lengths, fix it*/
#define FILTERSTRING_LEN 100
#define MAX_FILTER_EXP 5 

#define SFILTER_AND "<AND>"
#define SFILTER_OR "<OR>"
#define SFILTER_NOT "<NOT>"
#define SFILTER_SENDER "<SENDER>"
#define SFILTER_SUBJECT "<SUBJECT>"

/*widget variables used for most functions*/
static GtkWidget *filter_combo1[MAX_FILTER_EXP];
static GtkWidget *filter_combo2[MAX_FILTER_EXP];
static GtkWidget *filter_entry[MAX_FILTER_EXP];
static GtkWidget *filter_radio[2];
static GtkWidget *clear_button;

/*function to write the filter information to the file*/
static gboolean filter_write(mtc_account *paccount)
{

	FILE *outfile;
	gchar outfilename[NAME_MAX];
	gint i= 0;
	gint valid= 0;

	/*first check if we have values*/
	for(i= 0; i< MAX_FILTER_EXP; i++)
	{
		if(g_ascii_strcasecmp(gtk_entry_get_text(GTK_ENTRY(filter_entry[i])), "")!= 0)
			++valid;
	}

	if(!valid)
		return(!err_dlg(GTK_MESSAGE_WARNING, S_FILTERDLG_NO_FILTERS));
	
	/*get the full path of the filter file*/
	memset(outfilename, '\0', NAME_MAX);
	mtc_file(outfilename, FILTER_FILE, paccount->id);

	/*first check if file can be written to*/
	if((IS_FILE(outfilename))&& (g_remove(outfilename)== -1))
		err_exit(S_FILTERDLG_ERR_REMOVE_FILE, outfilename);
	
	/*open the file*/
	if((outfile= g_fopen(outfilename, "wt"))== NULL)
		err_exit(S_FILTERDLG_ERR_OPEN_FILE, outfilename);
	
	/*output whether to match all or match any*/
	g_fprintf(outfile, (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(filter_radio[0])))? SFILTER_AND"\n": SFILTER_OR"\n");
			
	/*for each filter entry*/
	for(i= 0; i< MAX_FILTER_EXP; i++)
	{
		/*test if the entry is empty*/
		if(g_ascii_strcasecmp(gtk_entry_get_text(GTK_ENTRY(filter_entry[i])), "")!= 0)
		{
			GtkTreeModel *model1, *model2;
			GtkTreeIter iter1, iter2;
			gchar *str1= NULL, *str2= NULL;
		
			/*get the active combo value for contains/does not contain and output*/
			model2= gtk_combo_box_get_model(GTK_COMBO_BOX(filter_combo2[i]));
			if(!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(filter_combo2[i]), &iter2))
				err_exit(S_FILTERDLG_ERR_COMBO_ITER);
			gtk_tree_model_get(model2, &iter2, 0, &str2, -1);
			
			if(g_ascii_strcasecmp(str2, S_FILTERDLG_COMBO_CONTAINS)!= 0) g_fprintf(outfile, SFILTER_NOT);
			g_free(str2);
			
			/*get the active combo value for sender/subject and output*/
			model1= gtk_combo_box_get_model(GTK_COMBO_BOX(filter_combo1[i]));
			if(!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(filter_combo1[i]), &iter1))
				err_exit(S_FILTERDLG_ERR_COMBO_ITER);
			gtk_tree_model_get(model1, &iter1, 0, &str1, -1);
			g_fprintf(outfile, (g_ascii_strcasecmp(str1, S_FILTERDLG_COMBO_SENDER)== 0)? SFILTER_SENDER: SFILTER_SUBJECT);
			g_free(str1);
			
			/*output the filter search string*/
			g_fprintf(outfile, "%s\n", gtk_entry_get_text(GTK_ENTRY(filter_entry[i])));
		}
	}

	/*close the file*/
	if(fclose(outfile)== EOF)
		err_exit(S_FILTERDLG_ERR_CLOSE_FILE, outfilename);

	/*now we must read the filter info back in!*/
	/*TODO this should never happen, but probably a better thing to do is error*/
	if(!filter_read(paccount))
		paccount->runfilter= FALSE;
	
	return TRUE;
}

/*function to read the filter information in*/
gboolean filter_read(mtc_account *paccount)
{
	FILE *infile;
	gchar infilename[NAME_MAX];
	gchar line[FILTERSTRING_LEN+ 15]; /*strlen("<NOT>")+ strlen("<SUBJECT>")+ 1;*/
	gchar *pstring= NULL;
	gint i= 0;
	gboolean retval= 0;
	mtc_filter *pfilter;
    guint notlen, subjlen, senderlen;

    notlen= strlen(SFILTER_NOT);
    subjlen= strlen(SFILTER_SUBJECT);
    senderlen= strlen(SFILTER_SENDER);

	/*get the full path of the filter file*/
	memset(infilename, '\0', NAME_MAX);
	mtc_file(infilename, FILTER_FILE, paccount->id);

	/*first test if there is an existing file*/
	if(!IS_FILE(infilename))
		return FALSE;
	
	/*open the file for reading and clear the struct*/
	if((infile= g_fopen(infilename, "rt"))== NULL)
		err_exit(S_FILTERDLG_ERR_OPEN_FILE, infilename);
	
	/*allocate memory for the filter struct*/
	paccount->pfilters= (mtc_filter *)g_malloc0(sizeof(mtc_filter));
	pfilter= paccount->pfilters;
	
	/*set default to contains and sender*/
	for(i= 0; i< MAX_FILTER_EXP; ++i)
	{
		pfilter->contains[i]= TRUE;
		pfilter->subject[i]= FALSE;
	}
	i= 0;
	
	/*get the first line (match all or any)*/
	fgets(line, sizeof(line), infile);
	if(g_ascii_strncasecmp(line, SFILTER_AND, strlen(SFILTER_AND))== 0)
		pfilter->matchall= TRUE;
	if(g_ascii_strncasecmp(line, SFILTER_OR, strlen(SFILTER_OR))== 0)
		pfilter->matchall= FALSE;
	else
		pfilter->matchall= TRUE;
	
	/*for each subsequent line*/
	while(!feof(infile)&& (i< MAX_FILTER_EXP))
	{
		/*clear the line and set the pointer to it*/
		pstring= line;
		memset(line, '\0', sizeof(line));
		
		/*get contains/not contains*/
		fgets(line, sizeof(line), infile);
		if(g_ascii_strncasecmp(pstring, SFILTER_NOT, notlen)== 0)
		{
			pfilter->contains[i]= FALSE;
			pstring+= notlen;
		}
			
		/*get subject/sender*/
		if(g_ascii_strncasecmp(pstring, SFILTER_SUBJECT, subjlen)== 0)
		{
			pfilter->subject[i]= TRUE;
			pstring+= subjlen;
		}
		else if(g_ascii_strncasecmp(pstring, SFILTER_SENDER, senderlen)== 0)
		{
			pfilter->subject[i]= FALSE;
			pstring+= senderlen;
		}
		
		/*get the filter search string*/
		if(strlen(pstring) > 1)
		{	
			g_strlcpy(pfilter->search_string[i], pstring, FILTERSTRING_LEN);
			if(pfilter->search_string[i][strlen(pfilter->search_string[i]) -1]== '\n')
				pfilter->search_string[i][strlen(pfilter->search_string[i]) -1]= '\0';
			retval= TRUE;
		}

		++i;
	}

	/*close the file*/
	if(fclose(infile)== EOF)
		err_exit(S_FILTERDLG_ERR_CLOSE_FILE, infilename);

	return(retval);
}

/*button to clear the filter entries*/
static void clear_button_pressed(void)
{
	gint i= 0;	
	for(i= 0; i< MAX_FILTER_EXP; ++i)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(filter_combo1[i]), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(filter_combo2[i]), 0);
		gtk_entry_set_text(GTK_ENTRY(filter_entry[i]), "");
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filter_radio[0]), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filter_radio[1]), FALSE);

}
		
/*display the filter dialog*/
gboolean filterdlg_run(mtc_account *paccount)
{
	GtkWidget *dialog;
	GtkWidget *filter_label;
	GtkWidget *main_table, *v_box_filter;
	gint i= -1;
	gint result= 0;
    gboolean saved= FALSE;
	mtc_filter *pfilter= paccount->pfilters;
	gchar *label= NULL;
		
	/*create the label*/
	label= (gchar *)g_malloc0(sizeof(gchar)* (strlen(S_FILTERDLG_LABEL_SELECT)+ 5));
	g_snprintf(label, strlen(S_FILTERDLG_LABEL_SELECT)+ 4, S_FILTERDLG_LABEL_SELECT, MAX_FILTER_EXP);
	filter_label= gtk_label_new(label);
	g_free(label);
	
	main_table= gtk_table_new(MAX_FILTER_EXP+ 2, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(main_table), filter_label, 0, 1, 0, 1);
	
	/*create n number of widgets*/
	while(i++ < (MAX_FILTER_EXP- 1))
	{
	
		/*create the sender/subject combo*/
		filter_combo1[i]= gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(filter_combo1[i]), S_FILTERDLG_COMBO_SENDER);
		gtk_combo_box_append_text(GTK_COMBO_BOX(filter_combo1[i]), S_FILTERDLG_COMBO_SUBJECT);
		gtk_combo_box_set_active(GTK_COMBO_BOX(filter_combo1[i]), 0);

		/*create the contains/does not contain combo*/
		filter_combo2[i]= gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(filter_combo2[i]), S_FILTERDLG_COMBO_CONTAINS);
		gtk_combo_box_append_text(GTK_COMBO_BOX(filter_combo2[i]), S_FILTERDLG_COMBO_NOTCONTAINS);
		gtk_combo_box_set_active(GTK_COMBO_BOX(filter_combo2[i]), 0);

		/*create the filter edit box*/
		filter_entry[i]= gtk_entry_new();
		gtk_entry_set_max_length(GTK_ENTRY(filter_entry[i]), FILTERSTRING_LEN);
		gtk_entry_set_width_chars(GTK_ENTRY(filter_entry[i]), 30); 
		
		/*pack the stuff into the boxes*/
		gtk_table_set_col_spacings(GTK_TABLE(main_table), 10);
		gtk_table_set_row_spacings(GTK_TABLE(main_table), 10);
		gtk_table_attach_defaults(GTK_TABLE(main_table), filter_combo1[i], 0, 1, i+ 1, i+ 2);
		gtk_table_attach_defaults(GTK_TABLE(main_table), filter_combo2[i], 1, 2, i+ 1, i+ 2);
		gtk_table_attach_defaults(GTK_TABLE(main_table), filter_entry[i], 2, 3, i+ 1, i+ 2);
		
		if(paccount->pfilters)
		{
			gtk_combo_box_set_active(GTK_COMBO_BOX(filter_combo1[i]), pfilter->subject[i]);
			gtk_combo_box_set_active(GTK_COMBO_BOX(filter_combo2[i]), !pfilter->contains[i]);
			gtk_entry_set_text(GTK_ENTRY(filter_entry[i]), pfilter->search_string[i]);
		}
	}
	
	/*set the button to clear the entries*/
	clear_button= gtk_button_new_with_label(S_FILTERDLG_BUTTON_CLEAR);
  	g_signal_connect(G_OBJECT(clear_button), "clicked", G_CALLBACK(clear_button_pressed), NULL);
	
	filter_radio[0]= gtk_radio_button_new_with_label(NULL, S_FILTERDLG_BUTTON_MATCHALL);
	filter_radio[1]= gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(filter_radio[0]), S_FILTERDLG_BUTTON_MATCHANY);
	if(paccount->pfilters)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filter_radio[0]), pfilter->matchall);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filter_radio[1]), pfilter->matchall);
	}
	
	gtk_table_attach(GTK_TABLE(main_table), clear_button, 2, 3, i+ 2, i+ 3, GTK_SHRINK, GTK_SHRINK, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(main_table), filter_radio[0], 0, 1, i+ 2 , i+ 3);
	gtk_table_attach_defaults(GTK_TABLE(main_table), filter_radio[1], 1, 2, i+ 2 , i+ 3);
	
	gtk_container_set_border_width(GTK_CONTAINER(main_table), 10);
	
	v_box_filter= gtk_vbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(v_box_filter), main_table, FALSE, FALSE, 10);
	
	/*create the filter dialog*/
	dialog= gtk_dialog_new_with_buttons(S_FILTERDLG_TITLE, NULL, GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), v_box_filter);
	gtk_widget_show_all(v_box_filter);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 80, 50);

	/*keep running dialog until details are saved (i.e all values entered)*/
	while(!saved)
	{
		result= gtk_dialog_run(GTK_DIALOG(dialog)); 
		switch(result)
		{
			/*if OK get the value of saved to check if details are saved correctly*/
			case GTK_RESPONSE_ACCEPT:
				saved= filter_write(paccount);
			break;
			/*if Cancel set saved to 1 so that the dialog will exit*/
			case GTK_RESPONSE_REJECT:
			default:
				saved= TRUE;
		}
	}
	/*destroy the dialog now that it is finished*/
	gtk_widget_destroy(dialog);
	
	return TRUE;
}


