/* define that we are a plugin. :) */
#ifndef PURPLE_PLUGINS
#define PURPLE_PLUGINS
#endif

/* gnu libc includes. */
#include <string.h>

/* purple and pidgin includes. */
#include "notify.h"
#include "plugin.h"
#include "version.h"
#include "gtkplugin.h"
#include "gtkutils.h"

/* xosd includes. */
#include "xosd.h"

/* pidgin-osd configuration includes. */
#include "config.h"

/* define an easy to use constant for the buddy name. */
#define PURPLE_BUDDY_DISP_NAME(buddy) ((buddy)->alias ? (buddy)->alias : ((buddy)->server_alias ? (buddy)->server_alias : (buddy)->name));

/* some global properties first used in osd_get_config_frame() */
GtkWidget	*g_osd_font_name	= NULL;
GtkWidget	*g_osd_font_size	= NULL;
GtkWidget	*g_osd_color		= NULL;
GtkWidget	*g_osd_align		= NULL;
GtkWidget	*g_osd_xoffset		= NULL;
GtkWidget	*g_osd_pos		= NULL;
GtkWidget	*g_osd_yoffset		= NULL;
GtkWidget	*g_osd_timeout		= NULL;
GtkWidget	*g_osd_shadow		= NULL;
GtkWidget	*g_osd_lines		= NULL;
GtkWidget	*g_osd_msgs		= NULL;
GtkTooltips	*g_tooltips		= NULL;


/* options to customize. */
static const char	*osd_color		= "#FFF000";
static const char	*osd_font_name		= "-misc-fixed-iso8859-15";
static const char	*osd_font_size		= "12";
static xosd_align	osd_align		= XOSD_left;
static xosd_pos		osd_pos			= XOSD_top;
static int		osd_timeout		= 15;
static int		osd_shadow		= 1;
static int		osd_lines		= 1;
static int		osd_xoffset		= 20;
static int		osd_yoffset		= 20;
static int		osd_msgs		= 0;

/* the xosd information. */
static PurplePlugin	*my_plugin		= NULL;
xosd			*osd			= NULL;
time_t			last_print		= 0;
static char		**x_font_names		= NULL;
static char		*x_font_sizes[]		= {
				"8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
				"24", "26", "28", "32", "36", "40", "48", "56", "64", "72", NULL
			};

/* this function fills the x_font_names variable. */
static void osd_fill_fonts() {

	/* common variables. */
	Display *disp;
	char **fnts;
	int cnt = 0;
	int i;
	int i2;

	/* try to open the x display. */
	disp = XOpenDisplay(NULL);

	/* check if display is available. */
	if (disp) {
		fnts = XListFonts(disp, "*", INT_MAX, &cnt);
		if (fnts && cnt) {

			/* fill available fonts. */
			x_font_names = (char **)g_malloc((cnt + 1) * sizeof(char *));
			x_font_names[cnt] = NULL;
			for (i = 0; i < cnt; i++) {
				char buffer [1000];
				char *ptr;
				char *old_ptr;

				x_font_names[i] = (char *)1;
				ptr = strchr(fnts[i] + 1, '-');

				/* check if font is valied. */
				if (!ptr) {
					continue;
				}
				ptr = strchr(ptr + 1, '-');

				/* check if font is valid. */
				if (!ptr) {
					continue;
				}
				strncpy(buffer, fnts[i] + 1, ptr - fnts[i] - 1);
				buffer[ptr - fnts[i] - 1] = 0;
				old_ptr = ptr = strrchr(fnts[i], '-');

				/* check if font is valid. */
				if (!ptr) {
					continue;
				}
				*old_ptr = 0;
				ptr = strrchr(fnts[i], '-');
				*old_ptr = '-';

				/* check if font is valid. */
				if (!ptr) {
					continue;
				}
				strcat(buffer, ptr);

				/* loop through already available fonts and check for duplicates. */
				for (i2 = 0; i2 < i; i2++) {
					if (x_font_names[i2] != (char *)1 && !strcmp(x_font_names[i2], buffer)) {
						break;
					}
				}
				if (i2 < i) {
					continue;
				}

				/* put new font to our internal font list. */
				x_font_names[i] = g_malloc(strlen(buffer) + 1);
				strcpy(x_font_names[i], buffer);
			}

			/* free the x font pointer. */
			XFreeFontNames(fnts);
		} else {
			fprintf(stderr, "%s: no X fonts available\n", PACKAGE_NAME);
		}

		/* close the x display. */
		XCloseDisplay (disp);
	} else {
		fprintf(stderr, "%s:no X display available\n", PACKAGE_NAME);
	}
}

/* this function initializes the default preferences. */
static void osd_init_prefs(void) {

	/* set some initial preferences. */
	purple_prefs_add_none("/plugins/gtk");
	purple_prefs_add_none("/plugins/gtk/X11");
	purple_prefs_add_none("/plugins/gtk/X11/pidgin-osd");
	purple_prefs_add_int("/plugins/gtk/X11/pidgin-osd/align", (int)osd_align);
	purple_prefs_add_int("/plugins/gtk/X11/pidgin-osd/position", (int)osd_pos);
	purple_prefs_add_string("/plugins/gtk/X11/pidgin-osd/font_name", osd_font_name);
	purple_prefs_add_string("/plugins/gtk/X11/pidgin-osd/font_size", osd_font_size);
	purple_prefs_add_string("/plugins/gtk/X11/pidgin-osd/color", osd_color);
	purple_prefs_add_int("/plugins/gtk/X11/pidgin-osd/timeout", osd_timeout);
	purple_prefs_add_int("/plugins/gtk/X11/pidgin-osd/shadow", osd_shadow);
	purple_prefs_add_int("/plugins/gtk/X11/pidgin-osd/lines", osd_lines);
	purple_prefs_add_int("/plugins/gtk/X11/pidgin-osd/xoffset", osd_xoffset);
	purple_prefs_add_int("/plugins/gtk/X11/pidgin-osd/yoffset", osd_yoffset);
	purple_prefs_add_int("/plugins/gtk/X11/pidgin-osd/msgs", osd_msgs);
}

/* this function gets the osd properties. */
static void osd_get_prefs(void) {
	osd_align	= (xosd_align)purple_prefs_get_int("/plugins/gtk/X11/pidgin-osd/align");
	osd_pos		= (xosd_pos)purple_prefs_get_int("/plugins/gtk/X11/pidgin-osd/position");
	osd_font_name	= purple_prefs_get_string("/plugins/gtk/X11/pidgin-osd/font_name");
	osd_font_size	= purple_prefs_get_string("/plugins/gtk/X11/pidgin-osd/font_size");
	osd_color	= purple_prefs_get_string("/plugins/gtk/X11/pidgin-osd/color");
	osd_timeout	= purple_prefs_get_int("/plugins/gtk/X11/pidgin-osd/timeout");
	osd_shadow	= purple_prefs_get_int("/plugins/gtk/X11/pidgin-osd/shadow");
	osd_lines	= purple_prefs_get_int("/plugins/gtk/X11/pidgin-osd/lines");
	osd_xoffset	= purple_prefs_get_int("/plugins/gtk/X11/pidgin-osd/xoffset");
	osd_yoffset	= purple_prefs_get_int("/plugins/gtk/X11/pidgin-osd/yoffset");
	osd_msgs	= purple_prefs_get_int("/plugins/gtk/X11/pidgin-osd/msgs");
}

/* this function sets the osd font to use. */
static void osd_set_font() {

	/* common variables. */
	char buffer[1000];
	char *inc_point;

	/* check if we have a correct font name. */
	inc_point = strchr(osd_font_name, '-');
	if (!inc_point) {
		return;
	}

	/* check if we have a correct font name. */
	inc_point = strchr(inc_point + 1, '-');
	if (!inc_point) {
		return;
	}

	/* set the font. */
	*inc_point = 0;
	sprintf(buffer, "-%s-*-*-*-*-%s-*-*-*-*-*-%s", osd_font_name, osd_font_size, inc_point + 1);
	xosd_set_font(osd, buffer);
	*inc_point = '-';
}

/* this function initialize the osd. */
static void osd_init() {

	/* initialize osd only if osd already exist. */
	if (!osd) {
		osd_get_prefs();
		osd = xosd_create(osd_lines);
		xosd_set_align(osd, osd_align);
		xosd_set_pos(osd, osd_pos);
		osd_set_font();
		xosd_set_colour(osd, osd_color);
		xosd_set_timeout(osd, osd_timeout);
		xosd_set_shadow_offset(osd, osd_shadow);
		xosd_set_horizontal_offset(osd, osd_xoffset);
		xosd_set_vertical_offset(osd, osd_yoffset);
	}
}

/* this function prints a text on the x console. */
static void osd_print(const char *text) {

	/* common variables. */
	time_t now;
	char *message;
	GError *err = NULL;

	/* initialize the osd. */
	osd_init();
	time(&now);

	/* check if something is already on osd. */
	if (last_print + osd_timeout > now) {
		xosd_scroll(osd, 1);
	} else {
		xosd_scroll(osd, osd_lines);
	}

	/* convert utf text to used locale. */
	message = g_locale_from_utf8(text, -1, NULL, NULL, &err);
	if (err != NULL) {

		/* display the text. */
		xosd_display(osd, osd_lines-1, XOSD_string, purple_markup_strip_html(text));
	} else {

		/* display the text. */
		xosd_display(osd, osd_lines-1, XOSD_string, purple_markup_strip_html(message));
	}

	/* store last print time. */
	last_print = now;
}

/* this function creates some necessary initial configuration values. */
void osd_set_prefs(void) {

	/* set some initial preferences. */
	purple_prefs_set_int("/plugins/gtk/X11/pidgin-osd/align", (int)osd_align);
	purple_prefs_set_int("/plugins/gtk/X11/pidgin-osd/position", (int)osd_pos);
	purple_prefs_set_string("/plugins/gtk/X11/pidgin-osd/font_name", osd_font_name);
	purple_prefs_set_string("/plugins/gtk/X11/pidgin-osd/font_size", osd_font_size);
	purple_prefs_set_string("/plugins/gtk/X11/pidgin-osd/color", osd_color);
	purple_prefs_set_int("/plugins/gtk/X11/pidgin-osd/timeout", osd_timeout);
	purple_prefs_set_int("/plugins/gtk/X11/pidgin-osd/shadow", osd_shadow);
	purple_prefs_set_int("/plugins/gtk/X11/pidgin-osd/lines", osd_lines);
	purple_prefs_set_int("/plugins/gtk/X11/pidgin-osd/xoffset", osd_xoffset);
	purple_prefs_set_int("/plugins/gtk/X11/pidgin-osd/yoffset", osd_yoffset);
	purple_prefs_set_int("/plugins/gtk/X11/pidgin-osd/msgs", osd_msgs);

	/* check if another osd exist. */
	if (osd) {
		xosd_destroy (osd);
		osd = NULL;
	}

	/* initialize new osd. */
	osd_init();

	/* if osd initilization was successful, set properties. */
	if (osd) {
		int i;

		xosd_set_align(osd, osd_align);
		xosd_set_pos (osd, osd_pos);
		osd_set_font();
		xosd_set_colour(osd, osd_color);
		xosd_set_timeout(osd, osd_timeout);
		xosd_set_shadow_offset(osd, osd_shadow);
		xosd_set_horizontal_offset(osd, osd_xoffset);
		xosd_set_vertical_offset(osd, osd_yoffset);

		for (i=0; i < osd_lines; i++) {
			osd_print("Sample text message.");
		}
	}
}

/* this function saves the changes from the configuration window. */
static void osd_set_values(GtkWidget *button, GtkWidget *data) {
	GdkColor	c;
	static char	buf[20];

	/* save osd color */
	gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(g_osd_color), &c);
	g_snprintf(buf, sizeof(buf), "#%04x%04x%04x", c.red, c.green, c.blue);
	osd_color = buf;

	/* save all entered values */
	osd_font_name	= g_object_get_data(G_OBJECT(gtk_menu_get_active(GTK_MENU(gtk_option_menu_get_menu(GTK_OPTION_MENU(g_osd_font_name))))), "val");
	osd_font_size	= g_object_get_data(G_OBJECT(gtk_menu_get_active(GTK_MENU(gtk_option_menu_get_menu(GTK_OPTION_MENU(g_osd_font_size))))), "val");
	osd_align	= GPOINTER_TO_INT(g_object_get_data(G_OBJECT(gtk_menu_get_active(GTK_MENU(gtk_option_menu_get_menu(GTK_OPTION_MENU(g_osd_align))))), "val"));
	osd_pos		= GPOINTER_TO_INT(g_object_get_data(G_OBJECT(gtk_menu_get_active(GTK_MENU(gtk_option_menu_get_menu(GTK_OPTION_MENU(g_osd_pos))))), "val"));
	osd_timeout	= gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(g_osd_timeout));
	osd_shadow	= gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(g_osd_shadow));
	osd_lines	= gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(g_osd_lines));
	osd_xoffset	= gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(g_osd_xoffset));
	osd_yoffset	= gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(g_osd_yoffset));
	osd_msgs	= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_osd_msgs)) == TRUE ? 1 : 0;

	/* save the configuration values */
	osd_set_prefs();
}

/* this function reads the osd values from the preferences dialogue. */
static void osd_read_values() {

	/* common variables. */
	GdkColor	c;
	GtkWidget	*align;
	GtkWidget	*pos;
	GtkWidget	*item;
	GtkWidget	*font_name;
	GtkWidget	*font_size;
	int i;
	int inx;
	int active_font_inx = 0;
	int active_font_size_inx = 0;

	/* get osd preferences. */
	osd_get_prefs();

	/* get font color. */
	gdk_color_parse(osd_color, &c);
	gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(g_osd_color), &c);
	gtk_color_selection_set_previous_color(GTK_COLOR_SELECTION(g_osd_color), &c);

	/* get font name. */
	font_name = gtk_menu_new ();
	if (x_font_names) {
		for (i = 0, inx = 0; x_font_names[i]; i++) {
			if (x_font_names[i] != (char *) 1) {
				item = gtk_menu_item_new_with_label(x_font_names[i]);
				g_object_set_data(G_OBJECT(item), "val", x_font_names[i]);
				gtk_menu_append(GTK_MENU(font_name), item);
				if (strcmp(osd_font_name, x_font_names[i]) == 0) {
					active_font_inx = inx;
				}
				inx++;
			}
		}
		gtk_menu_set_active(GTK_MENU(font_name), active_font_inx);
	}
	gtk_option_menu_set_menu(GTK_OPTION_MENU(g_osd_font_name), font_name);

	/* get font size. */
	font_size = gtk_menu_new ();
	for (i = 0; x_font_sizes[i]; i++) {
		item = gtk_menu_item_new_with_label (x_font_sizes[i]);
		g_object_set_data(G_OBJECT(item), "val", x_font_sizes[i]);
		gtk_menu_append(GTK_MENU(font_size), item);
		if (strcmp(osd_font_size, x_font_sizes[i]) == 0) {
			active_font_size_inx = i;
		}
	}
	gtk_menu_set_active(GTK_MENU(font_size), active_font_size_inx);
	gtk_option_menu_set_menu(GTK_OPTION_MENU(g_osd_font_size), font_size);

	/* get osd aliegnment. */
	align = gtk_menu_new();
	item = gtk_menu_item_new_with_label("Left");
	g_object_set_data(G_OBJECT(item), "val", GINT_TO_POINTER(XOSD_left));
	gtk_menu_append(GTK_MENU(align), item);
	item = gtk_menu_item_new_with_label("Center");
	g_object_set_data(G_OBJECT(item), "val", GINT_TO_POINTER(XOSD_center));
	gtk_menu_append(GTK_MENU(align), item);
	item = gtk_menu_item_new_with_label("Right");
	g_object_set_data(G_OBJECT(item), "val", GINT_TO_POINTER(XOSD_right));
	gtk_menu_append(GTK_MENU(align), item);
	gtk_menu_set_active(GTK_MENU(align), osd_align);
	gtk_option_menu_set_menu(GTK_OPTION_MENU(g_osd_align), align);

	/* get osd position. */
	pos = gtk_menu_new();
	item = gtk_menu_item_new_with_label("Top");
	g_object_set_data(G_OBJECT(item), "val", GINT_TO_POINTER(XOSD_top));
	gtk_menu_append(GTK_MENU (pos), item);
	item = gtk_menu_item_new_with_label("Bottom");
	g_object_set_data(G_OBJECT(item), "val", GINT_TO_POINTER(XOSD_bottom));
	gtk_menu_append(GTK_MENU (pos), item);
	item = gtk_menu_item_new_with_label("Middle");
	g_object_set_data(G_OBJECT(item), "val", GINT_TO_POINTER(XOSD_middle));
	gtk_menu_append(GTK_MENU (pos), item);
	gtk_menu_set_active(GTK_MENU (pos), osd_pos);
	gtk_option_menu_set_menu(GTK_OPTION_MENU (g_osd_pos), pos);

	/* get the osd timeout. */
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_osd_timeout), osd_timeout);

	/* get the osd shadow. */
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_osd_shadow), osd_shadow);

	/* get the osd number of lines. */
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_osd_lines), osd_lines);

	/* get the osd x and y offset for drawing. */
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_osd_xoffset), osd_xoffset);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_osd_yoffset), osd_yoffset);

	/* get the osd active status. */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_osd_msgs), osd_msgs != 0 ? TRUE : FALSE);
}

/* this function shows the configuration window. */
static GtkWidget *osd_get_config_frame(PurplePlugin *plugin) {

	/* common variables. */
	GtkWidget	*ret;
	GtkWidget 	*frame;
	GtkWidget	*label;
	GtkWidget	*vbox;
	GtkWidget	*table;
	GtkWidget	*hbox;
	GtkWidget	*button;
	GtkAttachOptions opts	= GTK_EXPAND | GTK_SHRINK | GTK_FILL;

	g_tooltips = gtk_tooltips_new();
        
	ret = gtk_vbox_new(FALSE, 18);
	gtk_container_set_border_width(GTK_CONTAINER(ret), 12);

	/* windows name */
	frame = (GtkWidget *)pidgin_make_frame(ret, "pidgin-osd display properties");

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	table = gtk_table_new (9, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 5);

	/* font properties */
	label = gtk_label_new("Font");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, opts, opts, 3, 3);
	hbox = gtk_hbox_new (FALSE, 5);
	g_osd_font_name = gtk_option_menu_new();
	gtk_box_pack_start(GTK_BOX(hbox), g_osd_font_name, FALSE, FALSE, 5);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(g_tooltips), g_osd_font_name, "The font used to display the notification messages", "");
	g_osd_font_size = gtk_option_menu_new();
	gtk_box_pack_start(GTK_BOX(hbox), g_osd_font_size, FALSE, FALSE, 5);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(g_tooltips), g_osd_font_size, "The font size used to display the notification messages (in points)", "");
	gtk_table_attach(GTK_TABLE(table), hbox, 1, 3, 0, 1, opts, opts, 3, 3);

	/* color properties */
	label = gtk_label_new("Color");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, opts, opts, 3, 3);
	g_osd_color = gtk_color_selection_new();
	gtk_table_attach(GTK_TABLE(table), g_osd_color, 1, 3, 1, 2, opts, opts, 3, 3);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(g_tooltips), g_osd_color, "The foreground color used to display the notification messages", "");

	/* align properties */
	label = gtk_label_new("Align");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, opts, opts, 3, 3);
	g_osd_align = gtk_option_menu_new();
	gtk_table_attach(GTK_TABLE(table), g_osd_align, 1, 3, 2, 3, opts, opts, 3, 3);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(g_tooltips), g_osd_align, "How the notification messages are aligned horizontally", "");

	/* x offset from alignment point of view */
	label = gtk_label_new("X Offset");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4, opts, opts, 3, 3);
	g_osd_xoffset = gtk_spin_button_new_with_range(1, 2000, 1);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (g_osd_xoffset), 0);
	gtk_table_attach(GTK_TABLE(table), g_osd_xoffset, 1, 3, 3, 4, opts, opts, 3, 3);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(g_tooltips), g_osd_xoffset, "Horizontal offset of notification message from the allignment point", "");

	/* alignment properties */
	label = gtk_label_new("Position");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, opts, opts, 3, 3);
	g_osd_pos = gtk_option_menu_new();
	gtk_table_attach(GTK_TABLE(table), g_osd_pos, 1, 3, 4, 5, opts, opts, 3, 3);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(g_tooltips), g_osd_align, "How the notification messages are aligned vertically", "");

	/* y offset from alignment point of view */
	label = gtk_label_new("Y Offset");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6, opts, opts, 3, 3);
	g_osd_yoffset = gtk_spin_button_new_with_range(1, 2000, 1);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(g_osd_yoffset), 0);
	gtk_table_attach(GTK_TABLE(table), g_osd_yoffset, 1, 3, 5, 6, opts, opts, 3, 3);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(g_tooltips), g_osd_yoffset, "Vertical offset of notification message from the allignment point", "");

	/* timeout for osd messages */
	label = gtk_label_new("Timeout (secs)");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 6, 7, opts, opts, 3, 3);
	g_osd_timeout = gtk_spin_button_new_with_range (1, 100, 1);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(g_osd_timeout), 0);
	gtk_table_attach(GTK_TABLE(table), g_osd_timeout, 1, 3, 6, 7, opts, opts, 3, 3);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(g_tooltips), g_osd_timeout, "How long to keep the messages on the screen", "");

	/* shadow properties */
	label = gtk_label_new("Shadow");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 7, 8, opts, opts, 3, 3);
	g_osd_shadow = gtk_spin_button_new_with_range(1, 10, 1);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(g_osd_shadow), 0);
	gtk_table_attach(GTK_TABLE(table), g_osd_shadow, 1, 3, 7, 8, opts, opts, 3, 3);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(g_tooltips), g_osd_shadow, "The size of the shadow thrown by letters used to display the notification messages", "");

	/* number of lines if multiple notifications recieved */
	label = gtk_label_new("Lines");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 8, 9, opts, opts, 3, 3);
	g_osd_lines = gtk_spin_button_new_with_range(1, 10, 1);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(g_osd_lines), 0);
	gtk_table_attach(GTK_TABLE(table), g_osd_lines, 1, 3, 8, 9, opts, opts, 3, 3);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(g_tooltips), g_osd_lines, "The number of lines that can be displayed when several messages appear at the same time", "");

	/* show incoming messages on osd */
	g_osd_msgs = gtk_check_button_new_with_label("Show incoming messages");
	gtk_table_attach(GTK_TABLE(table), g_osd_msgs, 0, 3, 9, 10, opts, opts, 3, 3);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(g_tooltips), g_osd_msgs, "If on will also show the incomming messages as OSD notifications", "");

	/* save settings */
	button = gtk_button_new_with_mnemonic("_Set");
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 5);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(osd_set_values), NULL);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(g_tooltips), button, "Writes & activates the new settings", "");

	/* read the configuration values */
	osd_read_values();

	/* show the widget. */
	gtk_widget_show_all(ret);

	/* return without error. */
	return ret;
}

/* this function creates the osd text for the buddy actions. */
static void osd_notify(PurpleBuddy *buddy, const char *buddy_action) {

	/* common variables. */
	char text_buffer[1000];
	char *disp_name = PURPLE_BUDDY_DISP_NAME(buddy);

	/* format and print the text. */
	sprintf(text_buffer, "%.300s %.300s", disp_name, buddy_action);
	osd_print(text_buffer);
}

/* this function creates the osd text for the incoming buddy messages. */
static void osd_notify_txt(const char *format, ...) {

	/* common variables. */
	char text_buffer[1000];
	va_list lst;

	/* format and print the text. */
	va_start(lst, format);
	vsnprintf(text_buffer, sizeof(text_buffer), format, lst);
	va_end(lst);
	text_buffer[sizeof(text_buffer) - 1] = 0;
	osd_print(text_buffer);
}

/* osd notification for away. */
static void buddy_away_cb(PurpleBuddy *buddy, void *data) {
	osd_notify(buddy, "is away");
}

/* osd notification for back. */
static void buddy_back_cb(PurpleBuddy *buddy, void *data) {
	osd_notify(buddy, "is back");
}

/* osd notification for idle. */
static void buddy_idle_cb(PurpleBuddy *buddy, void *data) {
	osd_notify(buddy, "is idle");
}

/* osd notification for unidle. */
static void buddy_unidle_cb(PurpleBuddy *buddy, void *data) {
	osd_notify(buddy, "is not idle");
}

/* osd notification for signed on. */
static void buddy_signed_on_cb(PurpleBuddy *buddy, void *data) {
	osd_notify(buddy, "signed on");
}

/* osd notification for signed off. */
static void buddy_signed_off_cb(PurpleBuddy *buddy, void *data) {
	osd_notify(buddy, "signed off");
}

/* osd notification for incoming message. */
static void buddy_msg_received_cb(PurpleAccount *acct, char *sender, char *message, PurpleConversation *conv, int *flags) {

	/* check if we have already a message. */
	if (osd_msgs != 0) {
		char *disp_name = sender;
		PurpleBuddy *buddy = purple_find_buddy(acct, sender);
		if (buddy) {
			disp_name = PURPLE_BUDDY_DISP_NAME(buddy);
		}
		osd_notify_txt("&lt;%.100s&gt; : %.300s", disp_name, message);
	}
}

/* this function loads and initializes the plugin. */
static gboolean plugin_load(PurplePlugin *plugin) {

	/* common variables. */
	void *blist_handle = NULL;
	my_plugin = plugin;

	/* create buddy list handle. */
	blist_handle = purple_blist_get_handle();

	/* buddy List subsystem signals. */
	purple_signal_connect(blist_handle, "buddy-away", plugin, PURPLE_CALLBACK(buddy_away_cb), NULL);
	purple_signal_connect(blist_handle, "buddy-back", plugin, PURPLE_CALLBACK(buddy_back_cb), NULL);
	purple_signal_connect(blist_handle, "buddy-idle", plugin, PURPLE_CALLBACK(buddy_idle_cb), NULL);
	purple_signal_connect(blist_handle, "buddy-unidle", plugin, PURPLE_CALLBACK(buddy_unidle_cb), NULL);
	purple_signal_connect(blist_handle, "buddy-signed-on", plugin, PURPLE_CALLBACK(buddy_signed_on_cb), NULL);
	purple_signal_connect(blist_handle, "buddy-signed-off", plugin, PURPLE_CALLBACK(buddy_signed_off_cb), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "received-im-msg", plugin, PURPLE_CALLBACK(buddy_msg_received_cb), NULL);

	/* return without error. */
	return TRUE;
}

/* this function unloads and destroys the plugin. */
static gboolean plugin_unload(PurplePlugin *plugin) {

	/* destroy osd handler. */
	xosd_wait_until_no_display(osd);
	xosd_destroy (osd);
	osd = NULL;

	/* return without error. */
	return TRUE;
}

/* plugin user interface. */
static PidginPluginUiInfo ui_info = {
	osd_get_config_frame
};

/* plugin information. */
static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,			/* plugin type */
	PIDGIN_PLUGIN_TYPE,			/* ui requirement */
	0,					/* plugin flags */
	NULL,					/* plugin dependencies */
	PURPLE_PRIORITY_DEFAULT,		/* plugin priority */
	PACKAGE_NAME,				/* plugin id */
	"OSD Notification",			/* plugin name */
	PACKAGE_VERSION,			/* plugin version */
	"OSD Notification",			/* plugin summary */
	"Displays messages on the X console",	/* plugin description */
	AUTHOR,					/* plugin author */
	"http://babelize.org/",			/* plugin homepage */
	plugin_load,				/* plugin pointer to init function */
	plugin_unload,				/* plugin pointer to unload function */
	NULL,					/* plugin pointer to destroy function */
	&ui_info,				/* plugin pointer to ui struct to give core plugins a ui configuration frame */
	NULL					/* plugin pointer to action function */
};

static void init_plugin(PurplePlugin *plugin) {

	/* some initlization function for the configuration window */
	osd_fill_fonts();
	osd_init_prefs();
}

PURPLE_INIT_PLUGIN(pidgin-osd, init_plugin, info);
