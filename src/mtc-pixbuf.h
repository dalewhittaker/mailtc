/* mtc-pixbuf.h
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

#ifndef __MAILTC_PIXBUF_H__
#define __MAILTC_PIXBUF_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_PIXBUF (mailtc_pixbuf_get_type ())

G_DECLARE_FINAL_TYPE       (MailtcPixbuf, mailtc_pixbuf, MAILTC, PIXBUF, GObject)

MailtcPixbuf*
mailtc_pixbuf_new          (void);

void
mailtc_pixbuf_set_colour   (MailtcPixbuf*       pixbuf,
                            const MailtcColour* colour);

GIcon*
mailtc_pixbuf_get_icon     (MailtcPixbuf*       pixbuf);

G_END_DECLS

#endif /* __MAILTC_PIXBUF_H__ */

