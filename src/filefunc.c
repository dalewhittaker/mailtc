/* filefunc.c 
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

#include <stdlib.h> /*exit*/
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "filefunc.h"
#include "plugfunc.h"
#include "filterdlg.h"

#ifdef MTC_USE_SSL
#include "encrypter.h"
#endif /*MTC_USE_SSL*/

#define BASE64_PASSWORD_LEN (PASSWORD_LEN* 4/ 3+ 6)

/*wrapper to create a directory*/
static void mk_dir(gchar *pfile)
{
    if(!IS_DIR(pfile))
	{
		if(FILE_EXISTS(pfile))
		{
			g_printerr(S_FILEFUNC_ERR_NOT_DIRECTORY, pfile, PACKAGE);
			exit(EXIT_FAILURE);
		}
		else
		{
            if(g_mkdir(pfile, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH)== -1)
		    {
                g_printerr("%s %s: %s\n", S_FILEFUNC_ERR_MKDIR, pfile, g_strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
	}
}

/*function to get $HOME/.config + program*/
gboolean mtc_dir(void)
{
	gchar *pcfg= NULL;
    gchar *pmtc= NULL;

	/*first create the .config dir*/
	if((pcfg= g_build_filename(g_get_home_dir(), ".config", NULL))== NULL)
		err_exit(S_FILEFUNC_ERR_GET_HOMEDIR);
	
    /*create the dir if it doesn't exist*/
    mk_dir(pcfg);

    /*second, create the .config/mailtc dir*/
    if((pmtc= g_build_filename(pcfg, "mailtc", NULL))== NULL)
		err_exit(S_FILEFUNC_ERR_GET_HOMEDIR);
	
    /*create the dir if it doesn't exist*/
    mk_dir(pmtc);
	
	g_strlcpy(config.dir, pmtc, sizeof(config.dir));
	
    g_free(pmtc);
	g_free(pcfg);

	return TRUE;
}

/*function to get relevant details/password/uidl filename*/
gchar *mtc_file(gchar *fullpath, gchar *filename, gint account)
{
	if(account>= 0)
		g_snprintf(fullpath, NAME_MAX, "%s%c%s%u", config.dir, G_DIR_SEPARATOR, filename, account);
	else
		g_snprintf(fullpath, NAME_MAX, "%s%c%s", config.dir, G_DIR_SEPARATOR, filename);
	return(fullpath);
}

/*Function to convert the old 'protocol' strings to the new plugin names wherever possible*/
static gchar *protocol_to_plugin(gchar *plgstring)
{
	/*Copy over default plugin names if we can*/
	mtc_plugin *pitem;
	GSList *pcurrent= plglist;
	
	while(pcurrent!= NULL)
	{
		pitem= (mtc_plugin *)pcurrent->data;
		
		/*if we find an old one, we overwrite it with the equivalent plugin name*/
		if(g_ascii_strcasecmp(pitem->name, plgstring)== 0)
		{
			gchar *pbasename;
            pbasename= plg_name(pitem);
			g_strlcpy(plgstring, pbasename, PROTOCOL_LEN);
			g_free(pbasename);
			return(plgstring);
		}
		pcurrent= g_slist_next(pcurrent);
	}
	return(plgstring);
}

/*libxml initialisation*/
static void xml_init(void)
{
    /*initialise the library and check for ABI mismatch*/
    LIBXML_TEST_VERSION

    xmlIndentTreeOutput= 1;
}

/*cleanup libxml*/
static void xml_cleanup(void)
{
    /*cleanup library*/
    xmlCleanupParser();

    /*debug memory for regression tests*/
    xmlMemoryDump();
}

/*function to read in the password (encrypted or not) from the file*/
/*TODO final one will check the attributes to see if it should be decrypted or not*/
static gboolean pw_copy(mtc_account *paccount, gchar *pwstring)
{
/*if OpenSSL is defined read in an encrypted password*/
#ifdef MTC_USE_SSL
	/*decrypt the password*/
    return(pw_decrypt(pwstring, paccount->password));

/*otherwise read in clear password*/
#else
    g_strlcpy(paccount->password, (gchar *)content, sizeof(paccount->password));
	return TRUE;
#endif
	
}

/*function to copy the config values*/
static gboolean acc_copy_element(mtc_account *paccount, xmlNodePtr node, xmlChar *content)
{
    /*TODO  BIIIIIIIIIIIIIGGGGGGGGGGGGGG tidy.*/
    /*TODO this will require duplicate checks still*/
    if(xmlStrEqual(node->name, BAD_CAST "name"))
        g_strlcpy(paccount->accname, (gchar *)content, sizeof(paccount->accname));
    else if(xmlStrEqual(node->name, BAD_CAST "plugin"))
        g_strlcpy(paccount->plgname, (gchar *)content, sizeof(paccount->plgname));
    else if(xmlStrEqual(node->name, BAD_CAST "server"))
        g_strlcpy(paccount->hostname, (gchar *)content, sizeof(paccount->hostname));
    else if(xmlStrEqual(node->name, BAD_CAST "port"))
        paccount->port= (gint)xmlXPathCastStringToNumber(content);
    else if(xmlStrEqual(node->name, BAD_CAST "username"))
        g_strlcpy(paccount->username, (gchar *)content, sizeof(paccount->username));
    /*TODO icon_colour needs much reviewing*/
    else if(xmlStrEqual(node->name, BAD_CAST "icon_colour"))
        g_strlcpy(paccount->icon.colour, (gchar *)content, sizeof(paccount->icon.colour));

    /*TODO password needs a hell of a lot of work*/
    else if(xmlStrEqual(node->name, BAD_CAST "password"))
        pw_copy(paccount, (gchar *)content);

    return TRUE;
}

/*function to read an account*/
static GSList *acc_read(xmlDocPtr doc, xmlNodePtr node)
{
    xmlChar *pcontent= NULL;
    xmlNodePtr child= NULL;
    mtc_account *pnew= NULL;
	mtc_account *pfirst;
    mtc_icon *picon;
	
	pfirst= (acclist== NULL)? NULL: (mtc_account *)acclist->data;

	/*allocate mem for new account and copy data to it*/
	pnew= (mtc_account *)g_malloc0(sizeof(mtc_account));
		
	/*copy the id (0 if it is first account)*/
	pnew->id= (pfirst== NULL)? 0: pfirst->id+ 1;
	
    picon= &pnew->icon;

    child= node->children;
    while(child!= NULL)
    {
                            
        /*TODO sort duplicates too*/
        /*TODO If there are duplicates (or any other error) then the account will need to be freed and simply the account list returned*/
        if((child->type== XML_ELEMENT_NODE)&&
            (child->children->type== XML_TEXT_NODE)&& !xmlIsBlankNode(child->children))
        {
            pcontent= xmlNodeListGetString(doc, child->children, 1);
            acc_copy_element(pnew, child, pcontent);        
            xmlFree(pcontent);
        }
        child= child->next;
    }

#ifdef MTC_NOTMINIMAL
    /*TODO needs reviewing*/
	pnew->pfilters= NULL;
			
	/*if(pnew->runfilter)
	{*/ if(!filter_read(pnew))
		    pnew->runfilter= FALSE; /*}*/
#endif /*MTC_NOTMINIMAL*/
	
	/*convert any old protocols to new plugin names*/
	protocol_to_plugin(pnew->plgname);

#ifdef MTC_NOTMINIMAL
	pnew->msginfo.num_messages= -1;
#else
    pnew->num_messages= -1;
#endif /*MTC_NOTMINIMAL*/

    /*create the image*/
    picon= icon_create(picon);

	/*next account points to last account*/
	return((acclist= g_slist_prepend(acclist, pnew)));
}

/*function to get the accounts*/
static gboolean read_accounts(xmlDocPtr doc, xmlNodePtr parent)
{
    xmlNodePtr node= NULL;
                
    /*Now get all the accounts*/
    node= parent->children;
    while(node!= NULL)
    {
                    
        /*account found, now get the values from it*/
        if((xmlStrEqual(node->name, BAD_CAST "account"))&& (node->type== XML_ELEMENT_NODE)&& xmlIsBlankNode(node->children))
		    acclist= acc_read(doc, node);
            
        node= node->next;
    }
    return TRUE;
}

/*function to copy the config values*/
static gboolean cfg_copy_element(xmlNodePtr node, xmlChar *content)
{
    /*TODO  BIIIIIIIIIIIIIGGGGGGGGGGGGGG tidy.*/
    /*TODO this will require duplicate checks still*/
    if(xmlStrEqual(node->name, BAD_CAST "read_command"))
        g_strlcpy(config.mail_program, (gchar *)content, sizeof(config.mail_program));
    else if(xmlStrEqual(node->name, BAD_CAST "interval"))
        config.check_delay= (gint)xmlXPathCastStringToNumber(content);
    else if(xmlStrEqual(node->name, BAD_CAST "icon_size"))
        config.icon_size= (gint)xmlXPathCastStringToNumber(content);
    
    /*TODO icon_colour needs much reviewing, and 'multiple' value needs removing*/
    else if(xmlStrEqual(node->name, BAD_CAST "icon_colour"))
    {
        g_strlcpy(config.icon.colour, (gchar *)content, sizeof(config.icon.colour));
        /*TODO something definitely needs doing here*/
        config.multiple= TRUE;
    }
    else if(xmlStrEqual(node->name, BAD_CAST "error_frequency"))
        config.err_freq= (gint)xmlXPathCastStringToNumber(content);
    else if(xmlStrEqual(node->name, BAD_CAST "newmail_command"))
        g_strlcpy(config.nmailcmd, (gchar *)content, sizeof(config.nmailcmd));
    
    return TRUE;
}

/*function to get the required elements from the document*/
static gboolean cfg_elements(xmlDocPtr doc)
{
    xmlNodePtr node= NULL;
    xmlNodePtr child= NULL;
    xmlChar *pcontent= NULL;
    gboolean retval= TRUE;

    /*get the root element and check it is named 'config'*/
    node= xmlDocGetRootElement(doc);
    if((node== NULL)|| (!xmlStrEqual(node->name, BAD_CAST "config")))
    {
        err_noexit("Error getting config root element\n");
        return FALSE;
    }
         
    /*get the config elements*/
    child= node->children;;
    while(child!= NULL)
    {
            
        /*if it is a string value, print it*/
        if((child->type== XML_ELEMENT_NODE)&& (child->children->type== XML_TEXT_NODE)&& !xmlIsBlankNode(child->children))
        {
            /*now copy the values*/
            pcontent= xmlNodeListGetString(doc, child->children, 1);
            
            cfg_copy_element(child, pcontent);    
            xmlFree(pcontent);
        }
        /*TODO duplicate checks here*/
        else if((xmlStrEqual(child->name, BAD_CAST "accounts")))
                retval= read_accounts(doc, child);

        child= child->next;
    }
    return(retval);
}

/*function to parse the config file*/
static gboolean cfg_parse(xmlParserCtxtPtr ctxt, gchar *filename)
{
    xmlDocPtr doc; /* the resulting document tree */
    gboolean retval= TRUE;

    /* parse the file*/
    doc= xmlCtxtReadFile(ctxt, filename, NULL, 0);
    if(doc== NULL)
    {
        err_noexit("Failed to parse %s\n", filename);
        return FALSE;
    }

    /*if file is valid, get the elements*/
    if(ctxt->valid== 0)
    {
        err_noexit("Failed to validate %s\n", filename);
        retval= FALSE;
    }
    else
        retval= cfg_elements(doc);
    
    xmlFreeDoc(doc);
    return(retval);;
}

/*TODO this will eventually become the config reading function*/
gboolean cfg_read(void)
{
	gchar configfilename[NAME_MAX];
    mtc_icon *picon= NULL;
    gboolean retval= TRUE;
    xmlParserCtxtPtr ctxt= NULL;
    
   	/*get the full path of the config file*/
	mtc_file(configfilename, CFG_FILE, -1);
	
    picon= &config.icon;
	
    /*if the file does not exist create default icon and return*/
	/*TODO icon really needs to be looked at in detail*/
    if(!IS_FILE(configfilename))
	{
        
        g_strlcpy(picon->colour, "#FFFFFF", sizeof(picon)->colour);
        picon= icon_create(picon);
        return FALSE;
    }

    /*initialise libxml*/
    xml_init();
    
    /* create a parser context */
    ctxt= xmlNewParserCtxt();
    if(ctxt== NULL)
        err_exit("Failed to allocate parser context\n");
 
    /*now do some parsing*/
    retval= cfg_parse(ctxt, configfilename);

    /*freeup and cleanup libxml*/
    xmlFreeParserCtxt(ctxt);
    xml_cleanup();

    /*create the image*/
    picon= icon_create(picon);

    /*TODO errdlg out here if retval is false*/
    return(retval);
}

/*function to get an item from an account*/
mtc_account *get_account(guint item)
{
	GSList *pcurrent= acclist;
	mtc_account *pcurrent_data= NULL;

	/*iterate until the id is found*/
	while(pcurrent!= NULL)
	{
		pcurrent_data= (mtc_account *)pcurrent->data;
		if(pcurrent_data->id== item)
			return(pcurrent_data);
		pcurrent= g_slist_next(pcurrent);
	}
	return(NULL);
}

/*Function to create a new account and add to list*/
GSList *create_account(void)
{
	/*allocate memory for new account and add it to the list*/
	mtc_account *pnew= NULL;
	mtc_account *pfirst;
    mtc_icon *picon= NULL;
	
	pfirst= (acclist== NULL)? NULL: (mtc_account *)acclist->data;

	pnew= (mtc_account *)g_malloc0(sizeof(mtc_account));
	pnew->id= (pfirst== NULL)? 0: pfirst->id+ 1;
#ifdef MTC_NOTMINIMAL    
	pnew->pfilters= NULL;
#endif /*MTC_NOTMINIMAL*/

    /*create default image*/
    picon= &pnew->icon;
	g_strlcpy(picon->colour, "#FFFFFF", sizeof(picon->colour));
    picon= icon_create(picon);

	return((acclist= g_slist_prepend(acclist, pnew)));
}

#ifdef MTC_NOTMINIMAL    
/*Function to free the message list of an account*/
static void msglist_free(gpointer data, gpointer user_data)
{
    msg_struct *pmsg= (msg_struct *)data;
    user_data= NULL;

    if(pmsg->header)
    {
        g_free(pmsg->header);
        pmsg->header= NULL;
    }
    if(pmsg->uid)
    {
        g_free(pmsg->uid);
        pmsg->uid= NULL;
    }
    g_free(pmsg);
    pmsg= NULL;
}
#endif /*MTC_NOTMINIMAL*/

/*Function to free an account*/
static void free_account(gpointer data, gpointer user_data)
{
	mtc_account *paccount= (mtc_account *)data;
    mtc_icon *picon;

    picon= &paccount->icon;

#ifdef MTC_NOTMINIMAL    
    GSList *msglist;

    msglist= paccount->msginfo.msglist;
    
    /*remove the message list if any*/
    if(msglist)
    {
    	g_slist_foreach(msglist, msglist_free, NULL);
	    g_slist_free(msglist);
	    msglist= NULL;
    }

	/*remove the filter struct, then the account*/
	if(paccount->pfilters)
	{
		g_free(paccount->pfilters);
		paccount->pfilters= NULL;
	}
#endif /*MTC_NOTMINIMAL*/

    /*unref the image for our account*/
   	if(picon->image)
		g_object_unref(G_OBJECT(picon->image));

#ifndef MTC_EGGTRAYICON
    if(picon->pixbuf)
        g_object_unref(G_OBJECT(picon->pixbuf));
#endif /*MTC_EGGTRAYICON*/

    g_free(paccount);
	paccount= NULL;
}

/*remove an account from the list*/
void remove_account(guint item)
{
	GSList *pcurrent= NULL;
	mtc_account *pcurrent_data;
	
	/*get the data to remove*/
	pcurrent_data= get_account(item);
	if(pcurrent_data== NULL) return;

	/*free the data*/
	free_account((gpointer)pcurrent_data, NULL);

	/*remove from list*/
	acclist= g_slist_remove(acclist, pcurrent_data);
	
	/*now shift ids*/
	pcurrent= acclist;
	while(pcurrent!= NULL)
	{
		pcurrent_data= (mtc_account *)pcurrent->data;
		if(pcurrent_data->id<= item)
			break;
		pcurrent_data->id--;
		
		pcurrent= g_slist_next(pcurrent);
	}

}

/*free the account list*/
void free_accounts(void)
{
	/*call the remove for each one*/
	g_slist_foreach(acclist, free_account, NULL);
	g_slist_free(acclist);
	acclist= NULL;

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
static xmlNodePtr put_node_int(xmlNodePtr parent, const gchar *name, const double content)
{
    xmlNodePtr node;
    xmlChar *pstring;

    /*don't add anything if there is no value*/
    if(name== NULL)
        return(NULL);

    /*add the string*/
    pstring= xmlXPathCastNumberToString(content);
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

/*write password to xml config file*/
static gboolean pw_write(xmlNodePtr acc_node, gchar *password)
{

#ifdef MTC_USE_SSL
    gchar *encstring= NULL;
    xmlNodePtr pw_node= NULL;

/*if OpenSSL is defined encrypt the password*/
	encstring= pw_encrypt(password);
    if(encstring== NULL)
        return FALSE;
    
    pw_node= put_node_str(acc_node, "password", encstring);
    xmlNewProp(pw_node, BAD_CAST "type", BAD_CAST "base64Binary");
    g_free(encstring);
/*otherwise write a clear password*/
#else
    put_node_str(acc_node, "password", password);
#endif
    return TRUE;
}

/*TODO final version will write to same file as config*/
static gboolean acc_write(xmlNodePtr root_node)
{
    xmlNodePtr accs_node= NULL;
    xmlNodePtr acc_node= NULL;
 
	GSList *pcurrent= NULL;
    mtc_account *paccount;
    mtc_icon *picon;

    accs_node= put_node_empty(root_node, "accounts");
 
    /*iterate through the accounts and write*/
	pcurrent= acclist;
	while(pcurrent!= NULL)
	{
		paccount= (mtc_account *)pcurrent->data;
        picon= &paccount->icon;
		
        acc_node= put_node_empty(accs_node, "account");
        put_node_str(acc_node, "name", paccount->accname);
        put_node_str(acc_node, "plugin", paccount->plgname);
        put_node_str(acc_node, "server", paccount->hostname);
        put_node_int(acc_node, "port", paccount->port);
        put_node_str(acc_node, "username", paccount->username);
        put_node_str(acc_node, "icon_colour", picon->colour);
    
        /*now write the password out*/
        if(!pw_write(acc_node, paccount->password))
            return FALSE;

        /*move to next item in the list*/
		pcurrent= g_slist_next(pcurrent);
	}
    return TRUE;
}


/*write the configuration to an xml file*/
gboolean cfg_write(void)
{
    xmlDocPtr doc= NULL;
    xmlNodePtr root_node= NULL;
	gchar cfgfilename[NAME_MAX];
    mtc_icon *picon;
    gboolean exists;
    gboolean retval= TRUE;

    picon= &config.icon;
    
    /*get the full path of the config file*/
	mtc_file(cfgfilename, CFG_FILE, -1);
	
    exists= IS_FILE(cfgfilename);
    
    /*set the permissions on the file so it can only be read*/
	if(exists&& (g_chmod(cfgfilename, S_IRUSR| S_IWUSR)== -1))
		err_exit(S_FILEFUNC_ERR_SET_PERM, cfgfilename);
	
    /*if the file cannot be accessed or removed report error*/
	if(exists&& (g_remove(cfgfilename)== -1))
		err_exit(S_FILEFUNC_ERR_ATTEMPT_WRITE, cfgfilename);
	
    /*initialise libxml*/
    xml_init();
    
    /*create a new document and node, and set as root node*/
    doc= xmlNewDoc(BAD_CAST "1.0");
    root_node= xmlNewNode(NULL, BAD_CAST "config");
    xmlDocSetRootElement(doc, root_node);

    /*create new node, and "attach" as child of root node*/
    /*TODO some of these will not be there if MTC_NOT_MINIMAL is not defined*/
    put_node_str(root_node, "read_command", config.mail_program);
    put_node_int(root_node, "interval", config.check_delay);
    put_node_int(root_node, "error_frequency", config.err_freq);
    put_node_int(root_node, "icon_size", config.icon_size);
    put_node_str(root_node, "icon_colour", picon->colour);
    put_node_str(root_node, "newmail_command", config.nmailcmd);
 
    /*write out each account*/
    if(!acc_write(root_node))
        retval= FALSE;
    
    /*save the created XML*/
    xmlSaveFormatFileEnc(cfgfilename, doc, "UTF-8", 1);
    
    /* free up the resulting document */
    xmlFreeDoc(doc);
    
    /*cleanup xml*/
    xml_cleanup();

    /*set the permissions on the file so it can only be read*/
	if(g_chmod(cfgfilename, S_IRUSR)== -1)
		err_exit(S_FILEFUNC_ERR_SET_PERM, cfgfilename);
	
    return(retval);
}

/*function to remove a file in ~/.PACKAGE and shift the files so they are in order*/
gboolean rm_mtc_file(gchar *shortname, gint count, gint fullcount)
{
	gint i= 0;

	/*allocate memory and get full path of the file to delete*/
	gchar full_filename[NAME_MAX];
	gchar new_filename[NAME_MAX];

	mtc_file(full_filename, shortname, count- 1);
	
	/*remove the file*/
	if((IS_FILE(full_filename))&& (g_remove(full_filename)== -1))
		err_exit(S_FILEFUNC_ERR_REMOVE_FILE, full_filename);

	/*traverse through each of the files after the removed one*/
	for(i= count; i< fullcount; i++)
	{
		/*get the name of the file and the file before it*/
		mtc_file(full_filename, shortname, i);
		mtc_file(new_filename, shortname, i- 1);
		
		/*rename file to file- 1 so that there are no gaps*/
		if((IS_FILE(full_filename))&& (g_rename(full_filename, new_filename)== -1))
			err_exit(S_FILEFUNC_ERR_RENAME_FILE, full_filename, new_filename);
	}
	return TRUE;
}

