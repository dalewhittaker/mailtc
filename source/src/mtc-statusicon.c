/* mtc-statusicon.c
 * Copyright (C) 2009 Dale Whittaker <dayul@users.sf.net>
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

#include "mtc-statusicon.h"
#include "mtc-envelope.h"

struct _MailtcStatusIconPrivate
{
    GtkWidget* envelope;
    GString* tooltip;
    guint nitems;
    GHashTable* items;
};

struct _MailtcStatusIcon
{
    GtkStatusIcon parent_instance;

    MailtcStatusIconPrivate* priv;
};

struct _MailtcStatusIconClass
{
    GtkStatusIconClass parent_class;
};

G_DEFINE_TYPE (MailtcStatusIcon, mailtc_status_icon, GTK_TYPE_STATUS_ICON)

typedef struct
{
    gchar* name;
    guint64 nmails;
    GdkColor* colour;

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

    if (item->colour)
        gdk_color_free (item->colour);

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

    if (MAILTC_IS_ENVELOPE (priv->envelope))
        g_object_unref (priv->envelope);

    G_OBJECT_CLASS (mailtc_status_icon_parent_class)->finalize (object);
}

static void
mailtc_status_icon_class_init (MailtcStatusIconClass* class)
{
    GObjectClass* gobject_class;

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = mailtc_status_icon_finalize;

    signals[READ_MAIL] = g_signal_new ("read-mail",
                                       G_TYPE_FROM_CLASS (gobject_class),
                                       G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                       0,
                                       NULL,
                                       NULL,
                                       g_cclosure_marshal_VOID__VOID,
                                       G_TYPE_NONE,
                                       0);

    signals[MARK_AS_READ] = g_signal_new ("mark-as-read",
                                          G_TYPE_FROM_CLASS (gobject_class),
                                          G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                          0,
                                          NULL,
                                          NULL,
                                          g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE,
                                          0);

    g_type_class_add_private (class, sizeof (MailtcStatusIconPrivate));
}

static void
mailtc_status_icon_init (MailtcStatusIcon* status_icon)
{
    MailtcStatusIconPrivate* priv;
    GtkWidget* envelope;

    status_icon->priv = G_TYPE_INSTANCE_GET_PRIVATE (status_icon,
                        MAILTC_TYPE_STATUS_ICON, MailtcStatusIconPrivate);
    priv = status_icon->priv;

    envelope = mailtc_envelope_new ();
    g_object_ref_sink (envelope);

    priv->envelope = envelope;
    priv->tooltip = g_string_new (NULL);
    priv->nitems = 0;
    priv->items = g_hash_table_new_full (g_int_hash,
                                         g_int_equal,
                                         g_free,
                                         (GDestroyNotify)mailtc_status_icon_free_item);

    g_signal_connect (status_icon, "button-press-event",
            G_CALLBACK (mailtc_status_icon_button_press_event_cb), NULL);
}

void
mailtc_status_icon_add_item (MailtcStatusIcon* status_icon,
                             const gchar*      account_name,
                             const GdkColor*   account_colour)
{
    MailtcStatusIconPrivate* priv;
    MailtcStatusIconItem* item;
    gint* index;

    g_return_if_fail (MAILTC_IS_STATUS_ICON (status_icon));

    priv = status_icon->priv;
    item = g_new0 (MailtcStatusIconItem, 1);

    if (account_colour)
        item->colour = gdk_color_copy (account_colour);

    index = g_new (gint, 1);

    /* If there is no name, it is assumed to be the default colour */
    if (account_name)
    {
        item->name = g_strdup (account_name);
        *index = priv->nitems;
        priv->nitems++;
    }
    else
        *index = -1;

    g_hash_table_insert (priv->items, index, item);
}

void
mailtc_status_icon_set_default_colour (MailtcStatusIcon* status_icon,
                                       const GdkColor*   colour)
{
    g_return_if_fail (MAILTC_IS_STATUS_ICON (status_icon));

    gtk_status_icon_set_visible (GTK_STATUS_ICON (status_icon), FALSE);
    mailtc_status_icon_add_item (status_icon, NULL, colour);
}

void
mailtc_status_icon_update (MailtcStatusIcon* status_icon,
                           guint             id,
                           guint64           nmails)
{
    MailtcStatusIconPrivate* priv;
    MailtcStatusIconItem* item;
    MailtcStatusIconItem* dflitem;
    GtkWidget* envelope;
    GdkPixbuf* pixbuf;
    gchar* tmp_str;
    GString* tooltip;
    GHashTableIter iter;
    GdkColor* colour;
    gint index;

    g_return_if_fail (MAILTC_IS_STATUS_ICON (status_icon));

    colour = NULL;
    pixbuf = NULL;
    tooltip = NULL;
    dflitem = NULL;
    index = -1;
    priv = status_icon->priv;
    envelope = priv->envelope;
    tooltip = priv->tooltip;

    g_assert (id < priv->nitems);

    /* This is here because modifying the value while iterating
     * invalidates the item pointer.
     */
    item = (MailtcStatusIconItem*) g_hash_table_lookup (priv->items, &id);
    g_assert (item);

    dflitem = (MailtcStatusIconItem*) g_hash_table_lookup (priv->items, &index);
    g_assert (dflitem);

    item->nmails = nmails;

    g_hash_table_iter_init (&iter, priv->items);
    while (g_hash_table_iter_next (&iter, NULL, (gpointer*) &item))
    {
        g_assert (item);
        if (item->name && item->nmails > 0)
        {
            if (tooltip->len > 0)
                tooltip = g_string_append (tooltip, "\n");

            tmp_str = g_strdup_printf ("%s: %" G_GINT64_FORMAT " new %s",
                                       item->name,
                                       item->nmails,
                                       (item->nmails > 1) ? "messages" : "message");
            tooltip = g_string_append (tooltip, tmp_str);
            g_free (tmp_str);

            /* If colour is already set there is more than one item, show the multi colour. */
            colour = colour ? dflitem->colour : item->colour;
        }
    }

    mailtc_envelope_set_envelope_colour (MAILTC_ENVELOPE (envelope), colour);
    pixbuf = mailtc_envelope_get_pixbuf (MAILTC_ENVELOPE (envelope));
    g_assert (pixbuf);

    gtk_status_icon_set_from_pixbuf (GTK_STATUS_ICON (status_icon), pixbuf);
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

    g_return_if_fail (MAILTC_IS_STATUS_ICON (status_icon));

    priv = status_icon->priv;

    g_hash_table_iter_init (&iter, priv->items);
    while (g_hash_table_iter_next (&iter, NULL, (gpointer*) &item))
    {
        g_assert (item);
        item->nmails = 0;
    }
    mailtc_envelope_set_envelope_colour (MAILTC_ENVELOPE (priv->envelope), NULL);
    gtk_status_icon_set_visible (GTK_STATUS_ICON (status_icon), FALSE);
}

MailtcStatusIcon*
mailtc_status_icon_new (void)
{
    return g_object_new (MAILTC_TYPE_STATUS_ICON, NULL);
}

