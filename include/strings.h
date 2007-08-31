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

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif /*HAVE_LOCALE_H*/

/*define the gettext stuff*/
#ifdef ENABLE_NLS
#include <glib/gi18n.h>
#else
#define _(text) text
#define N_(text) text
#endif /*ENABLE_NLS*/

/***DIALOG STRINGS***/

/*docklet.c strings*/
#define S_DOCKLET_NEW_MESSAGE _("%s: %d new message%s")
#define S_DOCKLET_NEW_MESSAGES _("%s: %d new messages%s")
#define S_DOCKLET_CONNECT_ERR _("There was an error connecting to the following servers:\n\n%s\nPlease check the %s log (%s/%s) for the error.\n")
#define S_DOCKLET_CONNECT_ERR_MULTI _("%d errors connecting to the following servers:\n\n%s\nPlease check the %s log (%s/%s) for the error.\n")

/*main.c strings*/
#define S_MAIN_INSTANCE_RUNNING _("An instance of %s is already running.\n")
#define S_MAIN_NO_CONFIG_FOUND _("No mail configuration was found.\nPlease enter your mail details.\n")
#define S_MAIN_OLD_VERSION_FOUND _("You are running a new version of %s.\nThis version uses a new format for the icons.\n\
\nPlease set the icon colours for your mail accounts.\n")
#define S_MAIN_LOAD_PLUGINS _("Error loading network plugins, please reinstall.\nIf the problem persists please report this as a bug.")

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
#define S_CONFIGDLG_ICON_COLOUR _("Icon colour:")
#define S_CONFIGDLG_ICON_COLOUR_MULTIPLE _("Icon colour (multiple accounts):")
#define S_CONFIGDLG_SETICONCOLOUR _("Select icon colour...")
#define S_CONFIGDLG_DETAILS_TITLE _("Mail Account Configuration")
#define S_CONFIGDLG_CONFIG_TITLE _("%s configuration")
#define S_CONFIGDLG_TAB_GENERAL _("General")
#define S_CONFIGDLG_TAB_DISPLAY _("Display")
#define S_CONFIGDLG_TAB_ACCOUNTS _("Mail accounts")
#define S_CONFIGDLG_INTERVAL _("Interval in minutes for mail check: ")
#define S_CONFIGDLG_SMALLICON _("Use small envelope icon")
#define S_CONFIGDLG_MAILAPP _("Mail reading command:")
#define S_CONFIGDLG_LTAB_ACCOUNT _("Mail Account")
#define S_CONFIGDLG_LTAB_PROTOCOL _("Protocol")
#define S_CONFIGDLG_MULTI_TOOLTIP _("Enable this option to display the number of new messages for all accounts, rather than for a single account at a time.\n\
\nThis option will also read all accounts at once, rather than a single account.")
#define S_CONFIGDLG_MULTIPLE_ACCOUNTS _("Read/Display new messages for multiple accounts")
#define S_CONFIGDLG_DISPLAY_PLG_INFO _("%s plugin by %s.\n\n%s")
#define S_CONFIGDLG_PLG_INFO_BUTTON _("Plugin information...")
#define S_CONFIGDLG_FIND_PLUGIN_MSG _("Cannot find the plugin '%s' used for account '%s'.\n\nYou will need to select and resave a plugin from the list for each account.")
#define S_CONFIGDLG_ENABLE_SUMMARY  _("Display mail summary")
#define S_CONFIGDLG_SUMOPTS _("summary options...")
#define S_CONFIGDLG_SHOW_NET_ERRDLG _("Show network connection error dialog:")
#define S_CONFIGDLG_SHOW_NEVER _("Never")
#define S_CONFIGDLG_SHOW_ALWAYS _("Always")
#define S_CONFIGDLG_SHOW_EVERY _("Every")
#define S_CONFIGDLG_FAILED_CONNECTIONS _("failed connections")
#define S_CONFIGDLG_RUN_NEWMAIL_CMD _("Run command when new mail received:")

#define S_FILEFUNC_DEFAULT_ACC_STRING _("Account %d")

/*summarydlg.c strings*/
#define S_SUMMARYDLG_OPTS _("Summary options")
#define S_SUMMARYDLG_WPOS_TL _("Top left")
#define S_SUMMARYDLG_WPOS_TR _("Top right")
#define S_SUMMARYDLG_WPOS_BL _("Bottom left")
#define S_SUMMARYDLG_WPOS_BR _("Bottom right")
#define S_SUMMARYDLG_WPOS _("Summary dialog window position: ")
#define S_SUMMARYDLG_FONT _("Summary dialog font: ")
#define S_SUMMARYDLG_WIN_TITLE _("New mail")
#define S_SUMMARYDLG_DATE _("Date:\t")
#define S_SUMMARYDLG_FROM _("From:\t")
#define S_SUMMARYDLG_TO _("To:\t")
#define S_SUMMARYDLG_CC _("Cc:\t")
#define S_SUMMARYDLG_SUBJECT _("Subject:\t")
#define S_SUMMARYDLG_SUMMARY _("Summary: ")

/***ERROR STRINGS***/

/*configdlg.c strings*/
#define S_CONFIGDLG_ERR_COMBO_ITER _("Error getting listbox iterator\n")
#define S_CONFIGDLG_ERR_REMOVE_FILE _("Error attempting to write to file %s\n")
#define S_CONFIGDLG_ERR_LISTBOX_ITER _("Error getting listbox iterator\n")
#define S_CONFIGDLG_ERR_CREATE_PIXBUF _("Error creating icon pixbuf\n")
#define S_CONFIGDLG_ERR_GET_ACCOUNT_INFO _("Error getting information for account %d\n")
#define S_CONFIGDLG_ERR_GET_ACTIVE_PLUGIN _("Error: cannot get active plugin from combo box\n")
#define S_CONFIGDLG_ERR_WRITE_PLUGIN _("Error writing plugin information.\n")
#define S_CONFIGDLG_ERR_FIND_PLUGIN _("Plugin not found")

/*docklet.c strings*/
#define S_DOCKLET_ERR_FIND_PLUGIN_MSG _("Cannot find the plugin '%s' used for account '%s'.\n\n\
You will need to select and resave a plugin from the list for each account.")
#define S_DOCKLET_ERR_CONNECT _("%s: connection error (%d)\n")

/*encrypter.c strings*/
#define S_ENCRYPTER_ERR_ENC_PW _("Error encrypting password\n")
#define S_ENCRYPTER_ERR_ENC_PW_FINAL _("Error finalising password encryption\n")
#define S_ENCRYPTER_ERR_DEC_PW _("Error decrypting password\n")
#define S_ENCRYPTER_ERR_DEC_PW_FINAL _("Error finalising password decryption\n")

/*filefunc.c strings*/
#define S_FILEFUNC_ERR_OPEN_FILE _("Error opening file %s\n")
#define S_FILEFUNC_ERR_CLOSE_FILE _("Error closing file %s\n")
#define S_FILEFUNC_ERR_GET_PW _("Error retrieving password\n")
#define S_FILEFUNC_ERR_SET_PERM _("Error setting permissions on file %s\n")
#define S_FILEFUNC_ERR_WRITE_PW _("Error writing password to file\n")
#define S_FILEFUNC_ERR_GET_DELAY _("Error retrieving mail checking delay information\n")
#define S_FILEFUNC_ERR_GET_MAILAPP _("Error retrieving mail reading application information\n")
#define S_FILEFUNC_ERR_GET_HOSTNAME _("Error retrieving server hostname information for account\n")
#define S_FILEFUNC_ERR_GET_PORT _("Error retrieving server port information for account\n")
#define S_FILEFUNC_ERR_GET_USERNAME _("Error retrieving username for account\n")
#define S_FILEFUNC_ERR_GET_ICONTYPE _("Error retrieving icon type for account\n")
#define S_FILEFUNC_DEFAULT_ACCNAME _("Account")
#define S_FILEFUNC_ERR_GET_PASSWORD _("Error retrieving password for account\n")
#define S_FILEFUNC_ERR_WRITE_DELAY _("Error writing mail checking delay information to file\n")
#define S_FILEFUNC_ERR_WRITE_MAILAPP _("Error writing mail reading application to file\n")
#define S_FILEFUNC_ERR_WRITE_ICONSIZE _("Error writing icon size information to file\n")
#define S_FILEFUNC_ERR_WRITE_MULTIPLE _("Error writing read multiple information to file\n")
#define S_FILEFUNC_ERR_WRITE_SUMMARY _("Error writing summary status to file\n")
#define S_FILEFUNC_ERR_WRITE_SUMMARY_WPOS _("Error writing summary window position to file\n")
#define S_FILEFUNC_ERR_WRITE_SUMMARY_SFONT _("Error writing summary font to file\n")
#define S_FILEFUNC_ERR_WRITE_M_ICON_COLOUR _("Error writing multiple icon colour information to file\n")
#define S_FILEFUNC_ERR_WRITE_FREQ _("Error writing connection error frequency\n")
#define S_FILEFUNC_ERR_WRITE_NMCMD _("Error writing new mail command\n")
#define S_FILEFUNC_ERR_WRITE_HOSTNAME _("Error writing server hostname to file\n")
#define S_FILEFUNC_ERR_WRITE_PORT _("Error writing server port to file\n")
#define S_FILEFUNC_ERR_WRITE_USERNAME _("Error writing username to file\n")
#define S_FILEFUNC_ERR_WRITE_ICONTYPE _("Error writing icon type to file\n")
#define S_FILEFUNC_ERR_WRITE_PROTOCOL _("Error writing mail protocol to file\n")
#define S_FILEFUNC_ERR_WRITE_ACCNAME _("Error writing account name to file\n")
#define S_FILEFUNC_ERR_ATTEMPT_WRITE _("Error attempting to write to file %s\n")
#define S_FILEFUNC_ERR_MOVE_FP _("Error moving file pointer for config file\n")
#define S_FILEFUNC_ERR_READ_PLUGIN	_("Error reading plugin for account %d\n")
#define S_FILEFUNC_ERR_GET_HOMEDIR _("Error getting home directory\n")
#define S_FILEFUNC_ERR_CREATE_PIXBUF _("Error creating icon pixbuf\n")
#define S_FILEFUNC_ERR_NOT_DIRECTORY _("%s exists, but is not a directory, you must remove this before %s can run\n")
#define S_FILEFUNC_ERR_MKDIR _("Error creating directory\n")
#define S_FILEFUNC_ERR_CFG_WRITE _("Error writing config file: %s\n")
#define S_FILEFUNC_ERR_PARSER_CTX _("Failed to allocate parser context\n")
#define S_FILEFUNC_ERR_CFG_PARSE _("Error parsing config file %s: %s\n\nYou will need to either fix this or re-enter your configuration.")
#define S_FILEFUNC_ERR_CFG_VALIDATE _("Failed to validate %s.")
#define S_FILEFUNC_ERR_NO_ROOT_ELEMENT _("Error getting config root element.\nPlease re-enter your configuration.")
#define S_FILEFUNC_ERR_ACC_ELEMENT_NOT_FOUND _("Error: '%s' element not found for account %d.\n")
#define S_FILEFUNC_ERR_ELEMENT_DUPLICATE _("Error: duplicate element '%s'\n")
#define S_FILEFUNC_ERR_PASSWORD_ELEMENTS _("Error: encrypted and unencrypted password elements found.\n")

/*main.c strings*/
#define S_MAIN_ERR_INSTANCE_RUNNING _("Error: Instance of %s already running\n")
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
#define S_MAIN_ERR_NO_ACCOUNTS _("No accounts found.\nPlease enter a mail account")

#define S_MAIN_ERR_PRINT_USAGE _("%s %s (C) 2006 Dale Whittaker\nUsage:\n\t%s (to run %s)\
\n\t%s -c (to configure mail details)\
\n\t%s -d (run in network debug mode)\
\n\t%s -k (kill all mailtc processes)\n")

#define S_MAIN_ERR_PRINT_USAGE_NOKILL _("%s %s (C) 2006 Dale Whittaker\nUsage:\n\t%s (to run %s)\
\n\t%s -c (to configure mail details)\
\n\t%s -d (run in network debug mode)\n")

#define S_MAIN_ERR_LOAD_PLUGINS ("Error loading network plugins\n")

/*plugin.c strings*/
#define S_PLUGIN_ERR_CLOSE_PLUGIN _("Error closing plugin %s: %s\n")
#define S_PLUGIN_ERR_MODULE_SUPPORT _("Error: module loading not supported on this system\n")
#define S_PLUGIN_ERR_OPEN_PLUGIN _("Error opening plugin %s: %s\n")
#define S_PLUGIN_ERR_PLUGIN_POINTER _("Error getting plugin pointer: %s\n")
#define S_PLUGIN_ERR_INIT_PLUGIN _("Error initialising plugin %s: %s\n")
#define S_PLUGIN_ERR_COMPATIBILITY _("Error: plugin %s is not compatible with %s %s\n")
#define S_PLUGIN_ERR_FIND_PLUGIN _("Error: cannot find plugin %s\n")



#endif /*DW_MAILTC_STRINGS_FILE*/
