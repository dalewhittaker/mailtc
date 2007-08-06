/* msg.c
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

#include "plg_common.h"

#if 0
gboolean print_msg_info(mtc_account *paccount)
{
    GSList *pcurrent;
    msg_struct *pitem;

    pcurrent= paccount->msginfo.msglist;
    while(pcurrent!= NULL)
    {
        pitem= (msg_struct *)pcurrent->data;

        g_print(pitem->uid);
        if(pitem->flags& MSG_NEW)
            g_print(" new");
        if(pitem->flags& MSG_ADDED)
            g_print(" added");
        g_print("\n");

        if(pitem->header)
            g_print("Date: %s\nFrom: %s\nTo: %s\nSubject: %s\n", pitem->header->pdate, pitem->header->pfrom, pitem->header->pto, pitem->header->psubject);
        
        pcurrent=g_slist_next(pcurrent);
    }
    return TRUE;
}
#endif

/*find the fields from the header struct*/
static gboolean msglist_hfield(GString *header, gchar *field, header_pos *hpos, gboolean required)
{
    gchar *offset= NULL, *ends= NULL;
    gint pos= -1;

    /*find the field*/
    /*if((offset= strstr(header->str, field))== NULL)*/
    /*NOTE TRUE is always returned, as some headers were found to be missing information*/
    if((pos= strstr_cins(header->str, field))== -1)
    {
        if(required)
            plg_err(S_MSG_ERR_NO_FIELD_HEADER, field+ 2);
        return(TRUE);
    }

    /*check it is end of field string*/
    offset= header->str+ pos+ strlen(field);
    if(*offset!= ':'&& *offset!= ' '&& *offset!= '\t')
        return FALSE;
    
    /*find start of data*/
    if(*offset!= ':'&& (offset= strchr(offset, ':'))== NULL)
        return FALSE;

    ++offset;
    while((*offset== ' '|| *offset== '\t')&& ((header->str+ header->len)> offset))
        ++offset;

    /*iterate until end of field body found*/
    ends= offset;
    while((header->str+ header->len)> ends++)
    {
       if(*ends== '\r'&& *(ends+ 1)== '\n')
       {
            /*end of string*/
            if(*(ends+ 2)!= ' '&& *(ends+ 2)!= '\t')
                break;

            /*otherwise move on*/
            ends+= 3;
       }
    }
    if(ends>= (header->str+ header->len))
        return FALSE;

    hpos->offset= offset- header->str;
    hpos->len= ends- offset;

    if(hpos->offset> header->len|| (hpos->offset+ hpos->len)> header->len)
        return FALSE;

    return TRUE;
}

/*get the header from the string*/
msg_header *msglist_header(msg_header *hstruct, GString *phstring)
{
    gint i;
    gboolean allgood= TRUE;
    guint header_size= 0, count= 0;
    gchar *pheader= NULL, *pdata= NULL;
    header_pos hpositions[N_HFIELDS];

    for(i= 0; i< N_HFIELDS; i++)
        memset(&hpositions[i], 0, sizeof(header_pos));

    /*first we find field offsets and sizes*/
    if(!msglist_hfield(phstring, S_MSG_HEADER_DATE, &hpositions[HEADER_DATE], TRUE))
        return NULL;
    if(!msglist_hfield(phstring, S_MSG_HEADER_FROM, &hpositions[HEADER_FROM], TRUE))
        return NULL;
    if(!msglist_hfield(phstring, S_MSG_HEADER_TO, &hpositions[HEADER_TO], TRUE))
        return NULL;
    if(!msglist_hfield(phstring, S_MSG_HEADER_CC, &hpositions[HEADER_CC], FALSE))
        return NULL;
    if(!msglist_hfield(phstring, S_MSG_HEADER_SUBJECT, &hpositions[HEADER_SUBJECT], FALSE))
        return NULL;

    /*then calculate full size to allocate*/
    for(i= 0; i< N_HFIELDS; i++)
        if(hpositions[i].len> 0) header_size= header_size+ hpositions[i].len+ 1;

    pheader= (gchar *)g_malloc0(header_size+ sizeof(msg_header));
    hstruct= (msg_header *)pheader;
    hstruct->bytes= header_size;
    pdata= pheader+ sizeof(msg_header);

    for(i= 0; i< N_HFIELDS; i++)
    {
        if(hpositions[i].len!= 0)
        {
            g_strlcpy(pdata, phstring->str+ hpositions[i].offset, hpositions[i].len+ 1);
            pdata[hpositions[i].len]= '\0';
     
            /*much more efficient way to do this but i can't think right now*/
            switch(i)
            {
                case HEADER_DATE: hstruct->pdate= pdata; break;
                case HEADER_FROM: hstruct->pfrom= pdata; break;
                case HEADER_TO: hstruct->pto= pdata; break;
                case HEADER_CC: hstruct->pcc= pdata; break;
                case HEADER_SUBJECT: hstruct->psubject= pdata; break;
                default:;
            }
            pdata+= (hpositions[i].len+ 1);
        }
    }

    /*Now we verify our data*/
    pdata= pheader+ sizeof(msg_header);
    for(i= 0; i< N_HFIELDS; i++)
    {
        if(hpositions[i].len!= 0)
        {
            /*check everything is pointing to the right place*/
            switch(i)
            {
                case HEADER_DATE: if(pdata!= hstruct->pdate) allgood= FALSE; break;
                case HEADER_FROM: if(pdata!= hstruct->pfrom) allgood= FALSE; break;
                case HEADER_TO: if(pdata!= hstruct->pto) allgood= FALSE; break;
                case HEADER_CC: if((pdata!= hstruct->pcc)) allgood= FALSE; break;
                case HEADER_SUBJECT: if(pdata!= hstruct->psubject) allgood= FALSE; break;
                default:;
            }
            /*check that all the '\0' are correct, and also the bytes is correct*/
            while(*pdata&& (hstruct->bytes> count))
            {
                ++pdata; ++count;
            }
            ++pdata; ++count;
        }
    }
    if(hstruct->bytes!= count)
        allgood= FALSE;

    if(!allgood)
    {
        plg_err(S_MSG_ERR_VERIFY_HEADER);
        g_free(pheader);
        return(NULL);
    }
    return(hstruct);
}

/*test if the message should be filtered*/
static gboolean msglist_filter(msg_header *pheader, mtc_filters *pfilters)
{
    gint i= 0, found= 0;
    gchar *field= NULL;
    GSList *pcurrent= NULL;
    mtc_filter *pfilter= NULL;

    if(pheader== NULL|| pfilters== NULL)
        return FALSE;
    
    pcurrent= pfilters->list;
    while(pcurrent!= NULL)
    {
        pfilter= (mtc_filter *)pcurrent->data;
        if(g_ascii_strcasecmp(pfilter->search_string, "")== 0)
            break;
        
        /*get the field we wish to search*/
        if(pheader)
        {
            switch(pfilter->field)
            {
                case HEADER_FROM:
                    field= pheader->pfrom;
                break;
                case HEADER_TO:
                    field= pheader->pto;
                break;
                case HEADER_CC:
                    field= pheader->pcc;
                break;
                case HEADER_SUBJECT:
                    field= pheader->psubject;
                break;
                default:
                    return FALSE;
            }
        }
        
        /*search the subject or from for a match*/
        if((field!= NULL)&& strstr(field, pfilter->search_string)!= NULL)
        {
            if(pfilter->contains)
                found++;
        }
        else
        {
            if(!pfilter->contains)
                found++;
        }
        i++;
        pcurrent= g_slist_next(pcurrent);
    }
    if(pfilters->matchall)
        return(((found> 0)&& (found== i))? TRUE: FALSE);
    else
        return((found> 0)? TRUE: FALSE);
}

/*create a new message and add to the list*/
static GSList *msglist_new(mtc_account *paccount, const gchar *uid, GString *header)
{
    msg_struct *msgnew= NULL;
    msg_struct *msgfirst;
    GSList *msglist= NULL;
    guint uidlen= 0;

    msglist= paccount->msginfo.msglist;
    msgfirst= (msglist== NULL)? NULL: (msg_struct *)msglist->data;

    /*create new message struct*/
    msgnew= (msg_struct *)g_malloc0(sizeof(msg_struct));

    /*set it as a new message*/
    msgnew->flags|= MSG_NEW;
    msgnew->flags|= MSG_ADDED;

    /*copy the message uid*/
    /*to save space the memory is allocated, otherwise it would be ~80 bytes per email which could grow large*/
    uidlen= strlen(uid)+ 1;
    msgnew->uid= (gchar *)g_malloc0(uidlen);
    g_strlcpy(msgnew->uid, uid, uidlen);

    /*if we need the header, add it here*/
    if(header)
    {
        /*NOTE if header retrieval fails, we still add the message, without a header*/
        if(!(msgnew->header= msglist_header(msgnew->header, header)))
            plg_err(S_MSG_ERR_GET_HEADER, uid);
    }
    /*only increment the messages if it is not filtered*/
    if(msglist_filter(msgnew->header, paccount->pfilters))
        msgnew->flags|= MSG_FILTERED;
    else
    {
        paccount->msginfo.num_messages++;
        paccount->msginfo.new_messages++;
    }

    return((msglist= g_slist_prepend(msglist, msgnew)));
}

/*add to list or set new if it is already there*/
GSList *msglist_add(mtc_account *paccount, gchar *uid, GString *header)
{
    GSList *msglist= NULL;
    msg_struct *list_data= NULL;

    msglist= paccount->msginfo.msglist;
    /*loop through until we find a match (if any)*/
    while(msglist!= NULL)
    {
        list_data= (msg_struct *)msglist->data;
        if(g_ascii_strcasecmp(list_data->uid, uid)== 0)
        {
            /*is already in the list, so set the flag and return*/
            list_data->flags|= MSG_NEW;
        
            /*if filtered do not increment*/
            if(msglist_filter(list_data->header, paccount->pfilters))
                list_data->flags|= MSG_FILTERED;
            else
                paccount->msginfo.num_messages++;
            return(paccount->msginfo.msglist);
        }
    
        msglist= g_slist_next(msglist);
    }
    /*not found, add to the list*/
    return(msglist_new(paccount, uid, header));
}

/*reset the list for a new scan*/
void msglist_reset(mtc_account *paccount)
{
    GSList *pcurrent= NULL;
    msg_struct *pcurrent_data= NULL;

    pcurrent= paccount->msginfo.msglist;
    while(pcurrent!= NULL)
    {
        pcurrent_data= (msg_struct *)pcurrent->data;
        pcurrent_data->flags= MSG_OLD;
        pcurrent= g_slist_next(pcurrent);
    }
    paccount->msginfo.num_messages= 0;
    paccount->msginfo.new_messages= 0;
}

/*remove old messages from list*/
GSList *msglist_cleanup(mtc_account *paccount)
{
    GSList *pcurrent= NULL;
    GSList *pfirst= NULL;
    msg_struct *pcurrent_data= NULL;

    pfirst= pcurrent= paccount->msginfo.msglist;
    while(pcurrent!= NULL)
    {
        pcurrent_data= (msg_struct *)pcurrent->data;
        /*move to next one before removing*/
        pcurrent= g_slist_next(pcurrent);
        if(pcurrent_data->flags== MSG_OLD)
        {
            if(pcurrent_data->header)
            {
                g_free(pcurrent_data->header);
                pcurrent_data->header= NULL;
            }
            if(pcurrent_data->uid)
            {
                g_free(pcurrent_data->uid);
                pcurrent_data->uid= NULL;
            }
            g_free(pcurrent_data);
            pfirst= g_slist_remove(pfirst, pcurrent_data);
        }
    }
    return(pfirst);
}

/*verify the message values are correct*/
gboolean msglist_verify(mtc_account *paccount)
{
    GSList *pcurrent= NULL;
    msg_struct *pcurrent_data= NULL;
    gint num_count= 0, new_count= 0;

    pcurrent= paccount->msginfo.msglist;
    while(pcurrent!= NULL)
    {
        pcurrent_data= (msg_struct *)pcurrent->data;
        if(!(pcurrent_data->flags& MSG_FILTERED))
        {
            num_count++;
            if(pcurrent_data->flags& MSG_ADDED)
                new_count++;
        }
        pcurrent= g_slist_next(pcurrent);
    }
    return((num_count== paccount->msginfo.num_messages&&
            new_count== paccount->msginfo.new_messages));
}

