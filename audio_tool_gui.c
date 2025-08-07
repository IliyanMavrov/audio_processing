
// audio_tool_gui.c
// GUI версия на аудио процесор с ефекти
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <pthread.h>
#include <portaudio.h>

#define SAMPLE_RATE         44100
#define FRAMES_PER_BUFFER   512
#define CHANNELS            1
#define BUFFER_SIZE         (SAMPLE_RATE * 10)
#define MAX_DELAY_SAMPLES   (SAMPLE_RATE * 2)

typedef struct {
    int16_t buffer[BUFFER_SIZE];
    int head, tail, count;
    pthread_mutex_t mutex;
} CircularBuffer;

typedef struct {
    int echoEnabled;
    int delayMs;
    float lowpassAlpha;
    int distortionEnabled;
    float distortionGain;
    int reverbEnabled;
} Config;

Config config = {0, 500, 0.1f, 0, 2.0f, 0};
CircularBuffer audioBuffer;

void initBuffer(CircularBuffer* cb) {
    cb->head = cb->tail = cb->count = 0;
    pthread_mutex_init(&cb->mutex, NULL);
    memset(cb->buffer, 0, sizeof(cb->buffer));
}

int writeBuffer(CircularBuffer* cb, const int16_t* data, int frames) {
    pthread_mutex_lock(&cb->mutex);
    if (cb->count + frames > BUFFER_SIZE) {
        pthread_mutex_unlock(&cb->mutex);
        return -1;
    }
    for (int i = 0; i < frames; i++) {
        cb->buffer[cb->head] = data[i];
        cb->head = (cb->head + 1) % BUFFER_SIZE;
    }
    cb->count += frames;
    pthread_mutex_unlock(&cb->mutex);
    return 0;
}

int readBuffer(CircularBuffer* cb, int16_t* out, int frames) {
    pthread_mutex_lock(&cb->mutex);
    if (cb->count < frames) {
        pthread_mutex_unlock(&cb->mutex);
        return -1;
    }
    for (int i = 0; i < frames; i++) {
        out[i] = cb->buffer[cb->tail];
        cb->tail = (cb->tail + 1) % BUFFER_SIZE;
    }
    cb->count -= frames;
    pthread_mutex_unlock(&cb->mutex);
    return 0;
}

void applyEffects(int16_t* samples, int frames) {
    static float echoBuffer[MAX_DELAY_SAMPLES] = {0};
    static int echoIndex = 0;
    static float lowpassMem = 0;
    static float reverbBuffer[6][MAX_DELAY_SAMPLES] = {{0}};
    static int reverbIndex = 0;

    int delaySamples = (config.delayMs * SAMPLE_RATE) / 1000;

    for (int i = 0; i < frames; i++) {
        float input = samples[i];
        if (config.lowpassAlpha > 0)
            lowpassMem = config.lowpassAlpha * input + (1.0f - config.lowpassAlpha) * lowpassMem;
        else
            lowpassMem = input;
        float out = lowpassMem;

        if (config.echoEnabled) {
            float echo = echoBuffer[(echoIndex + MAX_DELAY_SAMPLES - delaySamples) % MAX_DELAY_SAMPLES];
            out += echo * 0.5f;
            echoBuffer[echoIndex] = out;
        }

        if (config.reverbEnabled) {
            float rv = 0;
            for (int t = 0; t < 6; t++) {
                int idx = (reverbIndex + MAX_DELAY_SAMPLES - (delaySamples / (t + 1))) % MAX_DELAY_SAMPLES;
                rv += reverbBuffer[t][idx] * (0.3f / (t + 1));
                reverbBuffer[t][reverbIndex] = out;
            }
            out += rv;
        }

        if (config.distortionEnabled) {
            out *= config.distortionGain;
            if (out > 32767) out = 32767;
            if (out < -32768) out = -32768;
        }

        samples[i] = (int16_t)out;
        echoIndex = (echoIndex + 1) % MAX_DELAY_SAMPLES;
        reverbIndex = (reverbIndex + 1) % MAX_DELAY_SAMPLES;
    }
}

static int inputCallback(const void* input, void* output,
                         unsigned long frames, const PaStreamCallbackTimeInfo* t,
                         PaStreamCallbackFlags flags, void* userData) {
    const int16_t* in = (const int16_t*)input;
    if (in) writeBuffer(&audioBuffer, in, frames);
    return paContinue;
}

static int outputCallback(const void* input, void* output,
                          unsigned long frames, const PaStreamCallbackTimeInfo* t,
                          PaStreamCallbackFlags flags, void* userData) {
    int16_t* out = (int16_t*)output;
    memset(out, 0, frames * sizeof(int16_t));
    if (readBuffer(&audioBuffer, out, frames) == 0)
        applyEffects(out, frames);
    return paContinue;
}

void on_toggle_echo(GtkToggleButton *button, gpointer user_data) {
    config.echoEnabled = gtk_toggle_button_get_active(button);
}

void on_toggle_reverb(GtkToggleButton *button, gpointer user_data) {
    config.reverbEnabled = gtk_toggle_button_get_active(button);
}

void on_toggle_distortion(GtkToggleButton *button, gpointer user_data) {
    config.distortionEnabled = gtk_toggle_button_get_active(button);
}

void on_gain_changed(GtkRange *range, gpointer user_data) {
    config.distortionGain = gtk_range_get_value(range);
}

void on_lowpass_changed(GtkRange *range, gpointer user_data) {
    config.lowpassAlpha = gtk_range_get_value(range);
}

void on_delay_changed(GtkRange *range, gpointer user_data) {
    config.delayMs = gtk_range_get_value(range);
}

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    Pa_Initialize();
    initBuffer(&audioBuffer);

    PaStream *inStream, *outStream;
    Pa_OpenDefaultStream(&inStream, 1, 0, paInt16, SAMPLE_RATE, FRAMES_PER_BUFFER, inputCallback, NULL);
    Pa_OpenDefaultStream(&outStream, 0, 1, paInt16, SAMPLE_RATE, FRAMES_PER_BUFFER, outputCallback, NULL);
    Pa_StartStream(inStream);
    Pa_StartStream(outStream);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "🎛 Audio FX");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *echoBtn = gtk_check_button_new_with_label("Echo");
    GtkWidget *reverbBtn = gtk_check_button_new_with_label("Reverb");
    GtkWidget *distBtn = gtk_check_button_new_with_label("Distortion");
    gtk_box_pack_start(GTK_BOX(vbox), echoBtn, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), reverbBtn, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), distBtn, FALSE, FALSE, 2);

    GtkWidget *gainSlider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1.0, 10.0, 0.1);
    gtk_scale_set_value_pos(GTK_SCALE(gainSlider), GTK_POS_RIGHT);
    gtk_range_set_value(GTK_RANGE(gainSlider), config.distortionGain);
    gtk_box_pack_start(GTK_BOX(vbox), gainSlider, FALSE, FALSE, 2);

    GtkWidget *lowpassSlider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.05);
    gtk_range_set_value(GTK_RANGE(lowpassSlider), config.lowpassAlpha);
    gtk_box_pack_start(GTK_BOX(vbox), lowpassSlider, FALSE, FALSE, 2);

    GtkWidget *delaySlider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 100, 2000, 50);
    gtk_range_set_value(GTK_RANGE(delaySlider), config.delayMs);
    gtk_box_pack_start(GTK_BOX(vbox), delaySlider, FALSE, FALSE, 2);

    g_signal_connect(echoBtn, "toggled", G_CALLBACK(on_toggle_echo), NULL);
    g_signal_connect(reverbBtn, "toggled", G_CALLBACK(on_toggle_reverb), NULL);
    g_signal_connect(distBtn, "toggled", G_CALLBACK(on_toggle_distortion), NULL);
    g_signal_connect(gainSlider, "value-changed", G_CALLBACK(on_gain_changed), NULL);
    g_signal_connect(lowpassSlider, "value-changed", G_CALLBACK(on_lowpass_changed), NULL);
    g_signal_connect(delaySlider, "value-changed", G_CALLBACK(on_delay_changed), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    Pa_StopStream(inStream);
    Pa_StopStream(outStream);
    Pa_CloseStream(inStream);
    Pa_CloseStream(outStream);
    Pa_Terminate();
    return 0;
}
