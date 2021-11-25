#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <GL/glx.h>

#include <alsa/asoundlib.h>

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;

typedef signed char         s8;
typedef signed short        s16;
typedef signed int          s32;
typedef signed long long    s64;

typedef float  f32;
typedef double f64;

typedef u32 RGBA;

struct Glyph {
    u8 width;
    u8 height;
    u8 data[32];
};

int WINDOW_WIDTH  = 800;
int WINDOW_HEIGHT = 600;
const char *WINDOW_CAPTION = "Pong";

void set_projection_matrix(f32 width, f32 height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Convert from screen space (left-right, top-down) to NDC
    GLfloat proj_mat[] = {
        2.0f / width,  0.0f,           0.0f, -1.0f,
        0.0f,          2.0f / -height, 0.0f, 1.0f,
        0.0f,          0.0f,           1.0f, 0.0f,
        0.0f,          0.0f,           0.0f, 1.0f
    };
    glLoadTransposeMatrixf(proj_mat);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void draw_quad(f32 x, f32 y, f32 width, f32 height, RGBA color = 0) {
    set_projection_matrix(800.0f, 600.0f);

    glBegin(GL_QUADS);
        f32 r = ((color >> 24) & 0xFF) / 255.0f;
        f32 g = ((color >> 16) & 0xFF) / 255.0f;
        f32 b = ((color >>  8) & 0xFF) / 255.0f;
        glColor3f(r, g, b);
        glVertex2f(x, y);
        glVertex2f(x + width, y);
        glVertex2f(x + width, y + height);
        glVertex2f(x, y + height);
    glEnd();
}

void draw_line(f32 xstart, f32 ystart, f32 xend, f32 yend) {
    set_projection_matrix(800.0f, 600.0f);

    glBegin(GL_LINES);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex2f(xstart, ystart);
        glVertex2f(xend, yend);
    glEnd();
}

void draw_number(u8 number, f32 x, f32 y) {
    // assert(0 <= number <= 9);
    u8 font_data[10][3*5] = {
        {1,1,1,  1,0,1,  1,0,1,  1,0,1,  1,1,1},    // 0
        {1,1,0,  0,1,0,  0,1,0,  0,1,0,  1,1,1},    // 1
        {1,1,1,  0,0,1,  1,1,1,  1,0,0,  1,1,1},    // 2
        {1,1,1,  0,0,1,  0,1,1,  0,0,1,  1,1,1},    // 3
        {1,0,1,  1,0,1,  1,1,1,  0,0,1,  0,0,1},    // 4
        {1,1,1,  1,0,0,  1,1,1,  0,0,1,  1,1,1},    // 5
        {1,1,1,  1,0,0,  1,1,1,  1,0,1,  1,1,1},    // 6
        {1,1,1,  0,0,1,  0,0,1,  0,0,1,  0,0,1},    // 7
        {1,1,1,  1,0,1,  1,1,1,  1,0,1,  1,1,1},    // 8
        {1,1,1,  1,0,1,  1,1,1,  0,0,1,  1,1,1}     // 9
    };

    f32 pixel_dim = 10.0f;

    u8 *pixels = font_data[number];
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 3; col++) {
            if (pixels[3*row + col]) {
                draw_quad(x + col*pixel_dim, y + row*pixel_dim, pixel_dim, pixel_dim, 0xFFFFFFFF);
            }
        }
    }
}

f32 draw_character(char c, f32 x, f32 y, f32 pixel_dim = 5.0f) {
    Glyph glyphs[] = {
        {4, 5, {0,1,1,0,    1,0,0,1,    1,1,1,1,    1,0,0,1,    1,0,0,1}},  // A
        {4, 5, {1,1,1,0,    1,0,0,1,    1,1,1,1,    1,0,0,1,    1,1,1,0}},  // B
        {4, 5, {0,1,1,1,    1,0,0,0,    1,0,0,0,    1,0,0,0,    0,1,1,1}},  // C
        {4, 5, {1,1,1,0,    1,0,0,1,    1,0,0,1,    1,0,0,1,    1,1,1,0}},  // D
        {4, 5, {1,1,1,1,    1,0,0,0,    1,1,1,0,    1,0,0,0,    1,1,1,1}},  // E
        {4, 5, {1,1,1,1,    1,0,0,0,    1,1,1,0,    1,0,0,0,    1,0,0,0}},  // F
        {4, 5, {0,1,1,1,    1,0,0,0,    1,0,1,1,    1,0,0,1,    0,1,1,1}},  // G
        {4, 5, {1,0,0,1,    1,0,0,1,    1,1,1,1,    1,0,0,1,    1,0,0,1}},  // H
        {3, 5, {1,1,1,      0,1,0,      0,1,0,      0,1,0,      1,1,1}},    // I (3x5)
        {4, 5, {0,0,1,1,    0,0,0,1,    0,0,0,1,    1,0,0,1,    1,1,1,1}},  // J
        {4, 5, {1,0,0,1,    1,0,1,0,    1,1,1,1,    1,0,0,1,    1,0,0,1}},  // K
        {4, 5, {1,0,0,0,    1,0,0,0,    1,0,0,0,    1,0,0,1,    1,1,1,1}},  // L
        {5, 5, {1,0,0,0,1,  1,1,0,1,1,  1,0,1,0,1,  1,0,0,0,1,  1,0,0,0,1}},// M (5x5)
        {5, 5, {1,0,0,0,1,  1,1,0,0,1,  1,0,1,0,1,  1,0,0,1,1,  1,0,0,0,1}},// N (5x5)
        {4, 5, {0,1,1,0,    1,0,0,1,    1,0,0,1,    1,0,0,1,    0,1,1,0}},  // O
        {4, 5, {1,1,1,1,    1,0,0,1,    1,1,1,1,    1,0,0,0,    1,0,0,0}},  // P
        {5, 5, {0,1,1,0,0,  1,0,0,1,0,  1,0,0,1,0,  1,0,0,1,0,  0,1,1,1,1}},// Q (5x5)
        {4, 5, {1,1,1,0,    1,0,0,1,    1,1,1,1,    1,0,1,0,    1,0,0,1}},  // R
        {4, 5, {0,1,1,1,    1,0,0,0,    1,1,1,1,    0,0,0,1,    1,1,1,1}},  // S
        {5, 5, {1,1,1,1,1,  0,0,1,0,0,  0,0,1,0,0,  0,0,1,0,0,  0,1,1,1,0}},// T
        {4, 5, {1,0,0,1,    1,0,0,1,    1,0,0,1,    1,0,0,1,    0,1,1,1}},  // U
        {5, 5, {1,0,0,0,1,  1,0,0,0,1,  1,0,0,0,1,  0,1,0,1,0,  0,0,1,0,0}},// V (5x5)
        {5, 5, {1,0,0,0,1,  1,0,0,0,1,  1,0,1,0,1,  1,1,0,1,1,  1,0,0,0,1}},// W (5x5)
        {5, 5, {1,0,0,0,1,  0,1,0,1,0,  0,0,1,0,0,  0,1,0,1,0,  1,0,0,0,1}},// X (5x5)
        {4, 5, {1,0,0,1,    1,0,0,1,    1,1,1,1,    0,0,0,1,    1,1,1,0}},  // Y
        {4, 5, {1,1,1,1,    0,0,0,1,    0,1,1,0,    1,0,0,0,    1,1,1,1}}   // Z
    };
    if (c >= 'A' && c <= 'Z') {
        Glyph *glyph = glyphs + (c - 'A');
        for (int row = 0; row < glyph->height; row++) {
            for (int col = 0; col < glyph->width; col++) {
                if (glyph->data[glyph->width*row + col]) {
                    draw_quad(x + col*pixel_dim, y + row*pixel_dim, pixel_dim, pixel_dim, 0xFFFFFFFF);
                }
            }
        }
        return (glyph->width + 1) * pixel_dim;
    }
    return 3 * pixel_dim;
}

void draw_string(const char *str, f32 x, f32 y, f32 pixel_dim = 5.0f) {
    int n = strlen(str);
    f32 xoffset = x;
    for (int i = 0; i < n; i++) {
        f32 stride = draw_character(str[i], xoffset, y, pixel_dim);
        xoffset += stride;
    }
}

enum GameState {
    TUTORIAL,
    PLAY,
    GAMEOVER,
};

struct Mixer {
    snd_pcm_t *device;
    u16 sample_rate;
    u16 channel_count;

    int samples_to_play;
    int sample_offset;
    s16 samples[48000];
};

void check(int ret) {
    if (ret < 0) {
        fprintf(stderr, "error: %s (%d)\n", snd_strerror(ret), ret);
        exit(1);
    }
}

void mixer_init(Mixer *mixer) {
    mixer->sample_rate = 48000;
    mixer->channel_count = 1;

    check(snd_pcm_open(&mixer->device, "default", SND_PCM_STREAM_PLAYBACK, 0));
    snd_pcm_t *pcm = mixer->device;

    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params);

    check(snd_pcm_hw_params_any(pcm, hw_params));
    check(snd_pcm_hw_params_set_access(pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED));
    check(snd_pcm_hw_params_set_format(pcm, hw_params, SND_PCM_FORMAT_S16_LE));
    check(snd_pcm_hw_params_set_channels(pcm, hw_params, 1));
    check(snd_pcm_hw_params_set_rate(pcm, hw_params, 48000, 0));
    check(snd_pcm_hw_params_set_periods(pcm, hw_params, 4, 0));
    check(snd_pcm_hw_params_set_period_time(pcm, hw_params, 16667, 0));

    check(snd_pcm_hw_params(pcm, hw_params));
}

void mixer_update(Mixer *mixer) {
    if (mixer->samples_to_play <= 0) {
        s16 silence[800] = {0};
        check(snd_pcm_writei(mixer->device, silence, 800));
    } else {
        check(snd_pcm_writei(mixer->device, mixer->samples + mixer->sample_offset, 800));
        mixer->sample_offset = (mixer->sample_offset + 800) % 48000;
        mixer->samples_to_play -= 800;
    }
}

void mixer_shutdown(Mixer *mixer) {
    snd_pcm_drain(mixer->device);
    snd_pcm_close(mixer->device);
}

void play_sound(Mixer *mixer, f32 freq, u32 ms) {
    for (int i = 0; i < 48000; i++) {
        mixer->samples[i] = 10000 * sinf(((f32)i / 48000) * 2 * M_PI * freq);
    }

    mixer->samples_to_play += ms * 48000 / 1000;
}

int main() {
    Mixer mixer = {};
    mixer_init(&mixer);

    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        fprintf(stderr, "Unable to connect to display, check your XServer configuration\n");
        exit(1);
    }

    Window window = XCreateSimpleWindow(
            display,
            XDefaultRootWindow(display),
            0, 0,
            WINDOW_WIDTH, WINDOW_HEIGHT,
            0,
            0x00000000,
            0x00000000 // 0x00FF00FF
            );
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        exit(1);
    }

    XStoreName(display, window, WINDOW_CAPTION);
    XSelectInput(display, window, KeyPressMask|KeyReleaseMask);

    XMapWindow(display, window);

    XWindowAttributes win_attribs = {};
    XGetWindowAttributes(display, window, &win_attribs);
    printf("dimensions: (%d, %d)\n", win_attribs.width, win_attribs.height);

    int visual_attributes[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
    XVisualInfo *visual = glXChooseVisual(display, 0, visual_attributes);
    if (!visual) {
        fprintf(stderr, "Unable to choose a visual appropriate for OpenGL\n");
        exit(1);
    }
    GLXContext gl_context = glXCreateContext(display, visual, nullptr, True);
    if (!gl_context) {
        fprintf(stderr, "Failed to create GL context\n");
        exit(1);
    }
    glXMakeCurrent(display, window, gl_context);

    glViewport(0, 0, win_attribs.width, win_attribs.height); 

    RGBA paddle_color = 0xFFFFFFFF;
    RGBA ball_color   = 0xFFFFFFFF;

    f32 paddle_width   = WINDOW_WIDTH / 20.0f;
    f32 paddle_height  = paddle_width * 4; 
    f32 paddle_left_x  = 0.0f;
    f32 paddle_left_y  = WINDOW_HEIGHT / 2.0f - paddle_height / 2.0;
    f32 paddle_right_x = WINDOW_WIDTH - paddle_width;
    f32 paddle_right_y = paddle_left_y;
    f32 paddle_speed   = 10.0f;

    f32 ball_width  = paddle_width * 0.5;
    f32 ball_height = ball_width;
    f32 ball_start_x = (WINDOW_WIDTH / 2.0) - (ball_width / 2.0);
    f32 ball_start_y = (WINDOW_HEIGHT / 2.0f) - (ball_height / 2.0);
    f32 ball_x = ball_start_x;
    f32 ball_y = ball_start_y;
    f32 ball_vx = 2 * -5.0f;// / 10.0f; // TODO: Set velocity to a low value to troubleshoot collision
    f32 ball_vy = 2 * 2.0f;// / 10.0f;

    GameState state = TUTORIAL;
    bool multiplayer = false;
    int score_left = 0;
    int score_right = 0;

    bool first_move = false;
    bool up_pressed = false;
    bool down_pressed = false;
    bool w_pressed = false;
    bool s_pressed = false;

    bool quit = false;
    f32 dt = 1.0f;
    while (!quit) {
        while (XPending(display) > 0) {
            XEvent event = {};
            XNextEvent(display, &event);
            switch (event.type) {
                case KeyPress: {
                                   KeySym keysym = XLookupKeysym(&event.xkey, 0);
                                   if (keysym == XK_Escape) {
                                       printf("Exiting...\n");
                                       quit = true;
                                   }
                                   if (keysym == XK_Up)     {up_pressed = true;}
                                   if (keysym == XK_Down)   {down_pressed = true;}
                                   if (keysym == XK_w)  {w_pressed = true;}
                                   if (keysym == XK_s)  {s_pressed = true;}
                               } break;
                case KeyRelease: {
                                     KeySym keysym = XLookupKeysym(&event.xkey, 0);
                                     if (keysym == XK_Up)   {up_pressed = false;}
                                     if (keysym == XK_Down)     {down_pressed = false;}
                                     if (keysym == XK_w)    {w_pressed = false;}
                                     if (keysym == XK_s)    {s_pressed = false;}
                                 } break;
            }
        }

        // Update and Render
        mixer_update(&mixer);

        RGBA bg_color = 0x121210FF;
        f32 r = ((bg_color >> 24) & 0xFF) / 255.0;
        f32 g = ((bg_color >> 16) & 0xFF) / 255.0;
        f32 b = ((bg_color >>  8) & 0xFF) / 255.0;
        f32 a = ((bg_color >>  0) & 0xFF) / 255.0;
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        switch (state) {
            case TUTORIAL: {
                               draw_string("PRESS UP OR DOWN TO MOVE", WINDOW_WIDTH * 0.12, 20);
                               if (up_pressed || down_pressed || w_pressed || s_pressed) {
                                   state = PLAY;
                                   score_left = 0;
                                   score_right = 0;
                               }
                           } break;
            case PLAY: {
                           if (score_left >= 10 || score_right >= 10) {
                               state = GAMEOVER;
                           }
                       } break;
            case GAMEOVER: {
                               draw_string("GAME OVER", WINDOW_WIDTH * 0.2, 20, 10);
                               draw_string("PRESS UP OR DOWN TO MOVE", WINDOW_WIDTH * 0.12, 100);
                               if (up_pressed || down_pressed) {
                                   state = PLAY;
                                   score_left = 0;
                                   score_right = 0;
                               }
                           } break;
            default:
                           fprintf(stderr, "Invalid game state: %d\n", state);
                           exit(1);
        }


        if (state == PLAY) {
            // Update Paddles
            if (up_pressed) {
                paddle_right_y -= paddle_speed*dt;
            }
            if (down_pressed) {
                paddle_right_y += paddle_speed*dt;
            }
            if (w_pressed) {
                paddle_left_y -= paddle_speed*dt;
            }
            if (s_pressed) {
                paddle_left_y += paddle_speed*dt;
            }

            // Update Ball
            ball_x = ball_x + ball_vx*dt;
            ball_y = ball_y + ball_vy*dt;

            if (ball_x < 0.0f) {
                ball_x = ball_start_x;
                score_right += 1;
            }

            if (ball_x > WINDOW_WIDTH) {
                ball_x = ball_start_x;
                score_left += 1;
            }

            if (ball_x < (paddle_left_x + paddle_width) &&
                    (ball_y + ball_height) > paddle_left_x &&
                    ball_y < (paddle_left_y + paddle_height) &&
                    ball_vx < 0.0f)
            {
                play_sound(&mixer, 400.0f, 100);
                ball_vx = -ball_vx;
            }
            if ((ball_x + ball_width) > paddle_right_x &&
                    (ball_y + ball_height) > paddle_right_y &&
                    ball_y < (paddle_right_y + paddle_height) &&
                    ball_vx > 0.0f)
            {
                play_sound(&mixer, 200.0f, 100);
                ball_vx = -ball_vx;
            }


            // if ((ball_x + ball_width) > WINDOW_WIDTH) {ball_vx = -ball_vx;}
            if (ball_y < 0.0f || (ball_y + ball_height) > WINDOW_HEIGHT) {ball_vy = -ball_vy;}

            // Score
            draw_number(score_left,  WINDOW_WIDTH * 0.25, 20);
            draw_number(score_right, WINDOW_WIDTH * 0.75, 20);

            // Ball
            draw_quad(ball_x, ball_y, ball_width, ball_height, ball_color);
            if (!multiplayer) {
                if (ball_vx > 0.0 && ball_x > 0.8*WINDOW_WIDTH) {
                    f32 tx = (800.0 - ball_x) / ball_vx;
                    f32 new_y = (ball_y + ball_vy * tx) - paddle_height / 2.0;
                    if (new_y > 0.0 && new_y < WINDOW_HEIGHT) {
                        if (fabs(paddle_right_y - new_y) < 10) {up_pressed = false; down_pressed = false;}
                        else if (new_y < paddle_right_y) {up_pressed = true;  down_pressed = false;}
                        else if (new_y > paddle_right_y) {up_pressed = false; down_pressed = true;}
                    }
                    // draw_line(ball_x, ball_y, ball_x + ball_vx * tx, ball_y + ball_vy * tx);
                } else {
                    up_pressed = false;
                    down_pressed = false;
                }
            }

            // Net
            f32 net_width = 20.0f;
            int net_segment_count = 10;
            f32 net_segment_height = WINDOW_HEIGHT / (net_segment_count * 2.0);
            f32 x = WINDOW_WIDTH / 2.0 - net_width / 2.0;
            for (int i = 0; i < net_segment_count; i++) {
                f32 y = (2.0 * i * net_segment_height) + (net_segment_height / 2.0);
                draw_quad(x, y, net_width, net_segment_height, 0xFFFFFFFF);
            }
        }

        // Paddles
        draw_quad(paddle_left_x,  paddle_left_y,  paddle_width, paddle_height, paddle_color);
        draw_quad(paddle_right_x, paddle_right_y, paddle_width, paddle_height, paddle_color);

        glXSwapBuffers(display, window);
    }

    XCloseDisplay(display);
    mixer_shutdown(&mixer);

    return 0;
}
