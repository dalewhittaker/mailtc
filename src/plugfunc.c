/* plugin.c
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

#include <gmodule.h>
#include "plugfunc.h"

/*this is mainly for testing purposes*/
/*#undef LIBDIR*/
#ifndef LIBDIR
#define LIBDIR "../plugin/.libs"
#endif

/*define the shared library filename extension*/
#ifdef _POSIX_SOURCE
#define LIBEXT ".so"
#else
#define LIBEXT ".dll"
#endif /*_POSIX_SOURCE*/

/*function used to sort the items*/
static gint plg_compare(gconstpointer a, gconstpointer b)
{
	gint len1, len2;
	gint result;
	mtc_plugin *pitem1= (mtc_plugin *)b;
	mtc_plugin *pitem2= (mtc_plugin *)a;

	len1= strlen(pitem1->name);
	len2= strlen(pitem2->name);

	/*compare strings*/
	/*this assumes ascii strings (which they should be really)*/
	result= g_ascii_strncasecmp(pitem2->name, pitem1->name, 
		(len1< len2)? len1: len2);
		
	/*special case is if there is a match, we use the one with a shorter string*/
	if(result== 0)
		result= (len1< len2)? 1: -1;

	return(result);
}

/*function to unload a plugin*/
static void plg_unload(gpointer data, gpointer user_data)
{
	mtc_plugin *pitem= (mtc_plugin *)data;

    user_data= NULL;

	/*call the unload routine*/
	(*pitem->unload)();
	
	if(!g_module_close((GModule *)pitem->handle))
		msgbox_err(S_PLUGIN_ERR_CLOSE_PLUGIN, g_module_name((GModule *)pitem->handle), g_module_error());
	
}

/*function to unload the plugins*/
gboolean plg_unload_all(void)
{
	/*now free the list*/
	g_slist_foreach(plglist, plg_unload, NULL);
	
	g_slist_free(plglist);
	plglist= NULL;

	return(TRUE);
	
}

/*load a found network plugin*/
static gboolean plg_load(const gchar *plugin_name)
{
	GModule *module= NULL;
	mtc_plugin *pitem= NULL;
	mtc_plugin *(*init_plugin)(void);

	/*check if the system supports loading modules*/
	if(!g_module_supported())
	{
		msgbox_err(S_PLUGIN_ERR_MODULE_SUPPORT);
		return(FALSE);
	}

	/*open the shared library*/
	if((module= g_module_open(plugin_name, G_MODULE_BIND_LOCAL/*1*//*0*//*G_MODULE_BIND_LAZY*/))== NULL)
	{
		msgbox_warn(S_PLUGIN_ERR_OPEN_PLUGIN, plugin_name, g_module_error());
		return(TRUE);
	}
	
	/*get the plugin struct from the module*/
	if(!g_module_symbol(module, "init_plugin", (gpointer)&init_plugin))
	{
		msgbox_warn(S_PLUGIN_ERR_PLUGIN_POINTER, g_module_error());
		/*now close the module*/
		if(!g_module_close(module))
			msgbox_warn(S_PLUGIN_ERR_CLOSE_PLUGIN, g_module_name(module), g_module_error());
		
		return(TRUE);
	}
	
	/*initialise the plugin*/
	if((pitem= init_plugin())== NULL)
	{
		msgbox_warn(S_PLUGIN_ERR_INIT_PLUGIN, g_module_name(module), g_module_error());
		return(TRUE);
	}
	pitem->handle= module;

	/*if it is greater than the current mailtc version, report an error*/
	if(g_ascii_strcasecmp(VERSION, pitem->compatibility)!= 0)
	{
		msgbox_err(S_PLUGIN_ERR_COMPATIBILITY, g_module_name(module), PACKAGE, VERSION);
		plg_unload(pitem, NULL);
		return(FALSE);
	}

	/*add the bugger to the start of the list*/
	plglist= g_slist_insert_sorted(plglist, (gpointer)pitem, (GCompareFunc)plg_compare);

	/*call the plugin load routine*/
	(*pitem->load)(&config);

	return(TRUE);
}

/*get the plugin name from the plugin*/
gchar *plg_name(mtc_plugin *pplugin)
{
    return(g_path_get_basename(g_module_name((GModule *)pplugin->handle)));
}

/*compare function used by find_plugin*/
static gint plg_match(gconstpointer a, gconstpointer b)
{
	mtc_plugin *pitem= (mtc_plugin *)a;
	const gchar *plugin_name= (const gchar *)b;
	gchar *pbasename;
	gint retval= 0;

	/*pbasename= g_path_get_basename(g_module_name((GModule *)pitem->handle));*/
	pbasename= plg_name(pitem);
    retval= (g_str_equal(pbasename, plugin_name))? 0: 1;
	g_free(pbasename);
	return(retval);
}

/*function to find a plugin from the list*/
mtc_plugin *plg_find(const gchar *plugin_name)
{
	GSList *pfound= g_slist_find_custom(plglist, plugin_name, plg_match);
	
	/*if it returns NULL (i.e not found, we should report an error and then default to first in list*/
	if(pfound== NULL)
	{
		msgbox_warn(S_PLUGIN_ERR_FIND_PLUGIN, plugin_name);
		return(NULL);
	}
	return((mtc_plugin *)pfound->data);
}

/*test function to print the plugins*/
/*static gboolean print_plugins(void)
{
	mtc_plugin *pitem;
	GSList *pcurrent= plglist;
	
	while(pcurrent!= NULL)
	{
		pitem= (mtc_plugin *)pcurrent->data;
		g_print("Plugin file: %s, name: %s\n", g_module_name(pitem->handle), pitem->name);
		g_print("Plugin port: %d\n", pitem->default_port);
		pcurrent= g_slist_next(pcurrent);
	}
	return(TRUE);
}*/

/*function to traverse the directory and load any plugins found*/
gboolean plg_load_all(void)
{
	GDir *dir;
	GError *error= NULL;
	const gchar *file= NULL;
	gchar *path= NULL;

	/*initialise the list*/
	plglist= NULL;

	/*open the dir for reading*/
	if(!(dir= g_dir_open(LIBDIR, 0, &error)))
	{
		msgbox_err("%s\n", error->message);
		return(FALSE);
	}

	/*get the filename of each plugin to load*/
	while((file= g_dir_read_name(dir))!= NULL)
	{
		/*now here we load our plugin*/
		path= g_build_filename(LIBDIR, file, NULL);
		
		/*currently we only add plugins with a ".so" extension*/
		if(g_ascii_strcasecmp(path+ (strlen(path)- strlen(LIBEXT)), LIBEXT)== 0)
			plg_load(path);

		g_free(path);
	}

	/*close the dir*/
	g_dir_close(dir);

	return((plglist== NULL)? FALSE: TRUE);
}



