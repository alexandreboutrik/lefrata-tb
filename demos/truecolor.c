#define WITH_TRUECOLOR 1
#include <stdio.h>
#include "../src/termbox.h"

//input: ratio is between 0 to 1
//output: rgb color
long rgb(double ratio)
{
    //we want to normalize ratio so that it fits in to 6 regions
    //where each region is 256 units long
    double normalized = (ratio * 256 *6);

    //find the distance to the start of the closest region
    int x = (int)normalized % 256;

    uint64_t red = 0, grn = 0, blu = 0;
    switch((int)normalized / 256)
    {
    case 0: red = 255;      grn = x;        blu = 0;       break;//red
    case 1: red = 255 - x;  grn = 255;      blu = 0;       break;//yellow
    case 2: red = 0;        grn = 255;      blu = x;       break;//green
    case 3: red = 0;        grn = 255 - x;  blu = 255;     break;//cyan
    case 4: red = x;        grn = 0;        blu = 255;     break;//blue
    case 5: red = 255;      grn = 0;        blu = 255 - x; break;//magenta
    }

    return red + (grn << 8) + (blu << 16);
}
/*
static const char * runes[] = {
  " ",
  "░",
  "▒",
  "▓",
  "█"
};*/


int main() {
#ifdef WITH_TRUECOLOR
	tb_init();

	int w = tb_width();
	int h = tb_height();
	uint64_t bg = 0x000000, fg = 0x000000;
	//int z = 0;
	uint32_t ch;
    tb_utf8_char_to_unicode(&ch, " ");

     double range = w * h;
    int pos = 0;

	for (int y = 0; y < h; y++) {
        //tb_utf8_char_to_unicode(&ch, runes[ (y % 5) ]);
bg = tb_rgb(rgb( pos / range ));
		for (int x = 0; x < w; x++) {
            pos++;
             //bg =

            //bg = tb_rgb(rgb( x / range));
			//if (z % 2 == 0) fg |= TB_BOLD;
			//if (z % 3 == 0) fg |= TB_UNDERLINE;
            //if (z % 5 == 0) fg |= TB_REVERSE;

			tb_char(x, y, fg, bg, ch);
		}



        char buffer[10];

        snprintf(buffer, 10, "%#lx", bg);


        //write_title((char *) hex);
        tb_string_with_limit(0, y, bg, fg,
        buffer, 10
        //"Test"
        );
	}

	tb_render();

	while (1) {
		struct tb_event ev;
		int t = tb_poll_event(&ev);

		if (t == -1) {
			break;
		}

		if (t == TB_EVENT_KEY) {
			break;
		}
	}

	tb_shutdown();
	return 0;
#else
	printf("True color support not enabled. Please rebuild with WITH_TRUECOLOR option.\n");
	return 1;
#endif
}
