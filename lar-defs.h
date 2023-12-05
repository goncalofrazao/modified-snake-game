
typedef enum direction_t {UP, DOWN, LEFT, RIGHT} direction_t;
typedef enum type_t {LIZARD_CONNECT, ROACH_CONNECT, LIZARD_MOVE, ROACH_MOVE, LIZARD_DISCONNECT} type_t;

typedef struct msg_t {
    type_t type;
    char id; //character to display
    int password;
    direction_t direction;
} msg_t;

typedef struct reply_t {
    char id;
    int password;
    int score;
} reply_t;

typedef struct display_t {
    char ch;
    int pos_x, pos_y;
} display_t;
