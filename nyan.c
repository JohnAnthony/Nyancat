/* ============================================================================================ */
/* This software is created by LAMMJohnson and comes with no warranty of any kind.              */
/*                                                                                              */
/* If you like this software and would like to contribute to its continued improvement          */
/* then please feel free to submit bug reports here: www.github.com/LAMMJohnson                 */
/*                                                                                              */
/* This program is licensed under the GPLv3 and in support of Free and Open Source              */
/* Software in general. The full license can be found at http://www.gnu.org/licenses/gpl.html   */
/* ============================================================================================ */

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef XINERAMA
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */

/* Type definitions */
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

/* Predecs */
static void add_sparkle(void);
static void add_cat(unsigned int x, unsigned int y);
static void cleanup(void);
static void clear_screen(void);
static void draw_cats(unsigned int frame);
static void draw_sparkles(void);
static void errout(char *str);
static void fillsquare(SDL_Surface* surf, int x, int y, int w, int h, Uint32 col);
static void handle_args(int argc, char** argv);
static void handle_input(void);
static void init(void);
static void load_images(void);
static void stretch_images(void);
static SDL_Surface* load_image(const char* path);
static void load_music(void);
static void putpix(SDL_Surface* surf, int x, int y, Uint32 col);
static void remove_sparkle(sparkle_instance* s);
static void run(void);
static void update_sparkles(void);
#ifdef XINERAMA
static void xinerama_add_cats(void);
#endif /* XINERAMA */

/* Globals */
static unsigned int         FRAMERATE = 14;
static unsigned int         SCREEN_BPP = 32;
static SDL_Surface*         screen = NULL;
static SDL_Event            event;
static int                  running = 1;
static int                  SURF_TYPE = SDL_HWSURFACE;
static int                  sound = 1;
#ifdef XINERAMA
static Display* dpy;
#endif /* XINERAMA */
static cat_instance*        cat_list = NULL;
static int                  curr_frame = 0;
static int                  sparkle_spawn_counter = 0;
static Mix_Music*           music;
static SDL_Surface*         cat_img[5];
static SDL_Surface*         sparkle_img[5];
static SDL_Surface*         stretch_cat[5];
static sparkle_instance*    sparkles_list = NULL;
static Uint32               bgcolor;

static void
add_sparkle(void) {
    sparkle_instance* s = sparkles_list;
    sparkle_instance* new;

    new = malloc(sizeof(sparkle_instance));
    if(!new)
        errout("Could not allocate new sparkle instance memory in add_sparkle.");

    new->loc.x = screen->w + 80;
    new->loc.y = (rand() % (screen->h + sparkle_img[0]->h)) - sparkle_img[0]->h;
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

static void
add_cat(unsigned int x, unsigned int y) {
    cat_instance* c = cat_list;
    cat_instance* new;

    new = malloc(sizeof(cat_instance));
    if(!new)
        errout("Could not allocate new cat instance memory in add_cat.");

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

static void
cleanup(void) {
    Mix_HaltMusic();
    Mix_FreeMusic(music);
    Mix_CloseAudio();
    SDL_Quit();
}


static void
clear_screen(void) {
    sparkle_instance *s = sparkles_list;
    cat_instance *c = cat_list;

    while (c) {
        /* This is bad. These magic numbers are to make up for uneven image sizes */
        fillsquare(screen, c->loc.x, c->loc.y - (curr_frame < 2 ? 0 : 5), stretch_cat[curr_frame]->w + 6, stretch_cat[curr_frame]->h + 5, bgcolor);
        c = c->next;
    }

    while (s) {
        fillsquare(screen, s->loc.x, s->loc.y, sparkle_img[s->frame]->w, sparkle_img[s->frame]->h, bgcolor);
        s = s->next;
    }

}


static void
draw_cats(unsigned int frame) {
    cat_instance* c = cat_list;
    SDL_Rect pos;

    while (c) {
        pos.x = c->loc.x;
        pos.y = c->loc.y;

        if(frame < 2)
            pos.y -= 5;
        SDL_BlitSurface( stretch_cat[frame], NULL, screen, &pos );
        c = c->next;
    }
}

static void
draw_sparkles() {
    sparkle_instance* s = sparkles_list;
    SDL_Rect pos;

    while (s) {
        pos.x = s->loc.x;
        pos.y = s->loc.y;
        SDL_BlitSurface( sparkle_img[s->frame], NULL, screen, &pos );
        s = s->next;
    }
}

static void
errout (char *str) {
    if (str)
        puts(str);
    exit(1);
}

static void
fillsquare(SDL_Surface* surf, int x, int y, int w, int h, Uint32 col) {
    int i, e;

    if (x + w < 0 || y + h < 0 || x > surf->w || y > surf->h)
        return;

    /* Sanitising of inputs. Make sure we're not drawing off of the surface */
    if (x + w > surf->w)
        w = surf->w - x;
    if (y + h > surf->h)
        h = surf->h - y;
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }

    for (i = x; i < x + w; i++)
        for (e = y; e < y + h; e++)
            putpix(surf, i, e, col);
}

static void
handle_args(int argc, char **argv) {
    int i;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-hw"))
            SURF_TYPE = SDL_HWSURFACE;
        else if (!strcmp(argv[i], "-sw"))
            SURF_TYPE = SDL_SWSURFACE;
        else if(!strcmp(argv[i], "--nosound"))
            sound = 0;
    }
}

static void
handle_input(void) {
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

static void
init(void) {
    int i;

    srand( time(NULL) );

    SDL_Init( SDL_INIT_EVERYTHING );
    screen = SDL_SetVideoMode( 0, 0, SCREEN_BPP, SURF_TYPE | SDL_FULLSCREEN );
    SDL_ShowCursor(0);

    load_images();
    stretch_images();
    bgcolor = SDL_MapRGB(screen->format, 0x00, 0x33, 0x66);
    fillsquare(screen, 0, 0, screen->w, screen->h, bgcolor);

    if(sound) {
        Mix_OpenAudio( 44100, AUDIO_S16, 2, 256 );
        load_music();
        Mix_PlayMusic(music, 0);
    }

#ifdef XINERAMA
    if (!(dpy = XOpenDisplay(NULL)))
        puts("Failed to open Xinerama display information.");
    else
        xinerama_add_cats();
#else
    add_cat(0, (screen->h - stretch_cat[0]->h) / 2);
#endif /* Xinerama */

    /* clear initial input */
    while( SDL_PollEvent( &event ) ) {}

    /* Pre-populate with sparkles */
    for (i = 0; i < 200; i++)
        update_sparkles();
}

static void
load_images(void) {
    /* This obviously needs work */
    /* Needs to be replaced with a loop */
    cat_img[0] = load_image("res/frame00.png");
    if(!cat_img[0]) {
        cat_img[0] = load_image("/usr/share/nyancat/frame00.png");
        cat_img[1] = load_image("/usr/share/nyancat/frame01.png");
        cat_img[2] = load_image("/usr/share/nyancat/frame02.png");
        cat_img[3] = load_image("/usr/share/nyancat/frame03.png");
        cat_img[4] = load_image("/usr/share/nyancat/frame04.png");

        sparkle_img[0] = load_image("/usr/share/nyancat/sparkle0.png");
        sparkle_img[1] = load_image("/usr/share/nyancat/sparkle1.png");
        sparkle_img[2] = load_image("/usr/share/nyancat/sparkle2.png");
        sparkle_img[3] = load_image("/usr/share/nyancat/sparkle3.png");
        sparkle_img[4] = load_image("/usr/share/nyancat/sparkle4.png");
    }
    else {
        cat_img[1] = load_image("res/frame01.png");
        cat_img[2] = load_image("res/frame02.png");
        cat_img[3] = load_image("res/frame03.png");
        cat_img[4] = load_image("res/frame04.png");

        sparkle_img[0] = load_image("res/sparkle0.png");
        sparkle_img[1] = load_image("res/sparkle1.png");
        sparkle_img[2] = load_image("res/sparkle2.png");
        sparkle_img[3] = load_image("res/sparkle3.png");
        sparkle_img[4] = load_image("res/sparkle4.png");
    }

}

static void
stretch_images(void) {
    SDL_Rect stretchto;
    stretchto.w=screen->w;

    SDL_PixelFormat fmt = *(screen->format);
    for(int i=0;i<=4;i++) {
        stretchto.h=stretchto.w*cat_img[i]->h/cat_img[i]->w;
        stretch_cat[i] = SDL_CreateRGBSurface(SDL_SWSURFACE,stretchto.w,stretchto.h,SCREEN_BPP,fmt.Rmask,fmt.Gmask,fmt.Bmask,fmt.Amask);
        SDL_SoftStretch(cat_img[i],NULL,stretch_cat[i],NULL);
    }
}

static SDL_Surface*
load_image( const char* path ) {
    SDL_Surface* loadedImage = NULL;
    SDL_Surface* optimizedImage = NULL;

    loadedImage = IMG_Load( path );
    if(loadedImage) {
        optimizedImage = SDL_DisplayFormatAlpha( loadedImage );
        SDL_FreeSurface( loadedImage );
    }
    return optimizedImage;
}

static void
load_music(void) {
    music = Mix_LoadMUS("res/nyan.ogg");
    if (!music)
        music = Mix_LoadMUS("/usr/share/nyancat/nyan.ogg");
    if (!music)
        printf("Unable to load Ogg file: %s\n", Mix_GetError());
}

static void
putpix(SDL_Surface* surf, int x, int y, Uint32 col) {
    Uint32 *pix = (Uint32 *) surf->pixels;
    pix [ ( y * surf->w ) + x ] = col;
}

static void
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

static void
run(void) {
    unsigned int last_draw, draw_time;

    while( running )
    {
        last_draw = SDL_GetTicks();

        clear_screen();
        update_sparkles();
        draw_sparkles();
        draw_cats(curr_frame);

        handle_input();
        SDL_Flip(screen);

        /* Frame increment and looping */
        curr_frame++;
        if (curr_frame > 4)
            curr_frame = 0;

        draw_time = SDL_GetTicks() - last_draw;
        if (draw_time < (1000 / FRAMERATE))
            SDL_Delay((1000 / FRAMERATE) - draw_time);
    }
}



static void
update_sparkles(void) {
    sparkle_instance* next, *s = sparkles_list;

    sparkle_spawn_counter += rand() % screen->h;
    while(sparkle_spawn_counter >= 1000) {
        add_sparkle();
        sparkle_spawn_counter -= 1000;
    }

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

#ifdef XINERAMA
static void
xinerama_add_cats(void) {
    int i, nn;
    XineramaScreenInfo* info = XineramaQueryScreens(dpy, &nn);

    for (i = 0; i < nn; ++i)
        add_cat(0, info[i].y_org + ((info[i].height - stretch_cat[0]->h) / 2));

    XFree(info);
    XCloseDisplay(dpy);
}
#endif /* XINERAMA */

int main( int argc, char **argv )
{
    handle_args(argc, argv);
    init();
    run();
    cleanup();
    return 0;
}
