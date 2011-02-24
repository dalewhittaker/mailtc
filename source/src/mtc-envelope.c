/* mtc-envelope.c
 * Copyright (C) 2009-2011 Dale Whittaker <dayul@users.sf.net>
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

#include "mtc-envelope.h"

struct _MailtcEnvelopePrivate
{
    GdkPixbuf* pixbuf;
    guint8* data;
    gboolean pixbuf_is_envelope;
};

struct _MailtcEnvelope
{
    GtkImage parent_instance;

    MailtcEnvelopePrivate* priv;
    GdkColor* colour;
};

struct _MailtcEnvelopeClass
{
    GtkImageClass parent_class;
};

G_DEFINE_TYPE (MailtcEnvelope, mailtc_envelope, GTK_TYPE_IMAGE)

enum
{
    PROP_0,
    PROP_ENVELOPE_COLOUR
};

static void
mailtc_envelope_set_pixbuf_colour (GdkPixbuf* pixbuf,
                                   GdkColor*  colour)
{
    gint width;
    gint height;
    gint rowstride;
    gint n_channels;
    guchar* pixels;
    guchar* p;
    gint i;
    gint j;
    guint8 r;
    guint8 g;
    guint8 b;

    if (colour)
    {
        r = colour->red >> 8;
        g = colour->green >> 8;
        b = colour->blue >> 8;
    }
    else
        r = g = b = 255;

    n_channels = gdk_pixbuf_get_n_channels (pixbuf);
    g_assert (gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (pixbuf) == 8);
    g_assert (gdk_pixbuf_get_has_alpha (pixbuf));
    g_assert (n_channels == 4);

    width = gdk_pixbuf_get_width (pixbuf);
    height = gdk_pixbuf_get_height (pixbuf);
    rowstride = gdk_pixbuf_get_rowstride (pixbuf);
    pixels = gdk_pixbuf_get_pixels (pixbuf);

    for (i = 0; i < width; i++)
    {
        for (j = 0; j < height; j++)
        {
            p = pixels + i * rowstride + j * n_channels;
            if (p[0] == 0xFF && p[1] == 0xFF && p[2] == 0xFF && p[3] == 0xFF)
            {
                p[0] = r;
                p[1] = g;
                p[2] = b;
                p[3] = 0xFF;
            }
        }
    }
}

static GdkPixbuf*
mailtc_envelope_create_pixbuf (MailtcEnvelope* envelope)
{

    GdkPixbuf* pixbuf;

/* GdkPixbuf RGBA C-Source image dump */

#ifdef __SUNPRO_C
#pragma align 4 (my_pixbuf)
#endif
#ifdef __GNUC__
const guint8 envelope_data[] __attribute__ ((__aligned__ (4))) = 
#else
const guint8 envelope_data[] = 
#endif
{ ""
  /* Pixbuf magic (0x47646b50) */
  "GdkP"
  /* length: header (24) + pixel_data (703) */
  "\0\0\2\327"
  /* pixdata_type (0x2010002) */
  "\2\1\0\2"
  /* rowstride (88) */
  "\0\0\0X"
  /* width (22) */
  "\0\0\0\26"
  /* height (22) */
  "\0\0\0\26"
  /* pixel_data: */
  "\304\0\0\0\0\1\0\0\0\1\221\0\0\0\2\1\0\0\0\1\202\0\0\0\0\1\0\0\0f\221"
  "\0\0\0\377\4\0\0\0m\0\0\0\10\0\0\0\2\0\0\0\1\202\0\0\0\377\217\377\377"
  "\377\377\202\0\0\0\377\6\0\0\0\31\0\0\0\10\0\0\0\2\0\0\0\377\377\377"
  "\377\377\0\0\0\377\215\377\377\377\377\7\0\0\0\377\377\377\377\377\0"
  "\0\0\377\0\0\0(\0\0\0\17\0\0\0\2\0\0\0\377\202\377\377\377\377\1\0\0"
  "\0\377\213\377\377\377\377\1\0\0\0\377\202\377\377\377\377\5\0\0\0\377"
  "\0\0\0-\0\0\0\21\0\0\0\2\0\0\0\377\203\377\377\377\377\1\0\0\0\377\211"
  "\377\377\377\377\1\0\0\0\377\203\377\377\377\377\5\0\0\0\377\0\0\0-\0"
  "\0\0\21\0\0\0\2\0\0\0\377\204\377\377\377\377\1\0\0\0\377\207\377\377"
  "\377\377\1\0\0\0\377\204\377\377\377\377\5\0\0\0\377\0\0\0-\0\0\0\21"
  "\0\0\0\2\0\0\0\377\205\377\377\377\377\1\0\0\0\377\205\377\377\377\377"
  "\1\0\0\0\377\205\377\377\377\377\5\0\0\0\377\0\0\0-\0\0\0\21\0\0\0\2"
  "\0\0\0\377\205\377\377\377\377\202\0\0\0\377\203\377\377\377\377\202"
  "\0\0\0\377\205\377\377\377\377\5\0\0\0\377\0\0\0-\0\0\0\21\0\0\0\2\0"
  "\0\0\377\204\377\377\377\377\1\0\0\0\377\202\377\377\377\377\3\0\0\0"
  "\377\377\377\377\377\0\0\0\377\202\377\377\377\377\1\0\0\0\377\204\377"
  "\377\377\377\5\0\0\0\377\0\0\0-\0\0\0\21\0\0\0\2\0\0\0\377\203\377\377"
  "\377\377\1\0\0\0\377\204\377\377\377\377\1\0\0\0\377\204\377\377\377"
  "\377\1\0\0\0\377\203\377\377\377\377\5\0\0\0\377\0\0\0-\0\0\0\21\0\0"
  "\0\2\0\0\0\377\202\377\377\377\377\1\0\0\0\377\213\377\377\377\377\1"
  "\0\0\0\377\202\377\377\377\377\7\0\0\0\377\0\0\0-\0\0\0\21\0\0\0\2\0"
  "\0\0\377\377\377\377\377\0\0\0\377\215\377\377\377\377\6\0\0\0\377\377"
  "\377\377\377\0\0\0\377\0\0\0-\0\0\0\21\0\0\0\2\202\0\0\0\377\217\377"
  "\377\377\377\202\0\0\0\377\4\0\0\0-\0\0\0\21\0\0\0\2\0\0\0m\221\0\0\0"
  "\377\7\0\0\0\207\0\0\0(\0\0\0\17\0\0\0\1\0\0\0\10\0\0\0\31\0\0\0(\217"
  "\0\0\0-\7\0\0\0(\0\0\0\31\0\0\0\10\0\0\0\0\0\0\0\2\0\0\0\10\0\0\0\16"
  "\217\0\0\0\21\3\0\0\0\16\0\0\0\10\0\0\0\2\202\0\0\0\0\1\0\0\0\1\221\0"
  "\0\0\2\1\0\0\0\1\227\0\0\0\0"};

    g_return_val_if_fail (MAILTC_IS_ENVELOPE (envelope), NULL);
    if (!envelope->priv->data)
        envelope->priv->data = g_memdup (envelope_data, sizeof (envelope_data));

    pixbuf = gdk_pixbuf_new_from_inline (-1, envelope->priv->data, FALSE, NULL);
    if (!pixbuf)
        return NULL;

    return pixbuf;
}

static void
mailtc_envelope_notify_pixbuf_cb (GObject*    object,
                                  GParamSpec* pspec)
{
    MailtcEnvelope* envelope;
    gboolean is_envelope = FALSE;

    (void) pspec;
    envelope = MAILTC_ENVELOPE (object);

    if (gtk_image_get_storage_type (GTK_IMAGE (envelope)) == GTK_IMAGE_PIXBUF)
    {
        GdkPixbuf* pixbuf;

        pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (envelope));
        if (pixbuf)
        {
            is_envelope = GPOINTER_TO_UINT (
                    g_object_get_data (G_OBJECT (pixbuf), "mailtc_envelope"));
        }
    }

    envelope->priv->pixbuf_is_envelope = is_envelope;
}

void
mailtc_envelope_set_envelope_colour (MailtcEnvelope* envelope,
                                     GdkColor*       colour)
{
    MailtcEnvelopePrivate* priv;
    GdkPixbuf* pixbuf;

    g_return_if_fail (MAILTC_IS_ENVELOPE (envelope));

    priv = envelope->priv;
    if (gtk_image_get_storage_type (GTK_IMAGE (envelope)) == GTK_IMAGE_PIXBUF &&
        priv->pixbuf_is_envelope &&
        envelope->colour && colour &&
        gdk_color_equal (colour, envelope->colour))
        return;

    pixbuf = mailtc_envelope_create_pixbuf (envelope);

    g_assert (pixbuf);

    if (envelope->colour)
    {
        gdk_color_free (envelope->colour);
        envelope->colour = NULL;
    }

    if (colour)
    {
        mailtc_envelope_set_pixbuf_colour (pixbuf, colour);
        envelope->colour = gdk_color_copy (colour);
    }
    else
    {
        GdkColor dflcolour;

        dflcolour.red = dflcolour.green = dflcolour.blue = 65535;
        envelope->colour = gdk_color_copy (&dflcolour);
    }

    g_object_set_data (G_OBJECT (pixbuf),
                       "mailtc_envelope", GUINT_TO_POINTER (1));
    gtk_image_set_from_pixbuf (GTK_IMAGE (envelope), pixbuf);

    if (priv->pixbuf)
        g_object_unref (priv->pixbuf);

    priv->pixbuf = pixbuf;
}

GdkColor*
mailtc_envelope_get_envelope_colour (MailtcEnvelope* envelope)
{
    g_return_val_if_fail (MAILTC_IS_ENVELOPE (envelope), NULL);
    g_return_val_if_fail (envelope->colour, NULL);

    return gdk_color_copy (envelope->colour);
}

GdkPixbuf*
mailtc_envelope_get_pixbuf (MailtcEnvelope* envelope)
{
    GdkPixbuf* pixbuf = NULL;

    g_return_val_if_fail (MAILTC_IS_ENVELOPE (envelope), NULL);

    if (gtk_image_get_storage_type (GTK_IMAGE (envelope)) == GTK_IMAGE_PIXBUF)
        pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (envelope));

    return (pixbuf);
}

static void
mailtc_envelope_set_property (GObject*      object,
                              guint         prop_id,
                              const GValue* value,
                              GParamSpec*   pspec)
{
    MailtcEnvelope* envelope;

    envelope = MAILTC_ENVELOPE (object);

    switch (prop_id)
    {
        case PROP_ENVELOPE_COLOUR:
            mailtc_envelope_set_envelope_colour (envelope, g_value_get_boxed (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_envelope_get_property (GObject*    object,
                              guint       prop_id,
                              GValue*     value,
                              GParamSpec* pspec)
{
    MailtcEnvelope* envelope;

    envelope = MAILTC_ENVELOPE (object);

    switch (prop_id)
    {
        case PROP_ENVELOPE_COLOUR:
            g_value_set_boxed (value, envelope->colour);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_envelope_finalize (GObject* object)
{
    MailtcEnvelope* envelope;

    envelope = MAILTC_ENVELOPE (object);

    if (envelope->colour)
        gdk_color_free (envelope->colour);

    g_object_unref (envelope->priv->pixbuf);
    g_free (envelope->priv->data);

    G_OBJECT_CLASS (mailtc_envelope_parent_class)->finalize (object);
}

static void
mailtc_envelope_class_init (MailtcEnvelopeClass* class)
{
    GObjectClass* gobject_class;

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = mailtc_envelope_finalize;
    gobject_class->set_property = mailtc_envelope_set_property;
    gobject_class->get_property = mailtc_envelope_get_property;

    g_object_class_install_property (gobject_class,
                                     PROP_ENVELOPE_COLOUR,
                                     g_param_spec_boxed (
                                     "envelope-colour",
                                     "Envelope Colour",
                                     "The envelope colour",
                                     GDK_TYPE_COLOR,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_type_class_add_private (class, sizeof (MailtcEnvelopePrivate));
}

static void
mailtc_envelope_init (MailtcEnvelope* envelope)
{
    envelope->priv = G_TYPE_INSTANCE_GET_PRIVATE (envelope,
                     MAILTC_TYPE_ENVELOPE, MailtcEnvelopePrivate);
    envelope->colour = NULL;
    envelope->priv->data = NULL;
    envelope->priv->pixbuf = NULL;
    envelope->priv->pixbuf_is_envelope = FALSE;

    g_signal_connect (envelope, "notify::pixbuf",
        G_CALLBACK (mailtc_envelope_notify_pixbuf_cb), NULL);

    mailtc_envelope_set_envelope_colour (envelope, NULL);
}

GtkWidget*
mailtc_envelope_new (void)
{
    return g_object_new (MAILTC_TYPE_ENVELOPE, NULL);
}

