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
#include "mtc.h"

#define CONFIGDIR ".config"
#define POP_PORT 110
#define POPS_PORT 995
#define TIMEOUT_NET 5
#define TIMEOUT_FREQ 100

typedef struct
{
    guint command;
    guint protocol;
    guint port;
    GString* msg;
    GSocketConnection* connection;
    GMainLoop* loop;
    GThread* thread;
} server_data;

static gboolean
server_write (GIOChannel* channel,
              gchar*      data)
{
    GIOStatus status;
    gsize bytes;
    gsize written;

    bytes = strlen (data);
    status =  g_io_channel_write_chars (channel, data, bytes, &written, NULL);
    if (status != G_IO_STATUS_NORMAL || bytes != written)
        return FALSE;
    status =  g_io_channel_flush (channel, NULL);

    return (status == G_IO_STATUS_NORMAL) ? TRUE : FALSE;
}

static gboolean
server_read (GIOChannel*  channel,
             GIOCondition condition,
             server_data* data)
{
    GString* s;
    gchar** sv;
    guint svlen;
    GIOStatus ret;
    guint command;
    GError* error = NULL;

    (void) condition;

    /* FIXME proper error handles */
    s = data->msg;
    ret = g_io_channel_read_line_string (channel, s, NULL, &error);

    if (ret == G_IO_STATUS_EOF)
    {
        g_string_free (s, TRUE);
        return FALSE;
    }
    if (ret == G_IO_STATUS_ERROR)
    {
        g_print ("%s\n", error->message);
        g_test_message ("%s\n", error->message);
        g_clear_error (&error);
        g_object_unref (data->connection);
        g_string_free (s, TRUE);
        return FALSE;
    }

    command = data->command;
    data->command++;

    switch (command)
    {
        /* FIXME proper checks */
        /* FIXME much tidying */
        case 0:
            if (!server_write (channel, "+OK Give me a password,\r\n"))
                return FALSE;
            break;
        case 1:
            if (!server_write (channel, "+OK We are good.\r\n"))
                return FALSE;
            break;
        case 2:
            /* The plugin doesn't use the maildrop size param, so we can just use 1. */
            /* FIXME use a random value for no. of messages? */
            if (!server_write (channel, "+OK 3 1.\r\n"))
                return FALSE;
            break;
        case 3:
            sv = g_strsplit_set (s->str, " \r\n", 0);
            svlen = g_strv_length (sv);
            if (svlen != 4 || strlen (sv[2]) != 0 || strlen (sv[3]) != 0)
                return FALSE;
            if (atoi (sv[1]) < 3)
                data->command = command;
            g_string_printf (s, "+OK MTC-%s\r\n", sv[1]);
            g_strfreev (sv);
            if (!server_write (channel, s->str))
                return FALSE;
            break;
        case 4:
            if (!server_write (channel, "+OK See ya!.\r\n"))
                return FALSE;
            break;
        default:
            return FALSE;
    }
    /* FIXME cleanups etc...*/
    return TRUE;
}

static void
destroy_channel (server_data* data)
{
    g_thread_join (data->thread);
    g_main_loop_quit (data->loop);
}

static gboolean
service_incoming_cb (GSocketService*    service,
                     GSocketConnection* connection,
                     GObject*           source_object,
                     server_data*       data)
{
    GSocket* socket;
    GIOChannel* channel;
    gboolean success;
    gint fd;

    (void) service;
    (void) source_object;

    g_object_ref (connection);

    socket = g_socket_connection_get_socket (connection);
    fd = g_socket_get_fd (socket);
    channel = g_io_channel_unix_new (fd);

    /*data = g_new (server_data, 1);*/
    data->connection = connection;
    data->msg = g_string_new (NULL);
    data->command = 0;

    success = server_write (channel, "+OK MTC test server\r\n");
    g_assert (success);
    g_io_add_watch_full (channel, G_PRIORITY_DEFAULT, G_IO_IN, (GIOFunc) server_read, data, (GDestroyNotify) destroy_channel);

    return TRUE;
}

static gpointer
run_plugin_thread (server_data* data)
{
    mtc_account* account;
    mtc_plugin* plugin;
    mtc_plugin* (*plugin_init) (void);
    GDir* dir;
    gchar* directory;
    const gchar* filename;
    gchar* fullname;
    GModule* module;
    gboolean success;
    gint messages;
    GError* error = NULL;

    dir = g_dir_open (PLUGINDIR, 0, &error);
    g_assert (dir);

    /* Find the pop plugin. */
    while ((filename = g_dir_read_name (dir)))
    {
        if (g_str_has_prefix (filename, "pop") && g_str_has_suffix (filename, G_MODULE_SUFFIX))
            break;
    }

    g_assert (filename);

    /* Load it. */
    fullname = g_build_filename (PLUGINDIR, filename, NULL);
    g_assert (fullname);

    module = g_module_open (fullname, G_MODULE_BIND_LOCAL);
    g_free (fullname);
    g_assert (module);

    success = g_module_symbol (module, "plugin_init", (gpointer) &plugin_init);
    g_assert (success);
    g_assert (plugin_init);

    plugin = plugin_init ();
    g_assert (plugin);
    g_assert (g_str_equal (VERSION, plugin->compatibility));
    g_assert (data->protocol < plugin->protocols->len);

    plugin->module = module;

    account = g_new0 (mtc_account, 1);
    account->protocol = data->protocol;
    account->plugin = plugin;
    account->name = "test";
    account->server = "localhost";
    account->port = data->port;
    account->user = "testuser";
    account->password = "abc123";

    directory = g_build_filename (CONFIGDIR, PACKAGE, NULL);
    g_assert (directory);
    plugin->directory = g_build_filename (directory, filename, NULL);
    g_assert (plugin->directory);
    g_mkdir_with_parents (plugin->directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    g_dir_close (dir);

    g_assert (plugin->add_account);
    success = (*plugin->add_account) (account, &error);
    g_assert (success);

    g_assert (plugin->get_messages);
    messages = (*plugin->get_messages) (account, TRUE, &error);
    g_print ("messages = %d\n", messages);
    if (messages < 0)
    {
        g_print ("2: %s\n", error->message);
        g_test_message ("%s\n", error->message);
        g_clear_error (&error);
        g_assert (messages >= 0);
    }

    /* FIXME we'll want to mark as read too. */
    g_assert (plugin->terminate);
    (*plugin->terminate) (plugin);

    g_assert (g_remove (plugin->directory) == 0);
    g_assert (g_remove (directory) == 0);
    g_assert (g_remove (CONFIGDIR) == 0);

    success = g_module_close (module);
    g_assert (success);

    g_free (account);
    g_array_free (plugin->protocols, TRUE);
    g_free (plugin->directory);
    g_free (plugin);

    g_free (directory);

    return NULL;
}

static gboolean
run_plugin (server_data* data)
{
    data->thread = g_thread_create ((GThreadFunc) run_plugin_thread, data, TRUE, NULL);
    return FALSE;
}

static void
test_pop_protocol (guint protocol)
{
    GMainLoop* loop = NULL;
    GSocketService* service;
    GInetAddress* address;
    GSocketAddress* sock_address;
    GError* error = NULL;
    guint ports[] = { POP_PORT, POPS_PORT };
    guint port;
    server_data data;

    port = ports[protocol];
    g_print ("\n");
    service = g_socket_service_new ();
    address = g_inet_address_new_from_string ("127.0.0.1");
    sock_address = g_inet_socket_address_new (address, port);
    if (!g_socket_listener_add_address (G_SOCKET_LISTENER (service), sock_address,
            G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, NULL, NULL, &error))
    {
        g_print ("%s\n", error->message);
        g_test_message ("%s\n", error->message);
        g_clear_error (&error);
    }
    g_object_unref (sock_address);
    g_object_unref (address);

    g_signal_connect (service, "incoming", G_CALLBACK (service_incoming_cb), &data);

    g_socket_service_start (service);

    loop = g_main_loop_new (NULL, FALSE);
    g_assert (loop);
    data.loop = loop;
    data.protocol = protocol;
    data.port = port;

    g_idle_add ((GSourceFunc) run_plugin, &data);
    g_main_loop_run (loop);
    g_main_loop_unref (loop);

    g_socket_service_stop (service);
}

static void
test_pop (void)
{
    /* FIXME initialise the plugin here... */
    test_pop_protocol (0);
    /*test_pop_protocol (1);*/
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

