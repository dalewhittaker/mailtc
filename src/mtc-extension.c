/* mtc-extension.c
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
#include "mtc-extension.h"
#include "mtc-account.h"
#include "mtc-marshal.h"
#include "mtc-module.h"
#include "mtc-util.h"

#include <config.h>

#define MAILTC_EXTENSION_ERROR g_quark_from_string ("MAILTC_EXTENSION_ERROR")

#define MAILTC_EXTENSION_SET_STRING(extension,property) \
    mailtc_object_set_string (G_OBJECT (extension), MAILTC_TYPE_EXTENSION, \
                              #property, &extension->property, property)

#define MAILTC_EXTENSION_SET_OBJECT(extension,property) \
    mailtc_object_set_object (G_OBJECT (extension), MAILTC_TYPE_EXTENSION, \
                              #property, (GObject **) (&extension->property), G_OBJECT (property))

#define MAILTC_EXTENSION_SET_ARRAY(extension,property) \
    mailtc_object_set_array (G_OBJECT (extension), MAILTC_TYPE_EXTENSION, \
                              #property, &extension->property, property)

typedef enum
{
    MAILTC_EXTENSION_ERROR_TYPE = 0,
    MAILTC_EXTENSION_ERROR_COMPATIBILITY,
    MAILTC_EXTENSION_ERROR_INCOMPLETE
} MailtcExtensionError;

enum
{
    PROP_0,
    PROP_COMPATIBILITY,
    PROP_NAME,
    PROP_AUTHOR,
    PROP_DESCRIPTION,
    PROP_DIRECTORY,
    PROP_MODULE,
    PROP_PROTOCOLS
};

enum
{
    SIGNAL_ADD_ACCOUNT = 0,
    SIGNAL_REMOVE_ACCOUNT,
    SIGNAL_GET_MESSAGES,
    SIGNAL_READ_MESSAGES,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST];

G_DEFINE_TYPE (MailtcExtension, mailtc_extension, G_TYPE_OBJECT)

static void
mailtc_extension_set_compatibility (MailtcExtension* extension,
                                    const gchar*     compatibility)
{
    MAILTC_EXTENSION_SET_STRING (extension, compatibility);
}

const gchar*
mailtc_extension_get_compatibility (MailtcExtension* extension)
{
    g_assert (MAILTC_IS_EXTENSION (extension));

    return extension->compatibility;
}

static void
mailtc_extension_set_name (MailtcExtension* extension,
                           const gchar*     name)
{
    MAILTC_EXTENSION_SET_STRING (extension, name);
}

const gchar*
mailtc_extension_get_name (MailtcExtension* extension)
{
    g_assert (MAILTC_IS_EXTENSION (extension));

    return extension->name;
}

static void
mailtc_extension_set_author (MailtcExtension* extension,
                             const gchar*     author)
{
    MAILTC_EXTENSION_SET_STRING (extension, author);
}

const gchar*
mailtc_extension_get_author (MailtcExtension* extension)
{
    g_assert (MAILTC_IS_EXTENSION (extension));

    return extension->author;
}

static void
mailtc_extension_set_description (MailtcExtension* extension,
                                  const gchar*     description)
{
    MAILTC_EXTENSION_SET_STRING (extension, description);
}

const gchar*
mailtc_extension_get_description (MailtcExtension* extension)
{
    g_assert (MAILTC_IS_EXTENSION (extension));

    return extension->description;
}

void
mailtc_extension_set_directory (MailtcExtension* extension,
                                const gchar*     directory)
{
    MAILTC_EXTENSION_SET_STRING (extension, directory);
}

static const gchar*
mailtc_extension_get_directory (MailtcExtension* extension)
{
    g_assert (MAILTC_IS_EXTENSION (extension));

    return extension->directory;
}

void
mailtc_extension_set_module (MailtcExtension* extension,
                             GObject*         module)
{
    MAILTC_EXTENSION_SET_OBJECT (extension, module);
}

GObject*
mailtc_extension_get_module (MailtcExtension* extension)
{
    g_assert (MAILTC_IS_EXTENSION (extension));

    return extension->module ? g_object_ref (extension->module) : NULL;
}

void
mailtc_extension_set_protocols (MailtcExtension* extension,
                                GArray*          protocols)
{
    MAILTC_EXTENSION_SET_ARRAY (extension, protocols);
}

GArray*
mailtc_extension_get_protocols (MailtcExtension* extension)
{
    g_assert (MAILTC_IS_EXTENSION (extension));

    return extension->protocols ? g_array_ref (extension->protocols) : NULL;
}

gboolean
mailtc_extension_is_valid (MailtcExtension* extension,
                           GError**         error)
{
    gboolean report_error;
    gboolean compatible = FALSE;

    report_error = (error && !*error) ? TRUE : FALSE;

    if (extension)
    {
        if (MAILTC_IS_EXTENSION (extension))
        {
            const gchar* name;

            name = mailtc_extension_get_name (extension);

            if (g_strcmp0 (mailtc_extension_get_compatibility (extension), VERSION) == 0)
            {
                if (name)
                {
                    if (mailtc_extension_get_author (extension) && mailtc_extension_get_description (extension))
                    {
                        GArray* protocols;

                        protocols = mailtc_extension_get_protocols (extension);
                        if (protocols)
                        {
                            if (protocols->len > 0)
                            {
                                guint i;

                                compatible = TRUE;

                                for (i = 0; i < (guint) SIGNAL_LAST; i++)
                                {
                                    if (!g_signal_has_handler_pending (extension, signals[i], 0, FALSE))
                                    {
                                        compatible = FALSE;
                                        break;
                                    }
                                }
                            }
                            g_array_unref (protocols);
                        }
                    }
                    if (!compatible && report_error)
                    {
                        *error = g_error_new (MAILTC_EXTENSION_ERROR,
                                              MAILTC_EXTENSION_ERROR_INCOMPLETE,
                                              "Error: extension %s has incomplete information",
                                              name);
                    }
                }
                else if (report_error)
                {
                    *error = g_error_new (MAILTC_EXTENSION_ERROR,
                                          MAILTC_EXTENSION_ERROR_INCOMPLETE,
                                          "Error: extension has incomplete information");
                }
            }
            else if (report_error)
            {
                *error = g_error_new (MAILTC_EXTENSION_ERROR,
                                      MAILTC_EXTENSION_ERROR_COMPATIBILITY,
                                      "Error: extension %s has incompatible version",
                                      name);
            }
        }
        else if (report_error)
        {
            *error = g_error_new (MAILTC_EXTENSION_ERROR,
                                  MAILTC_EXTENSION_ERROR_TYPE,
                                  "Error: invalid extension type");
        }
        if (!compatible && G_IS_OBJECT (extension))
            g_object_unref (extension);
    }
    return compatible;
}

gboolean
mailtc_extension_add_account (MailtcExtension* extension,
                              GObject*         account)
{
    gboolean ret = FALSE;

    g_assert (MAILTC_IS_EXTENSION (extension));

    /* FIXME GError */
    g_signal_emit (extension, signals[SIGNAL_ADD_ACCOUNT], 0, account, &ret);

    return ret;
}

gboolean
mailtc_extension_remove_account (MailtcExtension* extension,
                                 GObject*         account)
{
    gboolean ret = FALSE;

    g_assert (MAILTC_IS_EXTENSION (extension));

    /* FIXME GError */
    g_signal_emit (extension, signals[SIGNAL_REMOVE_ACCOUNT], 0, account, &ret);

    return ret;
}

gboolean
mailtc_extension_read_messages (MailtcExtension* extension,
                                GObject*         account)
{
    gboolean ret = FALSE;

    g_assert (MAILTC_IS_EXTENSION (extension));

    /* FIXME GError */
    g_signal_emit (extension, signals[SIGNAL_READ_MESSAGES], 0, account, &ret);

    return ret;
}

gint64
mailtc_extension_get_messages (MailtcExtension* extension,
                               GObject*         account,
                               gboolean         debug)
{
    gint64 nmessages;

    g_assert (MAILTC_IS_EXTENSION (extension));

    /* FIXME GError */
    g_signal_emit (extension, signals[SIGNAL_GET_MESSAGES], 0, account, debug, &nmessages);

    return nmessages;
}

static void
mailtc_extension_finalize (GObject* object)
{
    MailtcExtension* extension = MAILTC_EXTENSION (object);

    if (extension->protocols)
        g_array_unref (extension->protocols);
    if (extension->module)
        g_object_unref (extension->module);

    g_free (extension->directory);
    g_free (extension->description);
    g_free (extension->author);
    g_free (extension->name);
    g_free (extension->compatibility);

    G_OBJECT_CLASS (mailtc_extension_parent_class)->finalize (object);
}

static void
mailtc_extension_set_property (GObject*      object,
                               guint         prop_id,
                               const GValue* value,
                               GParamSpec*   pspec)
{
    MailtcExtension* extension = MAILTC_EXTENSION (object);

    switch (prop_id)
    {
        case PROP_COMPATIBILITY:
            mailtc_extension_set_compatibility (extension, g_value_get_string (value));
            break;

        case PROP_NAME:
            mailtc_extension_set_name (extension, g_value_get_string (value));
            break;

        case PROP_AUTHOR:
            mailtc_extension_set_author (extension, g_value_get_string (value));
            break;

        case PROP_DESCRIPTION:
            mailtc_extension_set_description (extension, g_value_get_string (value));
            break;

        case PROP_DIRECTORY:
            mailtc_extension_set_directory (extension, g_value_get_string (value));
            break;

        case PROP_MODULE:
            mailtc_extension_set_module (extension, g_value_get_object (value));
            break;

        case PROP_PROTOCOLS:
            mailtc_extension_set_protocols (extension, g_value_get_boxed (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_extension_get_property (GObject*    object,
                               guint       prop_id,
                               GValue*     value,
                               GParamSpec* pspec)
{
    MailtcExtension* extension = MAILTC_EXTENSION (object);

    switch (prop_id)
    {
        case PROP_COMPATIBILITY:
            g_value_set_string (value, mailtc_extension_get_compatibility (extension));
            break;

        case PROP_NAME:
            g_value_set_string (value, mailtc_extension_get_name (extension));
            break;

        case PROP_AUTHOR:
            g_value_set_string (value, mailtc_extension_get_author (extension));
            break;

        case PROP_DESCRIPTION:
            g_value_set_string (value, mailtc_extension_get_description (extension));
            break;

        case PROP_DIRECTORY:
            g_value_set_string (value, mailtc_extension_get_directory (extension));
            break;

        case PROP_MODULE:
            g_value_set_object (value, mailtc_extension_get_module (extension));
            break;

        case PROP_PROTOCOLS:
            g_value_set_object (value, mailtc_extension_get_protocols (extension));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_extension_class_init (MailtcExtensionClass* class)
{
    GObjectClass* gobject_class;
    GParamFlags flags;

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = mailtc_extension_finalize;
    gobject_class->set_property = mailtc_extension_set_property;
    gobject_class->get_property = mailtc_extension_get_property;

    flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT;

    g_object_class_install_property (gobject_class,
                                     PROP_COMPATIBILITY,
                                     g_param_spec_string (
                                     MAILTC_EXTENSION_PROPERTY_COMPATIBILITY,
                                     "Compatibility",
                                     "The extension compatibility",
                                     NULL,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string (
                                     MAILTC_EXTENSION_PROPERTY_NAME,
                                     "Name",
                                     "The extension name",
                                     NULL,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_AUTHOR,
                                     g_param_spec_string (
                                     MAILTC_EXTENSION_PROPERTY_AUTHOR,
                                     "Author",
                                     "The extension author",
                                     NULL,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_DESCRIPTION,
                                     g_param_spec_string (
                                     MAILTC_EXTENSION_PROPERTY_DESCRIPTION,
                                     "Description",
                                     "The extension description",
                                     NULL,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_DIRECTORY,
                                     g_param_spec_string (
                                     MAILTC_EXTENSION_PROPERTY_DIRECTORY,
                                     "Directory",
                                     "The extension directory",
                                     NULL,
                                     flags));

    /* FIXME this really required? */
    g_object_class_install_property (gobject_class,
                                     PROP_MODULE,
                                     g_param_spec_object (
                                     MAILTC_EXTENSION_PROPERTY_MODULE,
                                     "Module",
                                     "The extension parent module",
                                     MAILTC_TYPE_MODULE,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_PROTOCOLS,
                                     g_param_spec_boxed (
                                     MAILTC_EXTENSION_PROPERTY_PROTOCOLS,
                                     "Accounts",
                                     "The extension protocols",
                                     G_TYPE_ARRAY,
                                     flags));

    signals[SIGNAL_ADD_ACCOUNT] = g_signal_new (MAILTC_EXTENSION_SIGNAL_ADD_ACCOUNT,
                                                G_TYPE_FROM_CLASS (gobject_class),
                                                G_SIGNAL_RUN_LAST,
                                                G_STRUCT_OFFSET (MailtcExtensionClass, add_account),
                                                NULL,
                                                NULL,
                                                mailtc_marshal_BOOLEAN__OBJECT,
                                                G_TYPE_BOOLEAN,
                                                1,
                                                MAILTC_TYPE_ACCOUNT);

    signals[SIGNAL_REMOVE_ACCOUNT] = g_signal_new (MAILTC_EXTENSION_SIGNAL_REMOVE_ACCOUNT,
                                                   G_TYPE_FROM_CLASS (gobject_class),
                                                   G_SIGNAL_RUN_LAST,
                                                   G_STRUCT_OFFSET (MailtcExtensionClass, remove_account),
                                                   NULL,
                                                   NULL,
                                                   mailtc_marshal_BOOLEAN__OBJECT,
                                                   G_TYPE_BOOLEAN,
                                                   1,
                                                   MAILTC_TYPE_ACCOUNT);

    signals[SIGNAL_GET_MESSAGES] = g_signal_new (MAILTC_EXTENSION_SIGNAL_GET_MESSAGES,
                                                 G_TYPE_FROM_CLASS (gobject_class),
                                                 G_SIGNAL_RUN_LAST,
                                                 G_STRUCT_OFFSET (MailtcExtensionClass, get_messages),
                                                 NULL,
                                                 NULL,
                                                 mailtc_marshal_INT64__OBJECT_BOOLEAN,
                                                 G_TYPE_INT64,
                                                 2,
                                                 MAILTC_TYPE_ACCOUNT,
                                                 G_TYPE_BOOLEAN);

    signals[SIGNAL_READ_MESSAGES] = g_signal_new (MAILTC_EXTENSION_SIGNAL_READ_MESSAGES,
                                                  G_TYPE_FROM_CLASS (gobject_class),
                                                  G_SIGNAL_RUN_LAST,
                                                  G_STRUCT_OFFSET (MailtcExtensionClass, read_messages),
                                                  NULL,
                                                  NULL,
                                                  mailtc_marshal_BOOLEAN__OBJECT,
                                                  G_TYPE_BOOLEAN,
                                                  1,
                                                  MAILTC_TYPE_ACCOUNT);
}

static void
mailtc_extension_init (MailtcExtension* extension)
{
    extension->module = NULL;
    extension->protocols = NULL;
}

MailtcExtension*
mailtc_extension_new (void)
{
    return g_object_new (MAILTC_TYPE_EXTENSION, NULL);
}
