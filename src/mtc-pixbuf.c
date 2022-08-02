/* mtc-pixbuf.c
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

#include "mtc-colour.h"
#include "mtc-pixbuf.h"
#include "mtc-util.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

#define MAILTC_PIXBUF_SET_COLOUR(pixbuf,property) \
    mailtc_object_set_colour (G_OBJECT (pixbuf), MAILTC_TYPE_PIXBUF, \
                              #property, &pixbuf->property, property)

#define MAILTC_PIXBUF_SET_OBJECT(pixbuf, property) \
    mailtc_object_set_object (G_OBJECT (pixbuf), MAILTC_TYPE_PIXBUF, \
                              #property, (GObject **) (&pixbuf->property), G_OBJECT (property))

struct _MailtcPixbuf
{
    GObject parent_instance;

    MailtcColour colour;
    GIcon* icon;
};

G_DEFINE_TYPE (MailtcPixbuf, mailtc_pixbuf, G_TYPE_OBJECT)

enum
{
    PROP_0,
    PROP_COLOUR,
    PROP_ICON
};

static void
mailtc_pixbuf_set_pixbuf_colour (GdkPixbuf*          pixbuf,
                                 const MailtcColour* colour)
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
        r = (guint8) (colour->red * 255);
        g = (guint8) (colour->green * 255);
        b = (guint8) (colour->blue * 255);
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

static void
mailtc_pixbuf_notify_colour_cb (GObject*    object,
                                GParamSpec* pspec)
{
    MailtcPixbuf* pixbuf;
    GInputStream *stream;
    GdkPixbuf* data;

    (void) pspec;
    pixbuf = MAILTC_PIXBUF (object);

    g_assert (MAILTC_IS_PIXBUF (object));

    stream = g_resources_open_stream ("/org/mailtc/icon/envelope.png", 0, NULL);
    data = gdk_pixbuf_new_from_stream (stream, NULL, NULL);
    g_object_unref (stream);

    g_assert (data);
    mailtc_pixbuf_set_pixbuf_colour (data, &pixbuf->colour);

    if (pixbuf->icon)
        g_object_unref (pixbuf->icon);

    pixbuf->icon = G_ICON (data);
}

static void
mailtc_pixbuf_set_icon (MailtcPixbuf* pixbuf,
                        GIcon*        icon)
{
    MAILTC_PIXBUF_SET_OBJECT (pixbuf, icon);
}

GIcon*
mailtc_pixbuf_get_icon (MailtcPixbuf* pixbuf)
{
    g_assert (MAILTC_IS_PIXBUF (pixbuf));

    return pixbuf->icon ? g_object_ref (pixbuf->icon) : NULL;
}

void
mailtc_pixbuf_set_colour (MailtcPixbuf*       pixbuf,
                          const MailtcColour* colour)
{
    MAILTC_PIXBUF_SET_COLOUR (pixbuf, colour);
}

static void
mailtc_pixbuf_get_colour (MailtcPixbuf* pixbuf,
                          MailtcColour* colour)
{
    g_assert (MAILTC_IS_PIXBUF (pixbuf));
    g_assert (colour);

    *colour = pixbuf->colour;
}

static void
mailtc_pixbuf_set_property (GObject*      object,
                            guint         prop_id,
                            const GValue* value,
                            GParamSpec*   pspec)
{
    MailtcPixbuf* pixbuf = MAILTC_PIXBUF (object);

    switch (prop_id)
    {
        case PROP_COLOUR:
            mailtc_pixbuf_set_colour (pixbuf, g_value_get_boxed (value));
            break;

        case PROP_ICON:
            mailtc_pixbuf_set_icon (pixbuf, g_value_get_object (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_pixbuf_get_property (GObject*    object,
                            guint       prop_id,
                            GValue*     value,
                            GParamSpec* pspec)
{
    MailtcColour colour;
    MailtcPixbuf* pixbuf = MAILTC_PIXBUF (object);

    switch (prop_id)
    {
        case PROP_COLOUR:
            mailtc_pixbuf_get_colour (pixbuf, &colour);
            g_value_set_boxed (value, &colour);
            break;

        case PROP_ICON:
            g_value_set_object (value, pixbuf->icon);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_pixbuf_finalize (GObject* object)
{
    MailtcPixbuf* pixbuf = MAILTC_PIXBUF (object);

    g_object_unref (pixbuf->icon);

    G_OBJECT_CLASS (mailtc_pixbuf_parent_class)->finalize (object);
}

static void
mailtc_pixbuf_class_init (MailtcPixbufClass* klass)
{
    GObjectClass* gobject_class;
    GParamFlags flags;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = mailtc_pixbuf_finalize;
    gobject_class->set_property = mailtc_pixbuf_set_property;
    gobject_class->get_property = mailtc_pixbuf_get_property;

    flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT;

    g_object_class_install_property (gobject_class,
                                     PROP_COLOUR,
                                     g_param_spec_boxed (
                                     "colour",
                                     "Colour",
                                     "The pixbuf colour",
                                     MAILTC_TYPE_COLOUR,
                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class,
                                     PROP_ICON,
                                     g_param_spec_object (
                                     "icon",
                                     "Icon",
                                     "The pixbuf icon",
                                     G_TYPE_ICON,
                                     flags));
}

static void
mailtc_pixbuf_init (MailtcPixbuf* pixbuf)
{
    pixbuf->icon = NULL;

    g_signal_connect (pixbuf, "notify::colour", G_CALLBACK (mailtc_pixbuf_notify_colour_cb), NULL);
}

MailtcPixbuf*
mailtc_pixbuf_new (void)
{
    return g_object_new (MAILTC_TYPE_PIXBUF, NULL);
}

