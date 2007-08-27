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
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "filefunc.h"
#include "plugfunc.h"

#ifdef MTC_NOTMINIMAL
#include "filterdlg.h"
#endif /*MTC_NOTMINIMAL*/

#ifdef MTC_USE_SSL
#include "encrypter.h"
#endif /*MTC_USE_SSL*/

/*define the various element names*/
#define ELEMENT_CONFIG         "config"
#define ELEMENT_READCOMMAND    "read_command"
#define ELEMENT_INTERVAL       "interval"
#define ELEMENT_ERRORFREQUENCY "error_frequency"
#define ELEMENT_MULTIPLEICON   "multiple_icon"
#define ELEMENT_ICONSIZE       "icon_size"
#define ELEMENT_ICONCOLOUR     "icon_colour"
#define ELEMENT_NEWMAILCOMMAND "newmail_command"
#define ELEMENT_ACCOUNTS       "accounts"
#define ELEMENT_ACCOUNT        "account"
#define ELEMENT_NAME           "name"
#define ELEMENT_PLUGINNAME     "plugin_name"
#define ELEMENT_SERVER         "server"
#define ELEMENT_PORT           "port"
#define ELEMENT_USERNAME       "username"
#define ELEMENT_ENCPASSWORD    "enc_password"
#define ELEMENT_PASSWORD       "password"

/*define the config file encoding*/
#define CFGFILE_ENCODING "UTF-8"

#define BASE64_PASSWORD_LEN (PASSWORD_LEN* 4/ 3+ 6)

#ifdef _POSIX_SOURCE
#define MKDIR_MODE (S_IRWXU| S_IRWXG| S_IROTH| S_IXOTH)
#else
#define MKDIR_MODE (S_IRWXU)
#endif /*_POSIX_SOURCE*/

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
            if(g_mkdir(pfile, MKDIR_MODE)== -1)
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
		msgbox_fatal(S_FILEFUNC_ERR_GET_HOMEDIR);
	
    /*create the dir if it doesn't exist*/
    mk_dir(pcfg);

    /*second, create the .config/mailtc dir*/
    if((pmtc= g_build_filename(pcfg, PACKAGE, NULL))== NULL)
	{
        g_free(pcfg);
		msgbox_fatal(S_FILEFUNC_ERR_GET_HOMEDIR);
	}

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

/*wrapper to report if there is a duplicate*/
static gboolean isduplicate(elist *element)
{
    gboolean retval= FALSE;

    if(element->found> 0)
    {
        msgbox_warn(S_FILEFUNC_ERR_ELEMENT_DUPLICATE, element->name);
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

/*copy an int element*/
static gboolean get_node_int(elist *element, const xmlChar *src)
{
    gint *pdest;

    if(isduplicate(element))
        return FALSE;

    pdest= (gint *)element->value;

    *pdest= (gint)xmlXPathCastStringToNumber(src);
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

/*function to read in the password (encrypted or not) from the file*/
static gboolean pw_read(elist *element, xmlChar *src)
{
    gchar *pdest;
    gchar *psrc;

    if(isduplicate(element))
        return FALSE;

    pdest= (gchar *)element->value;
    psrc= (gchar *)src;
    
    /*there are two possible password elements, the encrypted and non-encrypted form
     *check if both are present and it is invalid, so error*/
    if(*pdest!= 0)
    {
        msgbox_warn(S_FILEFUNC_ERR_PASSWORD_ELEMENTS);
        /*wipe the value*/
        memset(pdest, '\0', element->length);
        return FALSE;
    }

/*if OpenSSL is defined read in an encrypted password*/
#ifdef MTC_USE_SSL
    if(g_ascii_strcasecmp(element->name, ELEMENT_ENCPASSWORD)== 0)
	{
        /*decrypt the password*/
        return(pw_decrypt(psrc, pdest));
    }
    else
    {
#endif /*MTC_USE_SSL*/
/*otherwise read in clear password*/
        g_strlcpy(pdest, psrc, sizeof(pdest));
	    return TRUE;
#ifdef MTC_USE_SSL
    }
#endif /*MTC_USE_SSL*/
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
        case EL_INT:
            retval= get_node_int(pelement, content);
        break;
        case EL_BOOL:
            retval= get_node_bool(pelement, content);
        break;
        case EL_PW:
            retval= pw_read(pelement, content);
        break;
        default: ;
    }
    return(retval);
}

/*function to copy the config values*/
gboolean cfg_copy_element(xmlNodePtr node, elist *pelement, xmlChar *content)
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

/*function to read an account*/
static gboolean acc_read(xmlDocPtr doc, xmlNodePtr node)
{
    mtc_account *pnew= NULL;
	mtc_account *pfirst= NULL;
    mtc_icon *picon= NULL;
    gboolean retval= TRUE;

    pfirst= (acclist== NULL)? NULL: (mtc_account *)acclist->data;

	/*allocate mem for new account and copy data to it*/
	pnew= (mtc_account *)g_malloc0(sizeof(mtc_account));
    
	/*copy the id (0 if it is first account)*/
	pnew->id= (pfirst== NULL)? 0: pfirst->id+ 1;
	
    picon= &pnew->icon;

#ifdef MTC_NOTMINIMAL
	pnew->pfilters= NULL;
#endif /*MTC_NOTMINIMAL*/

    /*yeah, putting in brackets here aint great*/
    {
        xmlChar *pcontent= NULL;
        xmlNodePtr child= NULL;
        elist *pelement;
        
        elist elements[]=
        {
            { ELEMENT_NAME,         EL_STR,  pnew->name,        sizeof(pnew->name),        0 },
            { ELEMENT_PLUGINNAME,   EL_STR,  pnew->plgname,     sizeof(pnew->plgname),     0 },
            { ELEMENT_SERVER,       EL_STR,  pnew->server,      sizeof(pnew->server),      0 },
            { ELEMENT_PORT,         EL_INT,  &pnew->port,       sizeof(pnew->port),        0 },
            { ELEMENT_USERNAME,     EL_STR,  pnew->username,    sizeof(pnew->username),    0 },
            { ELEMENT_ICONCOLOUR,   EL_STR,  pnew->icon.colour, sizeof(pnew->icon.colour), 0 },
            { ELEMENT_ENCPASSWORD,  EL_PW,   pnew->password,    sizeof(pnew->password),    0 },
            { ELEMENT_PASSWORD,     EL_PW,   pnew->password,    sizeof(pnew->password),    0 },
            { NULL, EL_NULL, NULL, 0, 0 },
        };

        /*copy default values that are not required in config*/
        g_snprintf(pnew->name, sizeof(pnew->name), S_FILEFUNC_DEFAULT_ACC_STRING, pnew->id+ 1);

        child= node->children;
        while(child!= NULL)
        {
            /*if found, copy the account elements*/
            if((child->type== XML_ELEMENT_NODE)&& (child->children!= NULL)&&
                (child->children->type== XML_TEXT_NODE)&& !xmlIsBlankNode(child->children))
            {
                pcontent= xmlNodeListGetString(doc, child->children, 1);
        
                if(cfg_copy_element(child, elements, pcontent)== FALSE)
                    retval= FALSE;

                xmlFree(pcontent);
            }
#ifdef MTC_NOTMINIMAL
            else
            {    /*read in any filters*/
                if(read_filters(doc, child, pnew)== FALSE)
                    retval= FALSE;
            }
#endif /*MTC_NOTMINIMAL*/
            child= child->next;
        }

        /*now check for missing elements*/

        /*another hack, as the account name should not be checked.  this should really be revisted at some point*/
        pelement= &elements[1];
        while(pelement->name!= NULL)
        {
            /*this is admittedly a dirty hack, as the password element is an exception
             *either enc_password or password will have a 'found' of 0, so check the value*/
            if((!pelement->found&& (pelement->type!= EL_PW))||
                ((pelement->type== EL_PW)&& (pnew->password[0]== 0)))
            {
                msgbox_warn(S_FILEFUNC_ERR_ACC_ELEMENT_NOT_FOUND, pelement->name, pnew->id);           
                retval= FALSE;
            }
            pelement++;
        }

    }

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
	acclist= g_slist_prepend(acclist, pnew);
    return(retval);
}

/*function to get the accounts*/
static gboolean read_accounts(xmlDocPtr doc, xmlNodePtr parent)
{
    xmlNodePtr node= NULL;
    gboolean retval= TRUE;
                
    /*Now get all the accounts*/
    /*NOTE accounts are read backwards, this avoids them getting swapped when written.*/
    node= parent->last;
    while(node!= NULL)
    {
                    
        /*account found, now get the values from it*/
        if((xmlStrEqual(node->name, BAD_CAST ELEMENT_ACCOUNT))&& (node->type== XML_ELEMENT_NODE)&& xmlIsBlankNode(node->children))
            if(acc_read(doc, node)== FALSE)
                retval= FALSE;
        
        node= node->prev;
    }
    return(retval);
}

/*function to get the required elements from the document*/
static gboolean cfg_elements(xmlDocPtr doc)
{
    xmlNodePtr node= NULL;
    xmlNodePtr child= NULL;
    xmlChar *pcontent= NULL;
    gboolean retval= TRUE;
    elist elements[]=
    {
        { ELEMENT_READCOMMAND,    EL_STR,  config.read_command, sizeof(config.read_command), 0 },
        { ELEMENT_INTERVAL,       EL_INT,  &config.interval,    sizeof(config.interval),     0 },
        { ELEMENT_MULTIPLEICON,   EL_BOOL, &config.multiple,    sizeof(config.multiple),     0 },
        { ELEMENT_ICONSIZE,       EL_INT,  &config.icon_size,   sizeof(config.icon_size),    0 },
        { ELEMENT_ICONCOLOUR,     EL_STR,  config.icon.colour,  sizeof(config.icon.colour),  0 },
        { ELEMENT_ERRORFREQUENCY, EL_INT,  &config.err_freq,    sizeof(config.err_freq),     0 },
#ifdef MTC_NOTMINIMAL
        { ELEMENT_NEWMAILCOMMAND, EL_STR,  config.nmailcmd,     sizeof(config.nmailcmd),     0 },
#endif /*MTC_NOTMINIMAL*/
        { NULL, EL_NULL, NULL, 0, 0 },
    };

    /*copy some default values in case they are not present in the config*/
    g_strlcpy(config.icon.colour, "#FFFFFF", sizeof(config.icon.colour));
    config.interval= 1;
    config.icon_size= 24;
    config.err_freq= 1;

    /*get the root element and check it is named 'config'*/
    node= xmlDocGetRootElement(doc);
    if((node== NULL)|| (!xmlStrEqual(node->name, BAD_CAST ELEMENT_CONFIG)))
    {
        msgbox_warn(S_FILEFUNC_ERR_NO_ROOT_ELEMENT);
        return FALSE;
    }
         
    /*get the config elements*/
    child= node->children;;
    while(child!= NULL)
    {
            
        /*if it is a string value, print it*/
        if((child->type== XML_ELEMENT_NODE)&& (child->children!= NULL)&& 
            (child->children->type== XML_TEXT_NODE)&& !xmlIsBlankNode(child->children))
        {
            /*now copy the values*/
            pcontent= xmlNodeListGetString(doc, child->children, 1);
            
            if(cfg_copy_element(child, elements, pcontent)== FALSE)
                retval= FALSE;

            xmlFree(pcontent);

        }
        /*if the accounts are found, read them in*/
        else if(xmlIsBlankNode(child->children)&& (xmlStrEqual(child->name, BAD_CAST ELEMENT_ACCOUNTS)))
            if(read_accounts(doc, child)== FALSE)
                retval= FALSE;

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
    doc= xmlCtxtReadFile(ctxt, filename, NULL,
            XML_PARSE_NOWARNING| XML_PARSE_NOERROR| XML_PARSE_PEDANTIC);
    if(doc== NULL)
    {
        xmlErrorPtr perror;

        perror= xmlCtxtGetLastError(ctxt);
        msgbox_warn(S_FILEFUNC_ERR_CFG_PARSE, filename, perror->message);
        xmlCtxtResetLastError(ctxt);
        return FALSE;
    }

    /*if file is valid, get the elements*/
    if(ctxt->valid== 0)
    {
        msgbox_warn(S_FILEFUNC_ERR_CFG_VALIDATE, filename);
        retval= FALSE;
    }
    else
        retval= cfg_elements(doc);
    
    xmlFreeDoc(doc);
    return(retval);;
}

/*read in the configuration from the config file*/
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
    if(!IS_FILE(configfilename))
	{
        
        g_strlcpy(picon->colour, "#FFFFFF", sizeof(picon->colour));
        picon= icon_create(picon);
        msgbox_warn(S_MAIN_NO_CONFIG_FOUND);
        return FALSE;
    }
    /*initialise libxml*/
    xml_init();
    
    /* create a parser context */
    ctxt= xmlNewParserCtxt();
    if(ctxt== NULL)
    {
        /*this is a fatal error, so exit*/
        xml_cleanup();
        msgbox_fatal(S_FILEFUNC_ERR_PARSER_CTX);
    }
    /*now do some parsing*/
    retval= cfg_parse(ctxt, configfilename);

    /*freeup and cleanup libxml*/
    xmlFreeParserCtxt(ctxt);
    xml_cleanup();

    /*create the image*/
    if(retval!= TRUE)
        g_strlcpy(picon->colour, "#FFFFFF", sizeof(picon->colour));
    
    picon= icon_create(picon);

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
static void msglist_free(gpointer data)
{
    msg_struct *pmsg= (msg_struct *)data;

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
static void free_account(gpointer data)
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
    	g_slist_foreach(msglist, (GFunc)msglist_free, NULL);
	    g_slist_free(msglist);
	    msglist= NULL;
    }

	/*remove the filter struct, then the account*/
    free_filters(paccount);
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
	mtc_plugin *pitem= NULL;
	mtc_account *pcurrent_data;
    guint naccounts= 0;
	
	/*get the data to remove*/
	pcurrent_data= get_account(item);
	if(pcurrent_data== NULL) return;

    /*get the plugin and call it's remove function*/
	if(*pcurrent_data->plgname!= 0)
    {
        if((pitem= plg_find(pcurrent_data->plgname))== NULL)
		    msgbox_fatal(S_DOCKLET_ERR_FIND_PLUGIN_MSG, pcurrent_data->plgname, pcurrent_data->name);
    
        naccounts= g_slist_length(acclist);
	    if((*pitem->remove)(pcurrent_data, &naccounts)!= MTC_RETURN_TRUE)
            exit(EXIT_FAILURE);
    }

    /*free the data*/
	free_account((gpointer)pcurrent_data);

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
	g_slist_foreach(acclist, (GFunc)free_account, NULL);
	g_slist_free(acclist);
	acclist= NULL;

}

/*add a node of type string*/
xmlNodePtr put_node_str(xmlNodePtr parent, const gchar *name, const gchar *content)
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

/*add a node of type integer*/
xmlNodePtr put_node_bool(xmlNodePtr parent, const gchar *name, const gboolean content)
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
xmlNodePtr put_node_empty(xmlNodePtr parent, const gchar *name)
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
    
    /*TODO would be nice to use a common elist here*/
    pw_node= put_node_str(acc_node, ELEMENT_ENCPASSWORD, encstring);
    g_free(encstring);
/*otherwise write a clear password*/
#else
    put_node_str(acc_node, ELEMENT_PASSWORD, password);
#endif
    return TRUE;
}

/*write the accounts to the config file*/
static gboolean acc_write(xmlNodePtr root_node)
{
    xmlNodePtr accs_node= NULL;
    xmlNodePtr acc_node= NULL;
 
	GSList *pcurrent= NULL;
    mtc_account *paccount;
    mtc_icon *picon;

	pcurrent= acclist;
    
    if(pcurrent!= NULL)
        accs_node= put_node_empty(root_node, ELEMENT_ACCOUNTS);
	
    /*iterate through the accounts and write*/
    while(pcurrent!= NULL)
	{
		paccount= (mtc_account *)pcurrent->data;
        picon= &paccount->icon;
		
        /*TODO would be nice to use a common elist here*/
        acc_node= put_node_empty(accs_node, ELEMENT_ACCOUNT);
        put_node_str(acc_node, ELEMENT_NAME, paccount->name);
        put_node_str(acc_node, ELEMENT_PLUGINNAME, paccount->plgname);
        put_node_str(acc_node, ELEMENT_SERVER, paccount->server);
        put_node_int(acc_node, ELEMENT_PORT, paccount->port);
        put_node_str(acc_node, ELEMENT_USERNAME, paccount->username);
        put_node_str(acc_node, ELEMENT_ICONCOLOUR, picon->colour);
    
        /*now write the password out*/
        if(!pw_write(acc_node, paccount->password))
            return FALSE;
        
#ifdef MTC_NOTMINIMAL
        /*and then also the filter, if there is one*/
        if(!filter_write(acc_node, paccount))
            return FALSE;
#endif /*MTC_NOTMINIMAL*/

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
		msgbox_fatal(S_FILEFUNC_ERR_SET_PERM, cfgfilename);
	
    /*if the file cannot be accessed or removed report error*/
	if(exists&& (g_remove(cfgfilename)== -1))
		msgbox_fatal(S_FILEFUNC_ERR_ATTEMPT_WRITE, cfgfilename);
	
    /*initialise libxml*/
    xml_init();
    
    /*create a new document and node, and set as root node*/
    doc= xmlNewDoc(BAD_CAST "1.0");
    root_node= xmlNewNode(NULL, BAD_CAST ELEMENT_CONFIG);
    xmlDocSetRootElement(doc, root_node);

    /*create new node, and "attach" as child of root node*/
    /*TODO would be nice to use a common elist here*/
    put_node_str(root_node, ELEMENT_READCOMMAND, config.read_command);
    put_node_int(root_node, ELEMENT_INTERVAL, config.interval);
    put_node_int(root_node, ELEMENT_ERRORFREQUENCY, config.err_freq);
    put_node_bool(root_node, ELEMENT_MULTIPLEICON, config.multiple);
    put_node_int(root_node, ELEMENT_ICONSIZE, config.icon_size);
    put_node_str(root_node, ELEMENT_ICONCOLOUR, picon->colour);
#ifdef MTC_NOTMINIMAL
    put_node_str(root_node, ELEMENT_NEWMAILCOMMAND, config.nmailcmd);
#endif /*MTC_NOTMINIMAL*/
 
    /*write out each account*/
    if(!acc_write(root_node))
        retval= FALSE;
    
    /*save the created XML*/
    if(xmlSaveFormatFileEnc(cfgfilename, doc, CFGFILE_ENCODING, 1)== -1)
    {
        xmlErrorPtr perror;

        perror= xmlGetLastError();
        msgbox_err(S_FILEFUNC_ERR_CFG_WRITE, perror->message);
        xmlResetLastError();
        retval= FALSE;
    }
    
    /* free up the resulting document */
    xmlFreeDoc(doc);
    
    /*cleanup xml*/
    xml_cleanup();

    /*set the permissions on the file so it can only be read*/
	if(g_chmod(cfgfilename, S_IRUSR)== -1)
		msgbox_fatal(S_FILEFUNC_ERR_SET_PERM, cfgfilename);
	
    return(retval);
}

