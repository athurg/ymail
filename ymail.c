#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>
#include <time.h>

#include "mbox.h"

GtkBuilder *builder;
FILE *mbox_fp;
unsigned int unread=0, total=0;

gboolean status_icon_button_press_cb(GtkStatusIcon *status_icon, GdkEventButton *event, gpointer user_data)
{
	GtkWidget *window;


	if(event->button==1){//左键
		window = GTK_WIDGET(gtk_builder_get_object(builder,"main_window"));
		if(gtk_widget_get_visible(window))
			gtk_widget_hide(window);
		else
			gtk_widget_show(window);
	}
	printf("you press button %d\n", event->button);
}

void timer_cb(void)
{
	int status=0;
	GtkListStore *list_store;
	char buff[20];
	static unsigned int lock=0;
	struct mail_hdr hdr;


	sprintf(buff, "未读：%d ；总计：%d ", unread, total);
	gtk_status_icon_set_tooltip(GTK_STATUS_ICON(gtk_builder_get_object(builder, "status_icon")), buff);

	list_store = GTK_LIST_STORE(gtk_builder_get_object(builder,"liststore"));
	for(int i=0; i<30; i++){
		GtkTreeIter iter;
		int ret=0;
		char strstat[10]={0};
		char attach[10]={0};

		ret = parse_header(mbox_fp, &hdr);

		if(ret == MSG_STATUS_UNREAD)
			strcpy(strstat, "未读");
		else if(ret == MSG_STATUS_READED)
			strcpy(strstat, "已读");
		else if(ret == MSG_STATUS_NEWRCV)
			strcpy(strstat, "新来");
		else if(ret<0){
			break;
		}

		if(hdr.type=='a')
			strcpy(attach, "无附件");
		else if(hdr.type=='m')
			strcpy(attach, "有附件");
		else if(hdr.type=='r')
			strcpy(attach, "复合");
		else
			strcpy(attach, "未知");

		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter,
				0, strstat,
				1, hdr.time,
				2, hdr.sender,
				3, hdr.subject,
				4, attach,
				-1);
		free_mail_hdr(&hdr);
	}

}

int init_gui(int argc, char *argv[])
{
	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "ui.glade", NULL);
	gtk_builder_connect_signals(builder, NULL);

}

int main(int argc, char *argv[])
{
	init_gui(argc, argv);
	struct mail_hdr hdr={NULL};

	// 检查邮箱是否有效
	mbox_fp = fopen("/var/mail/athurg", "rb");
	if(mbox_fp == NULL){
		perror("打开邮箱失败");
		return -1;
	}

	gtk_timeout_add(1000, (GtkFunction) timer_cb, NULL);
	gtk_main();
	fclose(mbox_fp);
	return 0;
}


