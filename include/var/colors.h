#ifndef COLORS_H
#define COLORS_H

// Reset
#define RESET           "\033[0m"

// Standard colors
#define WHITE           "\033[37m"

// Bright colors
#define BR_RED          "\033[91m"
#define BR_GREEN        "\033[92m"
#define BR_YELLOW       "\033[93m"
#define BR_BLUE         "\033[94m"
#define BR_CYAN			"\033[106m"

// RGB Foreground colors
#define FG_WHITE        "\033[38;2;255;255;255m"
#define FG_LIGHT_GRAY   "\033[38;2;220;220;220m"
#define FG_PATH_BLUE    "\033[38;2;120;180;255m"
#define FG_PROMPT_GREEN "\033[38;2;120;255;180m"

// RGB Background colors
#define BG_SHELL        "\033[48;2;30;30;30m"
#define BG_BADGE        "\033[48;2;60;60;60m"

#endif