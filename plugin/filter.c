/* filter.c
 * Copyright (C) 2007 Dale Whittaker <dayul@users.sf.net>
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

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <gtk/gtkdialog.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtktable.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkscrolledwindow.h>

#include "filter.h"

/*define the filter element names that are used*/
#define ELEMENT_FILTERS  "filters"
#define ELEMENT_FILTER   "filter"
#define ELEMENT_MATCHALL "match_all"
#define ELEMENT_CONTAINS "contains"
#define ELEMENT_FIELD    "field"
#define ELEMENT_VALUE    "value"

#define FILTERSTRING_LEN 100
#define MAX_FILTERS 100

/*structs used for the filter widgets*/
typedef struct _filter_widgets
{
    GtkWidget *hbox;
    GtkWidget *combo_field;
    GtkWidget *combo_contains;
    GtkWidget *entry_value;
    GtkWidget *button_remove;

} filter_widgets;

typedef struct _filters_widgets
{
    GtkWidget *vbox;
    GtkWidget *radio_matchall[2];
    GtkWidget *button_clear;
    GtkWidget *button_add;

    GSList *list;

} filters_widgets;

typedef enum _eltype { EL_NULL= 0, EL_STR, EL_BOOL } eltype;

/*structure used when reading in the xml config elements*/
typedef struct _elist
{
    gchar *name; /*the element name*/
    eltype type; /*type used for copying*/
    gpointer value; /*the config value*/
    gint length; /*length in bytes of config value*/
    gboolean found; /*used to track duplicates, or not found at all*/
} elist;

static filters_widgets widgets;
static GtkWidget *ftable= NULL;
static GtkWidget *filter_button= NULL;
static GtkWidget *filter_checkbox= NULL;

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

/*show an error message dialog*/
static mtc_error filter_err(gchar *errmsg, ...)
{
    va_list list;
    gchar *fmsg= NULL;
    gsize msglen= 0;
    GtkWidget *dialog;
    mtc_cfg *pcfg= NULL;
    
    /*get the length of the string*/
	va_start(list, errmsg);
    msglen= g_printf_string_upper_bound(errmsg, list)+ 1;
	va_end(list);

    /*allocate/format the string*/
    fmsg= (gchar *)g_malloc0(msglen);
	va_start(list, errmsg);
    g_vsnprintf(fmsg, msglen, errmsg, list);
	va_end(list);

    /*display the messagebox*/
    dialog= gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, fmsg);
    gtk_window_set_title(GTK_WINDOW(dialog), PACKAGE);
    gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    /*NOTE va_list must be reset, otherwise it crashes some systems*/
    pcfg= cfg_get();
	if(pcfg&& pcfg->logfile)
    {
        g_fprintf(pcfg->logfile, fmsg);
	    fflush(pcfg->logfile);
	}

    g_free(fmsg);
	return(MTC_RETURN_TRUE);

}

/*function to free any filter data*/
mtc_error free_filters(mtc_account *paccount)
{
	if(paccount->plg_opts!= NULL)
	{
        GSList *flist;

        /*first free the list if any, then free the filters struct*/
        flist= ((mtc_filters *)(paccount->plg_opts))->list;
        g_slist_foreach(flist, (GFunc)g_free, NULL);
        g_slist_free(flist);

		g_free(paccount->plg_opts);
		paccount->plg_opts= NULL;
	}
    return(MTC_RETURN_TRUE);
}

/*function to store the filter struct*/
static gint filter_save(mtc_account *paccount)
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
	{
        filter_err(S_FILTER_NO_FILTERS);
        return FALSE;
	}

    /*allocate new filter if it doesn't already exist*/
    if(paccount->plg_opts== NULL)
        paccount->plg_opts= g_malloc0(sizeof(mtc_filters));

    pfilters= (mtc_filters *)paccount->plg_opts;
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
                filter_err(S_FILTER_ERR_COMBO_ITER);
                g_free(pnew);
                return(-1);
			}
            gtk_tree_model_get(model, &iter, 0, &str, -1);
			
            /*set the contains for each*/
			pnew->contains= (g_ascii_strcasecmp(str, S_FILTER_COMBO_CONTAINS)== 0)? TRUE: FALSE;
            g_free(str);
			
            /*get the active field*/
            j= gtk_combo_box_get_active(GTK_COMBO_BOX(pfwidgets->combo_field));
            if(j== -1)
            {
                filter_err(S_FILTER_ERR_COMBO_ITER);
                g_free(pnew);
                return(-1);
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
    return(1);
}

/*add a node of type string*/
static xmlNodePtr put_node_str(xmlNodePtr parent, const gchar *name, const gchar *content)
{
    xmlChar *pname;
    xmlChar *pcontent;

    /*don't add anything if there is no value*/
    if(name== NULL|| content== NULL|| *content== 0)
        return(NULL);

    pname= BAD_CAST name;
    pcontent= BAD_CAST content;

    /*now add the string*/
    return(xmlNewChild(parent, NULL, pname, pcontent));
}

/*add a node of type integer*/
static xmlNodePtr put_node_bool(xmlNodePtr parent, const gchar *name, const gboolean content)
{
    xmlNodePtr node;
    xmlChar *pstring;

    /*don't add anything if there is no value*/
    if(name== NULL)
        return(NULL);

    /*add the string*/
    pstring= xmlXPathCastBooleanToString(content);
    node= put_node_str(parent, name, (const gchar *)pstring);

    /*now free the string*/
    xmlFree(pstring);

    return(node);
}

/*add an empty node*/
static xmlNodePtr put_node_empty(xmlNodePtr parent, const gchar *name)
{
    xmlChar *pname;

    /*don't add anything if there is no value*/
    if(name== NULL)
        return(NULL);

    pname= BAD_CAST name;
    
    /*now add the string*/
    return(xmlNewChild(parent, NULL, pname, NULL));
}

/*function to write out the filter struct*/
mtc_error filter_write(xmlNodePtr acc_node, mtc_account *paccount)
{
    mtc_filters *pfilters;
    xmlNodePtr filters_node= NULL;

    pfilters= (mtc_filters *)paccount->plg_opts;
    
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
                filter_err(S_FILTER_ERR_ELEMENT_INVALID_FIELD, pfilter->field);
                return(MTC_RETURN_FALSE);
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
    return(MTC_RETURN_TRUE);
}

/*wrapper to report if there is a duplicate*/
static gboolean isduplicate(elist *element)
{
    gboolean retval= FALSE;

    if(element->found> 0)
    {
        filter_err(S_FILTER_ERR_ELEMENT_DUPLICATE, element->name);
        retval= TRUE;
    }
    element->found++;

    return(retval);
}

/*copy a string element*/
static gboolean get_node_str(elist *element, const xmlChar *src)
{
    const gchar *psrc;
    gchar *pdest;

    pdest= (gchar *)element->value;
    if(isduplicate(element))
    {
        /*wipe the value if it is a duplicate*/
        memset(pdest, '\0', element->length);
        return FALSE;
    }

    psrc= (const gchar *)src;

    g_strlcpy(pdest, psrc, element->length);
    return TRUE;
}

/*copy a boolean element*/
static gboolean get_node_bool(elist *element, const xmlChar *src)
{
    gboolean *pdest;

    if(isduplicate(element))
        return FALSE;

    pdest= (gboolean *)element->value;

    *pdest= (xmlStrcasecmp(src, BAD_CAST "true")== 0)? TRUE: FALSE;
    return TRUE;
}

/*the generic copy function, this will call the specific copy functions*/
static gboolean cfg_copy_func(elist *pelement, xmlChar *content)
{
    gboolean retval= TRUE;

    /*determine the copy function to call*/
    switch(pelement->type)
    {
        case EL_STR:
            retval= get_node_str(pelement, content);
        break;
        case EL_BOOL:
            retval= get_node_bool(pelement, content);
        break;
        default: ;
    }
    return(retval);
}

/*function to copy the config values*/
static gboolean cfg_copy_element(xmlNodePtr node, elist *pelement, xmlChar *content)
{
    /*check the element with each value in the list, and run the appropriate function*/
    while(pelement->name!= NULL)
    {
        if(xmlStrEqual(node->name, BAD_CAST pelement->name))
        {
            /*do the copying, returning if duplicate found*/
            if(!cfg_copy_func(pelement, content))
                return FALSE;

            /*break out if found*/
            break;
        }
        pelement++;
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

    pfilters= (mtc_filters *)paccount->plg_opts;
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
                filter_err(S_FILTER_ERR_ELEMENT_NOT_FOUND, pelement->name, paccount->id);           
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
mtc_error read_filters(xmlDocPtr doc, xmlNodePtr node, mtc_account *paccount)
{
    gboolean retval= MTC_RETURN_TRUE;
    xmlNodePtr parent= NULL;
    
    parent= node->children;
    while(parent!= NULL)
    {
        /*check if there are any filters defined*/
        if(xmlIsBlankNode(parent->children)&& (xmlStrEqual(parent->name, BAD_CAST ELEMENT_FILTERS)))
        {
            xmlNodePtr child= NULL;
            xmlChar *pcontent= NULL;
            mtc_filters *pfilter= NULL;

            gboolean match_found= FALSE;

            /*allocate for the filters*/
            paccount->plg_opts= g_malloc0(sizeof(mtc_filters));

            /*intially do not enable the filter*/
            pfilter= (mtc_filters *)paccount->plg_opts;
            pfilter->enabled= FALSE;
     
            /*NOTE must be read in backwards to avoid them getting swapped when written*/
            child= parent->last;
     
            /*get each filter child element and read it*/
            while(child!= NULL)
            {

                /*filter found, now get the values from it*/
                if((xmlStrEqual(child->name, BAD_CAST ELEMENT_FILTER))&& (child->type== XML_ELEMENT_NODE)&& xmlIsBlankNode(child->children))
                {
                    if(filter_read(doc, child->children, paccount)== FALSE)
                        retval= MTC_RETURN_FALSE;
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
                        filter_err(S_FILTER_ERR_ELEMENT_DUPLICATE, BAD_CAST child->name);
                    
                    match_found= TRUE;
                    xmlFree(pcontent);

                }
                child= child->prev;
            }
        }
        parent= parent->next;
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
static void filterdlg_destroyed(void)
{
    if(widgets.list!= NULL)
    {
        g_slist_foreach(widgets.list, (GFunc)g_free, NULL);
        g_slist_free(widgets.list);
    }
}

/*button to remove a filter from the list*/
static void remove_button_pressed(GtkButton *button, gpointer user_data)
{
    guint length;
    
    length= g_slist_length(widgets.list);
    if(length> 1)
    {
        filter_widgets *pfwidgets= (filter_widgets *)user_data;

        /*simply remove the widget and destroy it, then remove it from the list*/
        gtk_widget_hide_all(pfwidgets->hbox);
    
        /*first remove from the list, then remove from the vbox to destroy*/
        widgets.list= g_slist_remove(widgets.list, pfwidgets);
        gtk_container_remove(GTK_CONTAINER(widgets.vbox), pfwidgets->hbox);
        g_free(pfwidgets);
    }
}

/*function to create widgets*/
static filter_widgets *create_widgets(filters_widgets *pwidgets)
{
    filter_widgets *pfwidgets= NULL;
    guint j;
    
    /*create the widget struct*/
    pfwidgets= (filter_widgets *)g_malloc0(sizeof(filter_widgets));
    
    /*create the fields combo*/
    pfwidgets->combo_field= gtk_combo_box_new_text();
    for(j= 0; j< (sizeof(ffield)/ sizeof(ffield[0])); j++)
		    gtk_combo_box_append_text(GTK_COMBO_BOX(pfwidgets->combo_field), ffield[j]);
		
    gtk_combo_box_set_active(GTK_COMBO_BOX(pfwidgets->combo_field), 0);

	/*create the contains/does not contain combo*/
	pfwidgets->combo_contains= gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(pfwidgets->combo_contains), S_FILTER_COMBO_CONTAINS);
	gtk_combo_box_append_text(GTK_COMBO_BOX(pfwidgets->combo_contains), S_FILTER_COMBO_NOTCONTAINS);
	gtk_combo_box_set_active(GTK_COMBO_BOX(pfwidgets->combo_contains), 0);

	/*create the filter edit box*/
	pfwidgets->entry_value= gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(pfwidgets->entry_value), FILTERSTRING_LEN);
	gtk_entry_set_width_chars(GTK_ENTRY(pfwidgets->entry_value), 30); 
	
    /*create the remove button*/
    pfwidgets->button_remove= gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(pfwidgets->button_remove), gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON));

    /*pack the stuff into box*/
    pfwidgets->hbox= gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pfwidgets->hbox), pfwidgets->combo_field, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(pfwidgets->hbox), pfwidgets->combo_contains, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(pfwidgets->hbox), pfwidgets->entry_value, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(pfwidgets->hbox), pfwidgets->button_remove, TRUE, TRUE, 5);
	
    /*add the hbox to the scroll windows vbox*/
    /*g_object_ref(G_OBJECT(pfwidgets->hbox));*/
    gtk_box_pack_start(GTK_BOX(pwidgets->vbox), pfwidgets->hbox, FALSE, FALSE, 5);
  	g_signal_connect(G_OBJECT(pfwidgets->button_remove), "clicked", G_CALLBACK(remove_button_pressed), pfwidgets);
		
    /*now add the widgets to the widget list*/
    pwidgets->list= g_slist_prepend(pwidgets->list, pfwidgets);

    return(pfwidgets);
}

/*button to add a new filter to the list*/
static void add_button_pressed(void)
{
    guint length;

    length= g_slist_length(widgets.list);
    if(length< MAX_FILTERS)
    {
        filter_widgets *pfwidgets= NULL;

        pfwidgets= create_widgets(&widgets);
	    gtk_widget_show_all(widgets.vbox);
    }
    else
		filter_err(S_FILTER_ERR_MAX_REACHED, MAX_FILTERS);
}

/*display the filter dialog*/
gboolean filterdlg_run(mtc_account *paccount)
{
	GtkWidget *dialog;
	GtkWidget *filter_label;
	GtkWidget *v_box_filter;
    GtkWidget *scrolled_win;
    GtkWidget *h_box_filter;
	gint result= 0;
    gboolean saved= FALSE;
    gboolean retval= TRUE;
    GSList *pcurrent= NULL;
    mtc_filter *pfilter= NULL;
    filter_widgets *pfwidgets= NULL;

    widgets.list= NULL;

    /*set to the start of the list*/
    if(paccount&& paccount->plg_opts)
        pcurrent= ((mtc_filters *)paccount->plg_opts)->list;

	/*create the label*/
    h_box_filter= gtk_hbox_new(FALSE, 0);
	filter_label= gtk_label_new(S_FILTER_LABEL_SELECT);
    gtk_box_pack_start(GTK_BOX(h_box_filter), filter_label, FALSE, FALSE, 10);
	
	v_box_filter= gtk_vbox_new(FALSE, 0);
    
    widgets.vbox= gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(v_box_filter), h_box_filter, FALSE, FALSE, 10);

    /*create the scrolled window to add the widgets to*/
    scrolled_win= gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(scrolled_win), 10);
    gtk_widget_set_size_request(scrolled_win, 600, 300);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_win), GTK_SHADOW_IN);

    /*create a single empty widget*/
    if(pcurrent== NULL)
        pfwidgets= create_widgets(&widgets);
    else
    {
        /*iterate through the filters read from the config file, and add them to the dialog*/
        while(pcurrent!= NULL)
        {
            /*if(((mtc_filters *)paccount->plg_opts)->enabled)
		    {*/
            pfwidgets= create_widgets(&widgets);
            
            /*Add the widget data*/
            pfilter= (mtc_filter *)pcurrent->data;
			gtk_combo_box_set_active(GTK_COMBO_BOX(pfwidgets->combo_field), pfilter->field);
	    	gtk_combo_box_set_active(GTK_COMBO_BOX(pfwidgets->combo_contains), !pfilter->contains);
			gtk_entry_set_text(GTK_ENTRY(pfwidgets->entry_value), pfilter->search_string);

            pcurrent= g_slist_next(pcurrent);
		    /*}*/
        }
    }

    /*add the scrolled window to the main table*/
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_win), widgets.vbox);
    
    /*set the button to clear the entries*/
	widgets.button_clear= gtk_button_new_with_label(S_FILTER_BUTTON_CLEAR);
  	g_signal_connect(G_OBJECT(widgets.button_clear), "clicked", G_CALLBACK(clear_button_pressed), NULL);
	
    /*set the button to add new filters*/
	widgets.button_add= gtk_button_new_with_label(S_FILTER_BUTTON_ADD_FILTER);
    gtk_button_set_image(GTK_BUTTON(widgets.button_add), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON));
  	g_signal_connect(G_OBJECT(widgets.button_add), "clicked", G_CALLBACK(add_button_pressed), NULL);

	widgets.radio_matchall[0]= gtk_radio_button_new_with_label(NULL, S_FILTER_BUTTON_MATCHALL);
	widgets.radio_matchall[1]= gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(widgets.radio_matchall[0]), S_FILTER_BUTTON_MATCHANY);
	if(paccount->plg_opts)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.radio_matchall[0]), ((mtc_filters *)paccount->plg_opts)->matchall);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets.radio_matchall[1]), !((mtc_filters *)paccount->plg_opts)->matchall);
	}
	
    /*Add the scrolled window with filter widgets*/
	gtk_box_pack_start(GTK_BOX(v_box_filter), scrolled_win, FALSE, FALSE, 0);

    h_box_filter= gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(h_box_filter), widgets.radio_matchall[0], FALSE, FALSE, 10);
	gtk_box_pack_start(GTK_BOX(h_box_filter), widgets.radio_matchall[1], FALSE, FALSE, 10);
    gtk_box_pack_end(GTK_BOX(h_box_filter), widgets.button_clear, FALSE, FALSE, 10);
    gtk_box_pack_end(GTK_BOX(h_box_filter), widgets.button_add, FALSE, FALSE, 10);
	gtk_box_pack_start(GTK_BOX(v_box_filter), h_box_filter, FALSE, FALSE, 10);
	
	/*create the filter dialog*/
	dialog= gtk_dialog_new_with_buttons(S_FILTER_TITLE, NULL, GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    
    /*Set the destroy handler, and also various dialog properties*/
    g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(filterdlg_destroyed), NULL);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), v_box_filter);
	gtk_widget_show_all(v_box_filter);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 80, 80);

	/*keep running dialog until details are saved (i.e all values entered)*/
	while(saved== 0)
	{
		result= gtk_dialog_run(GTK_DIALOG(dialog)); 
		switch(result)
		{
			/*if OK save the filters to the struct*/
            case GTK_RESPONSE_ACCEPT:
				saved= filter_save(paccount);
                if(saved== -1)
                    retval= FALSE;
			break;
			/*if Cancel set saved to 1 so that the dialog will exit*/
			case GTK_RESPONSE_REJECT:
			default:
				saved= 1;
		}
	}
	/*destroy the dialog now that it is finished*/
    /*gtk_container_foreach(GTK_CONTAINER(widgets.vbox), (GtkCallback)g_object_unref, NULL);*/
    if(dialog&& GTK_IS_WIDGET(dialog))
        gtk_widget_destroy(dialog);

	return(retval);
}

/*signal called when filter button is pressed*/
static void filter_button_pressed(GtkWidget *widget, gpointer data)
{
	mtc_account *pcurrent= NULL;
	
    pcurrent= (mtc_account *)data;
	filterdlg_run(pcurrent);
}

/*signal called when filter checkbox is pressed*/
static void filter_checkbox_pressed(GtkWidget *widget)
{
	gtk_widget_set_sensitive(filter_button, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

/*put the enabled status of the filters into the struct*/
mtc_error filter_enabled(mtc_account *paccount)
{
    if(paccount->plg_opts!= NULL)
	    ((mtc_filters *)paccount->plg_opts)->enabled= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(filter_checkbox));
    
    return(MTC_RETURN_TRUE);
}

/*function to get create/get the filter table to pass to mailtc*/
GtkWidget *filter_table(mtc_account *paccount, gchar *plgname)
{
    mtc_filters *pfilter= NULL;
    GtkWidget *filter_label;
    gchar title[100];

    /*create a table containing widgets and return it to be shown*/
    g_snprintf(title, sizeof(title), "%s plugin options", plgname);
    filter_label= gtk_label_new(title);

	filter_checkbox= gtk_check_button_new_with_label(S_FILTER_ENABLEFILTERS);
  	g_signal_connect(G_OBJECT(filter_checkbox), "clicked", G_CALLBACK(filter_checkbox_pressed), NULL);
	filter_button= gtk_button_new_with_label(S_FILTER_CONFIGFILTERS);
     
  	g_signal_connect(G_OBJECT(filter_button), "clicked", G_CALLBACK(filter_button_pressed), paccount);
    
    ftable= gtk_table_new(2, 2, FALSE);
    gtk_table_set_col_spacings(GTK_TABLE(ftable), 10);
    gtk_table_set_row_spacings(GTK_TABLE(ftable), 20);
    gtk_container_set_border_width(GTK_CONTAINER(ftable), 10);
      
    gtk_table_attach(GTK_TABLE(ftable), filter_label, 0, 2, 0, 1, GTK_FILL| GTK_EXPAND, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE(ftable), filter_checkbox, 0, 1, 1, 2, GTK_SHRINK| GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE(ftable), filter_button, 1, 2, 1, 2, GTK_SHRINK| GTK_FILL, GTK_SHRINK, 0, 0);     
    
	/*set the button based on the checkbox value*/
    gtk_widget_set_sensitive(filter_button, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(filter_checkbox)));
        
    /*if there are plugin options, get them*/
    if(paccount!= NULL)
        pfilter= (mtc_filters *)paccount->plg_opts;
 
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filter_checkbox), ((pfilter!= NULL)&& (pfilter->enabled)));
	gtk_widget_set_sensitive(filter_button, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(filter_checkbox)));
 
    return(ftable);
}

/*function to destroy the plugin table widget when unloading*/
mtc_error filter_unload(void)
{
    /*destroy the table widget if it exists*/
    if((ftable!= NULL)&& GTK_IS_WIDGET(ftable))
        gtk_widget_destroy(ftable);

    return(MTC_RETURN_TRUE);
}

