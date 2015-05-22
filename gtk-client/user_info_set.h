#ifndef USER_INFO_SET_H_
#define USER_INFO_SET_H_

GtkWidget *create_user_info_dlg();
GtkWidget *create_user_pic_page();
GtkWidget *create_user_basic_info_page();
void user_image_click(GtkWidget *widget, GdkEventButton *event, gpointer data);

#endif /* USER_INFO_SET_H_ */
