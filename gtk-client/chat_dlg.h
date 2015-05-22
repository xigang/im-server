#ifndef CHATDIALOG_H_
#define CHATDIALOG_H_

typedef struct
{
	GtkWidget *chat_dlg;
	GtkWidget *textview_output, *textview_intput;
	GtkProgressBar *progress;
	GtkWidget *label_file;
	gboolean group;
} Widgets_Chat; // 聊天对话框数据结构


Widgets_Chat *create_chat_dlg(gchar *text, int face_num, gboolean group);
char *get_info_from_id(char *id_buf);
void face_select(GtkWidget *widget, gpointer data);
void face_click(GtkWidget *widget, GdkEventButton *event, gpointer data);
void font_select(GtkWidget *widget, gpointer data);
void color_select(GtkWidget *widget, gpointer data);
void show_history_msg(GtkWidget *widget, gpointer data);
void send_file(GtkButton *button, gpointer data);
char *get_cur_time();
void send_text_msg(GtkWidget *widget, gpointer data);
void receive_text_msg(char *text_title, char *id_from, int face_num,
		gboolean group, char *text_msg, int msg_len);
void scroll_textview(char *text_title);
void close_dlg(GtkWidget *widget, gpointer data);

#endif /* CHATDIALOG_H_ */
