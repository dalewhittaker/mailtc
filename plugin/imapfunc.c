/* imapfunc.c
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
#include "imapfunc.h"

#ifdef MTC_NOTMINIMAL
#include "msg.h"
#include "filter.h"
#endif /*MTC_NOTMINIMAL*/

/*define the various imap responses, note they can either be tagged or untagged
 *both are required.  Rather than define an id for each, use a macro to use higher word for tagged*/
#define MAX_RESPONSE_STRING 20
#define TAGGED_RESPONSE(response) ((response)<< 8)
typedef enum _imap_response
{ 
    IMAP_OK= 1,
    IMAP_BAD= 2,
    IMAP_NO= 4,
    IMAP_BYE= 8,
    IMAP_PREAUTH= 16,
    IMAP_CAPABILITY= 32,
    IMAP_MAXRESPONSE
} imap_response;

/*used to contain response strings*/
typedef struct _imap_response_list
{
    guint id;
    gchar *str;
} imap_response_list;

/*function to send a message to the pop3 server*/
static mtc_error imap_send(mtc_net *pnetinfo, mtc_account *paccount, gboolean sendmsgid, gchar *errcmd, gchar *buf, ...)
{
    gchar *imap_msg= NULL;
    gsize msglen= 0;
    gint idlen= 0;
    mtc_error retval= MTC_RETURN_TRUE;
    va_list list;
    
    /*calculate length of format string*/
    va_start(list, buf);
    msglen= g_printf_string_upper_bound(buf, list)+ 1;
    va_end(list);
    imap_msg= (gchar *)g_malloc0(sizeof(gchar)* (msglen+ G_ASCII_DTOSTR_BUF_SIZE+ 3));
   
    /*add the message id if desired*/
    if(sendmsgid)
    {
        g_snprintf(imap_msg, G_ASCII_DTOSTR_BUF_SIZE+ 3, "a%.4u ", pnetinfo->msgid++);
        idlen= strlen(imap_msg);
    }

    /*then add the message*/
    va_start(list, buf);
    if(g_vsnprintf(imap_msg+ idlen, msglen, buf, list)>= (gint)msglen)
        retval= MTC_ERR_CONNECT;
    else
        retval= net_send(pnetinfo, imap_msg, (g_ascii_strncasecmp("LOGIN ", imap_msg+ idlen, 6)== 0));
    
    va_end(list);
    g_free(imap_msg);
    if(retval!= MTC_RETURN_TRUE)
    {
        plg_err(S_IMAPFUNC_ERR_SEND, errcmd, paccount->server);
        net_disconnect(pnetinfo);
    }
    return(retval);
}

/*function to close the connection if an error has occured*/
static mtc_error imap_close(mtc_net *pnetinfo, mtc_account *paccount)
{
	/*basically try to logout then terminate the connection*/
	imap_send(pnetinfo, paccount, TRUE, "LOGOUT", "LOGOUT\r\n"); 
	plg_err(S_IMAPFUNC_ERR_CONNECT, PACKAGE);
    net_disconnect(pnetinfo);
	return(MTC_ERR_CONNECT);

}

/*find the correct IMAP response string*/
static gint imap_get_response(imap_response_list *presponses, guint idx)
{
    gint i= 0;

    /*iterate though the list to find correct one*/
    while((presponses!= NULL)&& (presponses->str!= NULL))
    {
        if((presponses->id== idx)|| (TAGGED_RESPONSE(presponses->id)== idx))
            return(i);
        
        ++i;
        ++presponses;
    }
    return -1;
}

/*function to check for a response*/
static gint imap_resp(gchar *msg, guint msgid)
{
    guint i;
    gint idx= -1;
    guint resplen= 0;
    gchar *spos= NULL;
    gint retval= 0;
    gchar sresponse[MAX_RESPONSE_STRING];

    /*list of possible strings matching*/
    imap_response_list responses[]= 
    {
        { IMAP_OK, "OK" },
        { IMAP_BAD, "BAD" },
        { IMAP_NO, "NO" },
        { IMAP_BYE, "BYE" },
        { IMAP_PREAUTH, "PREAUTH" },
        { IMAP_CAPABILITY, "CAPABILITY" },
        { 0, NULL }
    };

    /*Test to print out list of responses*/
    for(i= 1; i<= TAGGED_RESPONSE(IMAP_MAXRESPONSE); i<<= 1)
    {
        /*Ok, it has been found*/
        if((idx= imap_get_response(&responses[0], i))!= -1)
        {
            spos= NULL;

            /*its tagged*/
            if(i> IMAP_MAXRESPONSE- 1)
                g_snprintf(sresponse, sizeof(sresponse), "a%.4d %s", msgid, responses[idx].str);
            /*untagged*/
            else
                g_snprintf(sresponse, sizeof(sresponse), "* %s", responses[idx].str);

            resplen= strlen(sresponse);
            spos= strstr(msg, sresponse);

            /*add any found responses*/
            if((g_ascii_strncasecmp(msg, sresponse, resplen)== 0)||
                ((spos> msg+ 1)&& g_ascii_strncasecmp(spos- 2, "\r\n", 2)== 0))
            {
                /*g_print("response: '%s' found\n", sresponse);*/
                retval|= i;
            }
        }
    }
    return(retval);
}

static GString *imap_recv(mtc_net *pnetinfo, GString *msg, guint msgid, guint msgflags, guint desired)
{   
    gint numbytes= 1;
    gchar tmpbuf[MAXDATASIZE];
    guint found= 0;
    guint mask= 0;

    msg= g_string_new(NULL);

    while(net_available(pnetinfo))
	{
		if((numbytes= net_recv(pnetinfo, tmpbuf, sizeof(tmpbuf)))== MTC_ERR_CONNECT)
		{
            g_string_free(msg, TRUE);
			return(MTC_RETURN_FALSE);
		}
		
		if(!numbytes)
			break;

		/*add the read data to the buffer*/
		msg= g_string_append(msg, tmpbuf);
		
		/*added for speed improvements (doesnt check imap server again)*/
		/*check our response flags to see if we can break early*/
        found= imap_resp(msg->str, msgid);
    
        if((found & msgflags)&& (g_ascii_strncasecmp(msg->str+ (msg->len- 2), "\r\n", 2)== 0))
           break;
	}

	/*test that the data was fully received and successful*/
	if(msg->str== NULL/*|| (found!= desired)*/)
	{
        g_string_free(msg, TRUE);
        return(NULL);
	}

    /*check if any of our required responses are not found*/
    /*NOTE for now this looks to be acceptable, but may need to be changed should problems arise*/
    for(mask= 1; mask<= TAGGED_RESPONSE(IMAP_MAXRESPONSE); mask<<= 1)
    {
        if((desired& mask)&& !(found& mask))
        {
            g_string_free(msg, TRUE);
            return(NULL);
        }
    }
	return(msg);
}

/*function to login to IMAP server*/
static mtc_error imap_login(mtc_net *pnetinfo, mtc_account *paccount)
{
    mtc_error retval= MTC_RETURN_TRUE;
    GString *buf= NULL;

	/*send command to login with username and password and check login was successful*/
	if((retval= imap_send(pnetinfo, paccount, TRUE, "LOGIN", "LOGIN %s %s\r\n", paccount->username, paccount->password))!= MTC_RETURN_TRUE)
        return(retval);

	if(!(buf= imap_recv(pnetinfo, buf, pnetinfo->msgid- 1, 
        TAGGED_RESPONSE(IMAP_OK)| TAGGED_RESPONSE(IMAP_BAD)| TAGGED_RESPONSE(IMAP_NO)| IMAP_BYE, TAGGED_RESPONSE(IMAP_OK))))
	{	
		plg_err(S_IMAPFUNC_ERR_SEND_LOGIN, paccount->server);
        imap_close(pnetinfo, paccount);
		return(MTC_ERR_CONNECT);
	}
    g_string_free(buf, TRUE);
	return(MTC_RETURN_TRUE);
}

/*function to logout of IMAP server*/
static mtc_error imap_logout(mtc_net *pnetinfo, mtc_account *paccount)
{
    mtc_error retval= MTC_RETURN_TRUE;
    GString *buf= NULL;

	/*send the message to logout from imap server and check logout was successful*/
	if((retval= imap_send(pnetinfo, paccount, TRUE, "LOGOUT", "LOGOUT\r\n"))!= MTC_RETURN_TRUE)
        return(retval);
	
    if(!(buf= imap_recv(pnetinfo, buf, pnetinfo->msgid- 1, 
        TAGGED_RESPONSE(IMAP_OK)| TAGGED_RESPONSE(IMAP_BAD)| TAGGED_RESPONSE(IMAP_NO)| IMAP_BYE, IMAP_BYE| TAGGED_RESPONSE(IMAP_OK))))
	{
		plg_err(S_IMAPFUNC_ERR_SEND_LOGOUT, paccount->server);
        imap_close(pnetinfo, paccount);
		return(MTC_ERR_CONNECT);
	}
    g_string_free(buf, TRUE);
    return(net_disconnect(pnetinfo));
}

#ifdef SSL_PLUGIN
/*function to receive the CRAM-MD5 string from the server*/
static GString *imap_recv_crammd5(mtc_net *pnetinfo, GString *msg)
{
	/*clear the buffer*/
	gint numbytes= 1;
	gchar tmpbuf[MAXDATASIZE];
	
    msg= g_string_new(NULL);

    /*while there is data available read from server*/
	while(net_available(pnetinfo))
	{
		if((numbytes= net_recv(pnetinfo, tmpbuf, sizeof(tmpbuf)))== MTC_ERR_CONNECT)
		{
			g_string_free(msg, TRUE);
            return(NULL);
		}
		
		if(!numbytes)
			break;

		/*add the read data to the buffer*/
		msg= g_string_append(msg, tmpbuf);
		
		/*added for massive speed improvements (doesnt check imap server again)*/
		if(g_ascii_strncasecmp(msg->str+ (msg->len- 2), "\r\n", 2)== 0)
			break;
	}
	
	/*test that the data was fully received and successful*/
	if((msg->str!= NULL)&& ((g_ascii_strncasecmp(msg->str, "+ ", 2)!= 0)|| (msg->str[msg->len- 1]!= '\n'))) 
	{	
		g_string_free(msg, TRUE);
		return(NULL);
	}
	return(msg);
}

/*function to login to IMAP server with CRAM-MD5 auth*/
static mtc_error crammd5_login(mtc_net *pnetinfo, mtc_account *paccount)
{
	GString *buf= NULL;
	gchar *digest= NULL;
    mtc_error retval= MTC_RETURN_TRUE;
	
	/*send auth command*/
	if((retval= imap_send(pnetinfo, paccount, TRUE, "AUTHENTICATE CRAM-MD5", "AUTHENTICATE CRAM-MD5\r\n"))!= MTC_RETURN_TRUE)
	    return(retval);

	/*receive back from server to check username was sent ok*/
	if(!(buf= imap_recv_crammd5(pnetinfo, buf)))
	{
		plg_err(S_IMAPFUNC_ERR_SEND_AUTHENTICATE, paccount->server);
        imap_close(pnetinfo, paccount);
		return(MTC_ERR_CONNECT);
	}
	
	/*remove trailing whitespace*/
    g_strchomp(buf->str);

    /*create CRAM-MD5 string to send to server*/
	digest= mk_cramstr(paccount, buf->str+ 2, digest);
	g_string_free(buf, TRUE);
	buf= NULL;

	/*check the digest value before continuing*/
	if(digest== NULL)
		return(net_disconnect(pnetinfo));

	/*send the digest to log in*/
	retval= imap_send(pnetinfo, paccount, FALSE, "CRAM-MD5 digest", "%s\r\n", digest);
	g_free(digest);

    if(retval!= MTC_RETURN_TRUE)
        return(retval);

    /*receive back from server to check username was sent ok*/
    if(!(buf= imap_recv(pnetinfo, buf, pnetinfo->msgid- 1, 
        TAGGED_RESPONSE(IMAP_OK)| TAGGED_RESPONSE(IMAP_BAD)| TAGGED_RESPONSE(IMAP_NO)| IMAP_BYE, TAGGED_RESPONSE(IMAP_OK))))
	{	
		plg_err(S_IMAPFUNC_ERR_SEND_CRAM_MD5, paccount->server);
        imap_close(pnetinfo, paccount);
		return(MTC_ERR_CONNECT);
	}
    g_string_free(buf, TRUE);
	return(MTC_RETURN_TRUE);
}
#endif /*SSL_PLUGIN*/

/*test the capabilities of the IMAP server*/
static mtc_error imap_capa(mtc_net *pnetinfo, mtc_account *paccount)
{
	/*send command to test capabilities and check if successful*/
	GString *buf= NULL;
    mtc_error retval= MTC_RETURN_TRUE;

	if((retval= imap_send(pnetinfo, paccount, TRUE, "CAPABILITY", "CAPABILITY\r\n"))!= MTC_RETURN_TRUE)
        return(retval);

	/*get the capability string*/
	if(!(buf= imap_recv(pnetinfo, buf, pnetinfo->msgid- 1,
        TAGGED_RESPONSE(IMAP_OK)| TAGGED_RESPONSE(IMAP_BAD)| IMAP_BYE, IMAP_CAPABILITY| TAGGED_RESPONSE(IMAP_OK)))) 
	{	
		plg_err(S_IMAPFUNC_ERR_GET_IMAP_CAPABILITIES);
        imap_close(pnetinfo, paccount);
		return(MTC_ERR_CONNECT);
	}
	
	/*Test if CRAM-MD5 is supported*/
	if((pnetinfo->authtype== IMAPCRAM_PROTOCOL)&& (strstr(buf->str, "CRAM-MD5")== NULL))
	{
		plg_err(S_IMAPFUNC_ERR_CRAM_MD5_NOT_SUPPORTED);
		imap_close(pnetinfo, paccount);
		g_string_free(buf, TRUE);
		return(MTC_RETURN_FALSE);
	}
	g_string_free(buf, TRUE);
	return(MTC_RETURN_TRUE);
}

/*function to Select the INBOX as the mailbox to use*/
static GString *imap_select_inbox(mtc_net *pnetinfo, mtc_account *paccount, GString *buf)
{
	/*create the message to select the inbox and return if successful*/
    gchar *uidv= "UIDVALIDITY ";
	gchar *spos= NULL, *epos= NULL;
    gint uidvlen= 0;
    
    buf= NULL;

    /*send message to select inbox*/
	if(imap_send(pnetinfo, paccount, TRUE, "SELECT", "SELECT INBOX\r\n")!= MTC_RETURN_TRUE)
	    return(NULL);

    if(!(buf= imap_recv(pnetinfo, buf, pnetinfo->msgid- 1,
        TAGGED_RESPONSE(IMAP_OK)| TAGGED_RESPONSE(IMAP_BAD)| TAGGED_RESPONSE(IMAP_NO)| IMAP_BYE, TAGGED_RESPONSE(IMAP_OK)))) 
	{
		plg_err(S_IMAPFUNC_ERR_SEND_SELECT, paccount->server);
        imap_close(pnetinfo, paccount);
	}
    
    /*get the UIDVALIDITY value, copy 0 if not found*/
	if((spos= strstr(buf->str, uidv))== NULL)
		g_strlcpy(buf->str, "0", 2);
	
    uidvlen= strlen(uidv);
	if((epos= strchr(spos+ uidvlen, ']'))== NULL)
	{
        g_string_free(buf, TRUE);
		return(NULL);
	}
	else
	{
		gchar *tempbuf;
		
		tempbuf= (gchar *)g_malloc0(epos- spos);
		g_strlcpy(tempbuf, spos+ uidvlen, (epos- (spos+ uidvlen))+ 1);
		buf= g_string_assign(buf, tempbuf);
		g_free(tempbuf);
	}
	return(buf);
}

/*get the fetch data from a line, including UID's and flags*/
static gchar *imap_get_uid(gchar *uidstring, gchar *spos, gchar *epos, gchar *uidvalidity)
{
    gchar *sflag= NULL, *eflag= NULL;
    gchar *uid= NULL;
    gint uidlen= 0;
    
    /*find the "FETCH" to verify we have a correct line*/
    sflag= strstr(spos, " FETCH ");
    if((sflag== NULL)|| (sflag> epos))
       return(NULL);

    /*find the start parenthesis*/
    sflag= strchr(sflag+ 6, '(');
    if((sflag== NULL)|| (sflag> epos))
       return(NULL);

    /*check if the message is seen, don't add if that is the case*/
    eflag= strstr(sflag, "\\Seen");
    if((eflag!= NULL)&& (eflag< epos))
        return(NULL);
    
    /*get the UID from the message*/
    sflag= strstr(sflag, "UID ");
    if((sflag== NULL)|| (sflag> epos))
        return(NULL);

    sflag+= 4;
    eflag= strchr(sflag, ' ');
    if((eflag== NULL)|| (eflag> epos))
        return(NULL);

    /*create the unique string from the uid and uidvalidity values*/
    uid= g_strndup(sflag, eflag- sflag);

    /*3 takes into account the '-' and the '\n'*/
    uidlen= strlen(uid)+ strlen(uidvalidity)+ 3* sizeof(gchar);
    uidstring= (gchar *)g_malloc0(uidlen);
    g_snprintf(uidstring, uidlen, "%s-%s\n", uidvalidity, uid);
    g_free(uid);
    
    return(uidstring);
}

/*function to get the header of a message*/
#ifdef MTC_NOTMINIMAL
static GString *imap_get_header(mtc_net *pnetinfo, mtc_account *paccount, GString *buf, gchar *message)
{
    gchar *spos;

    /*find the UID part*/
    if((spos= strchr(message, '-'))== NULL)
	{
		plg_err(S_IMAPFUNC_ERR_GET_UID);
		return(NULL);
	}

    /*need to get rid of the \r\n*/
    g_strchomp(spos);
	
    /*get the header. Peek is used so as to not set the 'Seen' flag*/
    /*the received string will also contain the response, but this should not be a problem*/
    if(imap_send(pnetinfo, paccount, TRUE, "UID FETCH", "UID FETCH %s (FLAGS BODY.PEEK[HEADER])\r\n", spos+ 1)== MTC_RETURN_TRUE)
    {
        buf= imap_recv(pnetinfo, buf, pnetinfo->msgid- 1,
            TAGGED_RESPONSE(IMAP_OK)| TAGGED_RESPONSE(IMAP_BAD)| TAGGED_RESPONSE(IMAP_NO)| IMAP_BYE, TAGGED_RESPONSE(IMAP_OK));
	    
    }
    /*put the \r\n back*/
    g_strlcat(message, "\r\n", strlen(message)+ 1);
    return(buf);
}
#endif /*MTC_NOTMINIMAL*/

/*get the full fetch data, including UID's and flags*/
#ifdef MTC_EXPERIMENTAL
static mtc_error imap_fetch_data(mtc_net *pnetinfo, mtc_account *paccount, const mtc_cfg *pconfig, gchar *msgs, gchar *uidvalidity)
#else
static mtc_error imap_fetch_data(mtc_net *pnetinfo, mtc_account *paccount, gchar *msgs, gchar *uidvalidity)
#endif
{

    gchar *spos= NULL, *epos= NULL;
    gchar *uidstring= NULL;
    mtc_error retval= MTC_RETURN_TRUE;

    /*reset the message list (must be done after marking as read)*/
#ifdef MTC_NOTMINIMAL
    mtc_filters *pfilters= NULL;
	GString *header= NULL;
    msglist_reset(paccount);
#else
    paccount->num_messages= 0;
#endif /*MTC_NOTMINIMAL*/

    /*find the starting point of the first line*/
    spos= msgs;
    if((spos!= NULL)&& (*spos!= '*'))
    {
        if((spos= strstr(msgs, "\r\n*"))!= NULL)
            spos+= 2;
    }

    /*iterate through each valid line*/
    while(spos!= NULL)
    {
        if((epos= strstr(spos, "\r\n"))== NULL)
            break;
        
        /*end and start found, so add to list etc.*/
        uidstring= imap_get_uid(uidstring, spos, epos, uidvalidity);
        if(uidstring!= NULL)
        {
            
#ifdef MTC_NOTMINIMAL
             pfilters= (mtc_filters *)paccount->plg_opts;

            /*get/add the message header if we need to*/
#ifdef MTC_EXPERIMENTAL
	        if(((pfilters!= NULL)&& pfilters->enabled)|| pconfig->run_summary)
#else
	        if((pfilters!= NULL)&& pfilters->enabled)
#endif /*MTC_EXPERIMENTAL*/
            {
               if((header= imap_get_header(pnetinfo, paccount, header, uidstring))== NULL)
               {
                   retval= MTC_ERR_CONNECT;
                   g_free(uidstring);
                   break;
               }
            }
            paccount->msginfo.msglist= msglist_add(paccount, uidstring, header);
			if(header!= NULL)
            {
                g_string_free(header, TRUE);
                header= NULL;
            }
#else
            paccount->num_messages++;
#endif /*MTC_NOTMINIMAL*/

            g_free(uidstring);
        }

        if((spos= strstr(epos, "\r\n*"))!= NULL)
            spos+= 2;
    }

#ifdef MTC_NOTMINIMAL
    paccount->msginfo.msglist= msglist_cleanup(paccount);
    if(!msglist_verify(paccount))
    {
        plg_err(S_IMAPFUNC_ERR_VERIFY_MSGLIST);
        return(MTC_RETURN_FALSE);
    }
#endif /*MTC_NOTMINIMAL*/
    return(retval);
}

/*function to mark the messages as read*/
static mtc_error imap_mark_read(mtc_net *pnetinfo, mtc_account *paccount, const gchar *cfgdir)
{
    gchar line[LINE_LENGTH];
	FILE *infile= NULL;
	gchar *spos= NULL;
	GString *buf= NULL;
	mtc_error retval= MTC_RETURN_TRUE;
    gchar tmpuidlfile[NAME_MAX];

    /*get the full path for the uidl file and temp uidl file*/
	mtc_file(tmpuidlfile, cfgdir, TMP_UIDL_FILE, paccount->id);

    /*if uidl file exists, open it for reading*/
    if(!IS_FILE(tmpuidlfile))
        return(MTC_RETURN_TRUE);

    if((infile= g_fopen(tmpuidlfile, "rt"))== NULL)
	{
		plg_err(S_IMAPFUNC_ERR_OPEN_FILE, tmpuidlfile);
		return(MTC_ERR_EXIT);
    }

	memset(line, '\0', LINE_LENGTH);
	
	/*get each uid from the file and send message to mark it as read*/
    while(fgets(line, LINE_LENGTH, infile)!= NULL)
	{
		spos= NULL;

		/*strip the uid from the line*/ 
		g_strchomp(line);
        if((spos= strchr(line, '-'))== NULL)
		{
			plg_err(S_IMAPFUNC_ERR_GET_UID);
			fclose(infile);
            return(MTC_ERR_EXIT);
		}

		/*send message to mark as read and receive*/
        if((retval= imap_send(pnetinfo, paccount, TRUE,
           "UID STORE", "UID STORE %s +FLAGS.SILENT (\\Seen)\r\n", spos+ 1))!= MTC_RETURN_TRUE)
            return(retval);
 
        if(!(buf= imap_recv(pnetinfo, buf, pnetinfo->msgid- 1,
            TAGGED_RESPONSE(IMAP_OK)| TAGGED_RESPONSE(IMAP_BAD)| TAGGED_RESPONSE(IMAP_NO)| IMAP_BYE, TAGGED_RESPONSE(IMAP_OK)))) 
	    {
			plg_err(S_IMAPFUNC_ERR_SEND_STORE, paccount->server);
            imap_close(pnetinfo, paccount);
			fclose(infile);
            return(MTC_ERR_CONNECT);
	    }       
        g_string_free(buf, TRUE);
        buf= NULL;
		memset(line, '\0', LINE_LENGTH);
	}
	
	/*close file and cleanup*/
	if(fclose(infile)== EOF)
	{
		plg_err(S_IMAPFUNC_ERR_CLOSE_FILE, tmpuidlfile);
		return(MTC_ERR_EXIT);
	}
	
	/*remove the file after marking as read*/
	if(g_remove(tmpuidlfile)== -1)
	{	
		plg_err(S_IMAPFUNC_ERR_REMOVE_FILE, tmpuidlfile);
		return(MTC_ERR_EXIT);
	}
	return(MTC_RETURN_TRUE);
}

/*function to output the uids of each message*/
static mtc_error imap_get_msgs(mtc_net *pnetinfo, mtc_account *paccount, const mtc_cfg *pconfig)
{
	GString *uidvalidity= NULL;
	GString *buf= NULL;
	mtc_error retval= MTC_RETURN_TRUE;

	/*select INBOX first and get UIDVALIDITY value*/
	if((uidvalidity= imap_select_inbox(pnetinfo, paccount, uidvalidity))== NULL)
		return(MTC_ERR_CONNECT);
	
    /*mark as read if the temporary uid file exists*/
    if((retval= imap_mark_read(pnetinfo, paccount, pconfig->dir))!= MTC_RETURN_TRUE)
        return(retval);

    /*send message to get uids command was successful*/
	if((retval= imap_send(pnetinfo, paccount, TRUE, "UID FETCH", "UID FETCH 1:* FLAGS\r\n"))!= MTC_RETURN_TRUE)
        return(retval);

	/*get the UID's of the messages*/
    if(!(buf= imap_recv(pnetinfo, buf, pnetinfo->msgid- 1,
        TAGGED_RESPONSE(IMAP_OK)| TAGGED_RESPONSE(IMAP_BAD)| TAGGED_RESPONSE(IMAP_NO)| IMAP_BYE, TAGGED_RESPONSE(IMAP_OK)))) 
	{
		plg_err(S_IMAPFUNC_ERR_SEND_SELECT, paccount->server);
        imap_close(pnetinfo, paccount);
	    g_string_free(uidvalidity, TRUE);
        return(MTC_ERR_CONNECT);
	}
 
    /*get all the fetch data etc*/
#ifdef MTC_EXPERIMENTAL
    retval= imap_fetch_data(pnetinfo, paccount, pconfig, buf->str, uidvalidity->str);
#else
    retval= imap_fetch_data(pnetinfo, paccount, buf->str, uidvalidity->str);
#endif

    g_string_free(buf, TRUE);
	g_string_free(uidvalidity, TRUE);
	return(retval);
}

/*function to check mail (in clear text mode)*/
static mtc_error check_mail(mtc_net *pnetinfo, mtc_account *paccount, mtc_error (*authfunc)(mtc_net *, mtc_account *), const mtc_cfg *pconfig)
{
    mtc_error retval= MTC_RETURN_TRUE;
    GString *buf= NULL;

    pnetinfo->msgid= 0;

	/*connect to the imap server*/
	if(net_connect(pnetinfo, paccount)!= MTC_RETURN_TRUE)
		return(MTC_ERR_CONNECT);

	/*receive the string back from the server*/

	if(!(buf= imap_recv(pnetinfo, buf, pnetinfo->msgid, IMAP_OK| IMAP_PREAUTH| IMAP_BYE, IMAP_OK)))
        return(MTC_ERR_CONNECT);

    g_string_free(buf, TRUE);

	/*test the IMAP server capabilites*/
	if((retval= imap_capa(pnetinfo, paccount))!= MTC_RETURN_TRUE)
		return(retval);

    /*run the authentication function*/
    if((retval= (*authfunc)(pnetinfo, paccount))!= MTC_RETURN_TRUE)
        return(retval);

	/*get the number of messages and add to message list*/
	if((retval= imap_get_msgs(pnetinfo, paccount, pconfig))!= MTC_RETURN_TRUE)
		return(retval);

	/*logout from the imap server*/
	return(imap_logout(pnetinfo, paccount));
}

/*function to read the IMAP mail*/
mtc_error imap_read_mail(mtc_account *paccount, const mtc_cfg *pconfig)
{
	FILE *outfile= NULL;
#ifdef MTC_NOTMINIMAL
    GSList *pcurrent= NULL;
    msg_struct *pcurrent_data= NULL;
#endif /*MTC_NOTMINIMAL*/
    gchar tmpuidlfile[NAME_MAX];
	
	/*get full paths of files*/
	mtc_file(tmpuidlfile, pconfig->dir, TMP_UIDL_FILE, paccount->id);

	if((outfile= g_fopen(tmpuidlfile, "wt"))== NULL)
	{
		plg_err(S_IMAPFUNC_ERR_OPEN_FILE_WRITE, tmpuidlfile); 
		return(MTC_RETURN_FALSE);
	}

/*WARNING!!!
 *disabling the message list WILL NOT WORK for IMAP at the moment, it is broken
 *this is because the IMAP reading (i.e next lines) uses the message list to read
 *DON'T DISABLE IT.
 *if you wish to use imap, you must use the message list
 *for POP feel free to disable it*/
    /*Traverse list here and output UIDS to mark as seen*/
#ifdef MTC_NOTMINIMAL
    pcurrent= paccount->msginfo.msglist;
    while(pcurrent!= NULL)
    {
        pcurrent_data= (msg_struct *)pcurrent->data;
        
        fputs(pcurrent_data->uid, outfile);
        pcurrent= g_slist_next(pcurrent);
    }
#endif /*MTC_NOTMINIMAL*/

	/*Close the files*/
	if(fclose(outfile)== EOF)
	{
		plg_err(S_IMAPFUNC_ERR_CLOSE_FILE, tmpuidlfile); 
		return(MTC_RETURN_FALSE);
	}	
	return(MTC_RETURN_TRUE);
}

/*call check_mail for IMAP*/
mtc_error check_imap_mail(mtc_account *paccount, const mtc_cfg *pconfig)
{
    mtc_net netinfo;
	netinfo.authtype= IMAP_PROTOCOL;
	return(check_mail(&netinfo, paccount, &imap_login, pconfig));
}

#ifdef SSL_PLUGIN
/*call check_mail for IMAP (CRAM-MD5)*/
mtc_error check_cramimap_mail(mtc_account *paccount, const mtc_cfg *pconfig)
{
    mtc_net netinfo;
	netinfo.authtype= IMAPCRAM_PROTOCOL;
    return(check_mail(&netinfo, paccount, &crammd5_login, pconfig));
}

/*call check_mail for IMAP (SSL/TLS)*/
mtc_error check_imapssl_mail(mtc_account *paccount, const mtc_cfg *pconfig)
{
    mtc_net netinfo;
	netinfo.authtype= IMAPSSL_PROTOCOL;
	return(check_mail(&netinfo, paccount, &imap_login, pconfig));
}
#endif /*SSL_PLUGIN*/

