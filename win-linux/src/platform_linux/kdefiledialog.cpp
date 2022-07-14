#include <string>
extern "C" {
#include <stdio.h>
#include <gtk/gtk.h>
}
#include "kdefiledialog.h"


QStringList Kde::openNativeDialog(QWidget *parent,
                              const QString &name,
                              const QString &path,
                              const QString &filter,
                              const QString &sel_filter,
                              bool sel_multiple)
{
    qputenv("GTK_USE_PORTAL", "1");
    QStringList files;
    char **filenames = nullptr;
    int files_count = 0;
    nativeFileDialog(Mode::OPEN,
                     &filenames,
                     &files_count,
                     name.toLocal8Bit().data(),
                     "",
                     path.toLocal8Bit().data(),
                     filter.toLocal8Bit().data(),
                     sel_filter.toLocal8Bit().data(),
                     sel_multiple);

    for (int i = 0; i < files_count; i++) {
        //printf("\n%s  %d\n", filenames[i], files_count);
        if (filenames[i]) {
            files.append(QString::fromUtf8(filenames[i]));
            free(filenames[i]);
        }
    }
    if (filenames) {
        free(filenames);
    }

    return files;
}

QStringList Kde::saveNativeDialog(QWidget *parent,
                               const QString &name,
                               const QString &file_name,
                               const QString &filter,
                               const QString &sel_filter)
{
    qputenv("GTK_USE_PORTAL", "1");
    QStringList files;
    char **filenames = nullptr;
    int files_count = 0;
    nativeFileDialog(Mode::SAVE,
                     &filenames,
                     &files_count,
                     name.toLocal8Bit().data(),
                     file_name.toLocal8Bit().data(),
                     "",
                     filter.toLocal8Bit().data(),
                     sel_filter.toLocal8Bit().data(),
                     false);

    for (int i = 0; i < files_count; i++) {
        //printf("\n%s  %d\n", filenames[i], files_count);
        if (filenames[i]) {
            files.append(QString::fromUtf8(filenames[i]));
            free(filenames[i]);
        }
    }
    if (filenames) {
        free(filenames);
    }

    return files;
}

// Extern "C"
char* Kde::substr(const char *src, int m, int n)
{
    int len = n - m;
    char *dest = (char*)malloc(sizeof(char) * (len + 1));
    for (int i = m; i < n && (*(src + i) != '\0'); i++) {
        *dest = *(src + i);
        dest++;
    }
    *dest = '\0';
    return dest - len;
}

void Kde::parseString(GSList** list,
                      const char* str,
                      const char* delim)
{
    char *_str = strdup(str);
    char *token = strtok(_str, delim);
    while (token != NULL) {
        *list = g_slist_append(*list, (gpointer)strdup(token));
        token = strtok(NULL, delim);
    }
    free(_str);
}

void Kde::nativeFileDialog(Mode mode,
                           char*** filenames,
                           int* files_count,
                           const char* name,
                           const char* file,
                           const char* path,
                           const char* flt,
                           const char* sel_filter,
                           bool sel_multiple)
{
    gtk_init(NULL, NULL);
    GtkFileChooserNative *native;
    GtkFileFilter *filter;
    gpointer window = NULL;    
    native = gtk_file_chooser_native_new(name,
                                         GTK_WINDOW(window),
                                         mode == Mode::OPEN ? GTK_FILE_CHOOSER_ACTION_OPEN :
                                                              GTK_FILE_CHOOSER_ACTION_SAVE,
                                         mode == Mode::OPEN ? "_Open" : "_Save",
                                         "_Cancel");

    GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(native), TRUE);
    if (mode == Mode::OPEN) {
        gtk_file_chooser_set_current_folder(chooser, path);
        gtk_file_chooser_set_select_multiple (chooser, sel_multiple ? TRUE : FALSE);
    } else {
        gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
        gtk_file_chooser_set_current_name(chooser, file);
        //gtk_file_chooser_set_filename(chooser, "existing_filename");
    }
    //g_signal_connect(native, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Filters
    GSList *list = NULL, *iter = NULL;
    parseString(&list, flt, ";;");
    int ind;
    for (ind = 0; iter = g_slist_nth(list, ind); ind++) {
        char *flt_name = (char*)iter->data;
        if (flt_name != NULL) {
            filter = gtk_file_filter_new();
            gtk_file_filter_set_name(filter, flt_name);
            //g_print("%s\n", flt_name);
            char *start = strchr(flt_name, '(');
            char *end = strchr(flt_name, ')');
            if (start != NULL && end != NULL) {
                int start_index = (int)(start - flt_name);
                int end_index = (int)(end - flt_name);
                if (start_index < end_index) {
                    char *fltrs = substr(flt_name, start_index + 1, end_index);
                    //g_print("%s\n", fltrs);
                    GSList *flt_list = NULL, *flt_iter = NULL;
                    parseString(&flt_list, fltrs, " ");
                    free(fltrs);
                    int flt_ind;
                    for (flt_ind = 0; flt_iter = g_slist_nth(flt_list, flt_ind); flt_ind++) {
                        char *nm = (char*)flt_iter->data;
                        if (nm != NULL) {
                            gtk_file_filter_add_pattern(filter, nm);
                        }
                    }
                    g_slist_free(flt_list);
                }
            }
            gtk_file_chooser_add_filter(chooser, filter);
            if (strcmp(flt_name, sel_filter) == 0)
                gtk_file_chooser_set_filter(chooser, filter);
        }
    }

    //gdk_window_raise(gtk_widget_get_window((GtkWidget*)(native)));
    gint res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
    if (res == GTK_RESPONSE_ACCEPT) {
        if (sel_multiple) {
            GSList *filenames_list = gtk_file_chooser_get_filenames(chooser);
            GSList *f_iter = NULL;
            int f_ind;
            for (f_ind = 0; f_iter = g_slist_nth(filenames_list, f_ind); f_ind++);
            *files_count = f_ind;
            *filenames = (char**)calloc((size_t)(*files_count), sizeof(char*));
            for (int i = 0; f_iter = g_slist_nth(filenames_list, i); i++) {
                (*filenames)[i] = (char*)f_iter->data;
            }

        } else {
            *files_count = 1;
            *filenames = (char**)calloc((size_t)(*files_count), sizeof(char*));
            **filenames = gtk_file_chooser_get_filename(chooser);
        }

    }
    g_object_unref(native);
    g_slist_free(list);
    //gtk_main();
}
