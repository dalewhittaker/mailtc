/* plugin.c
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

/*function used to sort the items*/
static gint compare_plugin_names(gconstpointer a, gconstpointer b)
{
	int len1, len2;
	gint result;
	mtc_plugin_info *pitem1= (mtc_plugin_info *)b;
	mtc_plugin_info *pitem2= (mtc_plugin_info *)a;

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
static void unload_plugin(gpointer data, gpointer user_data)
{
	mtc_plugin_info *pitem= (mtc_plugin_info *)data;

	/*call the unload routine*/
	(*pitem->unload)();
	
	if(!g_module_close(pitem->handle))
		error_and_log_no_exit(S_PLUGIN_ERR_CLOSE_PLUGIN, g_module_name(pitem->handle), g_module_error());
	
}

/*function to unload the plugins*/
gboolean unload_plugins(void)
{
	/*now free the list*/
	g_slist_foreach(plglist, unload_plugin, NULL);
	
	g_slist_free(plglist);
	plglist= NULL;

	return(TRUE);
	
}

/*load a found network plugin*/
static gboolean load_plugin(const char *plugin_name)
{
	GModule *module= NULL;
	mtc_plugin_info *pitem= NULL;
	mtc_plugin_info *(*init_plugin)(void);

	/*check if the system supports loading modules*/
	if(!g_module_supported())
	{
		error_and_log_no_exit(S_PLUGIN_ERR_MODULE_SUPPORT);
		return(FALSE);
	}

	/*open the shared library*/
	if((module= g_module_open(plugin_name, G_MODULE_BIND_LOCAL/*1*//*0*//*G_MODULE_BIND_LAZY*/))== NULL)
	{
		error_and_log_no_exit(S_PLUGIN_ERR_OPEN_PLUGIN, plugin_name, g_module_error());
		return(TRUE);
	}
	
	/*get the plugin struct from the module*/
	if(!g_module_symbol(module, "init_plugin", (gpointer)&init_plugin))
	{
		error_and_log_no_exit(S_PLUGIN_ERR_PLUGIN_POINTER, g_module_error());
		/*now close the module*/
		if(!g_module_close(module))
			error_and_log_no_exit(S_PLUGIN_ERR_CLOSE_PLUGIN, g_module_name(module), g_module_error());
		
		return(TRUE);
	}
	
	/*initialise the plugin*/
	if((pitem= init_plugin())== NULL)
	{
		error_and_log_no_exit(S_PLUGIN_ERR_INIT_PLUGIN, g_module_name(module), g_module_error());
		return(TRUE);
	}
	pitem->handle= module;

	/*if it is greater than the current mailtc version, report an error*/
	if(strcmp(VERSION, pitem->compatibility)< 0)
	{
		error_and_log_no_exit(S_PLUGIN_ERR_COMPATIBILITY, g_module_name(module), PACKAGE, VERSION);
		unload_plugin(pitem, NULL);
		return(FALSE);
	}

	/*add the bugger to the start of the list*/
	plglist= g_slist_insert_sorted(plglist, (gpointer)pitem, (GCompareFunc)compare_plugin_names);

	/*call the plugin load routine*/
	(*pitem->load)(config.base_name, config.logfile, (config.net_debug)? MTC_DEBUG_MODE: 0);

	return(TRUE);
}

/*compare function used by find_plugin*/
static gint plugin_match(gconstpointer a, gconstpointer b)
{
	mtc_plugin_info *pitem= (mtc_plugin_info *)a;
	const gchar *plugin_name= (const gchar *)b;
	gchar *pbasename;
	gint retval= 0;

	pbasename= g_path_get_basename(g_module_name(pitem->handle));
	retval= (g_str_equal(pbasename, plugin_name))? 0: 1;
	g_free(pbasename);
	return(retval);
}

/*function to find a plugin from the list*/
mtc_plugin_info *find_plugin(const gchar *plugin_name)
{
	GSList *pfound= g_slist_find_custom(plglist, plugin_name, plugin_match);
	
	/*if it returns NULL (i.e not found, we should report an error and then default to first in list*/
	if(pfound== NULL)
	{
		error_and_log_no_exit(S_PLUGIN_ERR_FIND_PLUGIN, plugin_name);
		return(NULL);
	}
	return((mtc_plugin_info *)pfound->data);
}

/*TODO test function to print the plugins*/
/*static gboolean print_plugins(void)
{
	mtc_plugin_info *pitem;
	GSList *pcurrent= plglist;
	
	while(pcurrent!= NULL)
	{
		pitem= (mtc_plugin_info *)pcurrent->data;
		g_print("Plugin file: %s, name: %s\n", g_module_name(pitem->handle), pitem->name);
		g_print("Plugin port: %d\n", pitem->default_port);
		pcurrent= g_slist_next(pcurrent);
	}
	return(TRUE);
}*/

/*function to traverse the directory and load any plugins found*/
gboolean load_plugins(void)
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
		error_and_log_no_exit("%s\n", error->message);
		return(FALSE);
	}

	/*get the filename of each plugin to load*/
	while((file= g_dir_read_name(dir))!= NULL)
	{
		/*now here we load our plugin*/
		path= g_build_filename(LIBDIR, file, NULL);
		
		/*currently we only add plugins with a ".so" extension*/
		if(strcmp(path+ (strlen(path)- 3), ".so")== 0)
			load_plugin(path);

		g_free(path);
	}

	/*close the dir*/
	g_dir_close(dir);

	return((plglist== NULL)? FALSE: TRUE);
}



