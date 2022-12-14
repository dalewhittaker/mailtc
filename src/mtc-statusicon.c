/* mtc-statusicon.c
 * Copyright (C) 2009-2022 Dale Whittaker
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

/* FIXME - temporary hack to use GtkStatusIcon with gtk 3.14 */
#define GDK_DISABLE_DEPRECATION_WARNINGS 1
#undef GTK_DISABLE_DEPRECATED

#include "mtc-statusicon.h"
#include "mtc-colour.h"
#include "mtc-pixbuf.h"

#include <gtk/gtk.h>

#define MAILTC_STATUS_ICON_ERROR   -2
#define MAILTC_STATUS_ICON_DEFAULT -1

typedef struct
{
    MailtcPixbuf* pixbuf;
    GString* tooltip;
    guint nitems;
    GHashTable* items;
} MailtcStatusIconPrivate;

struct _MailtcStatusIcon
{
    GtkStatusIcon parent_instance;

    MailtcStatusIconPrivate* priv;
};

struct _MailtcStatusIconClass
{
    GtkStatusIconClass parent_class;

    void (*read_mail)    (MailtcStatusIcon* status_icon);
    void (*mark_as_read) (MailtcStatusIcon* status_icon);
};

G_DEFINE_TYPE_WITH_PRIVATE (MailtcStatusIcon, mailtc_status_icon, GTK_TYPE_STATUS_ICON)

typedef struct
{
    gchar* name;
    gint64 nmails;
    MailtcColour colour;
} MailtcStatusIconItem;

enum
{
    READ_MAIL,
    MARK_AS_READ,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static gboolean
mailtc_status_icon_button_press_event_cb (MailtcStatusIcon* status_icon,
                                          GdkEventButton*   event)
{
    if (event->button == 1 && event->type == GDK_2BUTTON_PRESS)
        g_signal_emit (status_icon, signals[READ_MAIL], 0);
    if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
        g_signal_emit (status_icon, signals[MARK_AS_READ], 0);

    return TRUE;
}

static void
mailtc_status_icon_free_item (MailtcStatusIconItem* item)
{
    g_assert (item);

    g_free (item->name);
    g_free (item);
}

static void
mailtc_status_icon_finalize (GObject* object)
{
    MailtcStatusIcon* status_icon;
    MailtcStatusIconPrivate* priv;

    status_icon = MAILTC_STATUS_ICON (object);
    priv = status_icon->priv;

    g_hash_table_destroy (priv->items);
    g_string_free (priv->tooltip, TRUE);

    if (MAILTC_IS_PIXBUF (priv->pixbuf))
        g_object_unref (priv->pixbuf);

    G_OBJECT_CLASS (mailtc_status_icon_parent_class)->finalize (object);
}

static void
mailtc_status_icon_class_init (MailtcStatusIconClass* klass)
{
    GObjectClass* gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = mailtc_status_icon_finalize;

    signals[READ_MAIL] = g_signal_new ("read-mail",
                                       G_TYPE_FROM_CLASS (gobject_class),
                                       G_SIGNAL_RUN_LAST,
                                       G_STRUCT_OFFSET (MailtcStatusIconClass, read_mail),
                                       NULL,
                                       NULL,
                                       g_cclosure_marshal_VOID__VOID,
                                       G_TYPE_NONE,
                                       0);

    signals[MARK_AS_READ] = g_signal_new ("mark-as-read",
                                          G_TYPE_FROM_CLASS (gobject_class),
                                          G_SIGNAL_RUN_LAST,
                                          G_STRUCT_OFFSET (MailtcStatusIconClass, mark_as_read),
                                          NULL,
                                          NULL,
                                          g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE,
                                          0);
}

static void
mailtc_status_icon_init (MailtcStatusIcon* status_icon)
{
    MailtcStatusIconPrivate* priv;
    MailtcPixbuf* pixbuf;

    status_icon->priv = mailtc_status_icon_get_instance_private (status_icon);
    priv = status_icon->priv;

    pixbuf = mailtc_pixbuf_new ();
    g_object_ref_sink (pixbuf);

    priv->pixbuf = pixbuf;
    priv->tooltip = g_string_new (NULL);
    priv->nitems = 0;
    priv->items = g_hash_table_new_full (g_int_hash,
                                         g_int_equal,
                                         g_free,
                                         (GDestroyNotify) mailtc_status_icon_free_item);

    g_signal_connect (status_icon, "button-press-event",
            G_CALLBACK (mailtc_status_icon_button_press_event_cb), NULL);
}

static void
mailtc_status_icon_insert_item (MailtcStatusIcon*   status_icon,
                                const gchar*        account_name,
                                const MailtcColour* account_colour,
                                const gint          ind)
{
    MailtcStatusIconPrivate* priv;
    MailtcStatusIconItem* item;
    gint* index;

    g_assert (MAILTC_IS_STATUS_ICON (status_icon));

    priv = status_icon->priv;
    item = g_new0 (MailtcStatusIconItem, 1);

    if (!account_colour)
        item->colour.red = item->colour.green = item->colour.blue = 1.0;
    else
        item->colour = *account_colour;

    index = g_new (gint, 1);

    if (account_name)
        item->name = g_strdup (account_name);

    *index = ind;

    g_hash_table_insert (priv->items, index, item);
}

void
mailtc_status_icon_add_item (MailtcStatusIcon*   status_icon,
                             const gchar*        account_name,
                             const MailtcColour* account_colour)
{
    g_assert (MAILTC_IS_STATUS_ICON (status_icon));

    mailtc_status_icon_insert_item (status_icon, account_name, account_colour, status_icon->priv->nitems++);
}

void
mailtc_status_icon_set_default_colour (MailtcStatusIcon*   status_icon,
                                       const MailtcColour* colour)
{
    g_assert (MAILTC_IS_STATUS_ICON (status_icon));

    gtk_status_icon_set_visible (GTK_STATUS_ICON (status_icon), FALSE);
    mailtc_status_icon_insert_item (status_icon, NULL, colour, MAILTC_STATUS_ICON_DEFAULT);
}

void
mailtc_status_icon_set_error_colour (MailtcStatusIcon*   status_icon,
                                    const MailtcColour* colour)
{
    g_assert (MAILTC_IS_STATUS_ICON (status_icon));

    gtk_status_icon_set_visible (GTK_STATUS_ICON (status_icon), FALSE);
    mailtc_status_icon_insert_item (status_icon, NULL, colour, MAILTC_STATUS_ICON_ERROR);
}

void
mailtc_status_icon_update (MailtcStatusIcon* status_icon,
                           guint             id,
                           gint64            nmails,
                           gint64            ntries)
{
    MailtcStatusIconPrivate* priv;
    MailtcStatusIconItem* item;
    MailtcStatusIconItem* dflitem;
    MailtcStatusIconItem* erritem;
    MailtcColour* colour;
    MailtcPixbuf* pixbuf;
    GIcon* icon;
    gchar* tmp_str;
    GString* tooltip;
    GHashTableIter iter;
    gint index;

    g_assert (MAILTC_IS_STATUS_ICON (status_icon));

    colour = NULL;
    icon = NULL;
    tooltip = NULL;
    dflitem = NULL;
    priv = status_icon->priv;
    pixbuf = priv->pixbuf;
    tooltip = priv->tooltip;

    g_assert (id < priv->nitems);

    /* This is here because modifying the value while iterating invalidates the item pointer. */
    item = (MailtcStatusIconItem*) g_hash_table_lookup (priv->items, &id);
    g_assert (item);

    index = MAILTC_STATUS_ICON_DEFAULT;
    dflitem = (MailtcStatusIconItem*) g_hash_table_lookup (priv->items, &index);
    g_assert (dflitem);

    index = MAILTC_STATUS_ICON_ERROR;
    erritem = (MailtcStatusIconItem*) g_hash_table_lookup (priv->items, &index);
    g_assert (dflitem);

    if (nmails < 0)
    {
        if (item->nmails < 0)
            nmails = --item->nmails;
        else
            item->nmails = -1;
    }
    else
        item->nmails = nmails;

    g_hash_table_iter_init (&iter, priv->items);
    while (g_hash_table_iter_next (&iter, NULL, (gpointer*) &item))
    {
        g_assert (item);
        if (item->name && (item->nmails > 0 || (ntries + item->nmails < 1)))
        {
            if (tooltip->len > 0)
                tooltip = g_string_append (tooltip, "\n");

            if (item->nmails > 0)
            {
                tmp_str = g_strdup_printf ("%s: %" G_GINT64_FORMAT " new %s",
                                           item->name,
                                           item->nmails,
                                           (item->nmails > 1) ? "messages" : "message");

                colour = colour ? &dflitem->colour : &item->colour;
            }
            else
            {
                tmp_str = g_strdup_printf ("%s: error", item->name);
                dflitem = erritem;
                colour = &dflitem->colour;
            }

            tooltip = g_string_append (tooltip, tmp_str);
            g_free (tmp_str);
        }
    }

    mailtc_pixbuf_set_colour (pixbuf, colour);
    icon = mailtc_pixbuf_get_icon (pixbuf);
    g_assert (icon);

    /*gtk_status_icon_set_from_gicon (GTK_STATUS_ICON (status_icon), icon);*/
    gtk_status_icon_set_from_pixbuf (GTK_STATUS_ICON (status_icon), GDK_PIXBUF (icon));
    gtk_status_icon_set_visible (GTK_STATUS_ICON (status_icon), colour ? TRUE : FALSE);

    gtk_status_icon_set_tooltip_text (GTK_STATUS_ICON (status_icon), tooltip->str);

    tooltip = g_string_set_size (tooltip, 0);
}

void
mailtc_status_icon_clear (MailtcStatusIcon* status_icon)
{
    MailtcStatusIconPrivate* priv;
    MailtcStatusIconItem* item;
    GHashTableIter iter;

    g_assert (MAILTC_IS_STATUS_ICON (status_icon));

    priv = status_icon->priv;

    g_hash_table_iter_init (&iter, priv->items);
    while (g_hash_table_iter_next (&iter, NULL, (gpointer*) &item))
    {
        g_assert (item);
        item->nmails = 0;
    }
    mailtc_pixbuf_set_colour (priv->pixbuf, NULL);
    gtk_status_icon_set_visible (GTK_STATUS_ICON (status_icon), FALSE);
}

MailtcStatusIcon*
mailtc_status_icon_new (void)
{
    return g_object_new (MAILTC_TYPE_STATUS_ICON, NULL);
}

