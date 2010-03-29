#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <nacl/nacl_av.h>
#include <nacl/nacl_srpc.h>

#include "test_tnl2.h"

// global properties used to setup demo
static const int kMaxWindow = 4096;
static const int kMaxFrames = 10000000;
static int g_window_width = 512;
static int g_window_height = 512;
static int g_num_frames = 9999;

// Simple Surface structure to hold a raster rectangle
struct Surface {
  int width, height, pitch;
  uint32_t *pixels;
  Surface(int w, int h) { width = w;
                          height = h;
                          pitch = w;
                          pixels = new uint32_t[width * height]; }
  ~Surface() { delete[] pixels; }
};


// Drawing class holds information and functionality needed to render
class DrawingDemo {
 public:
  void Display();
  void Update();
  bool PollEvents();
  explicit DrawingDemo(Surface *s);
  ~DrawingDemo();

 private:
  Surface *surf_;
};


// This update loop is run once per frame.
// AGG renders straight into the DrawingDemo's surf_ pixel array.
void DrawingDemo::Update() {
}


// Displays software rendered image on the screen
void DrawingDemo::Display() {
  int r;
  r = nacl_video_update(surf_->pixels);
  if (-1 == r) {
    printf("nacl_video_update() returned %d\n", errno);
  }
}


// Polls events and services them.
bool DrawingDemo::PollEvents() {
  NaClMultimediaEvent event;
  while (0 == nacl_video_poll_event(&event)) {
    if (event.type == NACL_EVENT_QUIT) {
      return false;
    }
  }
  return true;
}


// Sets up and initializes DrawingDemo.
DrawingDemo::DrawingDemo(Surface *surf) {
  surf_ = surf;
}


// Frees up resources.
DrawingDemo::~DrawingDemo() {
}


// Runs the demo and animate the image for kNumFrames
void RunDemo(Surface *surface) {
  DrawingDemo demo(surface);

  for (int i = 0; i < g_num_frames; ++i) {
    demo.Update();
    demo.Display();
    printf("Frame: %04d\b\b\b\b\b\b\b\b\b\b\b", i);
    fflush(stdout);
    if (!demo.PollEvents())
      break;
  }
}


// Initializes a window buffer.
Surface* Initialize() {
  int r;
  int width;
  int height;
  r = nacl_multimedia_init(NACL_SUBSYSTEM_VIDEO | NACL_SUBSYSTEM_EMBED);
  if (-1 == r) {
    printf("Multimedia system failed to initialize!  errno: %d\n", errno);
    exit(-1);
  }
  // if this call succeeds, use width & height from embedded html
  r = nacl_multimedia_get_embed_size(&width, &height);
  if (0 == r) {
    g_window_width = width;
    g_window_height = height;
  }
  r = nacl_video_init(g_window_width, g_window_height);
  if (-1 == r) {
    printf("Video subsystem failed to initialize!  errno; %d\n", errno);
    exit(-1);
  }
  Surface *surface = new Surface(g_window_width, g_window_height);
  return surface;
}


// Frees window buffer.
void Shutdown(Surface *surface) {
  delete surface;
  nacl_video_shutdown();
  nacl_multimedia_shutdown();
}


// If user specified options on cmd line, parse them
// here and update global settings as needed.
void ParseCmdLineArgs(int argc, char **argv) {
  // look for cmd line args
  if (argc > 1) {
    for (int i = 1; i < argc; i++) {
      if (argv[i][0] == '-' && argv[i][1] == 'w') {
        int w = atoi(&argv[i][2]);
        if ((w > 0) && (w < kMaxWindow)) {
          g_window_width = w;
        }
      } else if (argv[i][0] == '-' && argv[i][1] == 'h') {
        int h = atoi(&argv[i][2]);
        if ((h > 0) && (h < kMaxWindow)) {
          g_window_height = h;
        }
      } else if (argv[i][0] == '-' && argv[i][1] == 'f') {
        int f = atoi(&argv[i][2]);
        if ((f > 0) && (f < kMaxFrames)) {
          g_num_frames = f;
        }
      } else {
        printf("nacl_test_tnl2\n");
        printf("usage: -w<n>   width of window.\n");
        printf("       -h<n>   height of window.\n");
        printf("       -f<n>   number of frames.\n");
        printf("       --help  show this screen.\n");
        exit(0);
      }
    }
  }
}


// Parses cmd line options, initializes surface, runs the demo & shuts down.
int main(int argc, char **argv) {
  ParseCmdLineArgs(argc, argv);
  Surface *surface = Initialize();
  RunDemo(surface);
  Shutdown(surface);
  return 0;
}
