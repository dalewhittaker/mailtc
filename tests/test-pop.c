/* test-pop.c
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


/* NOTE - This test must be run with appropriate permissions. */

#include <string.h> /* strlen () */
#include <stdlib.h> /* atoi () */
#include <glib.h>
#include <glib/gstdio.h>
#include <config.h>

#include "mtc-account.h"
#include "mtc-extension.h"
#include "mtc-module.h"

#define CONFIGDIR ".config"
#define POP_PORT 110
#define POPS_PORT 995
#define TIMEOUT_NET 5
#define TIMEOUT_FREQ 100

#define POP_COMMAND_USER     0
#define POP_COMMAND_PASSWORD 1
#define POP_COMMAND_STAT     2
#define POP_COMMAND_UIDL     3
#define POP_COMMAND_QUIT     4

typedef struct
{
    GArray* protocols;
    guint command;
    guint protocol;
    gint nuids;
    char buf[512];
    GString* msg;
    GSocketService* service;
    GIOStream* connection;
    GInputStream* istream;
    GOutputStream* ostream;
    GMainLoop* loop;
    GThread* thread;
    MailtcExtension* extension;
} server_data;

static void
server_read (GInputStream* istream,
             server_data*  data);

static void
server_write (GOutputStream* ostream,
              gchar*         buf,
              server_data*   data);

static gpointer
run_plugin_thread (server_data* data)
{
    MailtcAccount* account;
    MailtcExtension* extension;
    MailtcProtocol* protocol;
    gint messages = 0;
    GError* error = NULL;

    g_assert (data);

    extension = data->extension;
    g_assert (MAILTC_IS_EXTENSION (extension));

    protocol = &g_array_index (data->protocols, MailtcProtocol, data->protocol);

    account = mailtc_account_new ();
    mailtc_account_set_protocol (account, data->protocol);
    mailtc_account_set_name (account, "test");
    mailtc_account_set_server (account, "localhost");
    mailtc_account_set_port (account, protocol->port);
    mailtc_account_set_user (account, "testuser");
    mailtc_account_set_password (account, "abc123");
    g_assert (mailtc_account_update_extension (account, extension, &error));

    messages = mailtc_extension_get_messages (extension, G_OBJECT (account), TRUE, &error);
    g_assert_no_error (error);
    g_print ("messages = %d\n", messages);

    /* FIXME we'll want to mark as read too. */

    g_object_unref (account);

    return NULL;
}

static gboolean
run_plugin (server_data* data)
{
    data->thread = g_thread_create ((GThreadFunc) run_plugin_thread, data, TRUE, NULL);
    return FALSE;
}

static void
server_close_stream_finish (GObject*      object,
                            GAsyncResult* res,
                            server_data*  data)
{
    GError* error = NULL;

    if (G_IS_INPUT_STREAM (object))
    {
        g_input_stream_close_finish (G_INPUT_STREAM (object), res, &error);
        data->istream = NULL;
    }
    else
    {
        g_assert (G_IS_OUTPUT_STREAM (object));
        g_output_stream_close_finish (G_OUTPUT_STREAM (object), res, &error);
        data->ostream = NULL;
    }
    g_assert_no_error (error);

    if (!data->istream && !data->ostream)
    {
        if (++data->protocol < data->protocols->len)
        {
            g_object_unref (data->connection);
            g_idle_add ((GSourceFunc) run_plugin, data);
        }
        else
        {
            GIOStream* base_connection;

            g_object_get (data->connection, "base-io-stream", &base_connection, NULL);
            g_object_unref (data->connection);
            g_object_unref (base_connection);
            g_thread_join (data->thread);
            g_main_loop_quit (data->loop);
        }
    }
}

static void
server_read_write_finish (GObject*      stream,
                          GAsyncResult* res,
                          server_data*  data)
{
    GString* s;
    gchar** sv;
    guint svlen;
    gssize read;
    guint command;
    GError* error = NULL;

    if (G_IS_OUTPUT_STREAM (stream))
    {
        g_output_stream_write_finish (G_OUTPUT_STREAM (stream), res, &error);
        g_assert_no_error (error);

        server_read (data->istream, data);

        if (data->command > 4)
            g_output_stream_close_async (G_OUTPUT_STREAM (stream), G_PRIORITY_DEFAULT, NULL, (GAsyncReadyCallback) server_close_stream_finish, data);

        return;
    }

    g_assert (G_IS_INPUT_STREAM (stream));
    read = g_input_stream_read_finish (G_INPUT_STREAM (stream), res, &error);
    g_assert_no_error (error);
    data->buf[read] = '\0';

    s = data->msg;
    command = data->command;
    data->command++;

    /* FIXME want to simulate server blocking errors at each point.
     * and also -ERR at each point.
     */
    switch (command)
    {
        case POP_COMMAND_USER:
            server_write (data->ostream, "+OK Give me a password,\r\n", data);
            break;

        case POP_COMMAND_PASSWORD:
            server_write (data->ostream, "+OK We are good.\r\n", data);
            break;

        case POP_COMMAND_STAT:
            /* The plugin doesn't use the maildrop size param, so we can just use 1. */
            data->nuids = g_random_int_range (1, 5);
            g_string_printf (s, "+OK %d 1.\r\n", data->nuids);
            server_write (data->ostream, s->str, data);
            break;

        case POP_COMMAND_UIDL:
            sv = g_strsplit_set (data->buf, " \r\n", 0);
            svlen = g_strv_length (sv);
            g_assert (svlen == 4 && strlen (sv[2]) == 0 && strlen (sv[3]) == 0);
            if (atoi (sv[1]) < data->nuids)
                data->command = POP_COMMAND_UIDL;
            g_string_printf (s, "+OK MTC-%s\r\n", sv[1]);
            g_strfreev (sv);
            server_write (data->ostream, s->str, data);
            break;

        case POP_COMMAND_QUIT:
            server_write (data->ostream, "+OK See ya!.\r\n", data);
            break;

        default:
            g_input_stream_close_async (G_INPUT_STREAM (stream), G_PRIORITY_DEFAULT, NULL, (GAsyncReadyCallback) server_close_stream_finish, data);
    }
}

static void
server_read (GInputStream* istream,
             server_data*  data)
{
    g_input_stream_read_async (istream, data->buf, sizeof (data->buf), G_PRIORITY_DEFAULT, NULL, (GAsyncReadyCallback) server_read_write_finish, data);
}

static void
server_write (GOutputStream* ostream,
              gchar*         buf,
              server_data*   data)
{
    gsize bytes;

    bytes = strlen (buf);
    g_output_stream_write_async (ostream, buf, bytes, G_PRIORITY_DEFAULT, NULL, (GAsyncReadyCallback) server_read_write_finish, data);
}

static gboolean
service_incoming_cb (GSocketService*    service,
                     GSocketConnection* connection,
                     GObject*           source_object,
                     server_data*       data)
{
    GIOStream* conn;

    (void) service;
    (void) source_object;

    if (data->protocol == 1)
    {
        GTlsCertificate* cert;
        GError* error = NULL;

        /* server.pem was a file generated as follows:
         * openssl genrsa -out key.pem 1024
         * openssl req -new -x509 -key.pem -out server.pem -days 1095
         * cat key.pem >> server.pem
         * FIXME autogenerate server.pem from either here or Makefile.am
         */
        cert = g_tls_certificate_new_from_file (SRCDIR "/server.pem", &error);
        g_assert_no_error (error);
        conn = g_tls_server_connection_new (G_IO_STREAM (connection), cert, &error);
        g_assert_no_error (error);
        g_object_unref (cert);
        g_tls_connection_set_require_close_notify (G_TLS_CONNECTION (conn), FALSE);
        g_tls_connection_handshake (G_TLS_CONNECTION (conn), NULL, &error);
        g_assert_no_error (error);
    }
    else
        conn = G_IO_STREAM (connection);

    g_object_ref (conn);
    data->connection = conn;
    data->msg = g_string_new (NULL);
    data->command = 0;
    data->istream = g_io_stream_get_input_stream (conn);
    data->ostream = g_io_stream_get_output_stream (conn);
    g_print ("\n");
    server_write (data->ostream, "+OK MTC test server\r\n", data);

    return TRUE;
}

static void
test_pop (void)
{
    MailtcModule* module;
    MailtcExtension* extension;
    MailtcProtocol* protocol;
    MailtcExtensionInitFunc extension_init;
    GMainLoop* loop = NULL;
    GInetAddress* address;
    GSocketAddress* sock_address;
    GSocketService* service;
    GSList* extensions;
    GArray* protocols;
    GDir* dir;
    GError* error = NULL;
    guint i;
    const gchar* filename;
    gchar* fullname;
    gchar* directory;
    gchar* edirectory;
    server_data data;

    dir = g_dir_open (PLUGINDIR, 0, &error);
    g_assert (dir);

    /* Find the pop plugin. */
    while ((filename = g_dir_read_name (dir)))
    {
        if (g_str_has_prefix (filename, "net") && g_str_has_suffix (filename, G_MODULE_SUFFIX))
            break;
    }
    g_assert (filename);

    /* Load it. */
    fullname = g_build_filename (PLUGINDIR, filename, NULL);
    g_assert (fullname);
    directory = g_build_filename (CONFIGDIR, PACKAGE, NULL);
    g_assert (directory);
    edirectory = g_build_filename (directory, filename, NULL);
    g_assert (edirectory);
    g_mkdir_with_parents (edirectory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    module = mailtc_module_new ();
    g_assert (module);
    g_assert (mailtc_module_load (module, fullname, &error));
    g_assert (mailtc_module_symbol (module, MAILTC_EXTENSION_SYMBOL_INIT, (gpointer*) &extension_init, &error));
    g_assert (extension_init);

    g_type_class_ref (MAILTC_TYPE_EXTENSION);

    extensions = extension_init (directory);

    /* FIXME assumes one extension. */
    extension = extensions->data;
    g_assert (extension);
    g_assert (mailtc_extension_is_valid (extension, NULL));
    protocols = mailtc_extension_get_protocols (extension);

    service = g_socket_service_new ();
    address = g_inet_address_new_from_string ("127.0.0.1");
    for (i = 0; i < protocols->len; i++)
    {
        protocol = &g_array_index (protocols, MailtcProtocol, i);
        sock_address = g_inet_socket_address_new (address, protocol->port);
        g_socket_listener_add_address (G_SOCKET_LISTENER (service), sock_address, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, NULL, NULL, &error);
        g_assert_no_error (error);
        g_object_unref (sock_address);
    }
    g_object_unref (address);

    g_signal_connect (service, "incoming", G_CALLBACK (service_incoming_cb), &data);

    g_socket_service_start (service);

    mailtc_extension_set_module (extension, G_OBJECT (module));

    loop = g_main_loop_new (NULL, FALSE);
    g_assert (loop);
    data.loop = loop;
    data.protocols = protocols;
    data.protocol = 0;
    data.extension = extension;
    g_array_unref (protocols);

    g_idle_add ((GSourceFunc) run_plugin, &data);
    g_main_loop_run (loop);

    g_main_loop_unref (loop);

    if (data.msg)
        g_string_free (data.msg, TRUE);

    g_slist_free (extensions);
    g_assert (mailtc_module_unload (module, &error));
    g_assert (g_remove (edirectory) == 0);
    g_assert (g_remove (directory) == 0);
    g_assert (g_remove (CONFIGDIR) == 0);

    g_object_unref (module);
    g_free (directory);
    g_free (fullname);
    g_dir_close (dir);

    g_socket_service_stop (service);
    g_object_unref (service);

    g_print ("done\n");
}

int
main (int    argc,
      char** argv)
{
    g_type_init ();

#if 1
    g_test_init (&argc, &argv, NULL);
    g_test_add_func ("/plugin/pop", test_pop);

    return g_test_run ();
#else
    (void) argc;
    (void) argv;
    test_pop ();

    return 0;
#endif
}

