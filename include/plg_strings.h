/* plg_strings.h
 * Copyright (C 2006 Dale Whittaker <dayul@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option any later version.
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

#ifndef MTC_PLUGIN_STRINGS
#define MTC_PLUGIN_STRINGS

/*All the strings for the plugins*/

/*plg_common.c strings*/
#define S_PLG_COMMON_ERR_ALLOC "Error allocating memory\n"
#define S_PLG_COMMON_ERR_REALLOC "Error reallocating memory\n"
#define S_PLG_COMMON_ERR_SASL_INIT "Cannot initialise SASL client (%d: %s)\n"
#define S_PLG_COMMON_ERR_CRAM_MD5_AUTH "CRAM-MD5 authentication error (%d: %s)\n"
#define S_PLG_COMMON_ERR_CTX "Error creating CTX\n"
#define S_PLG_COMMON_ERR_CREATE_SSL_STRUCT "Error creating SSL structure\n"
#define S_PLG_COMMON_ERR_SSL_DESC "Error setting SSL file descriptor\n"
#define S_PLG_COMMON_ERR_SSL_CONNECT "Error connecting with SSL\n"
#define S_PLG_COMMON_ERR_SSL_SHUTDOWN "Error shutting down SSL connection\n"
#define S_PLG_COMMON_ERR_BASE64_DECODE "Error: invalid length returned after base 64 decode\n"
#define S_PLG_COMMON_ERR_BASE64_ENCODE "Error: invalid length returned after base 64 decode\n"

/*plg_apop.c strings*/
#define S_PLG_APOP_ERR_DIGEST_INIT "Error initialising message digest\n"
#define S_PLG_APOP_ERR_DIGEST_CREATE "Error creating MD5 digest\n"
#define S_PLG_APOP_ERR_DIGEST_FINAL "Error finalising digest creation\n"

/*imapfunc.c strings*/
#define S_IMAPFUNC_ERR_CONNECT "Connection error. %s closing connection and retrying.\n"
#define S_IMAPFUNC_ERR_SEND_LOGIN "Error sending LOGIN to imap server %s\n"
#define S_IMAPFUNC_ERR_SEND_LOGOUT "Error sending LOGOUT command to imap server %s\n"
#define S_IMAPFUNC_ERR_SASL_INIT "SASL initialisation failure (%d): %s\n"
#define S_IMAPFUNC_ERR_SEND_AUTHENTICATE "Error sending AUTHENTICATE command to imap server %s\n"
#define S_IMAPFUNC_ERR_SEND_CRAM_MD5 "Error sending CRAM-MD5 string to imap server %s\n"
#define S_IMAPFUNC_ERR_NOT_IMAP_SERVER "Error: this does not appear to be a valid IMAP server\n"
#define S_IMAPFUNC_ERR_RECEIVE_CAPABILITY "Error receiving IMAP capability string\n"
#define S_IMAPFUNC_ERR_GET_IMAP_CAPABILITIES "Error getting IMAP server capabilities\n"
#define S_IMAPFUNC_ERR_CRAM_MD5_NOT_SUPPORTED "Error: CRAM-MD5 authentication does not appear to be supported\n"
#define S_IMAPFUNC_ERR_OPEN_READ_FILE "Error opening read file %s\n"
#define S_IMAPFUNC_ERR_GET_UID "Error retreiving uid\n"
#define S_IMAPFUNC_ERR_SEND_UID_FETCH "Error sending UID FETCH to imap server %s\n"
#define S_IMAPFUNC_ERR_SEND_STORE "Error sending STORE to imap server %s\n"
#define S_IMAPFUNC_ERR_CLOSE_READ_FILE "Error closing uid read file\n"
#define S_IMAPFUNC_ERR_OPEN_FILE "Error opening file %s\n"
#define S_IMAPFUNC_ERR_OPEN_TEMP_FILE "Error opening temp UID file for reading\n"
#define S_IMAPFUNC_ERR_CLOSE_TEMP_FILE "Error closing temp UID file\n"
#define S_IMAPFUNC_ERR_CLOSE_FILE "Error closing file %s\n"
#define S_IMAPFUNC_ERR_SEND_SELECT "Error sending SELECT to imap server %s\n"
#define S_IMAPFUNC_ERR_REMOVE_READ_FILE "Error removing uid read file\n"
#define S_IMAPFUNC_ERR_ACCESS_FILE "Error accessing %s when marking read\n"
#define S_IMAPFUNC_ERR_OPEN_FILE_READ "Error opening %s for reading\n"
#define S_IMAPFUNC_ERR_OPEN_FILE_WRITE "Error opening %s for writing\n"
#define S_IMAPFUNC_ERR_READ_FILE "Error reading file %s\n"
#define S_IMAPFUNC_ERR_WRITE_FILE "Error writing file %s\n"
#define S_IMAPFUNC_ERR_REMOVE_FILE "Error removing file %s\n"
#define S_IMAPFUNC_ERR_VERIFY_MSGLIST "Error verifying message list\n"
#define S_IMAPFUNC_ERR_SEND "Error sending %s to server %s\n"

/*msg.c strings*/
#define S_MSG_ERR_NO_FIELD_HEADER "Warning: header has no field '%s'\n"
#define S_MSG_ERR_VERIFY_HEADER "Message header verification failed!\n"
#define S_MSG_ERR_GET_HEADER "Error getting header for message %s\n"
#define S_MSG_HEADER_DATE "\r\nDate"
#define S_MSG_HEADER_FROM "\r\nFrom"
#define S_MSG_HEADER_TO "\r\nTo"
#define S_MSG_HEADER_CC "\r\nCc"
#define S_MSG_HEADER_SUBJECT "\r\nSubject"

/*netfunc.c strings*/
#define S_NETFUNC_ERR_WINSOCK_INIT "Error initialising windows sockets\n"
#define S_NETFUNC_ERR_IP "Error getting ip from hostname %s\n"
#define S_NETFUNC_ERR_SOCKET "Error getting socket\n"
#define S_NETFUNC_ERR_CONNECT "Error connecting to server %s\n"
#define S_NETFUNC_ERR_SEND "Error sending message to server\n"
#define S_NETFUNC_ERR_RECEIVE "Error receiving message from server\n"
#define S_NETFUNC_ERR_DATA_AVAILABLE "Error while checking if there is any data to receive from server\n"
#define S_NETFUNC_ERR_PW_STRING "Error getting start of password string\n"
#define S_NETFUNC_ERR_GET_TIMEOUT "Error getting current connection timeout value\n"
#define S_NETFUNC_ERR_SET_TIMEOUT "Error setting connection timeout value\n"
#define S_NETFUNC_ERR_RESET_TIMEOUT "Error resetting connection timeout value\n"

/*popfunc.c strings*/
#define S_POPFUNC_ERR_CONNECT "Connection error. %s closing connection and retrying.\n"
#define S_POPFUNC_ERR_BAD_MAIL_HEADER "Warning, mail header does not conform to spec!\n"
#define S_POPFUNC_ERR_APOP_NOT_SUPPORTED "Error: APOP does not appear to be supported\n"
#define S_POPFUNC_ERR_APOP_ENCRYPT_TIMESTAMP "Error encrypting APOP timestamp string\n"
#define S_POPFUNC_ERR_APOP_SEND_DETAILS "Error sending APOP details to pop server %s\n"
#define S_POPFUNC_ERR_APOP_NOT_COMPILED "Error: APOP support not compiled in\n"
#define S_POPFUNC_ERR_SEND_USERNAME "Error sending username to pop server %s\n"
#define S_POPFUNC_ERR_SEND_PASSWORD "Error sending password to pop server %s\n"
#define S_POPFUNC_ERR_SASL_INIT "SASL initialisation failure (%d): %s\n"
#define S_POPFUNC_ERR_SEND_CRAM_MD5_AUTH "Error sending auth command to pop server %s\n"
#define S_POPFUNC_ERR_RECEIVE_NUM_MESSAGES "Error retrieving number of messages on the pop server %s\n"
#define S_POPFUNC_ERR_GET_TOTAL_MESSAGES "Error getting total number of messages\n"
#define S_POPFUNC_ERR_CLOSE_FILE "Error closing file %s\n"
#define S_POPFUNC_ERR_OPEN_FILE "Error opening file %s\n"
#define S_POPFUNC_ERR_GET_UIDL "Error retrieving uidl value\n"
#define S_POPFUNC_ERR_RECEIVE_TOP "Error getting TOP for message %d\n"
#define S_POPFUNC_ERR_RECEIVE_UIDL "Error getting UIDL for message %d\n"
#define S_POPFUNC_ERR_INVALID_AUTH "Invalid auth method specified\n"
#define S_POPFUNC_ERR_SEND_QUIT "Error sending QUIT command to pop server %s\n"
#define S_POPFUNC_ERR_CRAM_MD5_NOT_SUPPORTED "Error: CRAM-MD5 authentication does not appear to be supported\n"
#define S_POPFUNC_ERR_TEST_CAPABILITIES "Error testing POP server capabilities, this does not appear to be a valid POP server\n"
#define S_POPFUNC_ERR_RENAME_FILE "Error renaming %s to %s\n"
#define S_POPFUNC_ERR_VERIFY_MSGLIST "Error verifying message list\n"
#define S_POPFUNC_ERR_SEND "Error sending %s to server %s\n"
#define S_POPFUNC_ERR_RECV "Error receiving %s for account %s\n"
#define S_POPFUNC_ERR_RECV_HEADER "Error receiving message header %d for account %s\n"

#endif /*MTC_PLUGIN_STRINGS*/
