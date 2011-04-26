#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <stdlib.h>
#include <time.h>

#define CLEARSCR()      (fillsquare(screen, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bgcolor))

typedef struct {
    int x, y;
} coords;

typedef struct cat_instance cat_instance;
struct cat_instance {
    coords loc;
    cat_instance* next;
};

typedef struct sparkle_instance sparkle_instance;
struct sparkle_instance {
    unsigned int frame, speed;
    int frame_mov;
    unsigned int layer;
    coords loc;
    sparkle_instance* next;
};

SDL_Surface* cat_img[5];
SDL_Surface* sparkle_img[5];
sparkle_instance* sparkles_list = NULL;
cat_instance* cat_list = NULL;
Uint8 bgcolor;

void add_sparkle(void);
void add_cat(unsigned int x, unsigned int y);
void draw_cats(unsigned int frame);
void draw_sparkles(unsigned int layer);
void fillsquare(SDL_Surface* surf, int x, int y, int w, int h, Uint32 col);
void handleinput(void);
void load_images(void);
void putpix(SDL_Surface* surf, int x, int y, Uint32 col);
void remove_sparkle(sparkle_instance* s);
void update_sparkles(void);

unsigned int            SCREEN_WIDTH = 1920;
unsigned int            SCREEN_HEIGHT = 1080;
unsigned int            FRAMERATE = 12;
unsigned int            SCREEN_BPP = 32;

static                  SDL_Surface *screen = NULL;
static                  SDL_Event event;
static int              running = 1;
static int              SURF_TYPE = SDL_HWSURFACE;

void
add_sparkle(void) {
    sparkle_instance* s = sparkles_list;
    sparkle_instance* new;

    new = malloc(sizeof(sparkle_instance));

    new->loc.x = SCREEN_WIDTH + 80;
    new->loc.y = (rand() % (SCREEN_HEIGHT + sparkle_img[0]->h)) - sparkle_img[0]->h;
    new->frame = rand() % 4;
    new->frame_mov = 1;
    new->speed = 10 + (rand() % 30);
    new->layer = rand() % 2;
    new->next = NULL;

    if (!sparkles_list) {
        sparkles_list = new;
        return;
    }

    /* Find end of list */
    while (s->next)
        s = s->next;
    
    s->next = new;
}

void
add_cat(unsigned int x, unsigned int y) {
    cat_instance* c = cat_list;
    cat_instance* new;

    new = malloc(sizeof(cat_instance));

    new->loc.x = x;
    new->loc.y = y;
    new->next = NULL;

    if (!cat_list) {
        cat_list = new;
        return;
    }

    /* Find end of list */
    while (c->next)
        c = c->next;

    c->next = new;
}

void
draw_cats(unsigned int frame) {
    cat_instance* c = cat_list;;
    SDL_Rect pos;

    while (c) {

        pos.x = c->loc.x;
        pos.y = c->loc.y;

        if(frame == 0)
            pos.y -= 5;
        
        SDL_BlitSurface( cat_img[frame], NULL, screen, &pos );

        c = c->next;
    }
}

void
draw_sparkles(unsigned int layer) {
    sparkle_instance* s = sparkles_list;
    SDL_Rect pos;

    while (s) {
        if (s->layer == layer) {
            pos.x = s->loc.x;
            pos.y = s->loc.y;
            SDL_BlitSurface( sparkle_img[s->frame], NULL, screen, &pos );
        }
        s = s->next;
    }
}

void
fillsquare(SDL_Surface* surf, int x, int y, int w, int h, Uint32 col) {
    int i, e;

    /* Placement of random pixels for every pixel on the screen */
    for (i = 0; i < h; i++)
        for (e = 0; e < w; e++) {
            putpix(surf, x + e, y + i, col);
        }
}

void
handleinput(void) {
    while( SDL_PollEvent( &event ) ) {   
        switch (event.type) {
            case SDL_KEYDOWN:
            case SDL_QUIT:
            case SDL_MOUSEMOTION:
                running = 0;
                break;
        }
    }
}

void
load_images(void) {
    cat_img[0] = IMG_Load("res/frame00.png");
    cat_img[1] = IMG_Load("res/frame01.png");
    cat_img[2] = IMG_Load("res/frame02.png");
    cat_img[3] = IMG_Load("res/frame03.png");
    cat_img[4] = IMG_Load("res/frame04.png");

    sparkle_img[0] = IMG_Load("res/sparkle0.png");
    sparkle_img[1] = IMG_Load("res/sparkle1.png");
    sparkle_img[2] = IMG_Load("res/sparkle2.png");
    sparkle_img[3] = IMG_Load("res/sparkle3.png");
    sparkle_img[4] = IMG_Load("res/sparkle4.png");
}

void
putpix(SDL_Surface* surf, int x, int y, Uint32 col) {
    Uint32 *pix = (Uint32 *) surf->pixels;
    pix [ ( y * surf->w ) + x ] = col;
}

void
remove_sparkle(sparkle_instance* s) {
    sparkle_instance* s2 = sparkles_list;

    if (s2 == s) {
        sparkles_list = s->next;
        free(s);
        return;
    }

    while (s2->next != s)
        s2 = s2->next;

    s2->next = s2->next->next;
    free(s);
}

void
update_sparkles(void) {
    sparkle_instance* next, *s = sparkles_list;

    while(s) {
        s->loc.x -= s->speed;
        next = s->next;

        s->frame += s->frame_mov;

        if(s->frame > 3 || s->frame < 1)
            s->frame_mov = 0 - s->frame_mov;

        if (s->loc.x < 0 - sparkle_img[0]->w)
            remove_sparkle(s);

        s = next;
    }
}

int main( int argc, char *argv[] )
{
    int i, draw_time, last_draw, curr_frame = 0;

    /* Handle flags */
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-hw"))
            SURF_TYPE = SDL_HWSURFACE;
        else if (!strcmp(argv[i], "-sw"))
            SURF_TYPE = SDL_SWSURFACE;
    }

    srand( time(NULL) );

    SDL_Init( SDL_INIT_EVERYTHING );
    screen = SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SURF_TYPE);

    bgcolor = SDL_MapRGB(screen->format, 0, 51, 102);

    load_images();

    add_cat((SCREEN_WIDTH - cat_img[0]->w) / 2 , (SCREEN_HEIGHT - cat_img[0]->h) / 2);

    /* clear initial input */
    while( SDL_PollEvent( &event ) ) {}

    /* Main loop */
    while( running )
    {
        last_draw = SDL_GetTicks();

        add_sparkle();
        add_sparkle();

        CLEARSCR();
        draw_sparkles(0);
        draw_cats(curr_frame);
        draw_sparkles(1);

        update_sparkles();
        handleinput();
        SDL_Flip(screen);

        /* Frame increment and looping */
        curr_frame++;
        if (curr_frame > 4)
            curr_frame = 0;

        draw_time = SDL_GetTicks() - last_draw;
        if (draw_time < (1000 / FRAMERATE))
            SDL_Delay((1000 / FRAMERATE) - draw_time);
    }

    SDL_Quit();
    return 0;
}
