/* mtc-uid.c
 * Copyright (C) 2009-2015 Dale Whittaker <dayul@users.sf.net>
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

#include "mtc-uid.h"

typedef enum
{
    UIDL_FLAG_NEW = 1,
    UIDL_FLAG_READ = 2
} uidl_flag;

struct _MailtcUidTablePrivate
{
    GHashTable* uids;
};

struct _MailtcUidTable
{
    GObject parent_instance;

    MailtcUidTablePrivate* priv;
    gchar* filename;
};

struct _MailtcUidTableClass
{
    GObjectClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (MailtcUidTable, mailtc_uid_table, G_TYPE_OBJECT, G_ADD_PRIVATE (MailtcUidTable))

enum
{
    PROP_0,
    PROP_FILENAME
};

static void
mailtc_uid_table_finalize (GObject* object)
{
    MailtcUidTable* uid_table;

    uid_table = MAILTC_UID_TABLE (object);
    g_free (uid_table->filename);
    g_hash_table_destroy (uid_table->priv->uids);
    uid_table->filename = NULL;

    G_OBJECT_CLASS (mailtc_uid_table_parent_class)->finalize (object);
}

gboolean
mailtc_uid_table_load (MailtcUidTable* uid_table,
                       GError**        error)
{
    MailtcUidTablePrivate* priv;
    GIOChannel* uidfile;
    gchar* uidl;
    gchar* flags;

    g_assert (MAILTC_IS_UID_TABLE (uid_table));
    g_assert (uid_table->filename);

    if (!g_file_test (uid_table->filename, G_FILE_TEST_EXISTS))
        return TRUE;

    priv = uid_table->priv;
    if (!(uidfile = g_io_channel_new_file (uid_table->filename, "r", error)))
        return FALSE;

    g_io_channel_set_line_term (uidfile, "\n", 1);
    if (g_io_channel_set_encoding (uidfile, NULL, error) != G_IO_STATUS_NORMAL)
        return FALSE;

    g_hash_table_remove_all (priv->uids);
    while (g_io_channel_read_line (uidfile, &uidl, NULL, NULL, error) == G_IO_STATUS_NORMAL)
    {
        flags = g_new (gchar, 1);
        *flags = UIDL_FLAG_READ;
        g_hash_table_insert (priv->uids, g_strchomp (uidl), flags);
    }
    if (g_io_channel_shutdown (uidfile, TRUE, error && *error ? NULL : error) == G_IO_STATUS_ERROR)
        return FALSE;

    g_io_channel_unref (uidfile);

    return TRUE;
}

static void
mailtc_uid_table_set_filename (MailtcUidTable* uid_table,
                               const gchar*    filename)
{
    g_assert (MAILTC_IS_UID_TABLE (uid_table));

    if (g_strcmp0 (uid_table->filename, filename) != 0)
    {
        g_free (uid_table->filename);

        uid_table->filename = g_strdup (filename);
        g_object_notify (G_OBJECT (uid_table), "filename");
    }
}

static void
mailtc_uid_table_set_property (GObject*      object,
                               guint         prop_id,
                               const GValue* value,
                               GParamSpec*   pspec)
{
    MailtcUidTable* uid_table = MAILTC_UID_TABLE (object);

    switch (prop_id)
    {
        case PROP_FILENAME:
            mailtc_uid_table_set_filename (uid_table, g_value_get_string (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_uid_table_get_property (GObject*    object,
                               guint       prop_id,
                               GValue*     value,
                               GParamSpec* pspec)
{
    MailtcUidTable* uid_table = MAILTC_UID_TABLE (object);

    switch (prop_id)
    {
        case PROP_FILENAME:
            g_value_set_string (value, uid_table->filename);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_uid_table_class_init (MailtcUidTableClass* klass)
{
    GObjectClass* gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = mailtc_uid_table_finalize;
    gobject_class->set_property = mailtc_uid_table_set_property;
    gobject_class->get_property = mailtc_uid_table_get_property;

    g_object_class_install_property (gobject_class,
                                     PROP_FILENAME,
                                     g_param_spec_string (
                                     "filename",
                                     "Filename",
                                     "The UID filename",
                                     NULL,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT));
}

static void
mailtc_uid_table_init (MailtcUidTable* uid_table)
{
    MailtcUidTablePrivate* priv;

    uid_table->priv = G_TYPE_INSTANCE_GET_PRIVATE (uid_table,
                 MAILTC_TYPE_UID_TABLE, MailtcUidTablePrivate);

    priv = uid_table->priv;
    priv->uids = g_hash_table_new_full (g_str_hash,
                                        g_str_equal,
                                        g_free,
                                        g_free);
}

void
mailtc_uid_table_age (MailtcUidTable* uid_table)
{
    MailtcUidTablePrivate* priv;
    GHashTableIter iter;
    gchar* flags;

    g_assert (MAILTC_IS_UID_TABLE (uid_table));

    priv = uid_table->priv;

    g_hash_table_iter_init (&iter, priv->uids);
    while (g_hash_table_iter_next (&iter, NULL, (gpointer*) &flags))
    {
        g_assert (flags);
        *flags &= ~UIDL_FLAG_NEW;
    }
}

static gboolean
mailtc_uid_table_remove_func (gchar* uidl,
                              gchar* flags)
{
    (void) uidl;
    return (!flags || !(*flags & UIDL_FLAG_NEW));
}

gint64
mailtc_uid_table_remove_old (MailtcUidTable* uid_table)
{
    MailtcUidTablePrivate* priv;
    GHashTableIter iter;
    gint64 n = 0;
    gchar* flags;

    g_assert (MAILTC_IS_UID_TABLE (uid_table));

    priv = uid_table->priv;
    g_hash_table_foreach_remove (priv->uids,
                (GHRFunc) mailtc_uid_table_remove_func, NULL);

    g_hash_table_iter_init (&iter, priv->uids);
    while (g_hash_table_iter_next (&iter, NULL, (gpointer*) &flags))
    {
        if (flags && !(*flags & UIDL_FLAG_READ))
            n++;
    }
    return n;
}

void
mailtc_uid_table_add (MailtcUidTable* uid_table,
                      gchar*          uidl)
{
    MailtcUidTablePrivate* priv;
    gchar* flags;
    gchar* puidl;

    g_assert (MAILTC_IS_UID_TABLE (uid_table));

    priv = uid_table->priv;

    puidl = g_strchomp (g_strdup (uidl));
    flags = (gchar*) g_hash_table_lookup (priv->uids, puidl);
    if (!flags)
    {
        flags = g_new (gchar, 1);
        *flags = UIDL_FLAG_NEW;
        g_hash_table_insert (priv->uids, puidl, flags);
    }
    else
    {
        g_free (puidl);
        *flags |= UIDL_FLAG_NEW;
    }
}

gboolean
mailtc_uid_table_mark_read (MailtcUidTable* uid_table,
                            GError**        error)
{
    MailtcUidTablePrivate* priv;
    GIOChannel* uidfile;
    GHashTableIter iter;
    gchar* uidl;
    gchar* flags;
    gsize bytes;

    g_assert (MAILTC_IS_UID_TABLE (uid_table));

    priv = uid_table->priv;

    /* TODO maybe first backup the file */
    if (!(uidfile = g_io_channel_new_file (uid_table->filename, "w", error)))
        return FALSE;

    if (g_io_channel_set_encoding (uidfile, NULL, error) != G_IO_STATUS_NORMAL)
        return FALSE;

    g_hash_table_iter_init (&iter, priv->uids);
    while (g_hash_table_iter_next (&iter, (gpointer*) &uidl, (gpointer*) &flags))
    {
        *flags |= UIDL_FLAG_READ;
        g_io_channel_write_chars (uidfile, uidl, -1, &bytes, NULL);
        g_io_channel_write_chars (uidfile, "\n", 1, &bytes, NULL);
    }

    g_io_channel_flush (uidfile, NULL);
    if (g_io_channel_shutdown (uidfile, TRUE, error) == G_IO_STATUS_ERROR)
        return FALSE;

    g_io_channel_unref (uidfile);

    return TRUE;
}

MailtcUidTable*
mailtc_uid_table_new (gchar* filename)
{
    return g_object_new (MAILTC_TYPE_UID_TABLE, "filename", filename, NULL);
}

