#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <time.h>

#include "mbox.h"

GtkBuilder *builder;
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

GtkFunction timer_cb(void)
{
	static unsigned int i=0;
	FILE *pipe;
	char buff[100];
	GtkStatusIcon *status_icon;

	printf("%d\n", i++);

	pipe = popen("grep '^Status' /var/mail/athurg | wc -l", "r");
	fscanf(pipe, "%d", &total);
	pclose(pipe);

	pipe = popen("grep '^Status: O' /var/mail/athurg | wc -l", "r");
	fscanf(pipe, "%d", &unread);
	pclose(pipe);

	sprintf(buff, "未读：%d ；总计：%d ", unread, total);
	status_icon = GTK_STATUS_ICON(gtk_builder_get_object(builder, "status_icon"));
	gtk_status_icon_set_tooltip(status_icon, buff);
}

int init_gui(int argc, char *argv[])
{
	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "ui.glade", NULL);
	gtk_builder_connect_signals(builder, NULL);

	gtk_timeout_add(1000*60, (GtkFunction) timer_cb, NULL);
}

int main(int argc, char *argv[])
{
	char name[100]={0};
	char addr[100]={0};
	char subject[500]={0};
	char date[50]={0};
	int status=0;
	FILE *mbox_fp;
	GtkListStore *list_store;

	init_gui(argc, argv);

	mbox_fp = fopen("/var/mail/athurg", "rb");
	if(mbox_fp == NULL)
		printf("Fail to open mailbox!\n");


	//printf("%s %s(%s) %s\n", date,name, addr, subject);


	list_store = GTK_LIST_STORE(gtk_builder_get_object(builder,"liststore"));


	for(int i=0; i<1000; i++){
		GtkTreeIter iter;
		int ret=0;
		char strstat[10]={0};

		bzero(subject, sizeof(subject));
		ret = parse_header(mbox_fp, name, addr, subject, date);
		if(ret == MSG_STATUS_UNREAD)
			strcpy(strstat, "未读");
		else if(ret == MSG_STATUS_READED)
			strcpy(strstat, "已读");
		else if(ret == MSG_STATUS_NEWRCV)
			strcpy(strstat, "新来");
		else if(ret<0)
			break;

		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter,
				0, strstat, 1, date, 2, name, 3, subject,-1);
	}
	gtk_main();
	return 0;
}


