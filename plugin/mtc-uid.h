/* mtc-uid.h
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

#ifndef __MAILTC_UID_H__
#define __MAILTC_UID_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_UID_TABLE (mailtc_uid_table_get_type  ())

G_DECLARE_FINAL_TYPE          (MailtcUidTable, mailtc_uid_table, MAILTC, UID_TABLE, GObject)

gboolean
mailtc_uid_table_load         (MailtcUidTable* uid_table,
                               GError**        error);

void
mailtc_uid_table_age          (MailtcUidTable* uid_table);

gint64
mailtc_uid_table_remove_old   (MailtcUidTable* uid_table);

void
mailtc_uid_table_add          (MailtcUidTable* uid_table,
                               gchar*          uid);

gboolean
mailtc_uid_table_mark_read    (MailtcUidTable* uid_table,
                               GError**        error);

MailtcUidTable*
mailtc_uid_table_new          (gchar* filename);

G_END_DECLS

#endif /* __MAILTC_UID_H__ */

