#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "vector.h"

#include <X11/Xlib.h>

#define GRID_SIZE 25
#define GRID_COLOR_1 0x00000000
#define GRID_COLOR_2 0x00111111

#define SNAKE_SIZE GRID_SIZE
#define SNAKE_HEAD_COLOR 0x0000AA00
#define SNAKE_COLOR 0x0000FF00

#define FOOD_SIZE GRID_SIZE
#define FOOD_PADDING GRID_SIZE/10
#define FOOD_COLOR 0x00F00F00

#define HEIGHT 40*GRID_SIZE
#define WIDTH 40*GRID_SIZE

Vector snake_body = {0};
Point velocity = (Point){GRID_SIZE, 0};
Point food_pos = {0};

int8_t in_bounds(int32_t x, int32_t y, int64_t width, int64_t height);
void gc_put_pixel(void *memory, int32_t x, int32_t y, uint32_t color);
void gc_fill_rectangle(void *memory, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);
void gc_fill_circle(void *memory, int32_t cx, int32_t cy, int32_t r, uint32_t color);
void update(Display *display, GC *graphics, Window *window, XImage *image);

void snake_apply_velocity()
{
    Node *tail = snake_body.tail;
    while(tail->last != NULL) {
        tail->value = tail->last->value;
        tail = tail->last;
    }
    Node *head = snake_body.head;
    head->value.x += (int)velocity.x;
    head->value.y += (int)velocity.y;
    if(head->value.x < 0) head->value.x = WIDTH-SNAKE_SIZE;
    else if(head->value.x > WIDTH-SNAKE_SIZE) head->value.x = 0;
    if(head->value.y < 0) head->value.y = HEIGHT-SNAKE_SIZE;
    else if(head->value.y > HEIGHT-SNAKE_SIZE) head->value.y = 0;

}

void generate_new_food()
{
    food_pos.x = rand()%(WIDTH/FOOD_SIZE);
    food_pos.y = rand()%(HEIGHT/FOOD_SIZE);
}

void snake_food_collision()
{
    Point snake_head_pos = snake_body.head->value;
    if(snake_head_pos.x/FOOD_SIZE == food_pos.x && snake_head_pos.y/FOOD_SIZE == food_pos.y) {
        generate_new_food();
        if(snake_body.size > 1) {
            Point last = snake_body.tail->value;
            v_push_back(&snake_body, last.x - velocity.x, last.y - velocity.y);
        } else {
            v_push_back(&snake_body, snake_head_pos.x - velocity.x, snake_head_pos.y - velocity.y);
        }
    }
}

void show_grid(void *memory)
{
    for(int j = 0; j < HEIGHT/GRID_SIZE; ++j) {
        for(int i = 0; i < WIDTH/GRID_SIZE; ++i) {
            if((j+i) % 2 == 0) {
                gc_fill_rectangle(memory, i*GRID_SIZE, j*GRID_SIZE, (i+1)*GRID_SIZE, (j+1)*GRID_SIZE, GRID_COLOR_1);
            } else {
                gc_fill_rectangle(memory, i*GRID_SIZE, j*GRID_SIZE, (i+1)*GRID_SIZE, (j+1)*GRID_SIZE, GRID_COLOR_2);
            }
        }
    }
}

void show_snake(void *memory)
{
    Node *head = snake_body.head;
    Point body_part = head->value;
    gc_fill_rectangle(
        memory,
        body_part.x, body_part.y,
        body_part.x+SNAKE_SIZE, body_part.y+SNAKE_SIZE,
        SNAKE_HEAD_COLOR
    );
    head = head->next;
    while(head != NULL) {
        body_part = head->value;
        gc_fill_rectangle(
            memory,
            body_part.x, body_part.y,
            body_part.x+SNAKE_SIZE, body_part.y+SNAKE_SIZE,
            SNAKE_COLOR
        );
        head = head->next;
    }
}


void show_food(void *memory)
{
    gc_fill_circle(
        memory,
        food_pos.x*FOOD_SIZE+FOOD_SIZE/2, food_pos.y*FOOD_SIZE+FOOD_SIZE/2,
        FOOD_SIZE/2-FOOD_PADDING,
        FOOD_COLOR
    );
}

uint32_t decodeRGB(uint8_t r, uint8_t g, uint8_t b)
{
    return (r << 16) + (g << 8) + b;
}

int8_t exitloop = 0;
int8_t auto_update = 0;

int main(void)
{
    v_push_back(&snake_body, WIDTH/2, HEIGHT/2);
    srand(time(NULL));
    Display *display = XOpenDisplay(NULL);

    int screen = DefaultScreen(display);

    Window window = XCreateSimpleWindow(
        display, RootWindow(display, screen),
        0, 0,
        WIDTH, HEIGHT,
        0, 0,
        0
    );

    char *memory = (char *) malloc(sizeof(uint32_t)*HEIGHT*WIDTH);

    XWindowAttributes winattr = {0};
    XGetWindowAttributes(display, window, &winattr);

    XImage *image = XCreateImage(
        display, winattr.visual, winattr.depth,
        ZPixmap, 0, memory,
        WIDTH, HEIGHT,
        32, WIDTH*4
    );

    GC graphics = XCreateGC(display, window, 0, NULL);

    Atom delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(display, window, &delete_window, 1);

    Mask key = KeyPressMask | KeyReleaseMask;
    XSelectInput(display, window, ExposureMask | key);

    XMapWindow(display, window);
    XSync(display, False);

    XEvent event;

    update(display, &graphics, &window, image);

    while(!exitloop) {
        while(XPending(display) > 0) {
            XNextEvent(display, &event);
            switch(event.type) {
                case Expose: {
                    update(display, &graphics, &window, image);
                    break;
                }
                case ClientMessage: {
                    if((Atom) event.xclient.data.l[0] == delete_window) {
                        exitloop = 1;   
                    }
                    break;
                }
                case KeyPress: {
                    if(event.xkey.keycode == 0x24) {
                        snake_apply_velocity();
                        snake_food_collision();
                        show_grid(memory);
                        show_snake(memory);
                        show_food(memory);
                        update(display, &graphics, &window, image);
                    }
                    if(event.xkey.keycode == 0x41) {
                        auto_update = !auto_update;
                    }
                    switch(event.xkey.keycode) {
                        case 0x6f: {
                            velocity = (Point){0, -GRID_SIZE};
                            break;
                        }
                        case 0x71: {
                            velocity = (Point){-GRID_SIZE, 0};
                            break;
                        }
                        case 0x74: {
                            velocity = (Point){0, GRID_SIZE};
                            break;
                        }
                        case 0x72: {
                            velocity = (Point){GRID_SIZE, 0};
                            break;
                        }
                    }
                    break;
                }
            }
        }

        if(auto_update) {
            snake_apply_velocity();
            snake_food_collision();
            show_grid(memory);
            show_snake(memory);
            show_food(memory);
            update(display, &graphics, &window, image);
            usleep(6944*12);
        }
    }


    XCloseDisplay(display);

    free(memory);

    return 0;
}

void update(Display *display, GC *graphics, Window *window, XImage *image)
{
    XPutImage(
        display,
        *window,
        *graphics,
        image,
        0, 0,
        0, 0,
        WIDTH, HEIGHT
    );

    XSync(display, False);
}

int8_t in_bounds(int32_t x, int32_t y, int64_t w, int64_t h)
{
    return (x >= 0 && x < w && y >= 0 && y < h);
}

void gc_put_pixel(void *memory, int32_t x, int32_t y, uint32_t color)
{
    if(in_bounds(x, y, WIDTH, HEIGHT))
        *((uint32_t *) memory + y * WIDTH + x) = color;
}

void gc_fill_rectangle(void *memory, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
{
    for(int j = y0; j <= y1; ++j) {
        for(int i = x0; i <= x1; ++i) {
            gc_put_pixel(memory, i, j, color);
        }
    }
}

void gc_fill_circle(void *memory, int32_t cx, int32_t cy, int32_t r, uint32_t color)
{
    for(int j = -r; j <= r; ++j) {
        for(int i = -r; i <= r; ++i) {
            int sqr_dist = j*j + i*i;
            if(sqr_dist <= r*r)
                gc_put_pixel(memory, cx + i, cy + j, color);
        }
    }
}





