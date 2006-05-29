#ifndef MTC_PLUGIN_HEADER
#define MTC_PLUGIN_HEADER

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#ifndef LOGIN_NAME_MAX
#define LOGIN_NAME_MAX 256
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#define PORT_LEN 20
#define PASSWORD_LEN 32
#define PROTOCOL_LEN 20
#define ICON_LEN 10

#define MAX_FILTER_EXP 5 
#define FILTERSTRING_LEN 100

/*the defines here are used for the flags param of the 'load' function*/
#define MTC_DEBUG_MODE 1 

/*the defines here are used for the flags passed to the core*/
#define MTC_ENABLE_FILTERS 1

/*structure used to hold the filter details if used*/
typedef struct _filter_details
{
	int matchall;
	int contains[MAX_FILTER_EXP];
	int subject[MAX_FILTER_EXP];
	char search_string[MAX_FILTER_EXP][FILTERSTRING_LEN+ 1];
	
} filter_details;

/*structure to hold mail account details*/
typedef struct _mail_details
{
	char accname[NAME_MAX+ 1];
	char hostname[LOGIN_NAME_MAX+ HOST_NAME_MAX+ 1];
	char port[PORT_LEN];
	char username[LOGIN_NAME_MAX+ HOST_NAME_MAX+ 1];
	char password[PASSWORD_LEN];
	char protocol[PROTOCOL_LEN];
	char icon[ICON_LEN];
	unsigned int runfilter;
	filter_details *pfilters;
	
	/*the linked list id and pointer to next struct*/
	unsigned int id;
	struct _mail_details *next;

	/*added for use with docklet and tooltip*/
	int num_messages;
	int active;
	
} mail_details;

typedef struct _mtc_plugin_info
{
	/*stuff passed to the core*/
	unsigned int revision; /*revision, so we know if it can be used with current mailtc*/
	const char *name; /*the name of the plugin (used for the protocol)*/
	const char *author; /*function to get the author*/
	const char *desc; /*description for the config dialog etc*/
	unsigned int flags; /*used to pass any additional information to the core*/

	/*stuff launched by the core*/
	int (*load)(mail_details *paccount, const char *cfgdir, unsigned int flags, FILE *plog);
	int (*unload)(mail_details *paccount);
	int (*clicked)(mail_details *paccount);
	
} mtc_plugin_info;

#endif /*MTC_PLUGIN_HEADER*/
