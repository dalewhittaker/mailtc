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

/*generic read a string from file function*/
static void fread_string_fail(FILE *pfile, gchar *pstring, gint slen, gchar *perr, const gchar *fname)
{
    if(fgets(pstring, slen, pfile)== NULL)
    {
        g_remove(fname);
        err_exit(perr);
    }
    else
        g_strchomp(pstring);
}

/*generic read a string from file function*/
static void fread_string(FILE *pfile, gchar *pstring, gint slen, gchar *dflstring)
{
    gchar *test= NULL;
    test= fgets(pstring, slen, pfile);

    if(test== NULL|| pstring== NULL)
	{
        if(dflstring== NULL)
            memset(pstring, 0, slen);
        else
            g_strlcpy(pstring, dflstring, slen);
    }
    g_strchomp(pstring);
}

/*generic read integer from file function*/
static void fread_integer(FILE *pfile, gint *pval, gint dflval)
{
    gchar scratch[G_ASCII_DTOSTR_BUF_SIZE];

    if(fgets(scratch, sizeof(scratch), pfile)== NULL)
        *pval= dflval;
    else
        *pval= (gint)g_ascii_strtod(scratch, NULL);
}

/*generic read integer from file function*/
static void fread_integer_fail(FILE *pfile, gint *pval, gchar *perr, const gchar *fname)
{
    gchar scratch[G_ASCII_DTOSTR_BUF_SIZE];

    if(fgets(scratch, sizeof(scratch), pfile)== NULL)
    {
        g_remove(fname);
        err_exit(perr);
    }
    *pval= (gint)g_ascii_strtod(scratch, NULL);
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

/*function to get individual element*/
/*TODO this needs loads of work*/
static gboolean get_node_str(xmlXPathContextPtr xpctx, const gchar *spath)
{
    int nodenr= 0;
    gboolean retval= TRUE;
    xmlXPathObjectPtr xpobj;
    xmlChar *path;

    path= BAD_CAST spath;

    /*get the list, if any*/
    xpobj= xmlXPathEvalExpression(path, xpctx);
    if(xpobj== NULL)
    {
        err_noexit("Error: unable to evaluate xpath expression \"%s\"\n", spath);
        return FALSE;
    }

    /*verify that there is only 1 node found*/
    nodenr= xpobj->nodesetval->nodeNr;

    /*TODO this will all probably change*/
    if(nodenr== 1)
    {
        xmlNodePtr node= NULL;
        xmlChar *pcontent= NULL;

        /*Ok we got a single node, now get the content*/
        node= xpobj->nodesetval->nodeTab[0];

        pcontent= xmlNodeGetContent(node);
        if(pcontent!= NULL)
        {
            /*TODO now, here it will either be converted to an int or just copied to a string*/
            g_print("%s = %s\n", spath, pcontent);
            xmlFree(pcontent);
        }
        /*TODO sort if there is no value (i.e either error or copy a default)*/
    }
    else
    {
        /*if it is greater than 1, error, as there cannot be duplicates
        if it is less then there is no value*/
        if(nodenr> 1)
        {
            err_noexit("Error: duplicate elements found for expression \"%s\"\n", spath);
            retval= FALSE;
        }
        /*TODO sort if there is no value (i.e either error or copy a default)*/
    }

    /*free the object*/
    xmlXPathFreeObject(xpobj);

    return(retval);
}

/*function to get the required elements from the document*/
static gboolean cfg_elements(xmlDocPtr doc)
{
    xmlNodePtr node= NULL;
    xmlXPathContextPtr xpctx;
    gboolean retval= TRUE;

    /*get the root element and check it is named 'config'*/
    node= xmlDocGetRootElement(doc);
    if((node== NULL)|| (!xmlStrEqual(node->name, BAD_CAST "config")))
    {
        err_noexit("Error getting config root element\n");
        return FALSE;
    }

    /* Create xpath evaluation context */
    xpctx= xmlXPathNewContext(doc);
    if(xpctx== NULL)
    {
        err_noexit("Error: unable to create new XPath context\n");
        retval= FALSE;
    }
    else
    {
        /*get all of the values
         *TODO this will most likely be changing somehow*/
        if(!get_node_str(xpctx, "/config/read_command")||
           !get_node_str(xpctx, "/config/interval")||
           !get_node_str(xpctx, "/config/error_frequency")||
           !get_node_str(xpctx, "/config/icon_size")||
           !get_node_str(xpctx, "/config/icon_colour")||
           !get_node_str(xpctx, "/config/newmail_command"))
        {
            retval= FALSE;
        }
        else
        {
            /*TODO continue and get the account stuff*/
        }
    }
    /*free the xpath evaluation context*/
    xmlXPathFreeContext(xpctx);

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
gboolean xml_cfg_read(void)
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

    /*TODO errdlg out here if retval is false*/
    return TRUE;
}

/* function to get details from config file into struct or if no config file exists ask for details*/
gboolean cfg_read(void)
{
	gchar configfilename[NAME_MAX];
	FILE *pfile;
    mtc_icon *picon= NULL;

	/*get the full path of the config file*/
	mtc_file(configfilename, CONFIG_FILE, -1);
	
    picon= &config.icon;
	
    /*if the file does not exist create default icon and return*/
	if(!IS_FILE(configfilename))
	{
        
        g_strlcpy(picon->colour, "#FFFFFF", sizeof(picon)->colour);
        picon= icon_create(picon);
        return FALSE;
    }

	/*open the config file for reading*/
	if((pfile= g_fopen(configfilename, "rt"))== NULL)
		err_exit(S_FILEFUNC_ERR_OPEN_FILE, configfilename);
	
    /*get the info from the file*/
    fread_integer_fail(pfile, (gint *)&config.check_delay, S_FILEFUNC_ERR_GET_DELAY, configfilename);
	fread_string_fail(pfile, config.mail_program, sizeof(config.mail_program), S_FILEFUNC_ERR_GET_MAILAPP, configfilename);
    fread_integer(pfile, (gint *)&config.icon_size, 24);
    fread_integer(pfile, (gint *)&config.multiple, FALSE);
	fread_string(pfile, picon->colour, sizeof(config.icon.colour), "#FFFFFF");
    fread_integer(pfile, &config.err_freq, 1);
#ifdef MTC_NOTMINIMAL
    fread_string(pfile, config.nmailcmd, sizeof(config.nmailcmd), NULL);
#endif /*MTC_NOTMINIMAL*/

#ifdef MTC_EXPERIMENTAL
    fread_integer(pfile, (gint *)&config.run_summary, FALSE);
    fread_integer(pfile, (gint *)&config.summary.wpos, WPOS_BOTTOMRIGHT);
    fread_string(pfile, config.summary.sfont, sizeof(config.summary.sfont), "NULL");
    if(g_ascii_strncasecmp(config.summary.sfont, "NULL", 4)== 0)
        memset(config.summary.sfont, 0, MAX_FONTNAME_LEN);
#endif

    /*create the image*/
    picon= icon_create(picon);

   	/*close the config file*/
	if(fclose(pfile)== EOF)
		err_exit(S_FILEFUNC_ERR_CLOSE_FILE, configfilename);
	
    /*temporary to test*/
    xml_cfg_read();
	return TRUE;
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
			/*pbasename= g_path_get_basename(g_module_name((GModule *)pitem->handle));*/
            pbasename= plg_name(pitem);
			g_strlcpy(plgstring, pbasename, PROTOCOL_LEN);
			g_free(pbasename);
			return(plgstring);
		}
		pcurrent= g_slist_next(pcurrent);
	}
	return(plgstring);
}

/*function to read in the password (encrypted or not) from the file*/
static gboolean pw_read(FILE *pfile, mtc_account *paccount)
{

#ifdef MTC_USE_SSL
	gchar encstring[BASE64_PASSWORD_LEN];
#endif
	
/*if OpenSSL is defined read in an encrypted password*/
#ifdef MTC_USE_SSL
	memset(encstring, '\0', BASE64_PASSWORD_LEN);
	
	/*read in the encrypted password and remove the password if there is an error reading it*/
    fread_string(pfile, encstring, sizeof(encstring), NULL);
	if(encstring[0]== 0)
		err_exit(S_FILEFUNC_ERR_GET_PW);
	
	/*decrypt the password*/
	pw_decrypt(encstring, paccount->password);

/*otherwise read in clear password*/
#else
	/*get the password value and remove the file if it cannot be read*/
	fread_string_fail(pfile, paccount->password, sizeof(paccount->password), S_FILEFUNC_ERR_GET_PW, passwordfilename);
#endif
	
	return TRUE;
}

/*Function to create/read in a new account*/
static GSList *acc_read(FILE *pfile, const gchar *detailsfilename)
{
	mtc_account *pnew= NULL;
	mtc_account *pfirst;
    mtc_icon *picon;
	
	pfirst= (acclist== NULL)? NULL: (mtc_account *)acclist->data;

	/*allocate mem for new account and copy data to it*/
	pnew= (mtc_account *)g_malloc0(sizeof(mtc_account));
		
	/*copy the id (0 if it is first account)*/
	pnew->id= (pfirst== NULL)? 0: pfirst->id+ 1;
	
    picon= &pnew->icon;

    /*read in our account info*/
    fread_string_fail(pfile, pnew->hostname, sizeof(pnew->hostname), S_FILEFUNC_ERR_GET_HOSTNAME, detailsfilename);
    fread_integer_fail(pfile, &pnew->port, S_FILEFUNC_ERR_GET_PORT, detailsfilename);
	fread_string_fail(pfile, pnew->username, sizeof(pnew->username), S_FILEFUNC_ERR_GET_USERNAME, detailsfilename);

    /*read in the password*/
	if(!pw_read(pfile, pnew))
		err_exit(S_FILEFUNC_ERR_GET_PASSWORD, pnew->id);
	
	fread_string_fail(pfile, picon->colour, sizeof(pnew->icon), S_FILEFUNC_ERR_GET_ICONTYPE, detailsfilename);
    fread_string_fail(pfile, pnew->plgname, sizeof(pnew->plgname), S_FILEFUNC_ERR_READ_PLUGIN, detailsfilename);
	fread_string(pfile, pnew->accname, sizeof(pnew->accname), S_FILEFUNC_DEFAULT_ACCNAME);

#ifdef MTC_NOTMINIMAL
	fread_integer(pfile, (gint *)&pnew->runfilter, FALSE);
#endif /*MTC_NOTMINIMAL*/

#ifdef MTC_NOTMINIMAL
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

/*Function to read all accounts from the files and add to the list*/
gboolean read_accounts(void)
{
	FILE* pfile;
	gchar detailsfilename[NAME_MAX];
	guint i= 0;
	
	/*iterate through each account*/
	mtc_file(detailsfilename, DETAILS_FILE, i);
	while(IS_FILE(detailsfilename))
	{
		
		/*open the details file for reading*/
		if((pfile= g_fopen(detailsfilename, "rt"))== NULL)
			err_exit(S_FILEFUNC_ERR_OPEN_FILE, detailsfilename);

		acclist= acc_read(pfile, detailsfilename);
		
		/*close the details file*/
		if(fclose(pfile)== EOF)
			err_exit(S_FILEFUNC_ERR_CLOSE_FILE, detailsfilename);
	
		mtc_file(detailsfilename, DETAILS_FILE, ++i);
		
	}
	return TRUE;
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
static void pw_xml_write(xmlNodePtr acc_node, gchar *password)
{

#ifdef MTC_USE_SSL
    gchar *encstring= NULL;
    xmlNodePtr pw_node= NULL;

/*if OpenSSL is defined encrypt the password*/
	encstring= pw_encrypt(password);
    pw_node= put_node_str(acc_node, "password", encstring);
    xmlNewProp(pw_node, BAD_CAST "type", BAD_CAST "base64Binary");
    g_free(encstring);
/*otherwise write a clear password*/
#else
    put_node_str(acc_node, "password", password);
#endif

}

/*TODO final version will write to same file as config*/
static void acc_xml_write(xmlNodePtr root_node)
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
        pw_xml_write(acc_node, paccount->password);

        /*move to next item in the list*/
		pcurrent= g_slist_next(pcurrent);
	}
}


/*write the configuration to an xml file*/
gboolean cfg_write(void)
{
    xmlDocPtr doc= NULL;
    xmlNodePtr root_node= NULL;
	gchar cfgfilename[NAME_MAX];
    mtc_icon *picon;
    gboolean exists;

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
    acc_xml_write(root_node);
    
    /*save the created XML*/
    xmlSaveFormatFileEnc(cfgfilename, doc, "UTF-8", 1);
    
    /* free up the resulting document */
    xmlFreeDoc(doc);
    
    /*cleanup xml*/
    xml_cleanup();

    /*set the permissions on the file so it can only be read*/
	if(g_chmod(cfgfilename, S_IRUSR)== -1)
		err_exit(S_FILEFUNC_ERR_SET_PERM, cfgfilename);
	
    return(TRUE);
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

