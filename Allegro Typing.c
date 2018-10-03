#include <allegro5\allegro.h>
#include <allegro5\allegro_font.h>
#include <allegro5\allegro_ttf.h>
#include <string>
#include <iostream>

using namespace std;

const int screenWidth = 640, screenHeight = 480;

int main() {
	bool done = false;
	bool redraw = true;

	al_init();
	al_init_font_addon();
	al_init_ttf_addon();
	al_install_keyboard();

	ALLEGRO_DISPLAY *display = NULL;
	ALLEGRO_EVENT_QUEUE *event_queue = NULL;
	ALLEGRO_TIMER *timer = NULL;
	ALLEGRO_FONT *font18 = NULL;
	ALLEGRO_USTR *input = al_ustr_new("");

	display = al_create_display(screenWidth, screenHeight);
	event_queue = al_create_event_queue();
	timer = al_create_timer(1.0 / 60.0);
	font18 = al_load_font("arial.ttf", 18, NULL);

	al_register_event_source(event_queue, al_get_display_event_source(display));
	al_register_event_source(event_queue, al_get_keyboard_event_source());
	al_register_event_source(event_queue, al_get_timer_event_source(timer));

	al_start_timer(timer);

	while (!done) {
		ALLEGRO_EVENT ev;
		al_wait_for_event(event_queue, &ev);

		if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
			done = true;
		else if (ev.type == ALLEGRO_EVENT_TIMER) {
			redraw = true;
		}
		else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
			switch (ev.keyboard.keycode) {
			case ALLEGRO_KEY_ESCAPE:
				done = true;
				break;

			case ALLEGRO_EVENT_KEY_CHAR: {
				int unichar = ev.keyboard.unichar;
				if (unichar >= 32)
					al_ustr_append_chr(input, unichar);
				break;
}

				cout << input << endl;
			}
		}

		if (redraw && al_is_event_queue_empty(event_queue)) {
			redraw = false;

			al_clear_to_color(al_map_rgb(0,0,0));
			
			al_draw_textf(font18, al_map_rgb(255, 255, 255), screenWidth / 2, screenHeight / 2, ALLEGRO_ALIGN_CENTRE, "%s", input);
			al_flip_display();
		}
	}
}