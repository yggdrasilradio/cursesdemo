#include <linux/input.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>
#include <ncurses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>
#define KEY_ESC 27

int tick = 0, fd;

enum COLOR {
	GREEN = 1,
	YELLOW,
	RED,
	WHITE,
	MAGENTA,
	CYAN,
	BLUE
};

struct particle {
	int x;
	int y;
	int dx;
	int dy;
	int color;
};

struct particle particles[100] = {
	{ 30, 13, 1, -1, RED },
	{ 33, 7, 1, 1, GREEN },
	{ 42, 3, -1, 1, CYAN },
	{ 40, 17, -1, -1, YELLOW },
	{ 41, 16, 1, 0, MAGENTA },
	{ 36, 10, 0, 1, WHITE },
	{ 46, 15, 1, -1, BLUE },
};

struct particle *Find_Particle(int x, int y) {

	int i, n;
	struct particle *p;

	n =  sizeof(particles) / sizeof(particles[0]);
	for (i = 0; i < n; i++) {
		p = &particles[i];
		if (p->x == x && p->y == y)
			return p;
	}
	return NULL;
}

int Random(int lower, int upper) {

	return (rand() % (upper - lower + 1)) + lower;
}

void Add_Particle(int x, int y) {

	int i, n;
	struct particle *p;

	n =  sizeof(particles) / sizeof(particles[0]);
	for (i = 0; i < n; i++) {

		p = &particles[i];

		// allocate new particle
		if (p->x == 0 && p->y == 0) {

			// position particle
			p->x = x;
			p->y = y;

			// generate random motion
			while (p->dx == 0 & p->dy == 0) {
				p->dx = Random(1, 3) - 2;
				p->dy = Random(1, 3) - 2;
			}

			// generate random color
			p->color = Random(1, 7);
			return;
		}

	}
}

void Animate_Particles() {

	int n, i, dx, dy, x, y, color, flags, newx, newy;
	struct particle *p, *p2;
	chtype ch;

	n =  sizeof(particles) / sizeof(particles[0]);
	for (i = 0; i < n; i++) {

		p = &particles[i];
		x = p->x;
		y = p->y;

		// ignore unallocated particles
		if (x == 0 && y == 0)
			continue;

		dx = p->dx;
		dy = p->dy;
		mvaddch(y, x, ' ');
		flags = 0;
		if (mvinch(y + dy, x) != ' ') {
			p2 = Find_Particle(x, y + dy);
			if (p2 != NULL)
				p2->dy = dy;
			flags |= 4;
		}
		if (mvinch(y + dy, x + dx) != ' ') {
			p2 = Find_Particle(x + dx, y + dy);
			if (p2 != NULL) {
				p2->dx = dx;
				p2->dy = dy;
			}
			flags |= 2;
		}
		if (mvinch(y, x + dx) != ' ') {
			p2 = Find_Particle(x + dx, y);
			if (p2 != NULL)
				p2->dx = dx;
			flags |= 1;
		}
		switch(flags) {
			case 1:
			case 3:
				dx = -dx;
				break;

			case 2:
			case 7:
				dx = -dx;
				dy = -dy;
				break;

			case 4:
			case 6:
				dy = -dy;
				break;
		}
		newx = x + dx;
		newy = y + dy;
		ch = mvinch(newy, newx);
		if (ch != ' ') {
			dx = -p->dx;
			dy = -p->dy;
			ch = mvinch(y + dy, x + dx);
			if (ch != ' ') {
				dx = 0;
				dy = 0;
			}

		}
		x = x + dx;
		y = y + dy;
		color = p->color;
		attron(COLOR_PAIR(color));
		mvaddch(y, x, ACS_BOARD);
		attroff(COLOR_PAIR(color));
		p->x = x;
		p->y = y;
		p->dx = dx;
		p->dy = dy;
	}
}

long getTouch(int *x, int *y) {

	int i, rb, n, rv;
	static int xval = -1, yval = -1;
	struct input_event *p, ev[64];
	fd_set set;
	struct timeval timeout;

	FD_ZERO(&set);
	FD_SET(fd, &set);
	timeout.tv_sec = 0;
	timeout.tv_usec = 10000;
	rv = select(fd + 1, &set, NULL, NULL, &timeout);
	if (rv <= 0)
		return FALSE;
	rb = read(fd, ev, sizeof(struct input_event) * 64);
	n = rb / sizeof(struct input_event);
	for (i = 0; i < n; i++) {
		p = &ev[i];
		if (p->code == BTN_TOUCH && p->value == 0) {
			if (xval >= 0 && yval >= 0) {
				*x = xval;
				*y = yval;
				xval = -1;
				yval = -1;
				return TRUE;
			}
		}
		if (p->value > 0 && p->type == EV_ABS) {
			switch(p->code) {
				case ABS_MT_POSITION_X:
				case ABS_X:
					xval = p->value;
					break;
				case ABS_MT_POSITION_Y:
				case ABS_Y:
					yval = p->value;
					break;
			}
		}
	}
	return FALSE;
}

void timer_handler(int signum) {

	tick++;
}

void init_timer() {

	struct itimerval timer;

	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 100000;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 100000;
	setitimer(ITIMER_REAL, &timer, 0);
	signal(SIGALRM, timer_handler);
}

void init_screen() {

	// init screen
	initscr();
	clear();
	noecho();
	raw();
	curs_set(0);
	keypad(stdscr, TRUE);

	// init colorsets
	start_color();
	init_color(COLOR_GREEN, 0, 1000, 0);
	init_color(COLOR_YELLOW, 1000, 1000, 0);
	init_color(COLOR_RED, 1000, 0, 0);
	init_color(COLOR_WHITE, 1000, 1000, 1000);
	init_color(COLOR_MAGENTA, 1000, 0, 1000);
	init_color(COLOR_CYAN, 0, 1000, 1000);
	init_color(COLOR_BLUE, 0, 0, 1000);

	init_pair(1, COLOR_GREEN, COLOR_BLACK);
	init_pair(2, COLOR_YELLOW, COLOR_BLACK);
	init_pair(3, COLOR_RED, COLOR_BLACK);
	init_pair(4, COLOR_WHITE, COLOR_BLACK);
	init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(6, COLOR_CYAN, COLOR_BLACK);
	init_pair(7, COLOR_BLUE, COLOR_BLACK);
}

void color_swatch(int x, int y, int pair, char *text) {

	attron(COLOR_PAIR(pair));
	attron(A_BOLD);
	mvaddch(y, x, ACS_BOARD);
	addch(ACS_CKBOARD);
	addch(ACS_BLOCK);
	addstr(" ");
	addstr(text);
	attroff(COLOR_PAIR(pair));
	attroff(A_BOLD);
}

void draw_grid(int xval, int yval, char *grid[], int pair) {

	int i, j, x, y, done;
	char *p;

	attron(COLOR_PAIR(pair));

	move(yval, xval);

	y = yval;
	done = FALSE;
	for (i = 0; !done; i++) {
		x = xval;
		for (p = grid[i]; *p != '\0'; p++) {
			switch(*p) {
				case '1':
					mvaddch(y, x, ACS_ULCORNER);
					break;
				case '2':
					mvaddch(y, x, ACS_TTEE);
					break;
				case '3':
					mvaddch(y, x, ACS_URCORNER);
					break;
				case '4':
					mvaddch(y, x, ACS_LTEE);
					break;
				case '5':
					mvaddch(y, x, ACS_PLUS);
					break;
				case '6':
					mvaddch(y, x, ACS_RTEE);
					break;
				case '7':
					mvaddch(y, x, ACS_LLCORNER);
					break;
				case '8':
					mvaddch(y, x, ACS_BTEE);
					break;
				case '9':
					mvaddch(y, x, ACS_LRCORNER);
					done = TRUE;
					break;
				case '-':
					mvaddch(y, x, ACS_HLINE);
					break;
				case '|':
					mvaddch(y, x, ACS_VLINE);
					break;

				default:
					if (*p != ' ')
						mvaddch(y, x, *p);
					break;
			}
			x++;
		}
		y++;
	}
	attroff(COLOR_PAIR(pair));
}

void draw_box(int x, int y, int w, int h) {

	// white on black
	attron(COLOR_PAIR(WHITE));

	// corners
	mvaddch(y, x, ACS_ULCORNER);
	mvaddch(y, x + w - 1, ACS_URCORNER);
	mvaddch(y + h - 1, x, ACS_LLCORNER);
	mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);

	// horiz lines
	mvhline(y, x + 1, ACS_HLINE, w - 2);
	mvhline(y + h - 1, x + 1, ACS_HLINE, w - 2);

	// vert lines
	mvvline(y + 1, x, ACS_VLINE, h - 2);
	mvvline(y + 1, x + w - 1, ACS_VLINE, h - 2);

	attroff(COLOR_PAIR(WHITE));
}

int main() {

	int c, x, y, n, i, newx, newy, done, touchx, touchy, line, col;
	char *grid[] = {
	"1---2---2---3",
	"| X |   | O |",
	"4---5---5---6",
	"|   | X |   |",
	"4---5---5---6",
	"|   |   | O |",
	"7---8---8---9",
	};

	// open touchscreen
	fd = open("/dev/input/event4", O_RDONLY);

	// initialize screen and colorsets
	init_screen();

	// initialize timer
	init_timer();

	// draw grid
	mvprintw(6, 41, "Grid test");
	draw_grid(40, 7, grid, WHITE);

	// color swatches
	mvprintw(2, 9, "Color test");
	draw_box(8, 3, 15,  15);
	color_swatch(10,  4, GREEN, "Green");
	color_swatch(10,  6, YELLOW, "Yellow");
	color_swatch(10,  8, RED, "Red");
	color_swatch(10, 10, WHITE, "White");
	color_swatch(10, 12, MAGENTA, "Magenta");
	color_swatch(10, 14, CYAN, "Cyan");
	color_swatch(10, 16, BLUE, "Blue");

	// draw box around entire display
	draw_box(0, 0, COLS, LINES);

	// draw initial cursor
	x = COLS - 2;
	y = 1;
	attron(COLOR_PAIR(GREEN));
	mvaddch(y, x, ACS_BOARD);
	attroff(COLOR_PAIR(GREEN));

	// animation loop
	done = FALSE;
	while (!done) {

		// read keyboard
		timeout(100);
		c = getch();
		timeout(0);
		newx = x;
		newy = y;
		switch (c) {
			case KEY_ESC:
				done = TRUE;
				break;
			case KEY_UP:
				newy--;
				break;
			case KEY_DOWN:
				newy++;
				break;
			case KEY_LEFT:
				newx--;
				break;
			case KEY_RIGHT:
				newx++;
				break;
		}

		// check for cursor collision
		if (mvinch(newy, newx) == ' ') {

			// update cursor
			mvaddch(y, x, ' ');
			attron(COLOR_PAIR(GREEN));
			mvaddch(newy, newx, ACS_BOARD);
			attroff(COLOR_PAIR(GREEN));
			x = newx;
			y = newy;

		} else {

			// read touchscreen
			if (getTouch(&touchx, &touchy)) {

				// coordinates of touch
				line = touchy / 23.95;
				col = touchx /  11.2535211;

				// if there's nothing there
				if (mvinch(line, col) == ' ')

					// add particle
					Add_Particle(col, line);

			}

			// update particles
			if (tick)
				Animate_Particles();

		}
	}

	// close screen
	clear();
	refresh();
	keypad(stdscr, FALSE);
	echo();
	curs_set(1);
	endwin();

	// close touchscreen
	close(fd);

	return 0;
}
