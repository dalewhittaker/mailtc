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

/*define the filter element names that are used*/
#define ELEMENT_FILTERS  "filters"
#define ELEMENT_FILTER   "filter"
#define ELEMENT_MATCHALL "match_all"
#define ELEMENT_CONTAINS "contains"
#define ELEMENT_FIELD    "field"
#define ELEMENT_VALUE    "value"

#define INITIAL_FILTERS 5
#define FILTERSTRING_LEN 100

/*structs used for the filter widgets*/
typedef struct _filter_widgets
{
    GtkWidget *combo_field;
    GtkWidget *combo_contains;
    GtkWidget *entry_value;

} filter_widgets;

typedef struct _filters_widgets
{
    GtkWidget *radio_matchall[2];
    GtkWidget *button_clear;

    GSList *list;

} filters_widgets;

static filters_widgets widgets;

/*a static list of the filter fields.
 *NOTE this must be the same order as the hfield enum
 *otherwise it won't work.  Add new ones as needed*/
 static gchar ffield[][10]=
 {
    "From",
    "Subject",
    "To",
    "Cc"
 } ;

/*function to free any filter data*/
void free_filters(mtc_account *paccount)
{
	if(paccount->pfilters)
	{
        GSList *flist;

        /*first free the list if any, then free the filters struct*/
        flist= paccount->pfilters->list;
        g_slist_foreach(flist, (GFunc)g_free, NULL);
        g_slist_free(flist);

		g_free(paccount->pfilters);
		paccount->pfilters= NULL;
	}
}

/*function to store the filter struct*/
static gboolean filter_save(mtc_account *paccount)
{
    gint valid= 0;
    gint j;
    GSList *pcurrent= NULL;
    GSList *pwlist= NULL;
    mtc_filter *pnew= NULL;
    mtc_filters *pfilters= NULL;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *str= NULL;
    filter_widgets *pfwidgets= NULL;
	
	/*first check if we have values*/
    pwlist= widgets.list;
    while(pwlist!= NULL)
    {
        pfwidgets= (filter_widgets *)pwlist->data;
		if(g_ascii_strcasecmp(gtk_entry_get_text(GTK_ENTRY(pfwidgets->entry_value)), "")!= 0)
		{
            ++valid;
            break;
        }
        pwlist= g_slist_next(pwlist);
    }

	if(!valid)
		return(!err_dlg(GTK_MESSAGE_WARNING, S_FILTERDLG_NO_FILTERS));
	
    /*allocate new filter if it doesn't already exist*/
    if(paccount->pfilters== NULL)
        paccount->pfilters= (mtc_filters *)g_malloc0(sizeof(mtc_filters));

    pfilters= paccount->pfilters;
    if(pfilters)
        pcurrent= pfilters->list;
    
    /*enable the filter*/
    pfilters->enabled= TRUE;
 
    /*Get the matchall field*/
    pfilters->matchall= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widgets.radio_matchall[0]))? TRUE: FALSE;
 
    /*remove any existing list and members*/
    if(pfilters->list)
    {
        g_slist_foreach(pfilters->list, (GFunc)g_free, NULL);
        g_slist_free(pfilters->list);
        pfilters->list= NULL;
    }

    pwlist= widgets.list;
    pfwidgets= NULL;

    /*iterate through each widget struct in list*/
    while(pwlist!= NULL)
    {
        pfwidgets= (filter_widgets *)pwlist->data;
        
        /*if the value entry is not empty, add the filter to the list*/
		if(g_ascii_strcasecmp(gtk_entry_get_text(GTK_ENTRY(pfwidgets->entry_value)), "")!= 0)
        {
            /*add a new member*/
            pnew= (mtc_filter *)g_malloc0(sizeof(mtc_filter));   

			/*get the active combo value for contains/does not contain and output*/
			model= gtk_combo_box_get_model(GTK_COMBO_BOX(pfwidgets->combo_contains));
	
            if(!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(pfwidgets->combo_contains), &iter))
			{
                err_dlg(GTK_MESSAGE_WARNING, S_FILTERDLG_ERR_COMBO_ITER);
                g_free(pnew);
                return FALSE;
			}
            gtk_tree_model_get(model, &iter, 0, &str, -1);
			
            /*set the contains for each*/
			pnew->contains= (g_ascii_strcasecmp(str, S_FILTERDLG_COMBO_CONTAINS)== 0)? TRUE: FALSE;
            g_free(str);
			
            /*get the active field*/
            j= gtk_combo_box_get_active(GTK_COMBO_BOX(pfwidgets->combo_field));
            if(j== -1)
            {
                err_dlg(GTK_MESSAGE_WARNING, S_FILTERDLG_ERR_COMBO_ITER);
                g_free(pnew);
                return FALSE;
			}
		    pnew->field= j;
			
			/*output the filter search string*/
            g_strlcpy(pnew->search_string, gtk_entry_get_text(GTK_ENTRY(pfwidgets->entry_value)), sizeof(pnew->search_string));
		    
            /*now add to list*/
            pfilters->list= g_slist_prepend(pfilters->list, pnew);

            pcurrent= g_slist_next(pcurrent);
 
        }
        pwlist= g_slist_next(pwlist);
    }
    return TRUE;
}

/*function to write out the filter struct*/
gboolean filter_write(xmlNodePtr acc_node, mtc_account *paccount)
{
    mtc_filters *pfilters;
    xmlNodePtr filters_node= NULL;

    pfilters= paccount->pfilters;

    /*write the filters out*/
    if((pfilters!= NULL)&& (pfilters->list!= NULL)&& pfilters->enabled)
    {
        xmlNodePtr filter_node= NULL;
        GSList *pcurrent= NULL;
        mtc_filter *pfilter= NULL;

        filters_node= put_node_empty(acc_node, ELEMENT_FILTERS);
        put_node_bool(filters_node, ELEMENT_MATCHALL, pfilters->matchall);

        pcurrent= pfilters->list;

        /*Now write each of the filters*/
        while(pcurrent!= NULL)
        {
            pfilter= (mtc_filter *)pcurrent->data;

            /*check there is a valid field value*/    
            if(pfilter->field>= (sizeof(ffield)/ sizeof(ffield[0])))
            {
                err_dlg(GTK_MESSAGE_WARNING, "Error: invalid 'field' element %d\n", pfilter->field);
                return FALSE;
            }

            /*only write if there is a string there*/
	        /*TODO would really like to use an elements struct in some way*/
            if(pfilter->search_string[0]!= 0)
            {
                filter_node= put_node_empty(filters_node, ELEMENT_FILTER);
            
                put_node_bool(filter_node, ELEMENT_CONTAINS, pfilter->contains);
                put_node_str(filter_node, ELEMENT_VALUE, pfilter->search_string);
                put_node_str(filter_node, ELEMENT_FIELD, ffield[pfilter->field]);
            }
            pcurrent= g_slist_next(pcurrent);
        }
    }
    return TRUE;
}

/*function to read in a filter*/
static gboolean filter_read(xmlDocPtr doc, xmlNodePtr node, mtc_account *paccount)
{

    mtc_filters *pfilters;
    mtc_filter *pnew;
    gchar tmpfield[FILTERSTRING_LEN];
    gboolean retval= TRUE;

    pfilters= paccount->pfilters;
    memset(tmpfield, '\0', sizeof(tmpfield));
   
    /*create the filter list member*/
    pnew= (mtc_filter *)g_malloc0(sizeof(mtc_filter));   

    /*brackets here, bad*/
    {
        xmlChar *pcontent= NULL;
        elist *pelement= NULL;
        elist elements[]=
        {
            { ELEMENT_CONTAINS, EL_BOOL, &pnew->contains,     sizeof(pnew->contains),       0 },
            { ELEMENT_FIELD,    EL_STR,  tmpfield,            sizeof(tmpfield),             0 },
            { ELEMENT_VALUE,    EL_STR,  pnew->search_string, sizeof(pnew->search_string),  0 },
            {  NULL,            EL_NULL, NULL, 0, 0 },
        };

        /*ok, now get each of the filters fields*/
        while(node!= NULL)
        {
            /*if it is a string value, print it*/
            if((node->type== XML_ELEMENT_NODE)&& (node->children!= NULL)&& 
                (node->children->type== XML_TEXT_NODE)&& !xmlIsBlankNode(node->children))
            {
                 /*now copy the values*/
                pcontent= xmlNodeListGetString(doc, node->children, 1);
            
                if(cfg_copy_element(node, elements, pcontent)== FALSE)
                    retval= FALSE;

                xmlFree(pcontent);
            }
            node= node->next;
        }
        /*get the correct field*/
        if(tmpfield[0]!= 0)
        {    
            guint i;
            for(i= 0; i< (sizeof(ffield)/ sizeof(ffield[0])); i++)
                if(g_ascii_strcasecmp(ffield[i], tmpfield)== 0)
                    pnew->field= i;
         }

        /*search for missing elements*/
        pelement= &elements[0];
        while(pelement->name!= NULL)
        {
            if(!pelement->found)
            {
                err_dlg(GTK_MESSAGE_WARNING, "Error: '%s' element not found for filter %d.\n", pelement->name, paccount->id);           
                retval= FALSE;
            }
            pelement++;
        }

        /*enable the filters if there is a string to search*/
        if(pnew->search_string[0]!= 0)
            pfilters->enabled= TRUE;
    }

    /*Now add to the list*/
    pfilters->list= g_slist_prepend(pfilters->list, pnew);
    
    return(retval);
}

/*function to read in any filter info from the config file*/
gboolean read_filters(xmlDocPtr doc, xmlNodePtr node, mtc_account *paccount)
{
    gboolean retval= TRUE;

    /*check if there are any filters defined*/
    if(xmlIsBlankNode(node->children)&& (xmlStrEqual(node->name, BAD_CAST ELEMENT_FILTERS)))
    {
        xmlNodePtr child= NULL;
        xmlChar *pcontent= NULL;
        mtc_filters *pfilter= NULL;

        gboolean match_found= FALSE;

        /*allocate for the filters*/
        paccount->pfilters= (mtc_filters *)g_malloc0(sizeof(mtc_filters));

        /*intially do not enable the filter*/
        pfilter= paccount->pfilters;
        pfilter->enabled= FALSE;
 
        /*NOTE must be read in backwards to avoid them getting swapped when written*/
        child= node->last;
 
        /*get each filter child element and read it*/
        while(child!= NULL)
        {

            /*filter found, now get the values from it*/
            if((xmlStrEqual(child->name, BAD_CAST ELEMENT_FILTER))&& (child->type== XML_ELEMENT_NODE)&& xmlIsBlankNode(child->children))
            {
                if(filter_read(doc, child->children, paccount)== FALSE)
                    retval= FALSE;
            }
            /*otherwise, if it is match all, treat it*/
            else if((xmlStrEqual(child->name, BAD_CAST ELEMENT_MATCHALL))&&
                    (child->type== XML_ELEMENT_NODE)&&
                    (child->children!= NULL)&&
                    (!xmlIsBlankNode(child->children))&&
                    (child->children->type== XML_TEXT_NODE))
            {
                pcontent= xmlNodeListGetString(doc, child->children, 1);

                pfilter->matchall= (xmlStrcasecmp(pcontent, BAD_CAST "true")== 0)? TRUE: FALSE;
                if(match_found)
                    err_dlg(GTK_MESSAGE_WARNING, "Error: duplicate element '%s'\n", BAD_CAST child->name);
                
                match_found= TRUE;
                xmlFree(pcontent);

            }
            child= child->prev;
        }
    }
    return(retval);
}

/*button to clear the filter entries*/
static void clear_button_pressed(void)
{
    GSList *pcurrent= NULL;
    filter_widgets *pfwidgets= NULL;
    
    pcurrent= widgets.list;
    
    /*iterate through the widget list and reset them*/
    while(pcurrent)
	{
        pfwidgets= (filter_widgets *)pcurrent->data;

		gtk_combo_box_set_active(GTK_COMBO_BOX(pfwidgets->combo_field), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(pfwidgets->combo_contains), 0);
		gtk_entry_set_text(GTK_ENTRY(pfwidgets->entry_value), "");

        pcurrent= g_slist_next(pcurrent);
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.radio_matchall[0]), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.radio_matchall[1]), FALSE);

}

/*function called when the dialog is destroyed*/
void filterdlg_destroyed(GtkWidget *widget, gpointer data)
{
    if(widgets.list!= NULL)
    {
        g_slist_foreach(widgets.list, (GFunc)g_free, NULL);
        g_slist_free(widgets.list);
    }
}

/*display the filter dialog*/
gboolean filterdlg_run(mtc_account *paccount)
{
	GtkWidget *dialog;
	GtkWidget *filter_label;
	GtkWidget *main_table, *v_box_filter;
	gint i= -1;
    guint j;
	gint result= 0;
    gboolean saved= FALSE;
    gboolean retval= TRUE;
    GSList *pcurrent= NULL;
    mtc_filter *pfilter= NULL;
    filter_widgets *pfwidgets= NULL;

    widgets.list= NULL;

    /*set to the start of the list*/
    if(paccount->pfilters)
        pcurrent= paccount->pfilters->list;

	/*create the label*/
	filter_label= gtk_label_new("Select mail fields to filter");
	
	main_table= gtk_table_new(INITIAL_FILTERS+ 2, 3, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(main_table), filter_label, 0, 1, 0, 1);
	
	/*create n number of widgets*/
    /*TODO will be a list eventually*/
	while(i++ < (INITIAL_FILTERS- 1))
	{
        /*create the widget struct*/
        pfwidgets= (filter_widgets *)g_malloc0(sizeof(filter_widgets));

		/*create the fields combo*/
		pfwidgets->combo_field= gtk_combo_box_new_text();
        for(j= 0; j< (sizeof(ffield)/ sizeof(ffield[0])); j++)
		    gtk_combo_box_append_text(GTK_COMBO_BOX(pfwidgets->combo_field), ffield[j]);
		
    	gtk_combo_box_set_active(GTK_COMBO_BOX(pfwidgets->combo_field), 0);

		/*create the contains/does not contain combo*/
		pfwidgets->combo_contains= gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(pfwidgets->combo_contains), S_FILTERDLG_COMBO_CONTAINS);
		gtk_combo_box_append_text(GTK_COMBO_BOX(pfwidgets->combo_contains), S_FILTERDLG_COMBO_NOTCONTAINS);
		gtk_combo_box_set_active(GTK_COMBO_BOX(pfwidgets->combo_contains), 0);

		/*create the filter edit box*/
		pfwidgets->entry_value= gtk_entry_new();
		gtk_entry_set_max_length(GTK_ENTRY(pfwidgets->entry_value), FILTERSTRING_LEN);
		gtk_entry_set_width_chars(GTK_ENTRY(pfwidgets->entry_value), 30); 
		
		/*pack the stuff into the boxes*/
		gtk_table_set_col_spacings(GTK_TABLE(main_table), 10);
		gtk_table_set_row_spacings(GTK_TABLE(main_table), 10);
		gtk_table_attach_defaults(GTK_TABLE(main_table), pfwidgets->combo_field, 0, 1, i+ 1, i+ 2);
		gtk_table_attach_defaults(GTK_TABLE(main_table), pfwidgets->combo_contains, 1, 2, i+ 1, i+ 2);
		gtk_table_attach_defaults(GTK_TABLE(main_table), pfwidgets->entry_value, 2, 3, i+ 1, i+ 2);
		
		if(pcurrent/*&& paccount->pfilters->enabled*/)
		{
            pfilter= (mtc_filter *)pcurrent->data;
			gtk_combo_box_set_active(GTK_COMBO_BOX(pfwidgets->combo_field), pfilter->field);
	    	gtk_combo_box_set_active(GTK_COMBO_BOX(pfwidgets->combo_contains), !pfilter->contains);
			gtk_entry_set_text(GTK_ENTRY(pfwidgets->entry_value), pfilter->search_string);
            pcurrent= g_slist_next(pcurrent);
		}

        /*now add the widgets to the widget list*/
        widgets.list= g_slist_prepend(widgets.list, pfwidgets);
	}
	
	/*set the button to clear the entries*/
	widgets.button_clear= gtk_button_new_with_label(S_FILTERDLG_BUTTON_CLEAR);
  	g_signal_connect(G_OBJECT(widgets.button_clear), "clicked", G_CALLBACK(clear_button_pressed), NULL);
	
	widgets.radio_matchall[0]= gtk_radio_button_new_with_label(NULL, S_FILTERDLG_BUTTON_MATCHALL);
	widgets.radio_matchall[1]= gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(widgets.radio_matchall[0]), S_FILTERDLG_BUTTON_MATCHANY);
	if(paccount->pfilters)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.radio_matchall[0]), paccount->pfilters->matchall);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.radio_matchall[1]), !paccount->pfilters->matchall);
	}
	
	gtk_table_attach(GTK_TABLE(main_table), widgets.button_clear, 2, 3, i+ 2, i+ 3, GTK_SHRINK, GTK_SHRINK, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(main_table), widgets.radio_matchall[0], 0, 1, i+ 2 , i+ 3);
	gtk_table_attach_defaults(GTK_TABLE(main_table), widgets.radio_matchall[1], 1, 2, i+ 2 , i+ 3);
	
	gtk_container_set_border_width(GTK_CONTAINER(main_table), 10);
	
	v_box_filter= gtk_vbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(v_box_filter), main_table, FALSE, FALSE, 10);
	
	/*create the filter dialog*/
	dialog= gtk_dialog_new_with_buttons(S_FILTERDLG_TITLE, NULL, GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    
    /*Set the destroy handler, and also various dialog properties*/
    g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(filterdlg_destroyed), NULL);
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
			/*if OK save the filters to the struct*/
            case GTK_RESPONSE_ACCEPT:
				saved= filter_save(paccount);
                if(!saved)
                {
                    saved= TRUE;
                    retval= FALSE;
                }
			break;
			/*if Cancel set saved to 1 so that the dialog will exit*/
			case GTK_RESPONSE_REJECT:
			default:
				saved= TRUE;
		}
	}
	/*destroy the dialog now that it is finished*/
	gtk_widget_destroy(dialog);

	return(retval);
}

