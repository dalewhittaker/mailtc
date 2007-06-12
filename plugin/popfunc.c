/* popfunc.c
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

#include "netfunc.h"
#include "popfunc.h"

#ifdef MTC_NOTMINIMAL
#include "msg.h"
#endif /*MTC_NOTMINIMAL*/

#define DIGEST_LEN 32

/* function to receive a message from the pop3 server */
static GString *pop_recvfunc(mtc_net *pnetinfo, GString *msg, gchar *endstring, gboolean getheader)
{
	gint numbytes= 1;
	gchar tmpbuf[MAXDATASIZE];
	guint endslen= 0;

    msg= g_string_new(NULL);
	endslen= strlen(endstring);

	/*while there is data available receive data*/
	while(net_available(pnetinfo))
	{

		if((numbytes= net_recv(pnetinfo, tmpbuf, sizeof(tmpbuf)))== MTC_ERR_CONNECT)
		{
			g_string_free(msg, TRUE);
			return(NULL);
		}
		if(!numbytes)
			break;

		/*add the received string to the buffer*/
        msg= g_string_append(msg, tmpbuf);

		/*added so we dont have to wait for select() timeout every time*/
		if((msg->len>= endslen)&& (g_ascii_strncasecmp(msg->str+ (msg->len- endslen), endstring, endslen)== 0))
			break;
	}
	
	/*check if there was an error with the command*/
	if((msg->str!= NULL)&& (g_ascii_strncasecmp(msg->str, "-ERR", 4))== 0)
	{	
		g_string_free(msg, TRUE);
		return(NULL);
	}
	
	/*check that the received string was receive fully (i.e ends with endstring)*/
	if((msg->len< endslen)||
	((msg->str!= NULL)&& (g_ascii_strncasecmp(msg->str+ (msg->len- endslen), endstring, endslen)!= 0)))
	{
        if(getheader)
        {
            /*It was found that some servers return headers from TOP that do not conform to spec
		    *such emails are handled here (although will be delayed as punishment ;)*/
            gint nendslen= strlen("\r\n.\r\n");
            if(g_ascii_strncasecmp(msg->str+ (msg->len- nendslen), "\r\n.\r\n", nendslen)== 0)
			{   
                plg_err(S_POPFUNC_ERR_BAD_MAIL_HEADER);
			    return(msg);
            }
		}
	
		g_string_free(msg, TRUE);
		return(NULL);
	}
	return(msg);
}

/*function to send a message to the pop3 server*/
static mtc_error pop_send(mtc_net *pnetinfo, mtc_account *paccount, gchar *errcmd, gchar *buf, ...)
{
    gchar *pop_msg= NULL;
    gsize msglen= 0;
    mtc_error retval= MTC_RETURN_TRUE;
    va_list list;
    
    /*calculate length of format string*/
    /*IMPORTANT. more than one operation with the list seems to crash it on 64-bit
     *so va_start, va_end encloses every list operation*/
    va_start(list, buf);
    msglen= g_printf_string_upper_bound(buf, list)+ 1;
    va_end(list);
    
    pop_msg= (gchar *)g_malloc0(msglen);
    
    va_start(list, buf);
    if(g_vsnprintf(pop_msg, msglen, buf, list)>= (gint)msglen)
        retval= MTC_ERR_CONNECT;
    else
        retval= net_send(pnetinfo, pop_msg, (g_ascii_strncasecmp("PASS ", pop_msg, 5)== 0));
    va_end(list);
    
    g_free(pop_msg);
    if(retval!= MTC_RETURN_TRUE)
    {
        plg_err(S_POPFUNC_ERR_SEND, errcmd, paccount->hostname);
        net_disconnect(pnetinfo);
    }
    return(retval);
}

/*this is a function to send a QUIT command to the pop server so that the program can retry to connect later*/
static mtc_error pop_close(mtc_net *pnetinfo, mtc_account *paccount)
{
	/*send the QUIT command to try and exit nicely then close the socket and report error*/
	pop_send(pnetinfo, paccount, "QUIT", "QUIT\r\n");
	plg_err(S_POPFUNC_ERR_CONNECT, PACKAGE);
	net_disconnect(pnetinfo);
    return(MTC_ERR_CONNECT);
}

/*function to receive a string from pop 3 server and handle any errors*/
static GString *pop_recv(mtc_net *pnetinfo, mtc_account *paccount, gchar *errcmd, GString *buf)
{
	if(!(buf= pop_recvfunc(pnetinfo, buf, "\r\n", FALSE)))
	{
        plg_err(S_POPFUNC_ERR_RECV, errcmd, paccount->hostname);
	    pop_close(pnetinfo, paccount);
        return(NULL);
    }
	return(buf);
}

/* function to receive a capability string from the pop3 server */
static GString *pop_recv_capa(mtc_net *pnetinfo, GString *buf)
{
    return(pop_recvfunc(pnetinfo, buf, ".\r\n", FALSE));
}

#ifdef MTC_NOTMINIMAL
/* function to receive a message header from the pop3 server */
static GString *pop_recv_header(mtc_net *pnetinfo, mtc_account *paccount, gint msgid, GString *buf)
{
    if(!(buf= pop_recvfunc(pnetinfo, buf, "\r\n\r\n.\r\n", TRUE)))
    {
        plg_err(S_POPFUNC_ERR_RECV_HEADER, msgid, paccount->hostname);
	    pop_close(pnetinfo, paccount);
        return(NULL);
    }
	return(buf);
}
#endif /*MTC_NOTMINIMAL*/

#ifdef SSL_PLUGIN
/*function to login to APOP server*/
static mtc_error apop_login(mtc_net *pnetinfo, mtc_account *paccount)
{
	gchar apopstring[LOGIN_NAME_MAX+ HOST_NAME_MAX+ PASSWORD_LEN+ 3];
	gchar digest[DIGEST_LEN+ 1];
	gchar *startpos= NULL, *endpos= NULL;
    GString *rstring= NULL;
    mtc_error retval= MTC_RETURN_TRUE;

	/*search for the timestamp character start and end*/
	if((pnetinfo->pdata->str)&& ((startpos= strchr(pnetinfo->pdata->str, '<'))!= NULL))
		endpos= strchr(startpos, '>');

	/*if timestamp character and end arent found report that APOP is not supported*/
	if((startpos== NULL)|| (endpos== NULL))
	{
		plg_err(S_POPFUNC_ERR_APOP_NOT_SUPPORTED);
		return(pop_close(pnetinfo, paccount));
	}
	
	/*copy the timestamp part of message to apopstring and append the password*/
	memset(apopstring, '\0', LOGIN_NAME_MAX+ HOST_NAME_MAX+ PASSWORD_LEN+ 3);
	g_strlcpy(apopstring, startpos, (endpos- startpos)+ 2);
	g_strlcat(apopstring, paccount->password, sizeof(apopstring));

	/*encrypt the apop string and copy it into the digest buffer*/
	memset(digest, '\0', DIGEST_LEN);
	if((apop_encrypt(apopstring, digest)!= ((guint)DIGEST_LEN/ 2)))
	{
		plg_err(S_POPFUNC_ERR_APOP_ENCRYPT_TIMESTAMP);
		pop_close(pnetinfo, paccount);
        return(MTC_ERR_EXIT);
	}
	/*send the APOP auth string and check it was successfull*/
	if((retval= pop_send(pnetinfo, paccount, "APOP", "APOP %s %s\r\n", paccount->username, digest))!= MTC_RETURN_TRUE)
        return(retval);
	
	if(!(rstring= pop_recv(pnetinfo, paccount, "APOP", rstring)))
	    return(MTC_ERR_CONNECT);
    g_string_free(rstring, TRUE);
		
	return(MTC_RETURN_TRUE);

}
#endif /*SSL_PLUGIN*/

/*function to login to POP server*/
static mtc_error pop_login(mtc_net *pnetinfo, mtc_account *paccount)
{
	/*create the string for the username and send it*/
	GString *buf= NULL;
	mtc_error retval= MTC_RETURN_TRUE;

    /*send/receive user*/
	if((retval= pop_send(pnetinfo, paccount, "USER", "USER %s\r\n", paccount->username))!= MTC_RETURN_TRUE)
        return(retval);
	if(!(buf= pop_recv(pnetinfo, paccount, "USER", buf)))
	    return(MTC_ERR_CONNECT);

    g_string_free(buf, TRUE);
    buf= NULL;

	/*send/receive password*/
	if((retval= pop_send(pnetinfo, paccount, "PASS", "PASS %s\r\n", paccount->password))!= MTC_RETURN_TRUE)
        return(retval);
	if(!(buf= pop_recv(pnetinfo, paccount, "PASS", buf)))
	    return(MTC_ERR_CONNECT);

    g_string_free(buf, TRUE);
	return(MTC_RETURN_TRUE);
}

#ifdef SSL_PLUGIN	
/*function to login to POP server with CRAM-MD5 auth*/
static mtc_error crammd5_login(mtc_net *pnetinfo, mtc_account *paccount)
{
	GString *buf= NULL;
	gchar *digest= NULL;
    mtc_error retval= MTC_RETURN_TRUE;

	/*send auth command*/
	if((retval= pop_send(pnetinfo, paccount, "AUTH CRAM-MD5", "AUTH CRAM-MD5\r\n"))!= MTC_RETURN_TRUE)
        return(retval);

	/*receive back from server to check username was sent ok*/
	if(!(buf= pop_recv(pnetinfo, paccount, "AUTH CRAM-MD5", buf)))
        return(MTC_ERR_CONNECT);

    /*remove trailing whitespace*/
    g_strchomp(buf->str);

	/*create CRAM-MD5 string to send to server*/
	digest= mk_cramstr(paccount, buf->str+ 2, digest);
	g_string_free(buf, TRUE);
	buf= NULL;

	/*check the digest value that is returned*/
	if(digest== NULL)
		return(net_disconnect(pnetinfo));

	/*send the digest to log in*/
	retval= pop_send(pnetinfo, paccount, "CRAM-MD5 digest", "%s\r\n", digest);
	g_free(digest);
	
    if(retval!= MTC_RETURN_TRUE)
        return(retval);

	/*receive back from server to check username was sent ok*/
	if(!(buf= pop_recv(pnetinfo, paccount, "CRAM-MD5 digest", buf)))
	    return(MTC_ERR_CONNECT);

    g_string_free(buf, TRUE);

	return(MTC_RETURN_TRUE);
}
#endif /*SSL_PLUGIN*/


/*function to get the number of messages from POP server*/
static mtc_error pop_get_msgs(mtc_net *pnetinfo, mtc_account *paccount)
{
    mtc_error retval= MTC_RETURN_TRUE;
	GString *buf= NULL;

	/*get the total number of messages from server*/
	if((retval= pop_send(pnetinfo, paccount, "STAT", "STAT\r\n"))!= MTC_RETURN_TRUE)
	    return(retval);
	if(!(buf= pop_recv(pnetinfo, paccount, "STAT", buf)))
	    return(MTC_ERR_CONNECT);

	/*if received ok, create the number of messages from string and return it*/
#ifdef MTC_NOTMINIMAL
    if(sscanf(buf->str, "%*s%d%*d", &paccount->msginfo.num_messages)!= 1)
#else
    if(sscanf(buf->str, "%*s%d%*d", &paccount->num_messages)!= 1)
#endif /*MTC_NOTMINIMAL*/
    {
		plg_err(S_POPFUNC_ERR_GET_TOTAL_MESSAGES);
		g_string_free(buf, TRUE);
		return(MTC_ERR_CONNECT);
	}
	g_string_free(buf, TRUE);

	return(MTC_RETURN_TRUE);
}

/*function to logout and close the pop connection*/
static mtc_error pop_logout(mtc_net *pnetinfo, mtc_account *paccount)
{
	GString *buf= NULL;
    mtc_error retval= MTC_RETURN_TRUE;

	/*send/receive the message to logout of the server*/
	if((retval= pop_send(pnetinfo, paccount, "QUIT", "QUIT\r\n"))!= MTC_RETURN_TRUE)
        return(retval);
	if(!(buf= pop_recv(pnetinfo, paccount, "QUIT", buf)))
		return(MTC_ERR_CONNECT);
	
    g_string_free(buf, TRUE);

    return(net_disconnect(pnetinfo));
}

/*function to test capabilities of POP/IMAP server*/
static mtc_error pop_capa(mtc_net *pnetinfo, mtc_account *paccount)
{
	GString *buf= NULL;
    mtc_error retval= MTC_RETURN_TRUE;

	/*send the message to check capabilities of the server*/
	if((retval= pop_send(pnetinfo, paccount, "CAPA", "CAPA\r\n"))!= MTC_RETURN_TRUE)
        return(retval);
	
	/*assumption is made that ERR means server does not have CAPA command*/
	/*also means that CRAM-MD5 is not supported*/
	if(!(buf= pop_recv_capa(pnetinfo, buf)))
	{
		if(pnetinfo->authtype== POPCRAM_PROTOCOL)
		{
			plg_err(S_POPFUNC_ERR_CRAM_MD5_NOT_SUPPORTED);
			return(pop_close(pnetinfo, paccount));
		}
		return(MTC_RETURN_TRUE);
	}
	/*First we check for OK, otherwise report that its not a valid POP server and return*/
	if(g_ascii_strncasecmp(buf->str, "+OK", 3)!= 0)
	{
		g_string_free(buf, TRUE);
		plg_err(S_POPFUNC_ERR_TEST_CAPABILITIES);
		return(pop_close(pnetinfo, paccount));
	}
	
	/*here we test the capabilites*/
	/*no need to test simple POP as it would just return OK*/
	/*APOP is not handled by CAPA command*/
	if(pnetinfo->authtype== POPCRAM_PROTOCOL)
	{
		/*If CRAM_MD5 is not found in the string it is not supported*/
		if(strstr(buf->str, "CRAM-MD5")== NULL)
		{
			g_string_free(buf, TRUE);
			plg_err(S_POPFUNC_ERR_CRAM_MD5_NOT_SUPPORTED);
			return(pop_close(pnetinfo, paccount));
		}
	}
	g_string_free(buf, TRUE);

	return(MTC_RETURN_TRUE);
}

/*function to get the header of a message*/
#ifdef MTC_NOTMINIMAL
static GString *pop_get_header(mtc_net *pnetinfo, mtc_account *paccount, GString *buf, gint message)
{
   	if(pop_send(pnetinfo, paccount, "TOP", "TOP %d 0\r\n", message)== MTC_RETURN_TRUE)
	    buf= pop_recv_header(pnetinfo, paccount, message, buf);
    
    return(buf);
}
#endif /*MTC_NOTMINIMAL*/

/*function to get the uidl for a message*/
static GString *pop_recv_uidl(mtc_net *pnetinfo, mtc_account *paccount, gint message, GString *buf)
{
	/*clear the buffer*/
	buf= NULL;

	/*send the uidl string to get the uidl for the current message*/
	if(pop_send(pnetinfo, paccount, "UIDL", "UIDL %d\r\n", message)== MTC_RETURN_TRUE)
	    buf= pop_recv(pnetinfo, paccount, "UIDL", buf);
    return(buf);
}

/*function to get the uidl from the received string*/
static mtc_error pop_get_uidl(GString *buf, gchar *uidl_string)
{
	/*int uidllen= 0;*/
	gchar *pos= NULL;

	if((pos= strrchr(buf->str, ' '))== NULL)
	{
		plg_err(S_POPFUNC_ERR_GET_UIDL);
		g_string_free(buf, TRUE);
		return(MTC_ERR_EXIT);
	}

	memset(uidl_string, '\0', UIDL_LEN);
	g_strlcpy(uidl_string, pos+ 1, UIDL_LEN);
    g_strlcat(g_strchomp(uidl_string), "\n", UIDL_LEN);
	
    return(MTC_RETURN_TRUE);
}

/* function to get uidl values of messages on server, 
 * and compare them with values in stored uidl file
 * to check if there are any new messages */
mtc_error pop_calc_new(mtc_net *pnetinfo, mtc_account *paccount, const mtc_cfg *pconfig)
{
	gint total_messages= 0, i;
	FILE *outfile;
	FILE *infile= NULL;
	gchar uidl_string[UIDL_LEN];
	gchar uidlfile[NAME_MAX]; 
	gchar tmpuidlfile[NAME_MAX];
	GString *buf= NULL;
	gboolean uidlfound= 0;
	gchar line[LINE_LENGTH];
    mtc_error retval= MTC_RETURN_TRUE;
    gboolean uexists = FALSE;

#ifdef MTC_NOTMINIMAL
    total_messages= paccount->msginfo.num_messages;
    
    /*reset the list so we know what is new*/
    msglist_reset(paccount);
#else
    total_messages= paccount->num_messages;
    paccount->num_messages= 0;
#endif /*MTC_NOTMINIMAL*/

	/*get the full path for the uidl file and temp uidl file*/
	mtc_file(uidlfile, pconfig->dir, UIDL_FILE, paccount->id);
	mtc_file(tmpuidlfile, pconfig->dir, TMP_UIDL_FILE, paccount->id);

	/*open temp file for writing*/
	if((outfile= g_fopen(tmpuidlfile, "w"))== NULL)
	{
		plg_err(S_POPFUNC_ERR_OPEN_FILE, tmpuidlfile);
		return(MTC_ERR_EXIT);
	}

    /*test if uidl file exists, we need to know for later*/
    uexists= IS_FILE(uidlfile);
    
    /*open the UIDL file if exists*/
    if(uexists)
    {
        if((infile= g_fopen(uidlfile, "r"))== NULL)
        {
		    plg_err(S_POPFUNC_ERR_OPEN_FILE, uidlfile);
		    return(MTC_ERR_EXIT);
	    }
    }
    
	/*for each message on server*/
	for(i= 1; i<= total_messages; ++i)
	{
		buf= NULL;

		/*get the uidl of the message*/
		if((buf= pop_recv_uidl(pnetinfo, paccount, i, buf)))
		{
            uidlfound= FALSE;
			
            /*get the uidl part from the received string*/
            if(pop_get_uidl(buf, uidl_string)!= MTC_RETURN_TRUE)
                return(MTC_ERR_EXIT);
		
			g_string_free(buf, TRUE);
		    buf= NULL;

			/*if there is a uidl file look for the UIDL*/
            /*NOTE if there is no uidl file it is assumed that this is
             *initial (first ever) read, so no messages are added*/
            if(uexists&& infile!= NULL)
			{
			    memset(line, '\0', LINE_LENGTH);
			    rewind(infile);

                /*get each line from uidl file and compare it with the uidl value received*/
				while(fgets(line, LINE_LENGTH, infile)!= NULL)
				{
					if(g_ascii_strcasecmp(uidl_string, line)== 0)
					{
                        uidlfound= TRUE; break;
			        }
                    memset(line, '\0', LINE_LENGTH);
				}
			    /*if the uidl was not found in the uidl file increment the new message count*/
			    /*also check if filters should be applied*/
			    if(!uidlfound)
			    {
#ifdef MTC_NOTMINIMAL

#ifdef MTC_EXPERIMENTAL
	                if(paccount->runfilter|| pconfig->run_summary)
#else
	                if(paccount->runfilter)
#endif /*MTC_EXPERIMENTAL*/
                    {
                        if((buf= pop_get_header(pnetinfo, paccount, buf, i))== NULL)
                            retval= MTC_ERR_CONNECT;
                    }
				    paccount->msginfo.msglist= msglist_add(paccount, uidl_string, buf);
			        if(buf!= NULL)
                        g_string_free(buf, TRUE);
#else
                    paccount->num_messages++;
#endif /*MTC_NOTMINIMAL*/

			    }   
                /*in case of error we must break after the file is closed*/
                if(retval!= MTC_RETURN_TRUE)
                    break;
			}
            /*output the uidl string to the temp file*/
			fputs(uidl_string, outfile);
		}
        else
        {
            retval= MTC_ERR_CONNECT;
            break;
        }
	}
    
    if((infile!= NULL)&& (fclose(infile)== EOF))
		plg_err(S_POPFUNC_ERR_CLOSE_FILE, uidlfile);
 
	if(fclose(outfile)== EOF)
		plg_err(S_POPFUNC_ERR_CLOSE_FILE, tmpuidlfile);
	
#ifdef MTC_NOTMINIMAL
    paccount->msginfo.msglist= msglist_cleanup(paccount);
    if(!msglist_verify(paccount))
    {
        plg_err(S_POPFUNC_ERR_VERIFY_MSGLIST);
        return(MTC_RETURN_FALSE);
    }
#endif /*MTC_NOTMINIMAL*/

    /*if the UID file did not exist, rename the temp file*/
    /*this effectively marks everything as read on the first pass*/
	if(!uexists)
    {
        /*rename the temp uidl file to uidl file if it exists*/
        if((IS_FILE(tmpuidlfile))&& (g_rename(tmpuidlfile, uidlfile)== -1))
	    {
		    plg_err(S_POPFUNC_ERR_RENAME_FILE, tmpuidlfile, uidlfile);
		    return(MTC_RETURN_FALSE);
	    }

	    /*remove the temp file and cleanup*/
	    g_remove(tmpuidlfile);
    }

    return(retval);
}

/*function to check mail (in clear text mode)*/
static mtc_error check_mail(mtc_net *pnetinfo, mtc_account *paccount, mtc_error (*authfunc)(mtc_net *, mtc_account *), const mtc_cfg *pconfig)
{
	mtc_error retval= MTC_RETURN_TRUE;
	
	/*connect to the server and receive the string back*/
	if(net_connect(pnetinfo, paccount)!= MTC_RETURN_TRUE)
		return(MTC_ERR_CONNECT);
	
    /*some servers don't give us anything to receive, so don't disconnect if we don't get anything*/
	pnetinfo->pdata= pop_recv(pnetinfo, paccount, "connect", pnetinfo->pdata);
    if(!pnetinfo->pdata)
        return(MTC_ERR_CONNECT);

	/*test server capabilities*/
	if(pop_capa(pnetinfo, paccount)!= MTC_RETURN_TRUE)
	{
		if(pnetinfo->pdata!= NULL) g_string_free(pnetinfo->pdata, TRUE);
		return(MTC_ERR_CONNECT);
	}

    /*run the authentication function*/
    retval= (*authfunc)(pnetinfo, paccount);
    
    if(pnetinfo->pdata!= NULL)
        g_string_free(pnetinfo->pdata, TRUE);
    
    if(retval!= MTC_RETURN_TRUE)
        return(retval);

	/*get the total number of messages from the server*/
	if((retval= pop_get_msgs(pnetinfo, paccount))!= MTC_RETURN_TRUE)
        return(retval);
	
    /*output the uidls to the temp file and get the number of new messages*/
	if((retval= pop_calc_new(pnetinfo, paccount, pconfig))!= MTC_RETURN_TRUE)
        return(retval);
	
    /*logout of the server and return the number of new messages*/
	return(pop_logout(pnetinfo, paccount));
}

/*function to read the POP mail*/
mtc_error pop_read_mail(mtc_account *paccount, const mtc_cfg *pconfig)
{
	gchar uidlfile[NAME_MAX], tmpuidlfile[NAME_MAX];

	/*get full paths of files*/
	mtc_file(uidlfile, pconfig->dir, UIDL_FILE, paccount->id);
	mtc_file(tmpuidlfile, pconfig->dir, TMP_UIDL_FILE, paccount->id);

	/*rename the temp uidl file to uidl file if it exists*/
    if((IS_FILE(tmpuidlfile))&& (g_rename(tmpuidlfile, uidlfile)== -1))
	{
		plg_err(S_POPFUNC_ERR_RENAME_FILE, tmpuidlfile, uidlfile);
		return(MTC_RETURN_FALSE);
	}

	/*remove the temp file and cleanup*/
	g_remove(tmpuidlfile);

	return(MTC_RETURN_TRUE);
}

/*call check_mail for POP*/
mtc_error check_pop_mail(mtc_account *paccount, const mtc_cfg *pconfig)
{
    mtc_net netinfo;
	netinfo.authtype= POP_PROTOCOL;
	return(check_mail(&netinfo, paccount, &pop_login, pconfig));
}

#ifdef SSL_PLUGIN
/*call check_mail for APOP*/
mtc_error check_apop_mail(mtc_account *paccount, const mtc_cfg *pconfig)
{
    mtc_net netinfo;
	netinfo.authtype= APOP_PROTOCOL;
	return(check_mail(&netinfo, paccount, &apop_login, pconfig));
}

/*call check_mail for POP (CRAM-MD5)*/
mtc_error check_crampop_mail(mtc_account *paccount, const mtc_cfg *pconfig)
{
    mtc_net netinfo;
	netinfo.authtype= POPCRAM_PROTOCOL;
	return(check_mail(&netinfo, paccount, &crammd5_login, pconfig));
}

/*call check_mail for POP (SSL/TLS)*/
mtc_error check_popssl_mail(mtc_account *paccount, const mtc_cfg *pconfig)
{
    mtc_net netinfo;
    netinfo.authtype= POPSSL_PROTOCOL;
	return(check_mail(&netinfo, paccount, &pop_login, pconfig));
}
#endif /*SSL_PLUGIN*/
