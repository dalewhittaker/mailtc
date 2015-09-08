/* mtc-colour.h
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

#ifndef __MAILTC_COLOUR_H__
#define __MAILTC_COLOUR_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_COLOUR (mailtc_colour_get_type ())

struct _MailtcColour
{
    gdouble red;
    gdouble green;
    gdouble blue;
};

typedef struct _MailtcColour MailtcColour;

GType
mailtc_colour_get_type    (void);

MailtcColour*
mailtc_colour_copy        (const MailtcColour* colour);

void
mailtc_colour_free        (MailtcColour*       colour);

gboolean
mailtc_colour_parse       (MailtcColour*       colour,
                           const gchar*        str);

gchar*
mailtc_colour_to_string   (const MailtcColour* colour);

gboolean
mailtc_colour_equal       (const MailtcColour* a,
                           const MailtcColour* b);

G_END_DECLS

#endif /* __MAILTC_COLOUR_H__ */

