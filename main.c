#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/Xrandr.h>
#include <alsa/asoundlib.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <math.h>

// Sample rates and timing
#define SAMPLE_RATE 44100
#define X11_UPDATE_RATE 120  // 120 FPS

#define WINDOW_SIZE (SAMPLE_RATE / X11_UPDATE_RATE)  // ~368 samples per update
#define PI (4.0 * atan(1.0))

// Carrier frequencies
#define FREQ_X 440.0    // A4
#define FREQ_Y 660.0    // E5
#define FREQ_KEYS 880.0 // A5

// Common monitor refresh rates
#define X11_REFRESH_RATE 60  // Most common, can be 60, 75, 120, 144, etc.
#define FRAME_TIME (1.0 / X11_REFRESH_RATE)

typedef struct {
    int16_t mono;
} AudioSample;

// Global variables should be declared before any functions
volatile bool running = true;

void handle_signal(int signum) {
    (void)signum;
    running = false;
}

// Function prototypes (declarations)
int playback_mode(const char *filename);
int record_mode(const char *filename);
int listen_mode(void);
int setup_alsa_capture(snd_pcm_t **capture_handle);
int get_monitor_refresh_rate(Display *display);

int get_monitor_refresh_rate(Display *display) { //Corrected return type to int
    Window root = DefaultRootWindow(display);
    XRRScreenConfiguration *conf = XRRGetScreenInfo(display, root);

    if (!conf) {
        fprintf(stderr, "Could not get screen info\n");
        return 60; // fallback to 60Hz
    }

    short current_rate = XRRConfigCurrentRate(conf);
    XRRFreeScreenConfigInfo(conf);

    return (current_rate > 0) ? current_rate : 60;
}

int playback_mode(const char *filename) {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    // Get X11 screen refresh rate
    Screen *screen = DefaultScreenOfDisplay(display);
    int screen_width = screen->width;
    int screen_height = screen->height;
    
    // Most X11 screens run at 60Hz, but let's be explicit
    int refresh_rate = 60;  // Hz
    double frame_time = 1.0 / refresh_rate;

    FILE *wav_file = fopen(filename, "rb");
    if (!wav_file) {
        fprintf(stderr, "Cannot open input file: %s\n", strerror(errno));
        XCloseDisplay(display);
        return 1;
    }

    // Skip WAV header
    fseek(wav_file, 44, SEEK_SET);

    AudioSample sample;
    double buffer[WINDOW_SIZE] = {0};
    int buffer_pos = 0;
    
    int last_x = screen_width / 2;
    int last_y = screen_height / 2;
    bool key_was_pressed = false;

    struct timespec start_time, current_time, next_frame_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    next_frame_time = start_time;

    printf("Playing %s at %dHz refresh rate...\n", filename, refresh_rate);
    printf("Screen dimensions: %dx%d\n", screen_width, screen_height);

    while (running && fread(&sample, sizeof(AudioSample), 1, wav_file) == 1) {
        buffer[buffer_pos] = sample.mono / 32767.0;
        buffer_pos++;

        if (buffer_pos >= WINDOW_SIZE) {
            clock_gettime(CLOCK_MONOTONIC, &current_time);
            
            // Wait until next frame time
            if (current_time.tv_sec < next_frame_time.tv_sec || 
                (current_time.tv_sec == next_frame_time.tv_sec && 
                 current_time.tv_nsec < next_frame_time.tv_nsec)) {
                nanosleep(&(struct timespec){
                    .tv_sec = next_frame_time.tv_sec - current_time.tv_sec,
                    .tv_nsec = next_frame_time.tv_nsec - current_time.tv_nsec
                }, NULL);
            }

            // Analyze frequencies
            double power_x = 0.0, power_y = 0.0, power_k = 0.0;
            
            for (int i = 0; i < WINDOW_SIZE; i++) {
                double t = (double)i / SAMPLE_RATE;
                power_x += buffer[i] * sin(2.0 * PI * FREQ_X * t);
                power_y += buffer[i] * sin(2.0 * PI * FREQ_Y * t);
                power_k += buffer[i] * sin(2.0 * PI * FREQ_KEYS * t);
            }

            power_x = fabs(power_x) / WINDOW_SIZE;
            power_y = fabs(power_y) / WINDOW_SIZE;
            power_k = fabs(power_k) / WINDOW_SIZE;

            // Convert to screen coordinates with smoothing
            int x = (int)(power_x * screen_width * 2);
            int y = (int)(power_y * screen_height * 2);
            
            x = (x + last_x * 3) / 4;
            y = (y + last_y * 3) / 4;
            
            x = x < 0 ? 0 : (x >= screen_width ? screen_width - 1 : x);
            y = y < 0 ? 0 : (y >= screen_height ? screen_height - 1 : y);

            // Move cursor
            XWarpPointer(display, None, DefaultRootWindow(display), 
                        0, 0, 0, 0, x, y);

            // Handle key state
            bool key_pressed = (power_k > 0.3);
            if (key_pressed != key_was_pressed) {
                XTestFakeKeyEvent(display, 65, key_pressed, 0);
                key_was_pressed = key_pressed;
            }

            XFlush(display);
            
            last_x = x;
            last_y = y;

            // Update next frame time
            next_frame_time.tv_nsec += (long)(frame_time * 1e9);
            if (next_frame_time.tv_nsec >= 1000000000) {
                next_frame_time.tv_sec += 1;
                next_frame_time.tv_nsec -= 1000000000;
            }

            // Reset buffer
            buffer_pos = 0;

            // Debug output
            printf("\rX: %.3f Y: %.3f Key: %.3f    ", power_x, power_y, power_k);
            fflush(stdout);
        }
    }

    if (key_was_pressed) {
        XTestFakeKeyEvent(display, 65, False, 0);
        XFlush(display);
    }

    fclose(wav_file);
    XCloseDisplay(display);
    return 0;
}


int record_mode(const char *filename) {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    FILE *wav_file = fopen(filename, "wb");
    if (!wav_file) {
        fprintf(stderr, "Cannot open output file: %s\n", strerror(errno));
        XCloseDisplay(display);
        return 1;
    }

    // Write WAV header (mono)
    fwrite("RIFF", 1, 4, wav_file);
    uint32_t file_size = 0;  // placeholder
    fwrite(&file_size, sizeof(uint32_t), 1, wav_file);
    fwrite("WAVE", 1, 4, wav_file);
    fwrite("fmt ", 1, 4, wav_file);
    uint32_t fmt_size = 16;
    fwrite(&fmt_size, sizeof(uint32_t), 1, wav_file);
    uint16_t audio_format = 1;  // PCM
    fwrite(&audio_format, sizeof(uint16_t), 1, wav_file);
    uint16_t num_channels = 1;  // Mono
    fwrite(&num_channels, sizeof(uint16_t), 1, wav_file);
    uint32_t sample_rate = SAMPLE_RATE;
    fwrite(&sample_rate, sizeof(uint32_t), 1, wav_file);
    uint32_t byte_rate = SAMPLE_RATE * sizeof(AudioSample);
    fwrite(&byte_rate, sizeof(uint32_t), 1, wav_file);
    uint16_t block_align = sizeof(AudioSample);
    fwrite(&block_align, sizeof(uint16_t), 1, wav_file);
    uint16_t bits_per_sample = 16;
    fwrite(&bits_per_sample, sizeof(uint16_t), 1, wav_file);
    fwrite("data", 1, 4, wav_file);
    uint32_t data_size = 0;  // placeholder
    fwrite(&data_size, sizeof(uint32_t), 1, wav_file);

    Screen *screen = DefaultScreenOfDisplay(display);
    int screen_width = screen->width;
    int screen_height = screen->height;

    Window root = DefaultRootWindow(display);
    XSelectInput(display, root, KeyPressMask | KeyReleaseMask);
    
    AudioSample sample;
    char keys[32];
    double time = 0.0;
    size_t total_samples = 0;
    
    printf("Recording... Press Ctrl+C to stop.\n");
    printf("Using carrier frequencies:\n");
    printf("X position: %.1f Hz\n", FREQ_X);
    printf("Y position: %.1f Hz\n", FREQ_Y);
    printf("Keystrokes: %.1f Hz\n", FREQ_KEYS);

    while (running) {
        Window root_return, child_return;
        int root_x, root_y, win_x, win_y;
        unsigned int mask;

        XQueryPointer(display, root, &root_return, &child_return,
                     &root_x, &root_y, &win_x, &win_y, &mask);

        // Normalize positions to 0.0 - 1.0
        double x_mod = (double)root_x / screen_width;
        double y_mod = (double)root_y / screen_height;

        // Get keyboard state
        XQueryKeymap(display, keys);
        int key_pressed = 0;
        for (int i = 0; i < 32; i++) {
            if (keys[i]) {
                key_pressed = 1;
                break;
            }
        }

        // Combine three FM signals
        double signal = 
            0.33 * sin(2.0 * PI * FREQ_X * time + x_mod * PI) +    // X position
            0.33 * sin(2.0 * PI * FREQ_Y * time + y_mod * PI) +    // Y position
            0.33 * sin(2.0 * PI * FREQ_KEYS * time) * (key_pressed ? 1.0 : 0.0); // Keys

        sample.mono = (int16_t)(signal * 32767.0);

        if (fwrite(&sample, sizeof(AudioSample), 1, wav_file) != 1) {
            fprintf(stderr, "Error writing to file: %s\n", strerror(errno));
            break;
        }

        total_samples++;
        time += 1.0 / SAMPLE_RATE;

        // Show status every second
        if (total_samples % SAMPLE_RATE == 0) {
            printf("\rRecording: %zu seconds... X: %.2f Y: %.2f Keys: %s", 
                   total_samples / SAMPLE_RATE, x_mod, y_mod,
                   key_pressed ? "PRESSED" : "RELEASED");
            fflush(stdout);
        }
    }

    // Update WAV header with final sizes
    data_size = total_samples * sizeof(AudioSample);
    file_size = data_size + 36;
    
    fseek(wav_file, 4, SEEK_SET);
    fwrite(&file_size, sizeof(uint32_t), 1, wav_file);
    fseek(wav_file, 40, SEEK_SET);
    fwrite(&data_size, sizeof(uint32_t), 1, wav_file);

    fclose(wav_file);
    XCloseDisplay(display);
    return 0;
}


int listen_mode(void) {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    Screen *screen = DefaultScreenOfDisplay(display);
    int screen_width = screen->width;
    int screen_height = screen->height;
    int refresh_rate = get_monitor_refresh_rate(display);

    snd_pcm_t *capture_handle;
    if (setup_alsa_capture(&capture_handle) != 0) {
        XCloseDisplay(display);
        return 1;
    }

    AudioSample buffer[WINDOW_SIZE] = {0};
    int last_x = screen_width / 2;
    int last_y = screen_height / 2;
    bool key_was_pressed = false;
    double max_power_x = 0.0, max_power_y = 0.0, max_power_k = 0.0;

    printf("Listening at %dHz refresh rate...\n", refresh_rate);
    printf("Screen dimensions: %dx%d\n", screen_width, screen_height);
    printf("Carrier frequencies: X=%.1fHz, Y=%.1fHz, Keys=%.1fHz\n", 
           FREQ_X, FREQ_Y, FREQ_KEYS);

    struct timespec last_update, current_time;
    clock_gettime(CLOCK_MONOTONIC, &last_update);
    double update_interval = 1.0 / refresh_rate;

    while (running) {
        int err = snd_pcm_readi(capture_handle, buffer, WINDOW_SIZE);
        if (err < 0) {
            if (err == -EPIPE) {
                snd_pcm_prepare(capture_handle);
                continue;
            }
            fprintf(stderr, "Read error: %s\n", snd_strerror(err));
            break;
        }

        clock_gettime(CLOCK_MONOTONIC, &current_time);
        double elapsed = (current_time.tv_sec - last_update.tv_sec) +
                        (current_time.tv_nsec - last_update.tv_nsec) / 1e9;

        if (elapsed >= update_interval) {
            // Analyze frequencies
            double power_x = 0.0, power_y = 0.0, power_k = 0.0;
            
            for (int i = 0; i < WINDOW_SIZE; i++) {
                double t = (double)i / SAMPLE_RATE;
                double sample = buffer[i].mono / 32767.0;
                
                power_x += sample * sin(2.0 * PI * FREQ_X * t);
                power_y += sample * sin(2.0 * PI * FREQ_Y * t);
                power_k += sample * sin(2.0 * PI * FREQ_KEYS * t);
            }

            // Normalize and process powers
            power_x = fabs(power_x) / WINDOW_SIZE;
            power_y = fabs(power_y) / WINDOW_SIZE;
            power_k = fabs(power_k) / WINDOW_SIZE;

            max_power_x = fmax(max_power_x, power_x);
            max_power_y = fmax(max_power_y, power_y);
            max_power_k = fmax(max_power_k, power_k);

            if (max_power_x > 0.0) power_x /= max_power_x;
            if (max_power_y > 0.0) power_y /= max_power_y;
            if (max_power_k > 0.0) power_k /= max_power_k;

            // Convert to screen coordinates
            int x = (int)(power_x * screen_width);
            int y = (int)(power_y * screen_height);
            
            x = (x + last_x * 7) / 8;
            y = (y + last_y * 7) / 8;
            
            x = x < 0 ? 0 : (x >= screen_width ? screen_width - 1 : x);
            y = y < 0 ? 0 : (y >= screen_height ? screen_height - 1 : y);

            if (power_x > 0.1 || power_y > 0.1) {
                XWarpPointer(display, None, DefaultRootWindow(display), 
                            0, 0, 0, 0, x, y);
            }

            bool key_pressed = (power_k > 0.5);
            if (key_pressed != key_was_pressed) {
                XTestFakeKeyEvent(display, 65, key_pressed, 0);
                key_was_pressed = key_pressed;
            }

            XFlush(display);
            
            last_x = x;
            last_y = y;
            last_update = current_time;

            printf("\rX: %4d (%.3f)  Y: %4d (%.3f)  Key: %.3f   ", 
                   x, power_x, y, power_y, power_k);
            fflush(stdout);
        }
    }

    if (key_was_pressed) {
        XTestFakeKeyEvent(display, 65, False, 0);
        XFlush(display);
    }

    snd_pcm_close(capture_handle);
    XCloseDisplay(display);
    return 0;
}

// ALSA setup function
int setup_alsa_capture(snd_pcm_t **capture_handle) {
    int err;
    if ((err = snd_pcm_open(capture_handle, "default", SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "Cannot open audio device: %s\n", snd_strerror(err));
        return 1;
    }

    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_malloc(&hw_params);
    snd_pcm_hw_params_any(*capture_handle, hw_params);
    snd_pcm_hw_params_set_access(*capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(*capture_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(*capture_handle, hw_params, 1);  // Mono
    
    unsigned int rate = SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(*capture_handle, hw_params, &rate, 0);
    
    snd_pcm_uframes_t buffer_size = WINDOW_SIZE;
    snd_pcm_hw_params_set_buffer_size_near(*capture_handle, hw_params, &buffer_size);

    if ((err = snd_pcm_hw_params(*capture_handle, hw_params)) < 0) {
        fprintf(stderr, "Cannot set parameters: %s\n", snd_strerror(err));
        snd_pcm_hw_params_free(hw_params);
        return 1;
    }

    snd_pcm_hw_params_free(hw_params);
    return 0;
}



int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [filename]\n", argv[0]);
        fprintf(stderr, "Commands:\n");
        fprintf(stderr, "  record <filename>  - Record mouse/keys to WAV\n");
        fprintf(stderr, "  play <filename>    - Play WAV file as mouse/keys\n");
        fprintf(stderr, "  listen            - Listen to audio input\n");
        return 1;
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (strcmp(argv[1], "record") == 0 && argc == 3) {
        return record_mode(argv[2]);
    } else if (strcmp(argv[1], "play") == 0 && argc == 3) {
        return playback_mode(argv[2]);
    } else if (strcmp(argv[1], "listen") == 0) {
        return listen_mode();
    } else {
        fprintf(stderr, "Invalid command or missing filename\n");
        return 1;
    }
}
