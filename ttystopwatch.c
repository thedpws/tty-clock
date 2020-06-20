/*
 *      TTY-CLOCK Main file.
 *      Copyright © 2009-2018 tty-stopwatch contributors
 *      Copyright © 2008 Martin Duquesnoy <xorg62@gmail.com>
 *      All rights reserved.
 *
 *      Redistribution and use in source and binary forms, with or without
 *      modification, are permitted provided that the following conditions are
 *      met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following disclaimer
 *        in the documentation and/or other materials provided with the
 *        distribution.
 *      * Neither the name of the  nor the names of its
 *        contributors may be used to endorse or promote products derived from
 *        this software without specific prior written permission.
 *
 *      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *      "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *      LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *      A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *      OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *      SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *      LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *      DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *      THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *      (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *      OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ttystopwatch.h"

void
init(void)
{
     struct sigaction sig;
     ttystopwatch.bg = COLOR_BLACK;

     /* Init ncurses */
     if (ttystopwatch.tty) {
          FILE *ftty = fopen(ttystopwatch.tty, "r+");
          if (!ftty) {
               fprintf(stderr, "tty-stopwatch: error: '%s' couldn't be opened: %s.\n",
                       ttystopwatch.tty, strerror(errno));
               exit(EXIT_FAILURE);
          }
          ttystopwatch.ttyscr = newterm(NULL, ftty, ftty);
          assert(ttystopwatch.ttyscr != NULL);
          set_term(ttystopwatch.ttyscr);
     } else
          initscr();

     cbreak();
     noecho();
     keypad(stdscr, true);
     start_color();
     curs_set(false);
     clear();

     /* Init default terminal color */
     if(use_default_colors() == OK)
          ttystopwatch.bg = -1;

     /* Init color pair */
     init_pair(0, ttystopwatch.bg, ttystopwatch.bg);
     init_pair(1, ttystopwatch.bg, ttystopwatch.option.color);
     init_pair(2, ttystopwatch.option.color, ttystopwatch.bg);
//     init_pair(0, ttystopwatch.bg, ttystopwatch.bg);
//     init_pair(1, ttystopwatch.bg, ttystopwatch.option.color);
//     init_pair(2, ttystopwatch.option.color, ttystopwatch.bg);
     refresh();

     /* Init signal handler */
     sig.sa_handler = signal_handler;
     sig.sa_flags   = 0;
     sigaction(SIGTERM,  &sig, NULL);
     sigaction(SIGINT,   &sig, NULL);
     sigaction(SIGSEGV,  &sig, NULL);

     /* Init global struct */
     ttystopwatch.running = true;
     if(!ttystopwatch.geo.x)
          ttystopwatch.geo.x = 0;
     if(!ttystopwatch.geo.y)
          ttystopwatch.geo.y = 0;
     if(!ttystopwatch.geo.a)
          ttystopwatch.geo.a = 1;
     if(!ttystopwatch.geo.b)
          ttystopwatch.geo.b = 1;
     ttystopwatch.geo.w = (ttystopwatch.option.second) ? SECFRAMEW : NORMFRAMEW;
     ttystopwatch.geo.h = 7;
     ttystopwatch.start_lt = time(NULL);
     update_hour();

     /* Create stopwatch win */
     ttystopwatch.framewin = newwin(ttystopwatch.geo.h,
                                ttystopwatch.geo.w,
                                ttystopwatch.geo.x,
                                ttystopwatch.geo.y);
     if(ttystopwatch.option.box) {
          box(ttystopwatch.framewin, 0, 0);
     }

     if (ttystopwatch.option.bold)
     {
          wattron(ttystopwatch.framewin, A_BLINK);
     }

     /* Create the date win */
     ttystopwatch.datewin = newwin(DATEWINH, strlen(ttystopwatch.date.datestr) + 2,
                               ttystopwatch.geo.x + ttystopwatch.geo.h - 1,
                               ttystopwatch.geo.y + (ttystopwatch.geo.w / 2) -
                               (strlen(ttystopwatch.date.datestr) / 2) - 1);
     if(ttystopwatch.option.box && ttystopwatch.option.date) {
          box(ttystopwatch.datewin, 0, 0);
     }
     clearok(ttystopwatch.datewin, true);

     set_center(ttystopwatch.option.center);

     nodelay(stdscr, true);

     if (ttystopwatch.option.date)
     {
          wrefresh(ttystopwatch.datewin);
     }

     wrefresh(ttystopwatch.framewin);

     return;
}

void
signal_handler(int signal)
{
     switch(signal)
     {
     case SIGINT:
     case SIGTERM:
          ttystopwatch.running = false;
          break;
          /* Segmentation fault signal */
     case SIGSEGV:
          endwin();
          fprintf(stderr, "Segmentation fault.\n");
          exit(EXIT_FAILURE);
          break;
     }

     return;
}

void
cleanup(void)
{
     if (ttystopwatch.ttyscr)
          delscreen(ttystopwatch.ttyscr);

     free(ttystopwatch.tty);
}

void
update_hour(void)
{
     int ihour;
     char tmpstr[128];

     time_t lt = time(NULL) - ttystopwatch.start_lt;

     ttystopwatch.tm = gmtime(&(lt));

     ihour = ttystopwatch.tm->tm_hour;

     ttystopwatch.meridiem = "\0";

     /* Set hour */
     ttystopwatch.date.hour[0] = ihour / 10;
     ttystopwatch.date.hour[1] = ihour % 10;

     /* Set minutes */
     ttystopwatch.date.minute[0] = ttystopwatch.tm->tm_min / 10;
     ttystopwatch.date.minute[1] = ttystopwatch.tm->tm_min % 10;

     /* Set date string */
     strftime(tmpstr,
              sizeof(tmpstr),
              ttystopwatch.option.format,
              ttystopwatch.tm);
     sprintf(ttystopwatch.date.datestr, "%s%s", tmpstr, ttystopwatch.meridiem);

     /* Set seconds */
     ttystopwatch.date.second[0] = ttystopwatch.tm->tm_sec / 10;
     ttystopwatch.date.second[1] = ttystopwatch.tm->tm_sec % 10;

     return;
}

void
draw_number(int n, int x, int y)
{
     int i, sy = y;

     for(i = 0; i < 30; ++i, ++sy)
     {
          if(sy == y + 6)
          {
               sy = y;
               ++x;
          }

          if (ttystopwatch.option.bold)
               wattron(ttystopwatch.framewin, A_BLINK);
          else
               wattroff(ttystopwatch.framewin, A_BLINK);

          wbkgdset(ttystopwatch.framewin, COLOR_PAIR(number[n][i/2]));
          mvwaddch(ttystopwatch.framewin, x, sy, ' ');
     }
     wrefresh(ttystopwatch.framewin);

     return;
}

void
draw_stopwatch(void)
{
     /* Draw hour numbers */
     draw_number(ttystopwatch.date.hour[0], 1, 1);
     draw_number(ttystopwatch.date.hour[1], 1, 8);
     chtype dotcolor = COLOR_PAIR(1);
     if (ttystopwatch.option.blink && time(NULL) % 2 == 0)
          dotcolor = COLOR_PAIR(2);

     /* 2 dot for number separation */
     wbkgdset(ttystopwatch.framewin, dotcolor);
     mvwaddstr(ttystopwatch.framewin, 2, 16, "  ");
     mvwaddstr(ttystopwatch.framewin, 4, 16, "  ");

     /* Draw minute numbers */
     draw_number(ttystopwatch.date.minute[0], 1, 20);
     draw_number(ttystopwatch.date.minute[1], 1, 27);

     /* Draw the date */
     if (ttystopwatch.option.bold)
          wattron(ttystopwatch.datewin, A_BOLD);
     else
          wattroff(ttystopwatch.datewin, A_BOLD);

     if (ttystopwatch.option.date)
     {
          wbkgdset(ttystopwatch.datewin, (COLOR_PAIR(2)));
          mvwprintw(ttystopwatch.datewin, (DATEWINH / 2), 1, ttystopwatch.date.datestr);
          wrefresh(ttystopwatch.datewin);
     }

     /* Draw second if the option is enabled */
     if(ttystopwatch.option.second)
     {
          /* Again 2 dot for number separation */
          wbkgdset(ttystopwatch.framewin, dotcolor);
          mvwaddstr(ttystopwatch.framewin, 2, NORMFRAMEW, "  ");
          mvwaddstr(ttystopwatch.framewin, 4, NORMFRAMEW, "  ");

          /* Draw second numbers */
          draw_number(ttystopwatch.date.second[0], 1, 39);
          draw_number(ttystopwatch.date.second[1], 1, 46);
     }

     return;
}

void
stopwatch_move(int x, int y, int w, int h)
{

     /* Erase border for a clean move */
     wbkgdset(ttystopwatch.framewin, COLOR_PAIR(0));
     wborder(ttystopwatch.framewin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
     werase(ttystopwatch.framewin);
     wrefresh(ttystopwatch.framewin);

     if (ttystopwatch.option.date)
     {
          wbkgdset(ttystopwatch.datewin, COLOR_PAIR(0));
          wborder(ttystopwatch.datewin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
          werase(ttystopwatch.datewin);
          wrefresh(ttystopwatch.datewin);
     }

     /* Frame win move */
     mvwin(ttystopwatch.framewin, (ttystopwatch.geo.x = x), (ttystopwatch.geo.y = y));
     wresize(ttystopwatch.framewin, (ttystopwatch.geo.h = h), (ttystopwatch.geo.w = w));

     /* Date win move */
     if (ttystopwatch.option.date)
     {
          mvwin(ttystopwatch.datewin,
                ttystopwatch.geo.x + ttystopwatch.geo.h - 1,
                ttystopwatch.geo.y + (ttystopwatch.geo.w / 2) - (strlen(ttystopwatch.date.datestr) / 2) - 1);
          wresize(ttystopwatch.datewin, DATEWINH, strlen(ttystopwatch.date.datestr) + 2);

          if (ttystopwatch.option.box) {
               box(ttystopwatch.datewin,  0, 0);
          }
     }

     if (ttystopwatch.option.box)
     {
          box(ttystopwatch.framewin, 0, 0);
     }

     wrefresh(ttystopwatch.framewin);
     wrefresh(ttystopwatch.datewin); 
     return;
}

/* Useless but fun :) */
void
stopwatch_rebound(void)
{
     if(!ttystopwatch.option.rebound)
          return;

     if(ttystopwatch.geo.x < 1)
          ttystopwatch.geo.a = 1;
     if(ttystopwatch.geo.x > (LINES - ttystopwatch.geo.h - DATEWINH))
          ttystopwatch.geo.a = -1;
     if(ttystopwatch.geo.y < 1)
          ttystopwatch.geo.b = 1;
     if(ttystopwatch.geo.y > (COLS - ttystopwatch.geo.w - 1))
          ttystopwatch.geo.b = -1;

     stopwatch_move(ttystopwatch.geo.x + ttystopwatch.geo.a,
                ttystopwatch.geo.y + ttystopwatch.geo.b,
                ttystopwatch.geo.w,
                ttystopwatch.geo.h);

     return;
}

void
set_second(void)
{
     int new_w = (((ttystopwatch.option.second = !ttystopwatch.option.second)) ? SECFRAMEW : NORMFRAMEW);
     int y_adj;

     for(y_adj = 0; (ttystopwatch.geo.y - y_adj) > (COLS - new_w - 1); ++y_adj);

     stopwatch_move(ttystopwatch.geo.x, (ttystopwatch.geo.y - y_adj), new_w, ttystopwatch.geo.h);

     set_center(ttystopwatch.option.center);

     return;
}

void
set_center(bool b)
{
     if((ttystopwatch.option.center = b))
     {
          ttystopwatch.option.rebound = false;

          stopwatch_move((LINES / 2 - (ttystopwatch.geo.h / 2)),
                     (COLS  / 2 - (ttystopwatch.geo.w / 2)),
                     ttystopwatch.geo.w,
                     ttystopwatch.geo.h);
     }

     return;
}

void
set_box(bool b)
{
     ttystopwatch.option.box = b;

     wbkgdset(ttystopwatch.framewin, COLOR_PAIR(0));
     wbkgdset(ttystopwatch.datewin, COLOR_PAIR(0));

     if(ttystopwatch.option.box) {
          wbkgdset(ttystopwatch.framewin, COLOR_PAIR(0));
          wbkgdset(ttystopwatch.datewin, COLOR_PAIR(0));
          box(ttystopwatch.framewin, 0, 0);
          box(ttystopwatch.datewin,  0, 0);
     }
     else {
          wborder(ttystopwatch.framewin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
          wborder(ttystopwatch.datewin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
     }

     wrefresh(ttystopwatch.datewin);
     wrefresh(ttystopwatch.framewin);
}

void
key_event(void)
{
     int i, c;

     struct timespec length = { ttystopwatch.option.delay, ttystopwatch.option.nsdelay };
     
     fd_set rfds;
     FD_ZERO(&rfds);
     FD_SET(STDIN_FILENO, &rfds);

     if (ttystopwatch.option.screensaver)
     {
          c = wgetch(stdscr);
          if(c != ERR && ttystopwatch.option.noquit == false)
          {
               ttystopwatch.running = false;
          }
          else
          {
               nanosleep(&length, NULL);
               for(i = 0; i < 8; ++i)
                    if(c == (i + '0'))
                    {
                         ttystopwatch.option.color = i;
                         init_pair(1, ttystopwatch.bg, i);
                         init_pair(2, i, ttystopwatch.bg);
                    }
          }
          return;
     }


     switch(c = wgetch(stdscr))
     {
     case KEY_RESIZE:
          endwin();
          init();
          break;

     case KEY_UP:
     case 'k':
     case 'K':
          if(ttystopwatch.geo.x >= 1
             && !ttystopwatch.option.center)
               stopwatch_move(ttystopwatch.geo.x - 1, ttystopwatch.geo.y, ttystopwatch.geo.w, ttystopwatch.geo.h);
          break;

     case KEY_DOWN:
     case 'j':
     case 'J':
          if(ttystopwatch.geo.x <= (LINES - ttystopwatch.geo.h - DATEWINH)
             && !ttystopwatch.option.center)
               stopwatch_move(ttystopwatch.geo.x + 1, ttystopwatch.geo.y, ttystopwatch.geo.w, ttystopwatch.geo.h);
          break;

     case KEY_LEFT:
     case 'h':
     case 'H':
          if(ttystopwatch.geo.y >= 1
             && !ttystopwatch.option.center)
               stopwatch_move(ttystopwatch.geo.x, ttystopwatch.geo.y - 1, ttystopwatch.geo.w, ttystopwatch.geo.h);
          break;

     case KEY_RIGHT:
     case 'l':
     case 'L':
          if(ttystopwatch.geo.y <= (COLS - ttystopwatch.geo.w - 1)
             && !ttystopwatch.option.center)
               stopwatch_move(ttystopwatch.geo.x, ttystopwatch.geo.y + 1, ttystopwatch.geo.w, ttystopwatch.geo.h);
          break;

     case 'q':
     case 'Q':
          if (ttystopwatch.option.noquit == false)
               ttystopwatch.running = false;
          break;

     case 's':
     case 'S':
          set_second();
          break;

     case 't':
     case 'T':
          ttystopwatch.option.twelve = !ttystopwatch.option.twelve;
          /* Set the new ttystopwatch.date.datestr to resize date window */
          update_hour();
          stopwatch_move(ttystopwatch.geo.x, ttystopwatch.geo.y, ttystopwatch.geo.w, ttystopwatch.geo.h);
          break;

     case 'c':
     case 'C':
          set_center(!ttystopwatch.option.center);
          break;

     case 'b':
     case 'B':
          ttystopwatch.option.bold = !ttystopwatch.option.bold;
          break;

     case 'r':
     case 'R':
          ttystopwatch.option.rebound = !ttystopwatch.option.rebound;
          if(ttystopwatch.option.rebound && ttystopwatch.option.center)
               ttystopwatch.option.center = false;
          break;

     case 'x':
     case 'X':
          set_box(!ttystopwatch.option.box);
          break;

     case '0': case '1': case '2': case '3':
     case '4': case '5': case '6': case '7':
          i = c - '0';
          ttystopwatch.option.color = i;
          init_pair(1, ttystopwatch.bg, i);
          init_pair(2, i, ttystopwatch.bg);
          break;

     default:
          pselect(1, &rfds, NULL, NULL, &length, NULL);
     }

     return;
}

int
main(int argc, char **argv)
{
     int c;

     /* Alloc ttystopwatch */
     memset(&ttystopwatch, 0, sizeof(ttystopwatch_t));

     ttystopwatch.option.date = true;

     /* Default date format */
     strncpy(ttystopwatch.option.format, "%F", sizeof (ttystopwatch.option.format));
     /* Default color */
     ttystopwatch.option.color = COLOR_GREEN; /* COLOR_GREEN = 2 */
     /* Default delay */
     ttystopwatch.option.delay = 1; /* 1FPS */
     ttystopwatch.option.nsdelay = 0; /* -0FPS */
     ttystopwatch.option.blink = false;

     atexit(cleanup);

     while ((c = getopt(argc, argv, "iuvsScbtrhBxnDC:f:d:T:a:")) != -1)
     {
          switch(c)
          {
          case 'h':
          default:
               printf("usage : tty-stopwatch [-iuvsScbtrahDBxn] [-C [0-7]] [-f format] [-d delay] [-a nsdelay] [-T tty] \n"
                      "    -s            Show seconds                                   \n"
                      "    -S            Screensaver mode                               \n"
                      "    -x            Show box                                       \n"
                      "    -c            Set the stopwatch at the center of the terminal    \n"
                      "    -C [0-7]      Set the stopwatch color                            \n"
                      "    -b            Use bold colors                                \n"
                      "    -t            Set the hour in 12h format                     \n"
                      "    -u            Use UTC time                                   \n"
                      "    -T tty        Display the stopwatch on the specified terminal    \n"
                      "    -r            Do rebound the stopwatch                           \n"
                      "    -f format     Set the date format                            \n"
                      "    -n            Don't quit on keypress                         \n"
                      "    -v            Show tty-stopwatch version                         \n"
                      "    -i            Show some info about tty-stopwatch                 \n"
                      "    -h            Show this page                                 \n"
                      "    -D            Hide date                                      \n"
                      "    -B            Enable blinking colon                          \n"
                      "    -d delay      Set the delay between two redraws of the stopwatch. Default 1s. \n"
                      "    -a nsdelay    Additional delay between two redraws in nanoseconds. Default 0ns.\n");
               exit(EXIT_SUCCESS);
               break;
          case 'i':
               puts("TTY-Clock 2 © by Martin Duquesnoy (xorg62@gmail.com), Grey (grey@greytheory.net)");
               exit(EXIT_SUCCESS);
               break;
          case 'u':
               ttystopwatch.option.utc = true;
               break;
          case 'v':
               puts("TTY-Clock 2 © devel version");
               exit(EXIT_SUCCESS);
               break;
          case 's':
               ttystopwatch.option.second = true;
               break;
          case 'S':
               ttystopwatch.option.screensaver = true;
               break;
          case 'c':
               ttystopwatch.option.center = true;
               break;
          case 'b':
               ttystopwatch.option.bold = true;
               break;
          case 'C':
               if(atoi(optarg) >= 0 && atoi(optarg) < 8)
                    ttystopwatch.option.color = atoi(optarg);
               break;
          case 't':
               ttystopwatch.option.twelve = true;
               break;
          case 'r':
               ttystopwatch.option.rebound = true;
               break;
          case 'f':
               strncpy(ttystopwatch.option.format, optarg, 100);
               break;
          case 'd':
               if(atol(optarg) >= 0 && atol(optarg) < 100)
                    ttystopwatch.option.delay = atol(optarg);
               break;
          case 'D':
               ttystopwatch.option.date = false;
               break;
          case 'B':
               ttystopwatch.option.blink = true;
               break;
          case 'a':
               if(atol(optarg) >= 0 && atol(optarg) < 1000000000)
                    ttystopwatch.option.nsdelay = atol(optarg);
               break;
          case 'x':
               ttystopwatch.option.box = true;
               break;
          case 'T': {
               struct stat sbuf;
               if (stat(optarg, &sbuf) == -1) {
                    fprintf(stderr, "tty-stopwatch: error: couldn't stat '%s': %s.\n",
                              optarg, strerror(errno));
                    exit(EXIT_FAILURE);
               } else if (!S_ISCHR(sbuf.st_mode)) {
                    fprintf(stderr, "tty-stopwatch: error: '%s' doesn't appear to be a character device.\n",
                              optarg);
                    exit(EXIT_FAILURE);
               } else {
                    free(ttystopwatch.tty);
                    ttystopwatch.tty = strdup(optarg);
               }}
               break;
          case 'n':
               ttystopwatch.option.noquit = true;
               break;
          }
     }

     init();
     attron(A_BLINK);
     while(ttystopwatch.running)
     {
          stopwatch_rebound();
          update_hour();
          draw_stopwatch();
          key_event();
     }

     endwin();

     return 0;
}

// vim: expandtab tabstop=5 softtabstop=5 shiftwidth=5
