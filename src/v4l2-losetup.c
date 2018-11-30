#include "common.h"
#include "decoder.h"
#include <signal.h>
#include <gtk/gtk.h>

socket_t video_socket = INVALID_SOCKET;

static void sigint_handle(int signo)
{
    if (signo == SIGINT) {
        disconnect(video_socket);
        decoder_cleanup();
        exit(EXIT_SUCCESS);
    }
}

static void modprobe(void)
{
    int status;
    const char *cmd[] = { "modprobe", "v4l2_loopback", NULL };

    pid_t wid, pid = fork();

    if (!pid) execvp(cmd[0], (char *const *)cmd);

    while ((wid = wait(&status)) > 0);
}

static int daemonize(void)
{
    int fdmax = (1UL << 10);
    pid_t pid = fork();

    if (pid != 0) return EXIT_SUCCESS;

    (setsid() < 0) ? exit(EXIT_FAILURE) : assert(1);

    pid = fork();

    (pid != 0) ? exit(EXIT_SUCCESS) : assert(1);

    umask(000);
    chdir("/");
    for (; fdmax >= 0; fdmax--)
        close(fdmax);

    pid = fork();

    (pid != 0) ? exit(EXIT_SUCCESS) : assert(1);

    return TRUE;
}

static void stream_video(char *ip, int port)
{
    pid_t pid = getpid();
    char spid[16];

    snprintf(spid, 16, "%d", pid);
    FILE *pidfile = fopen("/run/v4l2-losetup.pid", "w+");
    fputs(spid, pidfile);
    fflush(pidfile);
    fclose(pidfile);

    signal(SIGINT, sigint_handle);

    char buf[32];

    video_socket = connect_cam(ip, port);
    if (video_socket == INVALID_SOCKET)
        return;

    {
        int len = snprintf(buf, sizeof(buf), VIDEO_REQ, decoder_get_video_width(), decoder_get_video_height());
        if (iostream(1, buf, len, video_socket) <= 0) {
            errprint("Error sending request, Stream might be busy with another client.");
            goto _out;
        }
    }

    memset(buf, 0, sizeof(buf));
    if (iostream(0, buf, 5, video_socket) <= 0) {
        errprint("Connection reset by app!\nStream is probably busy with another client");
        goto _out;
    }

    if (decoder_prepare_video(buf) == FALSE)
        goto _out;

    while (1) {
        int frameLen;
        struct jpg_frame_s *f = decoder_get_next_frame();
        if (iostream(0, buf, 4, video_socket) == FALSE) break;
        make_int4(frameLen, buf[0], buf[1], buf[2], buf[3]);
        f->length = frameLen;
        char *p = (char *)f->data;
        while (frameLen > 4096) {
            if (iostream(0, p, 4096, video_socket) == FALSE)
                goto _out;
            frameLen -= 4096;
            p += 4096;
        }
        if (iostream(0, p, frameLen, video_socket) == FALSE) break;
    }

_out:
    disconnect(video_socket);
    decoder_cleanup();
    return;
}

static int save_config(const char *input)
{
    FILE *cfile;
    char *cpath = getenv("XDG_CONFIG_HOME");

    if (NULL == cpath) {
        cpath = getenv("HOME");
        g_strlcat(cpath, "/.config", PATH_MAX);
    }
    chdir(cpath);
    cfile = fopen("v4l2-loopback.conf", "w+");
    if (NULL == cfile) {
        return EXIT_FAILURE;
    } else {
        fputs(input, cfile);
        fflush(cfile);
        fclose(cfile);
        return EXIT_SUCCESS;
    }
}

static char *load_config(void)
{
    FILE *cfile;
    char *cpath = getenv("XDG_CONFIG_HOME");

    if (NULL == cpath) {
        cpath = getenv("HOME");
        g_strlcat(cpath, "/.config", PATH_MAX);
    }
    chdir(cpath);
    cfile = fopen("v4l2-loopback.conf", "r");
    if (NULL == cfile) {
        return strdup("<IP>:<PORT>");
    } else {
        char buf[PATH_MAX];
        fgets(buf, PATH_MAX, cfile);
        fclose(cfile);
        return strdup(buf);
    }
}

static void clicked_connect(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *entry = (GtkWidget *)user_data;
    const char *input = gtk_entry_get_text(GTK_ENTRY(entry));

    if (save_config(input))
        fprintf(stderr, "could not save configuration !\n");

    char *ip = strdup(input);
    char *tr = strchr(ip, ':');
    *tr = 0;
    int port = atoi((tr + 1));


    if (daemonize()) {
        modprobe();
        decoder_init();
        stream_video(ip, port);
        decoder_fini();
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_SUCCESS);
}

static void clicked_disconnect(GtkWidget *widget, gpointer user_data)
{
    FILE *pidfile = fopen("/run/v4l2-losetup.pid", "r");

    if (pidfile) {
        char spid[16];

        fread(spid, 16, 1, pidfile);
        fflush(pidfile);
        fclose(pidfile);
        kill(atoi(spid), SIGINT);
        unlink("/run/v4l2-losetup.pid");
    }
    exit(EXIT_SUCCESS);
}


static void v4l2_losetup(GtkApplication *app, gpointer user_data)
{
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *button_connect;
    GtkWidget *button_disconnect;
    GtkWidget *entry;
    GtkEntryBuffer *buffer = gtk_entry_buffer_new(NULL, 0);

    window = gtk_application_window_new(app);
    gtk_window_set_default_size(GTK_WINDOW(window), 100, 50);
    gtk_window_set_title(GTK_WINDOW(window), "V4L2-Loopback Setup");
    gtk_container_set_border_width(GTK_CONTAINER(window), 1);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    gtk_box_set_homogeneous(GTK_BOX(vbox), TRUE);
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(vbox));

    entry = gtk_entry_new_with_buffer(buffer);

    gtk_entry_set_text(GTK_ENTRY(entry), load_config());
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(entry), TRUE, TRUE, 5);

    button_connect = gtk_button_new_with_label("Connect");
    g_signal_connect(button_connect, "clicked", G_CALLBACK(clicked_connect), (void *)entry);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(button_connect), TRUE, TRUE, 5);

    button_disconnect = gtk_button_new_with_label("Disonnect");
    g_signal_connect(button_disconnect, "clicked", G_CALLBACK(clicked_disconnect), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(button_disconnect), TRUE, TRUE, 5);

    gtk_widget_show_all(GTK_WIDGET(window));
}

int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new("v4l2.losetup.bin", G_APPLICATION_FLAGS_NONE);

    g_signal_connect(app, "activate", G_CALLBACK(v4l2_losetup), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}
