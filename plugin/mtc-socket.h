/* mtc-socket.h
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

#ifndef __MAILTC_SOCKET_H__
#define __MAILTC_SOCKET_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_SOCKET (mailtc_socket_get_type  ())

G_DECLARE_FINAL_TYPE       (MailtcSocket, mailtc_socket, MAILTC, SOCKET, GObject)

gboolean
mailtc_socket_supports_tls (void);

void
mailtc_socket_set_tls      (MailtcSocket* sock,
                            gboolean      tls);

gboolean
mailtc_socket_connect      (MailtcSocket* sock,
                            const gchar*  server,
                            guint         port,
                            GError**      error);

void
mailtc_socket_disconnect   (MailtcSocket* sock);

gssize
mailtc_socket_read         (MailtcSocket* sock,
                            gchar*        buf,
                            gsize         len,
                            GError**      error);

gssize
mailtc_socket_write        (MailtcSocket* sock,
                            const gchar*  buf,
                            gsize         len,
                            GError**      error);

MailtcSocket*
mailtc_socket_new          (void);

G_END_DECLS

#endif /* __MAILTC_SOCKET_H__ */

