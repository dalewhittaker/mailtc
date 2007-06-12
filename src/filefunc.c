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

#include "filefunc.h"
#include "plugfunc.h"
#include "filterdlg.h"

#ifdef MTC_USE_SSL
#include "encrypter.h"
#endif /*MTC_USE_SSL*/

/*function to get $HOME + program*/
gboolean mtc_dir(void)
{
	gchar *pfile= NULL;
	
	/*get the home path*/
	if((pfile= g_build_filename(g_get_home_dir(), "." PACKAGE, NULL))== NULL)
		err_exit(S_FILEFUNC_ERR_GET_HOMEDIR);
		
	g_strlcpy(config.dir, pfile, sizeof(config.dir));
	g_free(pfile);

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

/*generic write a string to file function*/
static void fwrite_string(FILE *pfile, const gchar *pstring, gchar *perr)
{
    if((fputs(pstring, pfile)== EOF)|| (fputc('\n', pfile)== EOF))
        err_exit(perr);
}

/*generic write an integer to file function*/
static void fwrite_integer(FILE *pfile, gint pval, gchar *perr)
{
	/*char scratch[G_ASCII_DTOSTR_BUF_SIZE];
	g_ascii_dtostr(scratch, sizeof(scratch), pval);
    return(fwrite_string(pfile, scratch, gchar *perr));*/
    
	if(g_fprintf(pfile, "%d\n", pval)< 0)
        err_exit(perr);
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
	fread_string_fail(pfile, pnew->port, sizeof(pnew->port), S_FILEFUNC_ERR_GET_PORT, detailsfilename);
	fread_string_fail(pfile, pnew->username, sizeof(pnew->username), S_FILEFUNC_ERR_GET_USERNAME, detailsfilename);
	fread_string_fail(pfile, picon->colour, sizeof(pnew->icon), S_FILEFUNC_ERR_GET_ICONTYPE, detailsfilename);
    fread_string_fail(pfile, pnew->plgname, sizeof(pnew->plgname), S_FILEFUNC_ERR_READ_PLUGIN, detailsfilename);
	fread_string(pfile, pnew->accname, sizeof(pnew->accname), S_FILEFUNC_DEFAULT_ACCNAME);

#ifdef MTC_NOTMINIMAL
	fread_integer(pfile, (gint *)&pnew->runfilter, FALSE);
#endif /*MTC_NOTMINIMAL*/

	/*read in the password*/
	if(!pw_read(pnew))
		err_exit(S_FILEFUNC_ERR_GET_PASSWORD, pnew->id);
	
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

/*function to read in the password (encrypted or not) from the file*/
gboolean pw_read(mtc_account *paccount)
{

	FILE* pfile;
	gchar passwordfilename[NAME_MAX];
#ifdef MTC_USE_SSL
	gchar encstring[1024];
	gint len= 0;
#endif
	
	/*get the full path of the password file*/
	mtc_file(passwordfilename, PASSWORD_FILE, paccount->id);
	
	/*if the password file does not exist return*/
	if(!IS_FILE(passwordfilename))
		return FALSE;
	
	/*open the password file for reading*/
	if((pfile= g_fopen(passwordfilename, "rb"))== NULL)
		err_exit(S_FILEFUNC_ERR_OPEN_FILE, passwordfilename);

/*if OpenSSL is defined read in an encrypted password*/
#ifdef MTC_USE_SSL
	memset(encstring, '\0', 1024);
	
	/*read in the encrypted password and remove the password if there is an error reading it*/
	if((len= fread(encstring, 1, 1024, pfile))==0)
	{
		g_remove(passwordfilename);
		err_exit(S_FILEFUNC_ERR_GET_PW);
	}
	
	/*decrypt the password*/
	pw_decrypt(encstring, len, paccount->password);
	
/*otherwise read in clear password*/
#else
	/*get the password value and remove the file if it cannot be read*/
	fread_string_fail(pfile, paccount->password, sizeof(paccount->password), S_FILEFUNC_ERR_GET_PW, passwordfilename);
#endif
	
	/*close the password file*/
	if(fclose(pfile)== EOF)
		err_exit(S_FILEFUNC_ERR_CLOSE_FILE, passwordfilename);
	
	return TRUE;
}

/*function to write pop config to config file*/
gboolean cfg_write(void)
{
	FILE *pfile;
	gchar configfilename[NAME_MAX];
    mtc_icon *picon;

    picon= &config.icon;

	/*get the full path of the config file*/
	mtc_file(configfilename, CONFIG_FILE, -1);
	
	/*if the file cannot be accessed or removed report error*/
	if((IS_FILE(configfilename))&&(g_remove(configfilename)== -1))
		err_exit(S_FILEFUNC_ERR_ATTEMPT_WRITE, configfilename);
	
	/*open the config file for writing*/
	if((pfile= g_fopen(configfilename, "wt"))== NULL)
		err_exit(S_FILEFUNC_ERR_OPEN_FILE, configfilename);
	
	/*output the config information to the config file*/
	fwrite_integer(pfile, config.check_delay, S_FILEFUNC_ERR_WRITE_DELAY);
	fwrite_string(pfile, config.mail_program, S_FILEFUNC_ERR_WRITE_MAILAPP);
#ifndef MTC_NOTMINIMAL
    /*if it's a minimal build, icon size can only be 24,
     *for compatibility with config files the size value must be written*/
    config.icon_size= 24;
#endif /*MTC_NOTMINIMAL*/
	fwrite_integer(pfile, config.icon_size, S_FILEFUNC_ERR_WRITE_ICONSIZE);
    fwrite_string(pfile, (config.multiple)? "1": "0", S_FILEFUNC_ERR_WRITE_MULTIPLE);
	fwrite_string(pfile, picon->colour, S_FILEFUNC_ERR_WRITE_M_ICON_COLOUR);
	fwrite_integer(pfile, config.err_freq, S_FILEFUNC_ERR_WRITE_FREQ);
#ifdef MTC_NOTMINIMAL
    fwrite_string(pfile, config.nmailcmd, S_FILEFUNC_ERR_WRITE_NMCMD);
#endif /*MTC_NOTMINIMAL*/

    /*Experimental adding of summary dialog*/
#ifdef MTC_EXPERIMENTAL
    fwrite_string(pfile, (config.run_summary)? "1": "0", S_FILEFUNC_ERR_WRITE_SUMMARY);
	
    /*write the summary stuff*/
    if(config.run_summary)
    {
        fwrite_integer(pfile, config.summary.wpos, S_FILEFUNC_ERR_WRITE_SUMMARY_WPOS);
        fwrite_string(pfile, (config.summary.sfont[0]== 0)? "NULL": config.summary.sfont, S_FILEFUNC_ERR_WRITE_SUMMARY_SFONT);
    }
#endif
	fflush(pfile);

	/*close the config file*/
	if(fclose(pfile)== EOF)
		err_noexit(S_FILEFUNC_ERR_CLOSE_FILE, configfilename);
	
	/*set the permissions on the file so it can only be read*/
	if(g_chmod(configfilename, S_IRUSR)== -1)
		err_exit(S_FILEFUNC_ERR_SET_PERM, configfilename);
	
	return TRUE;
}

/*function to write pop details to details file*/
gboolean acc_write(mtc_account *pcurrent)
{
	FILE *pfile;
	gchar detailsfilename[NAME_MAX];
    mtc_icon *picon;

    picon= &pcurrent->icon;
	
	/*get the full path of the details file*/
	mtc_file(detailsfilename, DETAILS_FILE, pcurrent->id);
	
	/*if it exists and cannot be removed report error*/
	if((IS_FILE(detailsfilename))&&(g_remove(detailsfilename)== -1))
		err_exit(S_FILEFUNC_ERR_ATTEMPT_WRITE, detailsfilename);
	
	/*open the details file for writing*/
	if((pfile= g_fopen(detailsfilename, "wt"))== NULL)
		err_exit(S_FILEFUNC_ERR_OPEN_FILE, detailsfilename);
	
	/*output the details information to the details file*/
	fwrite_string(pfile, pcurrent->hostname, S_FILEFUNC_ERR_WRITE_HOSTNAME);
	fwrite_string(pfile, pcurrent->port, S_FILEFUNC_ERR_WRITE_PORT);
	fwrite_string(pfile, pcurrent->username, S_FILEFUNC_ERR_WRITE_USERNAME);
	fwrite_string(pfile, picon->colour, S_FILEFUNC_ERR_WRITE_ICONTYPE);
	fwrite_string(pfile, pcurrent->plgname, S_FILEFUNC_ERR_WRITE_PROTOCOL);
	fwrite_string(pfile, pcurrent->accname, S_FILEFUNC_ERR_WRITE_ACCNAME);
#ifdef MTC_NOTMINIMAL
    fwrite_integer(pfile, pcurrent->runfilter, S_FILEFUNC_ERR_WRITE_FILTER);
#endif /*MTC_NOTMINIMAL*/

	/*write the password to the password file*/
	pw_write(pcurrent);
		
	/*close the details file*/
	if(fclose(pfile)== EOF)
		err_noexit(S_FILEFUNC_ERR_CLOSE_FILE, detailsfilename);
	
	/*change the permissions so the file can only be read*/
	if(g_chmod(detailsfilename, S_IRUSR)== -1)
		err_exit(S_FILEFUNC_ERR_SET_PERM, detailsfilename);
	
	return TRUE;
}

/*function to write the password (encrypted or not) to the file*/
gboolean pw_write(mtc_account *paccount)
{
	FILE* pfile;
	gchar passwordfilename[NAME_MAX];
#ifdef MTC_USE_SSL	
	gchar encstring[1024];
	gulong len= 0;
#endif

	/*get the full path of the password file*/
	mtc_file(passwordfilename, PASSWORD_FILE, paccount->id);
	
	/*if the password exists but cannot be removed report error*/
	if((IS_FILE(passwordfilename))&&(g_remove(passwordfilename)== -1))
		err_exit(S_FILEFUNC_ERR_ATTEMPT_WRITE, passwordfilename);
	
	/*open the password file for writing*/
	if((pfile= g_fopen(passwordfilename, "wb"))== NULL)
		err_exit(S_FILEFUNC_ERR_OPEN_FILE, passwordfilename);

/*if OpenSSL is defined encrypt the password*/
#ifdef MTC_USE_SSL
	memset(encstring, '\0', 1024);
	len= pw_encrypt(paccount->password, encstring);
	
	/*write the encrypted password*/
	if(fwrite(encstring, 1, len, pfile)< len)
		err_exit(S_FILEFUNC_ERR_WRITE_PW);

/*otherwise write a clear password*/
#else
    fwrite_string(pfile, paccount->password, S_FILEFUNC_ERR_WRITE_PW);
#endif

	/*close the password file*/
	if(fclose(pfile)== EOF)
		err_exit(S_FILEFUNC_ERR_CLOSE_FILE, passwordfilename);
	
	/*change the permissions on the file so that it can only be read by user*/
	if(g_chmod(passwordfilename, S_IRUSR)== -1)
		err_exit(S_FILEFUNC_ERR_SET_PERM, passwordfilename);

	return TRUE;
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

