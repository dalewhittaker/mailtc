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

#include "core.h"

/*widget variables used for most functions*/
static GtkWidget *filter_combo1[MAX_FILTER_EXP];
static GtkWidget *filter_combo2[MAX_FILTER_EXP];
static GtkWidget *filter_entry[MAX_FILTER_EXP];
static GtkWidget *filter_radio[2];
static GtkWidget *clear_button;

/*function to write the filter information to the file*/
static int write_filter_info(mail_details *paccount)
{

	FILE *outfile;
	char outfilename[NAME_MAX];
	int i= 0;
	int valid= 0;

	/*first check if we have values*/
	for(i= 0; i< MAX_FILTER_EXP; i++)
	{
		if(g_ascii_strcasecmp(gtk_entry_get_text(GTK_ENTRY(filter_entry[i])), "")!= 0)
			++valid;
	}

	if(!valid)
		return(!run_error_dialog(S_FILTERDLG_NO_FILTERS));
	
	/*get the full path of the filter file*/
	memset(outfilename, '\0', NAME_MAX);
	get_account_file(outfilename, FILTER_FILE, paccount->id);

	/*first check if file can be written to*/
	if((IS_FILE(outfilename))&& (remove(outfilename)== -1))
		error_and_log(S_FILTERDLG_ERR_REMOVE_FILE, outfilename);
	
	/*open the file*/
	if((outfile= fopen(outfilename, "wt"))== NULL)
		error_and_log(S_FILTERDLG_ERR_OPEN_FILE, outfilename);
	
	/*output whether to match all or match any*/
	(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(filter_radio[0])))? fprintf(outfile, "<AND>\n"): fprintf(outfile, "<OR>\n");
			
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
				error_and_log(S_FILTERDLG_ERR_COMBO_ITER);
			gtk_tree_model_get(model2, &iter2, 0, &str2, -1);
			
			if(g_ascii_strcasecmp(str2, S_FILTERDLG_COMBO_CONTAINS)!= 0) fprintf(outfile, "<NOT>");
			g_free(str2);
			
			/*get the active combo value for sender/subject and output*/
			model1= gtk_combo_box_get_model(GTK_COMBO_BOX(filter_combo1[i]));
			if(!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(filter_combo1[i]), &iter1))
				error_and_log(S_FILTERDLG_ERR_COMBO_ITER);
			gtk_tree_model_get(model1, &iter1, 0, &str1, -1);
			(g_ascii_strcasecmp(str1, S_FILTERDLG_COMBO_SENDER)== 0)? fprintf(outfile, "<SENDER>"): fprintf(outfile, "<SUBJECT>");
			g_free(str1);
			
			/*output the filter search string*/
			fprintf(outfile, "%s\n", gtk_entry_get_text(GTK_ENTRY(filter_entry[i])));
		}
	}

	/*close the file*/
	if(fclose(outfile)== EOF)
		error_and_log(S_FILTERDLG_ERR_CLOSE_FILE, outfilename);

	/*now we must read the filter info back in!*/
	/*TODO this should never happen, but probably a better thing to do is error*/
	if(!read_filter_info(paccount))
		paccount->runfilter= 0;
	
	return 1;
}

/*function to read the filter information in*/
int read_filter_info(mail_details *paccount)
{
	FILE *infile;
	char infilename[NAME_MAX];
	char line[FILTERSTRING_LEN+ strlen("<NOT>")+ strlen("<SUBJECT>")+ 1];
	char *pstring= NULL;
	int i= 0;
	int retval= 0;
	filter_details *pfilter;

	/*get the full path of the filter file*/
	memset(infilename, '\0', NAME_MAX);
	get_account_file(infilename, FILTER_FILE, paccount->id);

	/*first test if there is an existing file*/
	if(!IS_FILE(infilename))
		return 0;
	
	/*open the file for reading and clear the struct*/
	if((infile= fopen(infilename, "rt"))== NULL)
		error_and_log(S_FILTERDLG_ERR_OPEN_FILE, infilename);
	
	/*allocate memory for the filter struct*/
	paccount->pfilters= g_malloc0(sizeof(filter_details));
	pfilter= paccount->pfilters;
	
	/*set default to contains and sender*/
	for(i= 0; i< MAX_FILTER_EXP; ++i)
	{
		pfilter->contains[i]= 1;
		pfilter->subject[i]= 0;
	}
	i= 0;
	
	/*get the first line (match all or any)*/
	fgets(line, sizeof(line), infile);
	if(strncmp(line, "<AND>", strlen("<AND>"))== 0)
		pfilter->matchall= 1;
	if(strncmp(line, "<OR>", strlen("<OR>"))== 0)
		pfilter->matchall= 0;
	else
		pfilter->matchall= 1;
	
	/*for each subsequent line*/
	while(!feof(infile)&& (i< MAX_FILTER_EXP))
	{
		/*clear the line and set the pointer to it*/
		pstring= line;
		memset(line, '\0', sizeof(line));
		
		/*get contains/not contains*/
		fgets(line, sizeof(line), infile);
		if(strncmp(pstring, "<NOT>", strlen("<NOT>"))== 0)
		{
			pfilter->contains[i]= 0;
			pstring+= strlen("<NOT>");
		}
			
		/*get subject/sender*/
		if(strncmp(pstring, "<SUBJECT>", strlen("<SUBJECT>"))== 0)
		{
			pfilter->subject[i]= 1;
			pstring+= strlen("<SUBJECT>");
		}
		else if(strncmp(pstring, "<SENDER>", strlen("<SENDER>"))== 0)
		{
			pfilter->subject[i]= 0;
			pstring+= strlen("<SENDER>");
		}
		
		/*get the filter search string*/
		if(strlen(pstring) > 1)
		{	
			g_strlcpy(pfilter->search_string[i], pstring, FILTERSTRING_LEN);
			if(pfilter->search_string[i][strlen(pfilter->search_string[i]) -1]== '\n')
				pfilter->search_string[i][strlen(pfilter->search_string[i]) -1]= '\0';
			retval= 1;
		}

		++i;
	}

	/*close the file*/
	if(fclose(infile)== EOF)
		error_and_log(S_FILTERDLG_ERR_CLOSE_FILE, infilename);

	return(retval);
}

/*button to clear the filter entries*/
static void clear_button_pressed(void)
{
	int i= 0;	
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
int run_filter_dialog(mail_details *paccount)
{
	GtkWidget *dialog;
	GtkWidget *filter_label;
	GtkWidget *main_table, *v_box_filter;
	int i= -1;
	gint result= 0, saved= 0;
	filter_details *pfilter= paccount->pfilters;
	char *label= NULL;
		
	/*create the label*/
	label= g_malloc0(strlen(S_FILTERDLG_LABEL_SELECT) + 5);
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
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filter_radio[0]), (pfilter->matchall)? TRUE: FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filter_radio[1]), (pfilter->matchall)? FALSE: TRUE);
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
				saved= write_filter_info(paccount);
			break;
			/*if Cancel set saved to 1 so that the dialog will exit*/
			case GTK_RESPONSE_REJECT:
			default:
				saved= 1;
		}
	}
	/*destroy the dialog now that it is finished*/
	gtk_widget_destroy(dialog);
	
	return 1;
}


