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

#define ANIM_FRAMES 5

/* Type definitions */
typedef struct {
    int x, y;
} coords;

typedef struct cat_instance cat_instance;
struct cat_instance {
    coords loc;
    cat_instance *next;
};

typedef struct sparkle_instance sparkle_instance;
struct sparkle_instance {
    unsigned int frame, speed;
    int frame_mov;
    unsigned int layer;
    coords loc;
    sparkle_instance *next;
};

/* Predecs */
static void add_sparkle(void);
static void add_cat(unsigned int x, unsigned int y);
static void cleanup(void);
static void clear_screen(void);
static void draw_cats(unsigned int frame);
static void draw_sparkles(void);
static void* ec_malloc(unsigned int size);
static void errout(char *str);
static void fillsquare(SDL_Surface* surf, int x, int y, int w, int h, Uint32 col);
static void handle_args(int argc, char** argv);
static void handle_input(void);
static void init(void);
static void load_images(void);
static SDL_Surface* load_image(const char* path);
static void load_music(void);
static void putpix(SDL_Surface* surf, int x, int y, Uint32 col);
static void remove_sparkle(sparkle_instance* s);
static void run(void);
static void stretch_images(void);
static void update_sparkles(void);
static void usage(char* exname);
#ifdef XINERAMA
static void xinerama_add_cats(void);
#endif /* XINERAMA */

/* Globals */
static unsigned int                 FRAMERATE = 14;
static unsigned int                 SCREEN_BPP = 32;
static unsigned int                 SCREEN_WIDTH = 800;
static unsigned int                 SCREEN_HEIGHT = 600;
static SDL_Surface*                 screen = NULL;
static SDL_Event                    event;
static int                          running = 1;
static int                          SURF_TYPE = SDL_HWSURFACE;
static int                          sound = 1;
static int                          sound_volume = 128;
static int                          fullscreen = 1;
static int                          catsize = 0;
static int                          cursor = 0;
#ifdef XINERAMA
static Display*                     dpy;
#endif /* XINERAMA */
static cat_instance*                cat_list = NULL;
static int                          curr_frame = 0;
static int                          sparkle_spawn_counter = 0;
static Mix_Music*                   music;
static SDL_Surface*                 cat_img[ANIM_FRAMES];
static SDL_Surface*                 sparkle_img[ANIM_FRAMES];
static SDL_Surface*                 stretch_cat[ANIM_FRAMES];
static SDL_Surface**                image_set = sparkle_img;
static sparkle_instance*            sparkles_list = NULL;
static Uint32                       bgcolor;

/* Function definitions */
static void
add_sparkle(void) {
    sparkle_instance* s = sparkles_list;
    sparkle_instance* new;

    new = ec_malloc(sizeof(sparkle_instance));

    new->loc.x = screen->w + 80;
    new->loc.y = (rand() % (screen->h + sparkle_img[0]->h)) - sparkle_img[0]->h;
    new->frame = 0;
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

    new = ec_malloc(sizeof(cat_instance));

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
        fillsquare(screen, c->loc.x, c->loc.y - (curr_frame < 2 ? 0 : 5),
          image_set[curr_frame]->w + 6, image_set[curr_frame]->h + 5, bgcolor);
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
        SDL_BlitSurface( image_set[frame], NULL, screen, &pos );
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

static void*
ec_malloc(unsigned int size) {
    void *ptr;
    ptr = malloc(size);
    if (!ptr)
        errout("In ec_malloc -- unable to allocate memory.");
    return ptr;
}

static void
errout (char *str) {
    if (str)
        puts(str);
    exit(-1);
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
        else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--fullscreen"))
            fullscreen = 1;
        else if(!(strcmp(argv[i], "-nf") || !strcmp(argv[i], "--nofullscreen")))
            fullscreen = 0;
        else if(!strcmp(argv[i], "-nc") || !strcmp(argv[i], "--nocursor"))
            cursor = 0;
        else if(!strcmp(argv[i], "-sc") || !strcmp(argv[i], "--cursor") || !strcmp(argv[i], "--showcursor"))
            cursor = 1;
        else if(!strcmp(argv[i], "-ns") || !strcmp(argv[i], "--nosound"))
            sound = 0;
        else if((!strcmp(argv[i], "-v") || !strcmp(argv[i], "--volume")) && i < argc - 1) {
            int vol = atoi(argv[++i]);
            if(vol >= 0 && vol <= 128){
                sound_volume = vol;
            }
            else {
                puts("Arguments for Volume are not valid. Disabling sound.");
                sound = 0;
            }
        }
        else if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) 
            usage(argv[0]);
        else if(!strcmp(argv[i], "-c") || !strcmp(argv[i], "--catsize")) {
            if (++i < argc) {
                if(!strcmp(argv[i], "full"))
                    catsize = 1;
                else if(!strcmp(argv[i], "small"))
                    catsize = 0;
                else
                    printf("Unrecognised scaling option: %s - please select either 'full' or 'small' cat size.\n", argv[i]);
            }
        }
        else if((!strcmp(argv[i], "-r") && strcmp(argv[i], "--resolution")) && i < argc - 2) {
            int dims[2] = { atoi(argv[++i]), atoi(argv[++i]) };
            if (dims[0] >= 0 && dims[0] < 10000 && dims[1] >= 0 && dims[1] < 5000) {           // Borrowed from PixelUnsticker, changed the variable name
                SCREEN_WIDTH = dims[0];
                SCREEN_HEIGHT = dims[1];
            }
            else
                puts("Arguments do not appear to be valid screen sizes. Defaulting.");
        }
        else
            printf("Unrecognised option: %s\n", argv[i]);
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
    if (fullscreen)
        screen = SDL_SetVideoMode( 0, 0, SCREEN_BPP, SURF_TYPE | SDL_FULLSCREEN );
    else
        screen = SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SURF_TYPE );
    if(!cursor)
        SDL_ShowCursor(0);

    load_images();
    bgcolor = SDL_MapRGB(screen->format, 0x00, 0x33, 0x66);
    fillsquare(screen, 0, 0, screen->w, screen->h, bgcolor);

    if(sound) {
        Mix_OpenAudio( 44100, AUDIO_S16, 2, 256 );
        load_music();
        Mix_PlayMusic(music, 0);
        Mix_VolumeMusic(sound_volume);
    }

    /* Choose our image set */
    if (catsize == 1)
        image_set = stretch_cat;
    else
        image_set = cat_img;

/* Ugly */
#ifdef XINERAMA
    if (!(dpy = XOpenDisplay(NULL)))
        puts("Failed to open Xinerama display information.");
    else{
        if(catsize == 1)
            stretch_images();
        xinerama_add_cats();
        XCloseDisplay(dpy);
    }
#else
    if(catsize == 1)
        add_cat(0, (screen->h - image_set[0]->h) / 2);
    else {
        add_cat((screen->w - cat_img[0]->w) / 2, (screen->h - cat_img[0]->h) / 2);
    }
#endif /* Xinerama */

    /* clear initial input */
    while( SDL_PollEvent( &event ) ) {}

    /* Pre-populate with sparkles */
    for (i = 0; i < 200; i++)
        update_sparkles();
}

static void
load_images(void) {
    int i;

    /* Local cat */
    static char *catimgpaths[] = {
            "res/basic/fg00.png",
            "res/basic/fg01.png",
            "res/basic/fg02.png",
            "res/basic/fg03.png",
            "res/basic/fg04.png"};
    /* Installed cat */
    static char *altcatimgpaths[] = {
            "/usr/share/nyancat/basic/fg00.png",
            "/usr/share/nyancat/basic/fg01.png",
            "/usr/share/nyancat/basic/fg02.png",
            "/usr/share/nyancat/basic/fg03.png",
            "/usr/share/nyancat/basic/fg04.png"};
    /* Local sparkles */
    static char *sparklepaths[] = {
            "res/basic/bg0.png",
            "res/basic/bg1.png",
            "res/basic/bg2.png",
            "res/basic/bg3.png",
            "res/basic/bg4.png"};
    /* Installed sparkles */
    static char *altsparklepaths[] = {
            "/usr/share/nyancat/basic/bg0.png",
            "/usr/share/nyancat/basic/bg1.png",
            "/usr/share/nyancat/basic/bg2.png",
            "/usr/share/nyancat/basic/bg3.png",
            "/usr/share/nyancat/basic/bg4.png"};

    /* Loading logic */
    for (i = 0; i < ANIM_FRAMES; ++i) {
        /* Cat images */
        cat_img[i] = load_image(catimgpaths[i]);
        if (!cat_img[i])
            cat_img[i] = load_image(altcatimgpaths[i]);
        if (!cat_img[i])
            errout("Error loading foreground images!");

        /* Sparkle images  */
        sparkle_img[i] = load_image(sparklepaths[i]);
        if (!sparkle_img[i])
            sparkle_img[i] = load_image(altsparklepaths[i]);
        if (!sparkle_img[i])
            errout("Error loading background images!");
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
    music = Mix_LoadMUS("res/basic/music.ogg");
    if (!music)
        music = Mix_LoadMUS("/usr/share/nyancat/basic/music.ogg");
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

    while( running ) {
        last_draw = SDL_GetTicks();

        clear_screen();
        update_sparkles();
        draw_sparkles();
        draw_cats(curr_frame);

        handle_input();
        SDL_Flip(screen);

        /* Frame increment and looping */
        curr_frame++;
        if (curr_frame >= ANIM_FRAMES)
            curr_frame = 0;

        draw_time = SDL_GetTicks() - last_draw;
        if (draw_time < (1000 / FRAMERATE))
            SDL_Delay((1000 / FRAMERATE) - draw_time);
    }
}

static void
stretch_images(void) {
    SDL_Rect stretchto;
    stretchto.w = 0;
    stretchto.h = 0;

    /*  Just use the x co-ordinate for scaling for now. This does, however,
        need to be changed to accomodate taller resolutions */
#ifdef XINERAMA
    int i, nn;
    XineramaScreenInfo* info = XineramaQueryScreens(dpy, &nn);

    for (i = 0; i < nn; ++i) {
        if(!stretchto.w || info[i].width < stretchto.w)
            stretchto.w = info[i].width;
    }

    XFree(info);
#endif /* XINERAMA */
    if (!stretchto.w)
        stretchto.w = screen->w;

    /* Handle a slight scaling down */
    stretchto.w *= 0.9;
    stretchto.h = stretchto.w * cat_img[0]->h / cat_img[0]->w;

    SDL_PixelFormat fmt = *(cat_img[0]->format);
    for(int i=0; i <= ANIM_FRAMES; i++) {
        stretch_cat[i] = SDL_CreateRGBSurface(SURF_TYPE, stretchto.w,
            stretchto.h,SCREEN_BPP,fmt.Rmask,fmt.Gmask,fmt.Bmask,fmt.Amask);
        SDL_SoftStretch(cat_img[i],NULL,stretch_cat[i],NULL);
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

        if(s->frame >= ANIM_FRAMES || s->frame < 1)
            s->frame_mov = 0 - s->frame_mov;

        if (s->loc.x < 0 - sparkle_img[0]->w)
            remove_sparkle(s);

        s = next;
    }
}

static void
usage(char* exname) {
    printf("Usage: %s [OPTIONS]\n\
    -h,  --help                    This help message\n\
    -f,  --fullscreen              Enable fullscreen mode (default)\n\
    -nf, --nofullscreen            Disable fullscreen mode\n\
    -c,  --catsize                 Choose size of cat, options are full and \n\
                                   small. Small is default. \"Full\" not\n\
                                   officially supported.\n\
    -nc, --nocursor                Don't show the cursor (default)\n\
    -sc, --cursor, --showcursor    Show the cursor\n\
    -ns, --nosound                 Don't play sound\n\
    -v, --volume                   Set Volume, if enabled, from 0 - 128\n\
    -r,  --resolution              Make next two arguments the screen \n\
                                   resolution to use (0 and 0 for full \n\
                                   resolution) (800x600 default)\n\
    -hw, -sw                       Use hardware or software SDL rendering, \n\
                                   respectively. Hardware is default\n", exname);
    exit(0);
}

#ifdef XINERAMA
static void
xinerama_add_cats(void) {
    int i, nn;
    XineramaScreenInfo* info = XineramaQueryScreens(dpy, &nn);

    for (i = 0; i < nn; ++i)
        if(fullscreen)
            add_cat(info[i].x_org + ((info[i].width - image_set[0]->w) / 2),
                info[i].y_org + ((info[i].height - image_set[0]->h) / 2));
        else
            add_cat((SCREEN_WIDTH - image_set[0]->w) / 2, 
                (SCREEN_HEIGHT - image_set[0]->h) / 2);

    XFree(info);
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
