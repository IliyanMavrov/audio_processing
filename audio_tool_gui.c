// audio_tool_gui.c
// GUI аудио процесор с ефекти + зареждане на WAV файл + визуализация на waveform

#include <gtk/gtk.h>
#include <portaudio.h>
#include <sndfile.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512
#define MAX_DELAY_SAMPLES (SAMPLE_RATE * 2)

float echo_buffer[MAX_DELAY_SAMPLES];
int echo_index = 0;

float gain = 1.0;
int enable_echo = 0, enable_distortion = 0, enable_reverb = 0;
float echo_delay_ms = 500.0;
float lowpass_alpha = 0.5;

float *file_audio_data = NULL;
sf_count_t file_audio_len = 0;
int playing_file = 0;
sf_count_t file_play_pos = 0;

GtkWidget *waveform_area;

// Проста low-pass филтрация
float lowpass(float input, float prev_output) {
    return lowpass_alpha * input + (1.0f - lowpass_alpha) * prev_output;
}

// Callback за PortAudio
int audio_callback(const void *input, void *output,
                   unsigned long frameCount,
                   const PaStreamCallbackTimeInfo* timeInfo,
                   PaStreamCallbackFlags statusFlags,
                   void *userData) {
    float *out = (float*)output;
    float prev = 0.0;
    for (unsigned int i = 0; i < frameCount; i++) {
        float sample = 0.0f;

        if (playing_file && file_audio_data && file_play_pos < file_audio_len) {
            sample = file_audio_data[file_play_pos++];
        }

        // Echo
        if (enable_echo) {
            int delay_samples = (int)(echo_delay_ms * SAMPLE_RATE / 1000.0);
            int delayed_index = (echo_index - delay_samples + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;
            sample += 0.6f * echo_buffer[delayed_index];
        }

        // Distortion
        if (enable_distortion) {
            sample *= gain;
            if (sample > 0.8f) sample = 0.8f;
            else if (sample < -0.8f) sample = -0.8f;
        }

        // Low-pass
        sample = lowpass(sample, prev);
        prev = sample;

        // Reverb (много проста)
        if (enable_reverb) {
            sample += 0.2f * sinf((float)i * 0.1f);
        }

        // Echo буфер
        echo_buffer[echo_index] = sample;
        echo_index = (echo_index + 1) % MAX_DELAY_SAMPLES;

        out[i] = sample;
    }
    return paContinue;
}

void draw_waveform(GtkWidget *widget, cairo_t *cr, gpointer data) {
    if (!file_audio_data || file_audio_len == 0) return;

    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    int w = allocation.width;
    int h = allocation.height;

    cairo_set_source_rgb(cr, 0.2, 0.8, 0.2);
    cairo_set_line_width(cr, 1);

    cairo_move_to(cr, 0, h/2);
    for (int x = 0; x < w; x++) {
        sf_count_t idx = (sf_count_t)((float)x / w * file_audio_len);
        float y = file_audio_data[idx] * (h/2);
        cairo_line_to(cr, x, h/2 - y);
    }
    cairo_stroke(cr);
}

void on_load_file(GtkButton *btn, gpointer user_data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Избор на WAV файл",
        GTK_WINDOW(user_data), GTK_FILE_CHOOSER_ACTION_OPEN,
        "Отказ", GTK_RESPONSE_CANCEL,
        "Отвори", GTK_RESPONSE_ACCEPT, NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        SF_INFO sfinfo;
        SNDFILE *sndfile = sf_open(filename, SFM_READ, &sfinfo);

        if (!sndfile) {
            g_printerr("Неуспешно зареждане на файла\n");
            gtk_widget_destroy(dialog);
            return;
        }

        free(file_audio_data);
        file_audio_len = sfinfo.frames;
        file_audio_data = malloc(file_audio_len * sizeof(float));
        sf_readf_float(sndfile, file_audio_data, file_audio_len);
        sf_close(sndfile);

        file_play_pos = 0;
        playing_file = 1;
        gtk_widget_queue_draw(waveform_area);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

void toggle_effect(GtkToggleButton *btn, gpointer user_data) {
    const char *name = gtk_widget_get_name(GTK_WIDGET(btn));
    gboolean active = gtk_toggle_button_get_active(btn);

    if (strcmp(name, "echo") == 0) enable_echo = active;
    else if (strcmp(name, "distortion") == 0) enable_distortion = active;
    else if (strcmp(name, "reverb") == 0) enable_reverb = active;
}

void create_main_window() {
    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "🎛 Audio Tool GUI");
    gtk_window_set_default_size(GTK_WINDOW(win), 600, 400);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    GtkWidget *btn_load = gtk_button_new_with_label("📂 Зареди WAV файл");
    g_signal_connect(btn_load, "clicked", G_CALLBACK(on_load_file), win);
    gtk_box_pack_start(GTK_BOX(vbox), btn_load, FALSE, FALSE, 0);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    GtkWidget *chk_echo = gtk_check_button_new_with_label("Echo");
    gtk_widget_set_name(chk_echo, "echo");
    g_signal_connect(chk_echo, "toggled", G_CALLBACK(toggle_effect), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), chk_echo, FALSE, FALSE, 0);

    GtkWidget *chk_dist = gtk_check_button_new_with_label("Distortion");
    gtk_widget_set_name(chk_dist, "distortion");
    g_signal_connect(chk_dist, "toggled", G_CALLBACK(toggle_effect), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), chk_dist, FALSE, FALSE, 0);

    GtkWidget *chk_rev = gtk_check_button_new_with_label("Reverb");
    gtk_widget_set_name(chk_rev, "reverb");
    g_signal_connect(chk_rev, "toggled", G_CALLBACK(toggle_effect), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), chk_rev, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    waveform_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(waveform_area, 600, 200);
    g_signal_connect(G_OBJECT(waveform_area), "draw", G_CALLBACK(draw_waveform), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), waveform_area, TRUE, TRUE, 0);

    gtk_widget_show_all(win);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    Pa_Initialize();

    PaStream *stream;
    Pa_OpenDefaultStream(&stream,
                         0, 1,
                         paFloat32,
                         SAMPLE_RATE,
                         FRAMES_PER_BUFFER,
                         audio_callback,
                         NULL);
    Pa_StartStream(stream);

    create_main_window();
    gtk_main();

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    free(file_audio_data);
    return 0;
}
