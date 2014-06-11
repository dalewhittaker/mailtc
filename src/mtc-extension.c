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
#include <config.h>

#include "mtc-extension.h"
#include "mtc-account.h"
#include "mtc-module.h"
#include "mtc-util.h"

#define MAILTC_EXTENSION_ERROR g_quark_from_string ("MAILTC_EXTENSION_ERROR")

#define MAILTC_EXTENSION_SET_STRING(extension, property) \
    mailtc_object_set_string (G_OBJECT (extension), MAILTC_TYPE_EXTENSION, \
                              #property, &extension->priv->property, property)

#define MAILTC_EXTENSION_SET_OBJECT(extension, property) \
    mailtc_object_set_object (G_OBJECT (extension), MAILTC_TYPE_EXTENSION, \
                              #property, (GObject **) (&extension->priv->property), G_OBJECT (property))

#define MAILTC_EXTENSION_GET_OBJECT(extension, property) \
                                    (extension)->priv->property ? g_object_ref ((extension)->priv->property) : NULL;

#define MAILTC_EXTENSION_SET_ARRAY(extension,property) \
    mailtc_object_set_array (G_OBJECT (extension), MAILTC_TYPE_EXTENSION, \
                              #property, &extension->priv->property, property)

#define MAILTC_EXTENSION_GET_ARRAY(extension, property) \
                                    (extension)->priv->property ? g_array_ref ((extension)->priv->property) : NULL;

struct _MailtcExtensionPrivate
{
    GArray* protocols;
    gchar* compatibility;
    gchar* name;
    gchar* author;
    gchar* description;
    gchar* directory;
    MailtcModule* module;
};

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

G_DEFINE_TYPE_WITH_CODE (MailtcExtension, mailtc_extension, G_TYPE_OBJECT, G_ADD_PRIVATE (MailtcExtension))

static void
mailtc_extension_set_compatibility (MailtcExtension* extension,
                                    const gchar*     compatibility)
{
    g_assert (MAILTC_IS_EXTENSION (extension) && extension->priv);

    MAILTC_EXTENSION_SET_STRING (extension, compatibility);
}

const gchar*
mailtc_extension_get_compatibility (MailtcExtension* extension)
{
    g_assert (MAILTC_IS_EXTENSION (extension) && extension->priv);

    return extension->priv->compatibility;
}

static void
mailtc_extension_set_name (MailtcExtension* extension,
                           const gchar*     name)
{
    g_assert (MAILTC_IS_EXTENSION (extension) && extension->priv);

    MAILTC_EXTENSION_SET_STRING (extension, name);
}

const gchar*
mailtc_extension_get_name (MailtcExtension* extension)
{
    g_assert (MAILTC_IS_EXTENSION (extension) && extension->priv);

    return extension->priv->name;
}

static void
mailtc_extension_set_author (MailtcExtension* extension,
                             const gchar*     author)
{
    g_assert (MAILTC_IS_EXTENSION (extension) && extension->priv);

    MAILTC_EXTENSION_SET_STRING (extension, author);
}

const gchar*
mailtc_extension_get_author (MailtcExtension* extension)
{
    g_assert (MAILTC_IS_EXTENSION (extension) && extension->priv);

    return extension->priv->author;
}

static void
mailtc_extension_set_description (MailtcExtension* extension,
                                  const gchar*     description)
{
    g_assert (MAILTC_IS_EXTENSION (extension) && extension->priv);

    MAILTC_EXTENSION_SET_STRING (extension, description);
}

const gchar*
mailtc_extension_get_description (MailtcExtension* extension)
{
    g_assert (MAILTC_IS_EXTENSION (extension) && extension->priv);

    return extension->priv->description;
}

void
mailtc_extension_set_directory (MailtcExtension* extension,
                                const gchar*     directory)
{
    g_assert (MAILTC_IS_EXTENSION (extension) && extension->priv);

    MAILTC_EXTENSION_SET_STRING (extension, directory);
}

static const gchar*
mailtc_extension_get_directory (MailtcExtension* extension)
{
    g_assert (MAILTC_IS_EXTENSION (extension) && extension->priv);

    return extension->priv->directory;
}

void
mailtc_extension_set_module (MailtcExtension* extension,
                             GObject*         module)
{
    g_assert (MAILTC_IS_EXTENSION (extension) && extension->priv);

    MAILTC_EXTENSION_SET_OBJECT (extension, module);
}

GObject*
mailtc_extension_get_module (MailtcExtension* extension)
{
    g_assert (MAILTC_IS_EXTENSION (extension) && extension->priv);

    return MAILTC_EXTENSION_GET_OBJECT (extension, module);
}

void
mailtc_extension_set_protocols (MailtcExtension* extension,
                                GArray*          protocols)
{
    g_assert (MAILTC_IS_EXTENSION (extension) && extension->priv);

    MAILTC_EXTENSION_SET_ARRAY (extension, protocols);
}

GArray*
mailtc_extension_get_protocols (MailtcExtension* extension)
{
    g_assert (MAILTC_IS_EXTENSION (extension) && extension->priv);

    return MAILTC_EXTENSION_GET_ARRAY (extension, protocols);
}

gboolean
mailtc_extension_is_valid (MailtcExtension* extension,
                           GError**         error)
{
    gboolean compatible = FALSE;

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
                                MailtcExtensionClass* klass;

                                klass = MAILTC_EXTENSION_GET_CLASS (extension);

                                if (klass->add_account && klass->remove_account && klass->read_messages && klass->get_messages)
                                    compatible = TRUE;
                            }
                            g_array_unref (protocols);
                        }
                    }
                    if (!compatible)
                    {
                        g_set_error (error,
                                     MAILTC_EXTENSION_ERROR,
                                     MAILTC_EXTENSION_ERROR_INCOMPLETE,
                                     "Error: extension %s has incomplete information",
                                     name);
                    }
                }
                else
                {
                    g_set_error_literal (error,
                                         MAILTC_EXTENSION_ERROR,
                                         MAILTC_EXTENSION_ERROR_INCOMPLETE,
                                         "Error: extension has incomplete information");
                }
            }
            else
            {
                g_set_error (error,
                             MAILTC_EXTENSION_ERROR,
                             MAILTC_EXTENSION_ERROR_COMPATIBILITY,
                             "Error: extension %s has incompatible version",
                             name);
            }
        }
        else
        {
            g_set_error_literal (error,
                                 MAILTC_EXTENSION_ERROR,
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
                              GObject*         account,
                              GError**         error)
{
    MailtcExtensionAddAccountFunc add_account;

    g_assert (MAILTC_IS_EXTENSION (extension));

    add_account = MAILTC_EXTENSION_GET_CLASS (extension)->add_account;

    if (!add_account)
    {
        g_set_error (error,
                     MAILTC_EXTENSION_ERROR,
                     MAILTC_EXTENSION_ERROR_INCOMPLETE,
                     "Error: extension %s is missing add_account",
                     mailtc_extension_get_name (extension));
        return FALSE;
    }

    return add_account (extension, account, error);
}

gboolean
mailtc_extension_remove_account (MailtcExtension* extension,
                                 GObject*         account,
                                 GError**         error)
{
    MailtcExtensionRemoveAccountFunc remove_account;

    g_assert (MAILTC_IS_EXTENSION (extension));

    remove_account = MAILTC_EXTENSION_GET_CLASS (extension)->remove_account;

    if (!remove_account)
    {
        g_set_error (error,
                     MAILTC_EXTENSION_ERROR,
                     MAILTC_EXTENSION_ERROR_INCOMPLETE,
                     "Error: extension %s is missing remove_account",
                     mailtc_extension_get_name (extension));
        return FALSE;
    }

    return remove_account (extension, account, error);
}

gboolean
mailtc_extension_read_messages (MailtcExtension* extension,
                                GObject*         account,
                                GError**         error)
{
    MailtcExtensionReadMessagesFunc read_messages;

    g_assert (MAILTC_IS_EXTENSION (extension));

    read_messages = MAILTC_EXTENSION_GET_CLASS (extension)->read_messages;

    if (!read_messages)
    {
        g_set_error (error,
                     MAILTC_EXTENSION_ERROR,
                     MAILTC_EXTENSION_ERROR_INCOMPLETE,
                     "Error: extension %s is missing read_messages",
                     mailtc_extension_get_name (extension));
        return FALSE;
    }

    return read_messages (extension, account, error);
}

gint64
mailtc_extension_get_messages (MailtcExtension* extension,
                               GObject*         account,
                               gboolean         debug,
                               GError**         error)
{
    MailtcExtensionGetMessagesFunc get_messages;

    g_assert (MAILTC_IS_EXTENSION (extension));

    get_messages = MAILTC_EXTENSION_GET_CLASS (extension)->get_messages;

    if (!get_messages)
    {
        g_set_error (error,
                     MAILTC_EXTENSION_ERROR,
                     MAILTC_EXTENSION_ERROR_INCOMPLETE,
                     "Error: extension %s is missing get_messages",
                     mailtc_extension_get_name (extension));
        return FALSE;
    }

    return get_messages (extension, account, debug, error);
}

static void
mailtc_extension_finalize (GObject* object)
{
    MailtcExtension* extension;
    MailtcExtensionPrivate* priv;

    extension = MAILTC_EXTENSION (object);
    priv = extension->priv;

    if (priv->protocols)
        g_array_unref (priv->protocols);
    if (priv->module)
        g_object_unref (priv->module);

    g_free (priv->directory);
    g_free (priv->description);
    g_free (priv->author);
    g_free (priv->name);
    g_free (priv->compatibility);

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
mailtc_extension_class_init (MailtcExtensionClass* klass)
{
    GObjectClass* gobject_class;
    GParamFlags flags;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = mailtc_extension_finalize;
    gobject_class->set_property = mailtc_extension_set_property;
    gobject_class->get_property = mailtc_extension_get_property;

    klass->add_account = NULL;
    klass->remove_account = NULL;
    klass->read_messages = NULL;
    klass->get_messages = NULL;

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
}

static void
mailtc_extension_init (MailtcExtension* extension)
{
    extension->priv = G_TYPE_INSTANCE_GET_PRIVATE (extension, MAILTC_TYPE_EXTENSION, MailtcExtensionPrivate);

    extension->priv->module = NULL;
    extension->priv->protocols = NULL;
}

MailtcExtension*
mailtc_extension_new (void)
{
    return g_object_new (MAILTC_TYPE_EXTENSION, NULL);
}
