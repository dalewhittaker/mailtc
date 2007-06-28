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

typedef enum _eltype { EL_NULL= 0, EL_STR, EL_INT, EL_BOOL, EL_PW } eltype;

/*structure used when reading in the xml config elements*/
typedef struct _elist
{
    gchar *name; /*the element name*/
    eltype type; /*type used for copying*/
    void *value; /*the config value*/
    gint length; /*length in bytes of config value*/
    gboolean found; /*used to track duplicates, or not found at all*/
} elist;

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

/*wrapper to report if there is a duplicate*/
static gboolean isduplicate(elist *element)
{
    gboolean retval= FALSE;

    if(element->found> 0)
    {
        err_noexit("Error: duplicate element '%s'\n", element->name);
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

    if(isduplicate(element))
        return FALSE;
    
    pdest= (gchar *)element->value;
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
        err_noexit("Error: encrypted and unencrypted password elements found.\n");
        return FALSE;
    }

/*if OpenSSL is defined read in an encrypted password*/
#ifdef MTC_USE_SSL
    if(g_ascii_strcasecmp(element->name, "enc_password")== 0)
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

/*function to read an account*/
static gboolean acc_read(xmlDocPtr doc, xmlNodePtr node)
{
    mtc_account *pnew= NULL;
	mtc_account *pfirst= NULL;
    mtc_icon *picon= NULL;

    pfirst= (acclist== NULL)? NULL: (mtc_account *)acclist->data;

	/*allocate mem for new account and copy data to it*/
	pnew= (mtc_account *)g_malloc0(sizeof(mtc_account));
    
	/*copy the id (0 if it is first account)*/
	pnew->id= (pfirst== NULL)? 0: pfirst->id+ 1;
	
    picon= &pnew->icon;

    /*yeah, putting in brackets here aint great*/
    {
        xmlChar *pcontent= NULL;
        xmlNodePtr child= NULL;
        gboolean retval= TRUE;
        
        elist elements[]=
        {
            { "name",         EL_STR,  pnew->accname,     sizeof(pnew->accname),     0 },
            { "plugin",       EL_STR,  pnew->plgname,     sizeof(pnew->plgname),     0 },
            { "server",       EL_STR,  pnew->hostname,    sizeof(pnew->hostname),    0 },
            { "port",         EL_INT,  &pnew->port,       sizeof(pnew->port),        0 },
            { "username",     EL_STR,  pnew->username,    sizeof(pnew->username),    0 },
            { "icon_colour",  EL_STR,  pnew->icon.colour, sizeof(pnew->icon.colour), 0 },
            { "enc_password", EL_PW,  pnew->password,     sizeof(pnew->password),    0 },
            { "password",     EL_PW,  pnew->password,     sizeof(pnew->password),    0 },
            { NULL, EL_NULL, NULL, 0, 0 },
        };

        /*copy default values that are not required in config*/
        g_snprintf(pnew->accname, sizeof(pnew->accname), "Account %d", pnew->id+ 1);

        child= node->children;
        while(child!= NULL)
        {
                
            /*if found, copy the account elements*/
            if((child->type== XML_ELEMENT_NODE)&&
                (child->children->type== XML_TEXT_NODE)&& !xmlIsBlankNode(child->children))
            {
                pcontent= xmlNodeListGetString(doc, child->children, 1);
                retval= cfg_copy_element(child, elements, pcontent);    
                xmlFree(pcontent);

                /*if it returned an error free the account then return*/
                if(retval== FALSE)
                    break;
            }
            child= child->next;
        }

        /*now check for missing elements*/
        if(retval== TRUE)
        {
            elist *pelement;

            /*another hack, as the account name should not be checked.  this should really be revisted at some point*/
            pelement= &elements[1];
            while(pelement->name!= NULL)
            {
                /*this is admittedly a dirty hack, as the password element is an exception
                 *either enc_password or password will have a 'found' of 0, so check the value*/
                if((!pelement->found&& (pelement->type!= EL_PW))||
                    ((pelement->type== EL_PW)&& (pnew->password[0]== 0)))
                {
                    err_noexit("Error: '%s' element not found for account %d\n", pelement->name, pnew->id);           
                    retval= FALSE;
                }
                pelement++;
            }
        }

        /*free if there were any errors*/
        if(retval== FALSE)
        {
            g_free(pnew);
            pnew= NULL;

            /*simply return the existing pointer to list*/
            /*return(NULL);*/
            return FALSE;
        }
   
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
	acclist= g_slist_prepend(acclist, pnew);
    return TRUE;
	/*return((acclist= g_slist_prepend(acclist, pnew)));*/
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
        if((xmlStrEqual(node->name, BAD_CAST "account"))&& (node->type== XML_ELEMENT_NODE)&& xmlIsBlankNode(node->children))
		{
            retval= acc_read(doc, node);
            /*acclist= acc_read(doc, node);*/
        }
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
        { "read_command",    EL_STR,  config.mail_program, sizeof(config.mail_program), 0 },
        { "interval",        EL_INT,  &config.check_delay, sizeof(config.check_delay),  0 },
        { "multiple_icon",   EL_BOOL, &config.multiple,    sizeof(config.multiple),     0 },
        { "icon_size",       EL_INT,  &config.icon_size,   sizeof(config.icon_size),    0 },
        { "icon_colour",     EL_STR,  config.icon.colour,  sizeof(config.icon.colour),  0 },
        { "error_frequency", EL_INT,  &config.err_freq,    sizeof(config.err_freq),     0 },
#ifdef MTC_NOTMINIMAL
        { "newmail_command", EL_STR,  config.nmailcmd,     sizeof(config.nmailcmd),     0 },
#endif /*MTC_NOTMINIMAL*/
        { NULL, EL_NULL, NULL, 0, 0 },
    };

    /*copy some default values in case they are not present in the config*/
    g_strlcpy(config.icon.colour, "#FFFFFF", sizeof(config.icon.colour));
    config.check_delay= 1;
    config.icon_size= 24;
    config.err_freq= 1;

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
            
            retval= cfg_copy_element(child, elements, pcontent);    
            xmlFree(pcontent);

            /*if it returned an error break out*/
            if(retval== FALSE)
                break;
        }
        /*if the accounts are found, read them in*/
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
    doc= xmlCtxtReadFile(ctxt, filename, NULL,
            XML_PARSE_NOWARNING| XML_PARSE_NOERROR| XML_PARSE_PEDANTIC);
    if(doc== NULL)
    {
        xmlErrorPtr perror;

        perror= xmlCtxtGetLastError(ctxt);
        err_noexit("Failed to parse %s: %s\n", filename, perror->message);
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
        return FALSE;
    }
    /*initialise libxml*/
    xml_init();
    
    /* create a parser context */
    ctxt= xmlNewParserCtxt();
    if(ctxt== NULL)
    {
        err_noexit("Failed to allocate parser context\n");
        xml_cleanup();
        return FALSE;
    }
    /*now do some parsing*/
    retval= cfg_parse(ctxt, configfilename);

    /*freeup and cleanup libxml*/
    xmlFreeParserCtxt(ctxt);
    xml_cleanup();

    /*create the image*/
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
    
    pw_node= put_node_str(acc_node, "enc_password", encstring);
    g_free(encstring);
/*otherwise write a clear password*/
#else
    put_node_str(acc_node, "password", password);
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
    put_node_str(root_node, "read_command", config.mail_program);
    put_node_int(root_node, "interval", config.check_delay);
    put_node_int(root_node, "error_frequency", config.err_freq);
    put_node_bool(root_node, "multiple_icon", config.multiple);
    put_node_int(root_node, "icon_size", config.icon_size);
    put_node_str(root_node, "icon_colour", picon->colour);
#ifdef MTC_NOTMINIMAL
    put_node_str(root_node, "newmail_command", config.nmailcmd);
#endif /*MTC_NOTMINIMAL*/
 
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

