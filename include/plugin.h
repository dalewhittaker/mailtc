#ifndef MTC_PLUGIN_HEADER
#define MTC_PLUGIN_HEADER

/*headers required for the various structs
 *would be nicer if the plugin file did not require these,
 *but, ho hum*/
#include <libxml/tree.h>
#include <glib/gprintf.h> /*also includes stdio.h*/
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtkmain.h>
#include <gtk/gtktooltips.h>

/*test GtkStatusIcon stuff by undeffing here*/
/*#undef MTC_EGGTRAYICON*/
#ifdef _POSIX_SOURCE
#include <limits.h> /*NAME_MAX etc*/
#endif /*_POSIX_SOURCE*/

#ifdef MTC_EGGTRAYICON
#include "eggtrayicon.h"
#else
#include <gtk/gtkstatusicon.h>
#endif /*MTC_EGGTRAYICON*/

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#ifndef LOGIN_NAME_MAX
#define LOGIN_NAME_MAX 256
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#define PASSWORD_LEN 32
#define PROTOCOL_LEN NAME_MAX
#define ICON_LEN 10

#define MAX_FILTER_EXP 5 
#define FILTERSTRING_LEN 100

/*As i don't know the maximum length a font name can be
 *it is currently set to maximum filename length*/
#define MAX_FONTNAME_LEN NAME_MAX

/*define some file routines*/
/*these routines are for compatibility with old versions of glib/GTK*/
/*2.6*/
#if(!(GLIB_CHECK_VERSION(2, 6, 0)))
#define g_fopen(f, m): (fopen((f), (m)))
#define g_mkdir(f, m): (mkdir((f), (m)))
#define g_remove(f): (remove((f)))
#define g_rename(f, n): (rename((f), (n)))
#endif

/*2.8*/
#if(!(GLIB_CHECK_VERSION(2, 8, 0)))
#define g_chmod(f, m) (chmod((f), (m)))
#endif

/*enable experimental/broken code. NOTE do not enable this unless you want things to be broken*/
#undef MTC_EXPERIMENTAL

#ifdef MTC_EXPERIMENTAL
/*the various positions the summary window can be*/
typedef enum _mtc_wpos
{
    WPOS_TOPLEFT= 0,
    WPOS_TOPRIGHT,
    WPOS_BOTTOMLEFT,
    WPOS_BOTTOMRIGHT
} mtc_wpos;

/*structure to hold all the summary configuration details*/
typedef struct _mtc_summary
{
    gchar sfont[MAX_FONTNAME_LEN]; 
    mtc_wpos wpos; /*the window position*/
    /*... and probably more stuff as we go along*/

} mtc_summary;
#endif

/*structure used for the icon*/
typedef struct _mtc_icon
{
    gchar colour[ICON_LEN];
    GtkWidget *image;

#ifndef MTC_EGGTRAYICON
    GdkPixbuf *pixbuf;
#endif /*MTC_EGGTRAYICON*/

} mtc_icon;

/*structure containing all the trayicon widgets*/
typedef struct _mtc_trayicon
{

#ifdef MTC_EGGTRAYICON
    EggTrayIcon *docklet;
    GtkWidget *box;
    GtkTooltips *tooltip;
#else
    GtkStatusIcon *docklet;
    gint active;
#endif /*MTC_EGGTRAYICON*/

} mtc_trayicon;

/*structure to hold mailtc configuration details*/
typedef struct _mtc_cfg
{
	guint interval;
	gchar read_command[NAME_MAX+ 1];
	guint icon_size;
	gboolean multiple;
	gboolean net_debug;
    mtc_icon icon;

#ifdef MTC_EXPERIMENTAL
	gboolean run_summary;
    mtc_summary summary;
#endif

    /*frequency that connection errors are reported*/
    gint err_freq;
	
#ifdef MTC_NOTMINIMAL
    /*command to run when new mail arrives*/
    gchar nmailcmd[NAME_MAX+ 1];
#endif /*MTC_NOTMINIMAL*/

	/*some additional stuff used*/
	gchar dir[NAME_MAX+ 1];
	FILE *logfile;

    /*the docklet used to add the icon to the sys tray*/ 
    mtc_trayicon trayicon;

    /*flag used to test whether config dialog is running*/
    gboolean isdlg;

} mtc_cfg;

/*the defines here are used for the flags param of the 'load' function*/
typedef enum _mtc_flags
{ 
    MTC_NORMAL= 0,
    MTC_DEBUG= 1
} mtc_flags;

/*the defines here are used for the flags passed to the core*/
#define MTC_ENABLE_FILTERS 1 /*no longer used, is left here as reminder that 1 cannot be used*/
#define MTC_HAS_PLUGIN_OPTS 2

/*define the possible plugin errors that can be returned*/
/*these two must be negative as the return value is also the num messages*/
typedef enum _mtc_error
{ 
    MTC_ERR_CONNECT= -2,
    MTC_ERR_EXIT= -1,
    MTC_RETURN_FALSE= 0,
    MTC_RETURN_TRUE= 1

} mtc_error;

typedef enum _hfield
{ 
    HEADER_FROM= 0,
    HEADER_SUBJECT,
    HEADER_TO,
    HEADER_CC,
    HEADER_DATE,
    N_HFIELDS

} hfield;

typedef enum _msg_flags
{ 
    MSG_OLD= 0,
    MSG_NEW= 1,
    MSG_ADDED= 2,
    MSG_FILTERED= 4

} msg_flags;

/*the message header struct*/
typedef struct _msg_header
{
    guint bytes; /*no of bytes data will contain*/
    gchar *pdate;
    gchar *pfrom;
    gchar *pto;
    gchar *pcc;
    gchar *psubject;

} msg_header;

/*structure used for finding header fields*/
typedef struct _header_pos
{
    guint offset;
    guint len;

} header_pos;

/*holds information about a single unread mail*/
typedef struct _msg_struct
{
    gchar *uid; /*The unique id string of the message*/
    msg_flags flags; /*flag to say whether message is new, */
    msg_header *header; /*this will contain all the header stuff of the message*/

} msg_struct;

/*holds all the message information*/
typedef struct _msg_info
{
	gint num_messages; /*the number of messages not marked as read by mailtc*/
    gint new_messages; /*the number of new messages since last check*/
    GSList *msglist; /*a list of each individual unread mail*/
   
} msg_info;

/*structure to hold mail account details*/
typedef struct _mtc_account
{
	gchar name[NAME_MAX+ 1];
	gchar server[LOGIN_NAME_MAX+ HOST_NAME_MAX+ 1];
	gint port;
	gchar username[LOGIN_NAME_MAX+ HOST_NAME_MAX+ 1];
	gchar password[PASSWORD_LEN];
	gchar plgname[PROTOCOL_LEN+ 1];
    mtc_icon icon;
	
	/*the linked list id*/
	guint id;

#ifdef MTC_NOTMINIMAL
    /*list to contain the mail summaries*/
    msg_info msginfo;
#else
    gint num_messages;
#endif /*MTC_NOTMINIMAL*/

    /*used for connection error counting*/
    gint cerr;

    /*holds the plugin specific options*/
    gpointer plg_opts;

} mtc_account;

typedef struct _mtc_plugin
{
	/*handle to the module pointer*/
	gpointer handle;

	/*stuff passed to the core*/
	const gchar *compatibility; /*revision, so we know if it can be used with current mailtc*/
	gchar *name; /*the name of the plugin (used for the protocol)*/
	gchar *author; /*function to get the author*/
	gchar *desc; /*description for the config dialog etc*/
	guint flags; /*used to pass any additional information to the core*/
	gint default_port; /*default port, set it to -1 if you don't want to set one*/

	/*stuff launched by the core*/
	mtc_error (*load)(gpointer);
	mtc_error (*unload)(void);
	mtc_error (*get_messages)(gpointer);
	mtc_error (*clicked)(gpointer);
	mtc_error (*removed)(gpointer, guint *);
    gpointer  (*get_config)(gpointer);
    mtc_error (*put_config)(gpointer);
    mtc_error (*read_config)(xmlDocPtr, xmlNodePtr, gpointer);
    mtc_error (*write_config)(xmlNodePtr, gpointer);
    mtc_error (*freed)(gpointer);

} mtc_plugin;

#endif /*MTC_PLUGIN_HEADER*/
