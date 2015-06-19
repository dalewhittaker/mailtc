/* mtc-pixbuf.h
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

#ifndef __MAILTC_PIXBUF_H__
#define __MAILTC_PIXBUF_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_PIXBUF            (mailtc_pixbuf_get_type ())
#define MAILTC_PIXBUF(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAILTC_TYPE_PIXBUF, MailtcPixbuf))
#define MAILTC_PIXBUF_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MAILTC_TYPE_PIXBUF, MailtcPixbufClass))
#define MAILTC_IS_PIXBUF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAILTC_TYPE_PIXBUF))
#define MAILTC_IS_PIXBUF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MAILTC_TYPE_PIXBUF))
#define MAILTC_PIXBUF_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MAILTC_TYPE_PIXBUF, MailtcPixbufClass))

typedef struct _MailtcPixbuf        MailtcPixbuf;
typedef struct _MailtcPixbufClass   MailtcPixbufClass;

GType
mailtc_pixbuf_get_type   (void);

MailtcPixbuf*
mailtc_pixbuf_new        (void);

void
mailtc_pixbuf_set_colour (MailtcPixbuf*       pixbuf,
                          const MailtcColour* colour);

GIcon*
mailtc_pixbuf_get_icon   (MailtcPixbuf*       pixbuf);

G_END_DECLS

#endif /* __MAILTC_PIXBUF_H__ */

