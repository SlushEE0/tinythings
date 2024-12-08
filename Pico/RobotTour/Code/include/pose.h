typedef struct t_point {
  int x;
  int y;
  double heading;
} t_point;

static const char *TAG = "Home";

// IF X is ODD, blocks are vertical; IF X is EVEN, blocks are horizontal
// IF Y is EVEN, blocks are vertical; IF Y is ODD, blocks are horizontal
t_point BLOCKS[] = {{.x = 3, .y = 6}};
t_point POINTS[] = {{.x = 1, .y = 6, .heading = 90},
                    {.x = 2, .y = 6, .heading = 180},
                    {.x = 2, .y = 4, .heading = 90},
                    {.x = 4, .y = 4, .heading = 0}};