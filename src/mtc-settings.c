/* mtc-settings.c
 * Copyright (C) 2009-2012 Dale Whittaker <dayul@users.sf.net>
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

#include "mtc-settings.h"
#include "mtc-module.h"
#include "mtc-util.h"
#include "mtc.h"

#include <glib/gstdio.h>
#include <gdk/gdk.h>

#define MAILTC_SETTINGS_GROUP_GLOBAL         "settings"
#define MAILTC_SETTINGS_PROPERTY_ACCOUNTS    "accounts"
#define MAILTC_SETTINGS_PROPERTY_COMMAND     "command"
#define MAILTC_SETTINGS_PROPERTY_FILENAME    "filename"
#define MAILTC_SETTINGS_PROPERTY_ICON_COLOUR "iconcolour"
#define MAILTC_SETTINGS_PROPERTY_INTERVAL    "interval"
#define MAILTC_SETTINGS_PROPERTY_MODULES     "modules"
#define MAILTC_SETTINGS_PROPERTY_NET_ERROR   "neterror"

struct _MailtcSettingsPrivate
{
    GKeyFile* key_file;
    GError* error;
};

struct _MailtcSettings
{
    GObject parent_instance;

    MailtcSettingsPrivate* priv;
    GPtrArray* accounts;
    GPtrArray* modules;
    GdkColor iconcolour;
    guint interval;
    guint neterror;
    gchar* command;
    gchar* filename;
};

struct _MailtcSettingsClass
{
    GObjectClass parent_class;
};

enum
{
    PROP_0,
    PROP_ACCOUNTS,
    PROP_FILENAME,
    PROP_COMMAND,
    PROP_INTERVAL,
    PROP_MODULES,
    PROP_NET_ERROR,
    PROP_ICON_COLOUR
};

static gboolean
mailtc_settings_initable_init (GInitable*    initable,
                               GCancellable* cancellable,
                               GError**      error)
{
    MailtcSettings* settings;
    MailtcSettingsPrivate* priv;

    (void) cancellable;

    g_assert (MAILTC_IS_SETTINGS (initable));

    settings = MAILTC_SETTINGS (initable);
    priv = settings->priv;

    if (priv->error)
    {
        if (error)
            *error = g_error_copy (priv->error);

        return FALSE;
    }
    return TRUE;
}

static void
mailtc_settings_initable_iface_init (GInitableIface* iface)
{
    iface->init = mailtc_settings_initable_init;
}

G_DEFINE_TYPE_WITH_CODE (MailtcSettings, mailtc_settings, G_TYPE_OBJECT,
        G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, mailtc_settings_initable_iface_init))

static void
mailtc_settings_string_to_colour (const gchar* str,
                                  GdkColor*    colour)
{
    guint64 cvalue;

    g_assert (colour);

    cvalue = g_ascii_strtoull (str, NULL, 16);
    colour->red = (guint16) ((cvalue >> 32) & 0xFFFF);
    colour->green = (guint16) ((cvalue >> 16) & 0xFFFF);
    colour->blue = (guint16) (cvalue & 0xFFFF);
}

static gboolean
mailtc_settings_keyfile_read_uint (MailtcSettings* settings,
                                   const gchar*    name,
                                   GError**        error)
{
    GKeyFile* key_file;
    guint value;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    value = (guint) g_key_file_get_integer (key_file, MAILTC_SETTINGS_GROUP_GLOBAL, name, error);
    if (error && *error)
        return FALSE;

    g_object_set (G_OBJECT (settings), name, value, NULL);

    return TRUE;
}

static gboolean
mailtc_settings_keyfile_read_string (MailtcSettings* settings,
                                     const gchar*    name,
                                     GError**        error)
{
    GKeyFile* key_file;
    gchar* value;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    value = g_key_file_get_string (key_file, MAILTC_SETTINGS_GROUP_GLOBAL, name, error);
    if (error && *error)
        return FALSE;

    g_object_set (G_OBJECT (settings), name, value, NULL);
    g_free (value);

    return TRUE;
}

static gboolean
mailtc_settings_keyfile_read_colour (MailtcSettings* settings,
                                     const gchar*    name,
                                     GError**        error)
{
    GKeyFile* key_file;
    gchar* value;
    GdkColor colour;

    g_assert (MAILTC_IS_SETTINGS (settings));

    key_file = settings->priv->key_file;

    value = g_key_file_get_string (key_file, MAILTC_SETTINGS_GROUP_GLOBAL, name, error);
    if (error && *error)
        return FALSE;

    mailtc_settings_string_to_colour (value, &colour);
    g_free (value);

    g_object_set (G_OBJECT (settings), name, &colour, NULL);

    return TRUE;
}

static gboolean
mailtc_settings_keyfile_read_accounts (MailtcSettings* settings,
                                       GError**        error)
{
    GKeyFile* key_file;
    gchar** account_groups;
    gsize naccounts;

    g_assert (MAILTC_IS_SETTINGS (settings));
    g_assert (settings->modules);

    key_file = settings->priv->key_file;

    account_groups = g_key_file_get_string_list (key_file, MAILTC_SETTINGS_GROUP_GLOBAL,
                                                 MAILTC_SETTINGS_PROPERTY_ACCOUNTS, &naccounts, error);
    if (!account_groups)
        return FALSE;

    if (naccounts > 0)
    {
        GPtrArray* accounts;
        GPtrArray* modules;
        GdkColor colour;
        gchar* str;
        const gchar* key_group;
        const gchar* module_name;
        gsize i;
        gsize j;
        gsize len;
        gboolean success;
        mtc_account* account;
        mtc_plugin* plugin = NULL;

        accounts = g_ptr_array_new ();
        modules = settings->modules;

        for (i = 0; i < naccounts; i++)
        {
            success = FALSE;

            key_group = account_groups[i];
            if (!g_key_file_has_group (key_file, key_group))
                continue;

            account = g_new0 (mtc_account, 1);

            account->name = g_key_file_get_string (key_file, key_group, "name", error);
            if (!*error)
                account->server = g_key_file_get_string (key_file, key_group, "server", error);
            if (!*error)
                account->port = g_key_file_get_integer (key_file, key_group, "port", error);
            if (!*error)
                account->user = g_key_file_get_string (key_file, key_group, "user", error);
            if (!*error)
                account->protocol = g_key_file_get_integer (key_file, key_group, "protocol", error);
            if (!*error)
            {
                str = g_key_file_get_string (key_file, key_group, "plugin", error);

                for (j = 0; j < modules->len; j++)
                {
                    plugin = g_ptr_array_index (modules, j);

                    module_name = mailtc_module_get_name (plugin->module);
                    if (g_str_equal (str, module_name))
                    {
                        account->plugin = plugin;
                        break;
                    }
                }
                g_free (str);
            }
            if (!*error)
            {
                str = g_key_file_get_string (key_file, key_group, "iconcolour", error);
                mailtc_settings_string_to_colour (str, &colour);
                account->icon_colour = gdk_color_copy (&colour);
                g_free (str);
            }
            if (!*error)
            {
                str = g_key_file_get_string (key_file, key_group, "password", error);
                account->password = (gchar*) g_base64_decode (str, &len);
                g_free (str);

                if (*error)
                {
                    g_free (account->password);
                    account->password = NULL;
                }
            }
            if (!*error)
            {
                g_assert (account->name &&
                          account->server &&
                          account->port &&
                          account->user &&
                          account->password &&
                          account->plugin &&
                          account->icon_colour);

                if (plugin->add_account)
                    success = (*plugin->add_account) (account, error);
                else
                    success = TRUE;
            }
            if (success)
                g_ptr_array_add (accounts, account);
            else
                mailtc_free_account (account, error);
        }
        if (accounts->len > 0)
            g_object_set (G_OBJECT (settings), MAILTC_SETTINGS_PROPERTY_ACCOUNTS, accounts, NULL);

        g_ptr_array_unref (accounts);
    }
    g_strfreev (account_groups);

    return TRUE;
}

static gboolean
mailtc_settings_keyfile_read (MailtcSettings* settings,
                              GError**        error)
{

    if (!mailtc_settings_keyfile_read_uint (settings, MAILTC_SETTINGS_PROPERTY_INTERVAL, error))
        return FALSE;
    if (!mailtc_settings_keyfile_read_uint (settings, MAILTC_SETTINGS_PROPERTY_NET_ERROR, error))
        return FALSE;
    if (!mailtc_settings_keyfile_read_string (settings, MAILTC_SETTINGS_PROPERTY_COMMAND, error))
        return FALSE;
    if (!mailtc_settings_keyfile_read_colour (settings, MAILTC_SETTINGS_PROPERTY_ICON_COLOUR, error))
        return FALSE;

    return mailtc_settings_keyfile_read_accounts (settings, error);
}

static void
mailtc_settings_set_colour (MailtcSettings* settings,
                            const GdkColor* colour)
{
    GdkColor* iconcolour;

    g_assert (MAILTC_IS_SETTINGS (settings));

    iconcolour = &settings->iconcolour;

    if (colour)
    {
        iconcolour->red = colour->red;
        iconcolour->green = colour->green;
        iconcolour->blue = colour->blue;
    }
    else
        iconcolour->red = iconcolour->green = iconcolour->blue = 0xFFFF;

    /* FIXME notify signal? */
}

static void
mailtc_settings_get_colour (MailtcSettings* settings,
                            GdkColor*       colour)
{
    GdkColor* iconcolour;

    g_assert (MAILTC_IS_SETTINGS (settings));

    iconcolour = &settings->iconcolour;

    colour->red = iconcolour->red;
    colour->green = iconcolour->green;
    colour->blue = iconcolour->blue;
}

static void
mailtc_settings_set_property (GObject*      object,
                              guint         prop_id,
                              const GValue* value,
                              GParamSpec*   pspec)
{
    MailtcSettings* settings = MAILTC_SETTINGS (object);

    /* FIXME we probably need to free these before assigning,
     * otherwise there will be a memory leak...
     */
    /* FIXME can also check if they are equal, if they are
     * no point in freeing...
     *
     */
    switch (prop_id)
    {
        case PROP_COMMAND:
            settings->command = g_value_dup_string (value);
            break;

        case PROP_FILENAME:
            settings->filename = g_value_dup_string (value);
            break;

        case PROP_INTERVAL:
            settings->interval = g_value_get_uint (value);
            break;

        case PROP_NET_ERROR:
            settings->neterror = g_value_get_uint (value);
            break;

        case PROP_ICON_COLOUR:
            mailtc_settings_set_colour (settings, g_value_get_boxed (value));
            break;

        case PROP_ACCOUNTS:
            settings->accounts = g_value_dup_boxed (value);
            break;

        case PROP_MODULES:
            settings->modules = g_value_dup_boxed (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_settings_get_property (GObject*    object,
                              guint       prop_id,
                              GValue*     value,
                              GParamSpec* pspec)
{
    MailtcSettings* settings;
    GdkColor colour;

    settings = MAILTC_SETTINGS (object);

    switch (prop_id)
    {
        case PROP_FILENAME:
            g_value_set_string (value, settings->filename);
            break;

        case PROP_COMMAND:
            g_value_set_string (value, settings->command);
            break;

        case PROP_INTERVAL:
            g_value_set_uint (value, settings->interval);
            break;

        case PROP_NET_ERROR:
            g_value_set_uint (value, settings->neterror);
            break;

        case PROP_ICON_COLOUR:
            mailtc_settings_get_colour (settings, &colour);
            g_value_set_boxed (value, &colour);
            break;

        case PROP_ACCOUNTS:
            g_value_set_boxed (value, settings->accounts);
            break;

        case PROP_MODULES:
            g_value_set_boxed (value, settings->modules);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_settings_finalize (GObject* object)
{
    MailtcSettings* settings;
    MailtcSettingsPrivate* priv;

    settings = MAILTC_SETTINGS (object);
    priv = settings->priv;

    g_ptr_array_unref (settings->modules);
    settings->modules = NULL;
    /* FIXME should be something like mailtc_application_free_accounts */
    g_ptr_array_unref (settings->accounts);
    settings->accounts = NULL;
    g_free (settings->command);
    settings->command = NULL;
    g_free (settings->filename);
    settings->filename = NULL;

    g_clear_error (&priv->error);
    priv->error = NULL;

    if (priv->key_file)
    {
        g_key_file_free (priv->key_file);
        priv->key_file = NULL;

    }

    G_OBJECT_CLASS (mailtc_settings_parent_class)->finalize (object);
}

static void
mailtc_settings_constructed (GObject* object)
{
    MailtcSettings* settings;
    MailtcSettingsPrivate* priv;

    settings = MAILTC_SETTINGS (object);
    priv = settings->priv;

    priv->key_file = g_key_file_new ();

    if (g_key_file_load_from_file (priv->key_file, settings->filename, G_KEY_FILE_NONE, &priv->error))
        mailtc_settings_keyfile_read (settings, &priv->error);

    G_OBJECT_CLASS (mailtc_settings_parent_class)->constructed (object);
}

static void
mailtc_settings_class_init (MailtcSettingsClass* class)
{
    GObjectClass* gobject_class;
    GParamFlags flags;

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = mailtc_settings_finalize;
    gobject_class->constructed = mailtc_settings_constructed;
    gobject_class->set_property = mailtc_settings_set_property;
    gobject_class->get_property = mailtc_settings_get_property;

    flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT;

    g_object_class_install_property (gobject_class,
                                     PROP_FILENAME,
                                     g_param_spec_string (
                                     MAILTC_SETTINGS_PROPERTY_FILENAME,
                                     "Filename",
                                     "The settings filename",
                                     NULL,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_INTERVAL,
                                     g_param_spec_uint (
                                     MAILTC_SETTINGS_PROPERTY_INTERVAL,
                                     "Interval",
                                     "The mail checking interval in minutes",
                                     1,
                                     60,
                                     3,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_NET_ERROR,
                                     g_param_spec_uint (
                                     MAILTC_SETTINGS_PROPERTY_NET_ERROR,
                                     "Neterror",
                                     "The number of network errors before reporting",
                                     0,
                                     5,
                                     1,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_ICON_COLOUR,
                                     g_param_spec_boxed (
                                     MAILTC_SETTINGS_PROPERTY_ICON_COLOUR,
                                     "Iconcolour",
                                     "The icon colour",
                                     GDK_TYPE_COLOR,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_COMMAND,
                                     g_param_spec_string (
                                     MAILTC_SETTINGS_PROPERTY_COMMAND,
                                     "Command",
                                     "The mail command to execute",
                                     NULL,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_ACCOUNTS,
                                     g_param_spec_boxed (
                                     MAILTC_SETTINGS_PROPERTY_ACCOUNTS,
                                     "Accounts",
                                     "The mail accounts",
                                     G_TYPE_PTR_ARRAY,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_MODULES,
                                     g_param_spec_boxed (
                                     MAILTC_SETTINGS_PROPERTY_MODULES,
                                     "Modules",
                                     "The mail plugin modules",
                                     G_TYPE_PTR_ARRAY,
                                     flags));

    g_type_class_add_private (class, sizeof (MailtcSettingsPrivate));
}

static void
mailtc_settings_init (MailtcSettings* settings)
{
    MailtcSettingsPrivate* priv;

    priv = settings->priv = G_TYPE_INSTANCE_GET_PRIVATE (settings,
                        MAILTC_TYPE_SETTINGS, MailtcSettingsPrivate);

    priv->key_file = NULL;
    priv->error = NULL;
}

MailtcSettings*
mailtc_settings_new (gchar*     filename,
                     GPtrArray* modules,
                     GError**   error)
{
    return g_initable_new (MAILTC_TYPE_SETTINGS, NULL, error,
                           MAILTC_SETTINGS_PROPERTY_FILENAME, filename,
                           MAILTC_SETTINGS_PROPERTY_MODULES, modules,
                           NULL);
}

