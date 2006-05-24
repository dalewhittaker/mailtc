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

/*define the gettext stuff*/
/*TODO i don't know exactly how correct this is, although should work on most systems*/
#ifdef ENABLE_NLS

#include <libintl.h>
#include <locale.h>
#define _(text) gettext(text)
#define N_(text) text
#else

#define _(text) text
#define N_(text) text

#endif /*ENABLE_NLS*/


/***DIALOG STRINGS***/

/*docklet.c strings*/
#define S_DOCKLET_NEW_MESSAGE _("%s: %d new message%s")
#define S_DOCKLET_NEW_MESSAGES _("%s: %d new messages%s")
/*#define S_DOCKLET_NEW_MESSAGES _("%s: %d new message%s%s")*/
#define S_DOCKLET_CONNECT_ERR _("There was an error connecting to the following servers:\n\n%s\nPlease check the %s log for the error.\n")
#define S_DOCKLET_MULTIPLE_ACCOUNTS _("Read/Display new messages for multiple accounts")

/*main.c strings*/
#define S_MAIN_INSTANCE_RUNNING _("An instance of %s is already running.\n")
#define S_MAIN_NO_CONFIG_FOUND _("No mail configuration was found.\nPlease enter your mail details.\n")
#define S_MAIN_OLD_VERSION_FOUND _("You are running a new version of %s.\nThis version uses a new format for the icons.\n\
						\nPlease set the icon colours for your mail accounts.\n")

/*configdlg.c strings*/
#define S_CONFIGDLG_PASSWORD _("password")
#define S_CONFIGDLG_USERNAME _("username")
#define S_CONFIGDLG_PORT _("port")
#define S_CONFIGDLG_HOSTNAME _("hostname")
#define S_CONFIGDLG_ACCNAME _("account name")
#define S_CONFIGDLG_DETAILS_INCOMPLETE _("You must enter a %s.\n")
#define S_CONFIGDLG_READYTORUN _("Now run %s to check mail.")
#define S_CONFIGDLG_BUTTON_ICONCOLOUR _("Select Icon Colour")
#define S_CONFIGDLG_DETAILS_ACNAME _("Account name:")
#define S_CONFIGDLG_DETAILS_SERVER _("Mail server:")
#define S_CONFIGDLG_DETAILS_PORT _("Port:")
#define S_CONFIGDLG_DETAILS_USERNAME _("Username:")
#define S_CONFIGDLG_DETAILS_PASSWORD _("Password:")
#define S_CONFIGDLG_DETAILS_PROTOCOL _("Mail protocol:")
#define S_CONFIGDLG_ENABLEFILTERS  _("Use mail filters")
#define S_CONFIGDLG_CONFIGFILTERS _("Configure filters...")  
#define S_CONFIGDLG_ICON_COLOUR _("Icon colour:")
#define S_CONFIGDLG_ICON_COLOUR_MULTIPLE _("Icon colour (multiple accounts):")
#define S_CONFIGDLG_SETICONCOLOUR _("Select icon colour...")
#define S_CONFIGDLG_DETAILS_TITLE _("Mail Account Configuration")
#define S_CONFIGDLG_CONFIG_TITLE _("%s configuration")
#define S_CONFIGDLG_TAB_GENERAL _("General")
#define S_CONFIGDLG_TAB_ACCOUNTS _("Mail accounts")
#define S_CONFIGDLG_INTERVAL _("Interval in minutes for mail check: ")
#define S_CONFIGDLG_SMALLICON _("Use small envelope icon")
#define S_CONFIGDLG_MAILAPP _("Mail reading program:")
#define S_CONFIGDLG_LTAB_ACCOUNT _("Mail Account")
#define S_CONFIGDLG_LTAB_PROTOCOL _("Protocol")
#define S_CONFIGDLG_MULTI_TOOLTIP _("Enable this option to display the number of new messages for all accounts, rather than for a single account at a time.\n\
								\nThis option will also read all accounts at once, rather than a single account.")

/*filterdlg.c strings*/
#define S_FILTERDLG_NO_FILTERS _("You must enter at least one filter string")
#define S_FILTERDLG_COMBO_SENDER _("Sender")
#define S_FILTERDLG_COMBO_SUBJECT _("Subject")
#define S_FILTERDLG_COMBO_CONTAINS _("Contains")
#define S_FILTERDLG_COMBO_NOTCONTAINS _("Does not contain")
#define S_FILTERDLG_TITLE _("Mail filters")
#define S_FILTERDLG_BUTTON_MATCHALL _("Match all conditions")
#define S_FILTERDLG_BUTTON_MATCHANY _("Match any condition")
#define S_FILTERDLG_BUTTON_CLEAR _("Clear filters")
#define S_FILTERDLG_LABEL_SELECT _("Select up to %u filters:")

/***ERROR STRINGS***/

/*common.c strings*/
#define S_COMMON_ERR_ALLOC _("Error allocating memory\n")
#define S_COMMON_ERR_REALLOC _("Error reallocating memory\n")
#define S_COMMON_ERR_STR_INS _("str_ins error: invalid position!\n")

/*configdlg.c strings*/
#define S_CONFIGDLG_ERR_COMBO_ITER _("Error getting listbox iterator\n")
#define S_CONFIGDLG_ERR_REMOVE_FILE _("Error attempting to write to file %s\n")
#define S_CONFIGDLG_ERR_LISTBOX_ITER _("Error getting listbox iterator\n")
#define S_CONFIGDLG_ERR_CREATE_PIXBUF _("Error creating icon pixbuf\n")
#define S_CONFIGDLG_ERR_GET_ACCOUNT_INFO _("Error getting information for account %d\n")

/*docklet.c strings*/
#define S_DOCKLET_ERR_FORK_APP _("Error creating fork for mail application %s\n")
#define S_DOCKLET_ERR_EXEC_APP _("Error executing applicaiton %s\n")
#define S_DOCKLET_ERR_UNKNOWN_PROTOCOL _("Unknown mail protocol selected\n")
#define S_DOCKLET_ERR_CLICK_INVALID_PROTOCOL _("Error, invalid protocol while processing docklet click\n")
#define S_DOCKLET_ERR_CLOSE_FILE _("Error closing file %s\n")
#define S_DOCKLET_ERR_OPEN_FILE_WRITE _("Error opening %s for writing\n")
#define S_DOCKLET_ERR_OPEN_FILE_READ _("Error opening %s for reading\n")
#define S_DOCKLET_ERR_READ_FILE _("Error reading file %s\n")
#define S_DOCKLET_ERR_WRITE_FILE _("Error writing file %s\n")
#define S_DOCKLET_ERR_ACCESS_FILE _("Error accessing %s when marking read\n")
#define S_DOCKLET_ERR_RENAME_FILE _("Error renaming %s to %s\n")

/*encrypter.c strings*/
#define S_ENCRYPTER_ERR_ENC_PW _("Error encrypting password\n")
#define S_ENCRYPTER_ERR_ENC_PW_FINAL _("Error finalising password encryption\n")
#define S_ENCRYPTER_ERR_DEC_PW _("Error decrypting password\n")
#define S_ENCRYPTER_ERR_DEC_PW_FINAL _("Error finalising password decryption\n")
#define S_ENCRYPTER_ERR_APOP_DIGEST_INIT _("Error initialising message digest\n")
#define S_ENCRYPTER_ERR_APOP_DIGEST_CREATE _("Error creating MD5 digest\n")
#define S_ENCRYPTER_ERR_APOP_DIGEST_FINAL _("Error finalising digest creation\n")

/*filterdlg.c strings*/
#define S_FILTERDLG_ERR_REMOVE_FILE _("Error attempting to write to file %s\n")
#define S_FILTERDLG_ERR_OPEN_FILE _("Error opening file %s\n")
#define S_FILTERDLG_ERR_COMBO_ITER _("Error getting combobox iterator\n")
#define S_FILTERDLG_ERR_CLOSE_FILE _("Error closing file %s\n")

/*filefunc.c strings*/
#define S_FILEFUNC_ERR_OPEN_FILE _("Error opening file %s\n")
#define S_FILEFUNC_ERR_CLOSE_FILE _("Error closing file %s\n")
#define S_FILEFUNC_ERR_GET_PW _("Error retrieving password\n")
#define S_FILEFUNC_ERR_SET_PERM _("Error setting permissions on file %s\n")
#define S_FILEFUNC_ERR_REMOVE_FILE _("Error removing file %s\n")
#define S_FILEFUNC_ERR_RENAME_FILE _("Error renaming %s to %s\n")
#define S_FILEFUNC_ERR_WRITE_PW _("Error writing password to file\n")
#define S_FILEFUNC_ERR_GET_DELAY _("Error retrieving mail checking delay information\n")
#define S_FILEFUNC_ERR_GET_MAILAPP _("Error retrieving mail reading application information\n")
#define S_FILEFUNC_ERR_GET_HOSTNAME _("Error retrieving server hostname information for account %d\n")
#define S_FILEFUNC_ERR_GET_PORT _("Error retrieving server port information for account %d\n")
#define S_FILEFUNC_ERR_GET_USERNAME _("Error retrieving username for account %d\n")
#define S_FILEFUNC_ERR_GET_ICONTYPE _("Error retrieving icon type for account %d\n")
#define S_FILEFUNC_DEFAULT_ACCNAME _("Account %d")
#define S_FILEFUNC_ERR_GET_FILTER _("Error getting filter details %d\n")
#define S_FILEFUNC_ERR_GET_PASSWORD _("Error retrieving password for account %d\n")
#define S_FILEFUNC_ERR_WRITE_DELAY _("Error writing mail checking delay information to file\n")
#define S_FILEFUNC_ERR_WRITE_MAILAPP _("Error writing mail reading application to file\n")
#define S_FILEFUNC_ERR_WRITE_ICONSIZE _("Error writing icon size information to file\n")
#define S_FILEFUNC_ERR_WRITE_MULTIPLE _("Error writing read multiple information to file\n")
#define S_FILEFUNC_ERR_WRITE_M_ICON_COLOUR _("Error writing multiple icon colour information to file\n")
#define S_FILEFUNC_ERR_WRITE_HOSTNAME _("Error writing server hostname to file\n")
#define S_FILEFUNC_ERR_WRITE_PORT _("Error writing server port to file\n")
#define S_FILEFUNC_ERR_WRITE_USERNAME _("Error writing username to file\n")
#define S_FILEFUNC_ERR_WRITE_ICONTYPE _("Error writing icon type to file\n")
#define S_FILEFUNC_ERR_WRITE_PROTOCOL _("Error writing mail protocol to file\n")
#define S_FILEFUNC_ERR_WRITE_ACCNAME _("Error writing account name to file\n")
#define S_FILEFUNC_ERR_WRITE_FILTER _("Error writing filter status to file\n")
#define S_FILEFUNC_ERR_ATTEMPT_WRITE _("Error attempting to write to file %s\n")
#define S_FILEFUNCT_ERR_MOVE_FP _("Error moving file pointer for config file\n")
		
/*main.c strings*/
#define S_MAIN_ERR_DELAY_INFO _("Error getting delay information\n")
#define S_MAIN_ERR_INSTANCE_RUNNING _("Error: Instance of %s already running\n")
#define S_MAIN_ERR_ATEXIT_FUNC _("Error cannot set exit function: ")
#define S_MAIN_ERR_CLOSE_LOGFILE _("Error closing log file: ")
#define S_MAIN_ERR_OPEN_LOGFILE _("Error writing header to log file: ")
#define S_MAIN_ERR_APP_KILLED _("%s killed.\n")
#define S_MAIN_ERR_SEGFAULT _("%s encountered a segmentation fault.  Please report this bug\n")
#define S_MAIN_ERR_CLOSE_PIDFILE _("Error closing PID file\n")
#define S_MAIN_ERR_CANNOT_KILL _("Error: cannot kill process: %d\n")
#define S_MAIN_ERR_OPEN_PIDFILE_WRITE _("Error opening pid file %s for writing\n")
#define S_MAIN_ERR_OPEN_PIDFILE_READ _("Error reading PID file %s\n")
#define S_MAIN_ERR_RENAME_PIDFILE _("Error renaming pid file %s to temp pid file %s\n")
#define S_MAIN_LOG_STARTED _("%s started %s\n")
#define S_MAIN_ERR_PRINT_USAGE _("%s %s (C) 2006 Dale Whittaker\nUsage:\n\t%s (to run %s program)\
					\n\t%s -c (to configure mail details)\
					\n\t%s -d (run in network debug mode)\
					\n\t%s -k (kill all mailtc processes)\n")

/*imapfunc.c strings*/
#define S_IMAPFUNC_ERR_CONNECT _("Connection error. %s closing connection and retrying.\n")
#define S_IMAPFUNC_ERR_SEND_LOGIN _("Error sending LOGIN to imap server %s\n")
#define S_IMAPFUNC_ERR_SEND_LOGOUT _("Error sending LOGOUT command to imap server %s\n")
#define S_IMAPFUNC_ERR_SASL_INIT _("SASL initialisation failure (%d): %s\n")
#define S_IMAPFUNC_ERR_SEND_AUTHENTICATE _("Error sending AUTHENTICATE command to imap server %s\n")
#define S_IMAPFUNC_ERR_SEND_CRAM_MD5 _("Error sending CRAM-MD5 string to imap server %s\n")
#define S_IMAPFUNC_ERR_NOT_IMAP_SERVER _("Error: this does not appear to be a valid IMAP server\n")
#define S_IMAPFUNC_ERR_RECEIVE_CAPABILITY _("Error receiving IMAP capability string\n")
#define S_IMAPFUNC_ERR_GET_IMAP_CAPABILITIES _("Error getting IMAP server capabilities\n")
#define S_IMAPFUNC_ERR_CRAM_MD5_NOT_SUPPORTED _("Error: CRAM-MD5 authentication does not appear to be supported\n")
#define S_IMAPFUNC_ERR_OPEN_READ_FILE _("Error opening read file %s\n")
#define S_IMAPFUNC_ERR_GET_UID _("Error retreiving uid\n")
#define S_IMAPFUNC_ERR_SEND_UID_FETCH _("Error sending UID FETCH to imap server %s\n")
#define S_IMAPFUNC_ERR_SEND_STORE _("Error sending STORE to imap server %s\n")
#define S_IMAPFUNC_ERR_CLOSE_READ_FILE _("Error closing uid read file\n")
#define S_IMAPFUNC_ERR_OPEN_FILE _("Error opening file %s\n")
#define S_IMAPFUNC_ERR_OPEN_TEMP_FILE _("Error opening temp UID file for reading\n")
#define S_IMAPFUNC_ERR_CLOSE_TEMP_FILE _("Error closing temp UID file\n")
#define S_IMAPFUNC_ERR_CLOSE_FILE _("Error closing file %s\n")
#define S_IMAPFUNC_ERR_SEND_SELECT _("Error sending SELECT to imap server %s\n")
#define S_IMAPFUNC_ERR_REMOVE_READ_FILE _("Error removing uid read file\n")

/*netfunc.c strings*/
#define S_NETFUNC_ERR_IP _("Error getting ip from hostname %s\n")
#define S_NETFUNC_ERR_SOCKET _("Error getting socket\n")
#define S_NETFUNC_ERR_CONNECT _("Error connecting to server %s\n")
#define S_NETFUNC_ERR_SEND _("Error sending message to server\n")
#define S_NETFUNC_ERR_RECEIVE _("Error receiving message from server\n")
#define S_NETFUNC_ERR_DATA_AVAILABLE _("Error while checking if there is any data to receive from server\n")

/*popfunc.c strings*/
#define S_POPFUNC_ERR_CONNECT _("Connection error. %s closing connection and retrying.\n")
#define S_POPFUNC_ERR_BAD_MAIL_HEADER _("Warning, mail header does not conform to spec!\n")
#define S_POPFUNC_ERR_APOP_NOT_SUPPORTED _("Error: APOP does not appear to be supported\n")
#define S_POPFUNC_ERR_APOP_ENCRYPT_TIMESTAMP _("Error encrypting APOP timestamp string\n")
#define S_POPFUNC_ERR_APOP_SEND_DETAILS _("Error sending APOP details to pop server %s\n")
#define S_POPFUNC_ERR_APOP_NOT_COMPILED _("Error: APOP support not compiled in\n")
#define S_POPFUNC_ERR_SEND_USERNAME _("Error sending username to pop server %s\n")
#define S_POPFUNC_ERR_SEND_PASSWORD _("Error sending password to pop server %s\n")
#define S_POPFUNC_ERR_SASL_INIT _("SASL initialisation failure (%d): %s\n")
#define S_POPFUNC_ERR_SEND_CRAM_MD5_AUTH _("Error sending auth command to pop server %s\n")
#define S_POPFUNC_ERR_RECEIVE_NUM_MESSAGES _("Error retrieving number of messages on the pop server %s\n")
#define S_POPFUNC_ERR_GET_TOTAL_MESSAGES _("Error getting total number of messages\n")
#define S_POPFUNC_ERR_CLOSE_FILE _("Error closing file %s\n")
#define S_POPFUNC_ERR_OPEN_FILE _("Error opening file %s\n")
#define S_POPFUNC_ERR_GET_UIDL _("Error retrieving uidl value\n")
#define S_POPFUNC_ERR_RECEIVE_TOP _("Error getting TOP for message %d\n")
#define S_POPFUNC_ERR_RECEIVE_UIDL _("Error getting UIDL for message %d\n")
#define S_POPFUNC_ERR_INVALID_AUTH _("Invalid auth method specified\n")
#define S_POPFUNC_ERR_SEND_QUIT _("Error sending QUIT command to pop server %s\n")
#define S_POPFUNC_ERR_CRAM_MD5_NOT_SUPPORTED _("Error: CRAM-MD5 authentication does not appear to be supported\n")
#define S_POPFUNC_ERR_TEST_CAPABILITIES _("Error testing POP server capabilities, this does not appear to be a valid POP server\n")
	
/*sasl.c strings*/
#define S_SASL_ERR_INITIALISE _("Cannot initialise SASL client (%d): %s\n")
#define S_SASL_ERR_CRAM_MD5_AUTH _("CRAM-MD5 authentication error (%d): %s\n")

/*tls.c strings*/
#define S_TLS_ERR_CTX _("Error creating CTX\n")
#define S_TLS_ERR_CREATE_STRUCT _("Error creating SSL structure\n")
#define S_TLS_ERR_SSL_DESC _("Error setting SSL file descriptor\n")
#define S_TLS_ERR_SSL_CONNECT _("Error connecting with SSL\n")
#define S_TLS_ERR_SSL_SHUTDOWN _("Error shutting down SSL connection\n")

#endif /*DW_MAILTC_STRINGS_FILE*/
