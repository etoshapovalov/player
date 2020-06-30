#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include <gdk/gdk.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined (GDK_WINDOWING_WIN32)
#include <gdk/gdkwin32.h>
#elif defined (GDK_WINDOWING_QUARTZ)
#include <gdk/gdkquartz.h>
#endif

#include <stdio.h>

typedef struct _PlaybackData {
    GstElement *playbin;           /* Our one and only pipeline */

    GtkWidget *slider;              /* Slider widget to keep track of current position */
    GtkWidget *streams_list;        /* Text widget to display info about the streams */
    gulong slider_update_signal_id;

    GstState state;                 /* Current state of the pipeline */
    gint64 duration;                /* Duration of the clip, in nanoseconds */

    GtkWidget* window;
} PlaybackData;

//On player initialization
static void realize_cb (GtkWidget *widget, PlaybackData *data) {
    GdkWindow *window = gtk_widget_get_window (widget);
    guintptr window_handle;

    if (!gdk_window_ensure_native (window))
        g_error ("Couldn't create native window needed for GstVideoOverlay!");

    // Retrieve window handler from GDK
    #if defined (GDK_WINDOWING_WIN32)
    window_handle = (guintptr)GDK_WINDOW_HWND (window);
    #elif defined (GDK_WINDOWING_QUARTZ)
    window_handle = gdk_quartz_window_get_nsview (window);
    #elif defined (GDK_WINDOWING_X11)
    window_handle = GDK_WINDOW_XID (window);
    #endif

    gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (data->playbin), window_handle);
}

//On video draw
static gboolean draw_cb (GtkWidget *widget, cairo_t *cr, PlaybackData *data) {
    if (data->state < GST_STATE_PAUSED) {
        GtkAllocation allocation;

        gtk_widget_get_allocation (widget, &allocation);
        cairo_set_source_rgb (cr, 0, 0, 0);
        cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
        cairo_fill (cr);
    }

    return FALSE;
}
static void delete_event_cb (GtkWidget *widget, GdkEvent *event) {
    gtk_main_quit ();
}

static void configure_event_cb (GtkWidget *widget, GdkEventConfigure *event) {

}
static void play_cb(GtkWidget *widget, PlaybackData *data){
    GstState s;
    gst_element_get_state(data->playbin, &s, NULL, GST_CLOCK_TIME_NONE);
    if(s == GST_STATE_PAUSED){
        gst_element_set_state(data->playbin, GST_STATE_PLAYING);
    } else {
        gst_element_set_state(data->playbin, GST_STATE_PAUSED);
    }

}
static void slider_cb (GtkRange *range, PlaybackData *data) {
    gdouble value = gtk_range_get_value (GTK_RANGE (data->slider));
    gst_element_seek_simple (data->playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
      (gint64)(value/2 * GST_SECOND));
}

//UI creation
static void createUi(PlaybackData *data){
    GtkWidget *main_window;
    GtkWidget *video_window;
    GtkWidget *main_box;
    GtkWidget *video_box;
    GtkWidget *controls_box;
    GtkWidget *menu_box;
    //Create window
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect (main_window, "configure-event", G_CALLBACK (configure_event_cb), NULL);
    g_signal_connect (G_OBJECT (main_window), "delete-event", G_CALLBACK (delete_event_cb), NULL);
    data->window = main_window;
    //Create boxes
    main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    video_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    controls_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    menu_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    //Create widget with video
    video_window = gtk_drawing_area_new();
    gtk_widget_set_double_buffered (video_window, FALSE);
    g_signal_connect (video_window, "realize", G_CALLBACK (realize_cb), data);
    g_signal_connect (video_window, "draw", G_CALLBACK (draw_cb), data);
    gtk_box_pack_start (GTK_BOX (video_box), video_window, TRUE, TRUE, 0);

    //Create buttons
    GtkWidget* playButton;
    playButton = gtk_button_new_from_icon_name ("media-playback-pause", GTK_ICON_SIZE_SMALL_TOOLBAR);
    g_signal_connect (G_OBJECT (playButton), "clicked", G_CALLBACK (play_cb), data);
    gtk_box_pack_start(GTK_BOX(controls_box), playButton, FALSE, FALSE, 0);

    //Create slider
    data->slider = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value (GTK_SCALE (data->slider), 0);
    data->slider_update_signal_id = g_signal_connect (G_OBJECT (data->slider), "value-changed", G_CALLBACK (slider_cb), data);
    gtk_box_pack_start(GTK_BOX(controls_box), data->slider, TRUE, TRUE, 0);

    //Create menu
    GtkWidget* menuBar = gtk_menu_bar_new();
    GtkWidget* menuItem1 = gtk_menu_item_new_with_mnemonic ("File");
    GtkWidget* submenu1 = gtk_menu_new ();
    //GtkWidget* item_open = gtk_menu_item_new_with_label ("Open...");
    GtkWidget* item_quit = gtk_menu_item_new_with_label ("Quit");
    //g_signal_connect_swapped (item_open, "activate", G_CALLBACK (open_file_cb), data);
    g_signal_connect (item_quit, "activate", G_CALLBACK (delete_event_cb), NULL);

    //gtk_menu_shell_append (GTK_MENU_SHELL (submenu1), item_open);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu1), item_quit);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuItem1), submenu1);
    gtk_menu_shell_append (GTK_MENU_SHELL (menuBar), menuItem1);
    gtk_box_pack_start (GTK_BOX (menu_box), menuBar, TRUE, TRUE, 0);

    //Combine all widgets into one window
    gtk_box_pack_start (GTK_BOX (main_box), menu_box, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (main_box), video_box, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (main_box), controls_box, FALSE, FALSE, 2);

    gtk_container_add (GTK_CONTAINER (main_window), main_box);
    gtk_window_set_default_size (GTK_WINDOW (main_window), 640, 480);

    gtk_widget_show_all (main_window);
}
static gboolean refresh_ui (PlaybackData *data) {
    gint64 current = -1;

    if (data->state == GST_STATE_PAUSED || data->state == GST_STATE_READY)
        return TRUE;

    /* If we didn't know it yet, query the stream duration */
    if (!GST_CLOCK_TIME_IS_VALID (data->duration)) {
        if (!gst_element_query_duration (data->playbin, GST_FORMAT_TIME, &(data->duration))) {
        g_printerr ("Could not query current duration.\n");
        } else {
        /* Set the range of the slider to the clip duration, in SECONDS */
        gtk_range_set_range (GTK_RANGE (data->slider), 0, ((gdouble)data->duration*2 / GST_SECOND));
        }
    }

    if (gst_element_query_position (data->playbin, GST_FORMAT_TIME, &current)) {
        g_signal_handler_block (data->slider, data->slider_update_signal_id);

        gtk_range_set_value (GTK_RANGE (data->slider), (gdouble)current*2 / GST_SECOND);

        g_signal_handler_unblock (data->slider, data->slider_update_signal_id);
    }

    return TRUE;
}
int main(int argc, char **argv){
    PlaybackData pbData;
    GstStateChangeReturn ret;
    gtk_init (&argc, &argv);
    gst_init (&argc, &argv);

    memset(&pbData, 0, sizeof(pbData));
    pbData.playbin = gst_element_factory_make ("playbin", "playbin");

    if (!pbData.playbin) {
        return -1;
    }
    char str[256];
    strcpy(str, "file://");
    strcat(str, argv[1]);

    g_object_set (pbData.playbin, "uri", str, NULL);
    createUi(&pbData);

    ret = gst_element_set_state (pbData.playbin, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (pbData.playbin);
        return -1;
    }

    g_timeout_add_seconds (1, (GSourceFunc)refresh_ui, &pbData);

    gtk_main ();

    gst_element_set_state (pbData.playbin, GST_STATE_NULL);
    gst_object_unref (pbData.playbin);
    return 0;

}