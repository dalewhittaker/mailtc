/* strings.h
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

#ifndef DW_MAILTC_STRINGS_FILE
#define DW_MAILTC_STRINGS_FILE

/****************************************************************
 * This file contains all the GTK strings for the application   *
 * so that they can be easily localised.                        *
 *                                                              *
 * If you wish to use accented strings, you MUST convert this   *
 * file from ASCII to UTF8 before compiling, otherwise GTK will *
 * not read the accented characters correctly.                  *
 ***************************************************************/

/***DIALOG STRINGS***/

/*docklet.c strings*/
#define S_DOCKLET_NEW_MESSAGES "%s: %d new message%s%s"
#define S_DOCKLET_CONNECT_ERR "There was an error connecting to the following servers:\n\n%s\nPlease check the %s log for the error.\n"
#define S_DOCKLET_MULTIPLE_ACCOUNTS "Read/Display new messages for multiple accounts"

/*main.c strings*/
#define S_MAIN_INSTANCE_RUNNING "An instance of %s is already running.\n"
#define S_MAIN_NO_CONFIG_FOUND "No mail configuration was found.\nPlease enter your mail details.\n"
#define S_MAIN_OLD_VERSION_FOUND "You are running a new version of %s.\nThis version uses a new format for the icons.\n\
						\nPlease set the icon colours for your mail accounts.\n"

/*configdlg.c strings*/
#define S_CONFIGDLG_PASSWORD "password"
#define S_CONFIGDLG_USERNAME "username"
#define S_CONFIGDLG_PORT "port"
#define S_CONFIGDLG_HOSTNAME "hostname"
#define S_CONFIGDLG_ACCNAME "account name"
#define S_CONFIGDLG_DETAILS_INCOMPLETE "You must enter a %s.\n"
#define S_CONFIGDLG_READYTORUN "Now run %s to check mail."
#define S_CONFIGDLG_BUTTON_ICONCOLOUR "Select Icon Colour"
#define S_CONFIGDLG_DETAILS_ACNAME "Account name:"
#define S_CONFIGDLG_DETAILS_SERVER "Mail server:"
#define S_CONFIGDLG_DETAILS_PORT "Port:"
#define S_CONFIGDLG_DETAILS_USERNAME "Username:"
#define S_CONFIGDLG_DETAILS_PASSWORD "Password:"
#define S_CONFIGDLG_DETAILS_PROTOCOL "Mail protocol:"
#define S_CONFIGDLG_ENABLEFILTERS  "Use mail filters"
#define S_CONFIGDLG_CONFIGFILTERS "Configure filters..."  
#define S_CONFIGDLG_ICON_COLOUR "Icon colour:"
#define S_CONFIGDLG_ICON_COLOUR_MULTIPLE "Icon colour (multiple accounts):"
#define S_CONFIGDLG_SETICONCOLOUR "Select icon colour..."
#define S_CONFIGDLG_DETAILS_TITLE "Mail Account Configuration"
#define S_CONFIGDLG_CONFIG_TITLE "%s configuration"
#define S_CONFIGDLG_TAB_GENERAL "General"
#define S_CONFIGDLG_TAB_ACCOUNTS "Mail accounts"
#define S_CONFIGDLG_INTERVAL "Interval in minutes for mail check: "
#define S_CONFIGDLG_SMALLICON "Use small envelope icon"
#define S_CONFIGDLG_MAILAPP "Mail reading program:"
#define S_CONFIGDLG_LTAB_ACCOUNT "Mail Account"
#define S_CONFIGDLG_LTAB_PROTOCOL "Protocol"
#define S_CONFIGDLG_MULTI_TOOLTIP "Enable this option to display the number of new messages for all accounts, rather than for a single account at a time.\n\
								\nThis option will also read all accounts at once, rather than a single account."

/*filterdlg.c strings*/
#define S_FILTERDLG_NO_FILTERS "You must enter at least one filter string"
#define S_FILTERDLG_COMBO_SENDER "Sender"
#define S_FILTERDLG_COMBO_SUBJECT "Subject"
#define S_FILTERDLG_COMBO_CONTAINS "Contains"
#define S_FILTERDLG_COMBO_NOTCONTAINS "Does not contain"
#define S_FILTERDLG_TITLE "Mail filters"
#define S_FILTERDLG_BUTTON_MATCHALL "Match all conditions"
#define S_FILTERDLG_BUTTON_MATCHANY "Match any condition"
#define S_FILTERDLG_BUTTON_CLEAR "Clear filters"
#define S_FILTERDLG_LABEL_SELECT "Select up to %u filters:"

/***ERROR STRINGS***/

/*common.c strings*/
#define S_COMMON_ERR_ALLOC "Error allocating memory\n"
#define S_COMMON_ERR_REALLOC "Error reallocating memory\n"
#define S_COMMON_ERR_STR_INS "str_ins error: invalid position!\n"

/*configdlg.c strings*/
#define S_CONFIGDLG_ERR_COMBO_ITER "Error getting listbox iterator\n"
#define S_CONFIGDLG_ERR_REMOVE_FILE "Error attempting to write to file %s\n"
#define S_CONFIGDLG_ERR_LISTBOX_ITER "Error getting listbox iterator\n"
#define S_CONFIGDLG_ERR_CREATE_PIXBUF "Error creating icon pixbuf\n"
#define S_CONFIGDLG_ERR_GET_ACCOUNT_INFO "Error getting information for account %d\n"

/*docklet.c strings*/
#define S_DOCKLET_ERR_FORK_APP "Error creating fork for mail application %s\n"
#define S_DOCKLET_ERR_EXEC_APP "Error executing applicaiton %s\n"
#define S_DOCKLET_ERR_UNKNOWN_PROTOCOL "Unknown mail protocol selected\n"
#define S_DOCKLET_ERR_CLICK_INVALID_PROTOCOL "Error, invalid protocol while processing docklet click\n"
#define S_DOCKLET_ERR_CLOSE_FILE "Error closing file %s\n"
#define S_DOCKLET_ERR_OPEN_FILE_WRITE "Error opening %s for writing\n"
#define S_DOCKLET_ERR_OPEN_FILE_READ "Error opening %s for reading\n"
#define S_DOCKLET_ERR_READ_FILE "Error reading file %s\n"
#define S_DOCKLET_ERR_WRITE_FILE "Error writing file %s\n"
#define S_DOCKLET_ERR_ACCESS_FILE "Error accessing %s when marking read\n"
#define S_DOCKLET_ERR_RENAME_FILE "Error renaming %s to %s\n"

/*encrypter.c strings*/
#define S_ENCRYPTER_ERR_ENC_PW "Error encrypting password\n"
#define S_ENCRYPTER_ERR_ENC_PW_FINAL "Error finalising password encryption\n"
#define S_ENCRYPTER_ERR_DEC_PW "Error decrypting password\n"
#define S_ENCRYPTER_ERR_DEC_PW_FINAL "Error finalising password decryption\n"
#define S_ENCRYPTER_ERR_APOP_DIGEST_INIT "Error initialising message digest\n"
#define S_ENCRYPTER_ERR_APOP_DIGEST_CREATE "Error creating MD5 digest\n"
#define S_ENCRYPTER_ERR_APOP_DIGEST_FINAL "Error finalising digest creation\n"

/*filterdlg.c strings*/
#define S_FILTERDLG_ERR_REMOVE_FILE "Error attempting to write to file %s\n"
#define S_FILTERDLG_ERR_OPEN_FILE "Error opening file %s\n"
#define S_FILTERDLG_ERR_COMBO_ITER "Error getting combobox iterator\n"
#define S_FILTERDLG_ERR_CLOSE_FILE "Error closing file %s\n"

/*filefunc.c strings*/
#define S_FILEFUNC_ERR_OPEN_FILE "Error opening file %s\n"
#define S_FILEFUNC_ERR_CLOSE_FILE "Error closing file %s\n"
#define S_FILEFUNC_ERR_GET_PW "Error retrieving password\n"
#define S_FILEFUNC_ERR_SET_PERM "Error setting permissions on file %s\n"
#define S_FILEFUNC_ERR_REMOVE_FILE "Error removing file %s\n"
#define S_FILEFUNC_ERR_RENAME_FILE "Error renaming %s to %s\n"
#define S_FILEFUNC_ERR_WRITE_PW "Error writing password to file\n"
#define S_FILEFUNC_ERR_GET_DELAY "Error retrieving mail checking delay information\n"
#define S_FILEFUNC_ERR_GET_MAILAPP "Error retrieving mail reading application information\n"
#define S_FILEFUNC_ERR_GET_HOSTNAME "Error retrieving server hostname information for account %d\n"
#define S_FILEFUNC_ERR_GET_PORT "Error retrieving server port information for account %d\n"
#define S_FILEFUNC_ERR_GET_USERNAME "Error retrieving username for account %d\n"
#define S_FILEFUNC_ERR_GET_ICONTYPE "Error retrieving icon type for account %d\n"
#define S_FILEFUNC_DEFAULT_ACCNAME "Account %d"
#define S_FILEFUNC_ERR_GET_FILTER "Error getting filter details %d\n"
#define S_FILEFUNC_ERR_GET_PASSWORD "Error retrieving password for account %d\n"
#define S_FILEFUNC_ERR_WRITE_DELAY "Error writing mail checking delay information to file\n"
#define S_FILEFUNC_ERR_WRITE_MAILAPP "Error writing mail reading application to file\n"
#define S_FILEFUNC_ERR_WRITE_ICONSIZE "Error writing icon size information to file\n"
#define S_FILEFUNC_ERR_WRITE_MULTIPLE "Error writing read multiple information to file\n"
#define S_FILEFUNC_ERR_WRITE_M_ICON_COLOUR "Error writing multiple icon colour information to file\n"
#define S_FILEFUNC_ERR_WRITE_HOSTNAME "Error writing server hostname to file\n"
#define S_FILEFUNC_ERR_WRITE_PORT "Error writing server port to file\n"
#define S_FILEFUNC_ERR_WRITE_USERNAME "Error writing username to file\n"
#define S_FILEFUNC_ERR_WRITE_ICONTYPE "Error writing icon type to file\n"
#define S_FILEFUNC_ERR_WRITE_PROTOCOL "Error writing mail protocol to file\n"
#define S_FILEFUNC_ERR_WRITE_ACCNAME "Error writing account name to file\n"
#define S_FILEFUNC_ERR_WRITE_FILTER "Error writing filter status to file\n"
#define S_FILEFUNC_ERR_ATTEMPT_WRITE "Error attempting to write to file %s\n"
#define S_FILEFUNCT_ERR_MOVE_FP "Error moving file pointer for config file\n"
		
/*main.c strings*/
#define S_MAIN_ERR_DELAY_INFO "Error getting delay information\n"
#define S_MAIN_ERR_INSTANCE_RUNNING "Error: Instance of %s already running\n"
#define S_MAIN_ERR_ATEXIT_FUNC "Error cannot set exit function: "
#define S_MAIN_ERR_CLOSE_LOGFILE "Error closing log file: "
#define S_MAIN_ERR_OPEN_LOGFILE "Error writing header to log file: "
#define S_MAIN_ERR_APP_KILLED "%s killed.\n"
#define S_MAIN_ERR_SEGFAULT "%s encountered a segmentation fault.  Please report this bug\n"
#define S_MAIN_ERR_CLOSE_PIDFILE "Error closing PID file\n"
#define S_MAIN_ERR_CANNOT_KILL "Error: cannot kill process: %d\n"
#define S_MAIN_ERR_OPEN_PIDFILE_WRITE "Error opening pid file %s for writing\n"
#define S_MAIN_ERR_OPEN_PIDFILE_READ "Error reading PID file %s\n"
#define S_MAIN_ERR_RENAME_PIDFILE "Error renaming pid file %s to temp pid file %s\n"
#define S_MAIN_LOG_STARTED "%s started %s\n"
#define S_MAIN_ERR_PRINT_USAGE "%s %s (C) 2006 Dale Whittaker\nUsage:\n\t%s (to run %s program)\
					\n\t%s -c (to configure mail details)\
					\n\t%s -d (run in network debug mode)\
					\n\t%s -k (kill all mailtc processes)\n"

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

/*netfunc.c strings*/
#define S_NETFUNC_ERR_IP "Error getting ip from hostname %s\n"
#define S_NETFUNC_ERR_SOCKET "Error getting socket\n"
#define S_NETFUNC_ERR_CONNECT "Error connecting to server %s\n"
#define S_NETFUNC_ERR_SEND "Error sending message to server\n"
#define S_NETFUNC_ERR_RECEIVE "Error receiving message from server\n"
#define S_NETFUNC_ERR_DATA_AVAILABLE "Error while checking if there is any data to receive from server\n"

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
	
/*sasl.c strings*/
#define S_SASL_ERR_INITIALISE "Cannot initialise SASL client (%d): %s\n"
#define S_SASL_ERR_CRAM_MD5_AUTH "CRAM-MD5 authentication error (%d): %s\n"

/*tls.c strings*/
#define S_TLS_ERR_CTX "Error creating CTX\n"
#define S_TLS_ERR_CREATE_STRUCT "Error creating SSL structure\n"
#define S_TLS_ERR_SSL_DESC "Error setting SSL file descriptor\n"
#define S_TLS_ERR_SSL_CONNECT "Error connecting with SSL\n"
#define S_TLS_ERR_SSL_SHUTDOWN "Error shutting down SSL connection\n"

#endif /*DW_MAILTC_STRINGS_FILE*/
