#include <allegro5\allegro.h>
#include <allegro5\allegro_font.h>
#include <allegro5\allegro_ttf.h>
#include <allegro5\allegro_primitives.h>
#include <allegro5\allegro_image.h>
#include <cmath>

const int width = 640, height = 480;

void InitBall(ALLEGRO_BITMAP *ballImage);
void UpdateBall();

struct moving
{
	float x;
	float y;
	float velX;
	float velY;
	int width;
	int height;
	int radius;
	int rotateSpeed;
	float degree;
}ball;

int main() {
	bool done = false;
	bool redraw = true;
	float FPS = 60;
	float distance;
	int mouseX = 0;
	int mouseY = 0;
	bool mouseHold = false;
	bool ballHold = false;
	bool showInfo = false;

	al_init();
	al_init_primitives_addon();
	al_init_image_addon();
	al_init_font_addon();
	al_init_ttf_addon();
	al_install_keyboard();
	al_install_mouse();

	ALLEGRO_TIMER *timer = NULL;
	ALLEGRO_EVENT_QUEUE *event_queue = NULL;
	ALLEGRO_BITMAP *background = NULL;
	ALLEGRO_FONT *font18 = NULL;
	ALLEGRO_DISPLAY *display = NULL;
	ALLEGRO_BITMAP *ballImage = NULL;

	display = al_create_display(width, height);
	timer = al_create_timer(1.0 / FPS);
	event_queue = al_create_event_queue();

	font18 = al_load_font("arial.ttf", 18, 0);
	ballImage = al_load_bitmap("ball.png");
	InitBall(ballImage);

	al_register_event_source(event_queue, al_get_display_event_source(display));
	al_register_event_source(event_queue, al_get_timer_event_source(timer));
	al_register_event_source(event_queue, al_get_keyboard_event_source());
	al_register_event_source(event_queue, al_get_mouse_event_source());

	al_start_timer(timer);

	while (!done) {
		ALLEGRO_EVENT ev;
		al_wait_for_event(event_queue, &ev);

		if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
			done = true;
		else if (ev.type == ALLEGRO_EVENT_TIMER) {
			redraw = true;

			UpdateBall();
			}
		else if (ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
			mouseX = ev.mouse.x;
			mouseY = ev.mouse.y;
		}
		else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
			if (ev.mouse.button == 1) {
				mouseHold = true;
				
				distance = sqrt(pow(float(mouseX - ball.x), 2) + pow(float(mouseY - ball.y), 2));
				if (distance <= ball.radius) {
					ballHold = true;
					ball.velY -= rand() % 30 + 20;
					ball.velX = rand() % 50 - 25;
				}
				else
					ballHold = false;
			}
		}
		else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
			mouseHold = false;
			ballHold = false;
		}
		else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
			switch (ev.keyboard.keycode) {
			case ALLEGRO_KEY_ESCAPE:
				done = true;
				break;
			case ALLEGRO_KEY_LEFT:
				ball.degree -= 10;
				if (ball.degree <= 0)
					ball.degree = 360;
				break;
			case ALLEGRO_KEY_RIGHT:
				ball.degree += 10;
				if (ball.degree >= 360)
					ball.degree = 0;
				break;
			case ALLEGRO_KEY_F3:
				showInfo = !showInfo;
				break;
			}
		}


		if (redraw) {
			al_clear_to_color(al_map_rgb(0,0,0));
			al_draw_rotated_bitmap(ballImage, ball.width / 2, ball.height / 2, ball.x, ball.y, ball.degree * 3.14159 / 180, 0);
			if (showInfo) {
				al_draw_textf(font18, al_map_rgb(255, 255, 255), 0, 0, 0, "Ball X: %i   Ball Y: %i   Ball velX: %f   Ball velY: %f", int(ball.x), int(ball.y), ball.velX, ball.velY);
				al_draw_textf(font18, al_map_rgb(255, 255, 255), 0, 20, 0, "Ball Degree: %f", ball.degree);
				al_draw_textf(font18, al_map_rgb(255, 255, 255), 0, 40, 0, "Mouse X: %i    Mouse Y: %i", mouseX, mouseY);
				al_draw_textf(font18, al_map_rgb(255, 255, 255), 0, 60, 0, "Mouse button down: %i", mouseHold);
				al_draw_filled_circle(ball.x, ball.y, ball.radius, al_map_rgba(255, 0, 255, .6));
			}
			al_flip_display();
		}
	}
}

void InitBall(ALLEGRO_BITMAP *ballImage) {
	ball.x = width / 2;
	ball.y = height / 2;
	ball.width = al_get_bitmap_width(ballImage);
	ball.height = al_get_bitmap_height(ballImage);
	ball.radius = ball.width / 2 - 3;
	ball.velX = 0;
	ball.velY = 0;
	ball.degree = 0;
	ball.rotateSpeed = 0;
}

void UpdateBall() {
	if (ball.y + ball.radius >= height) {
		ball.velY = ball.velY * -.5;
		ball.y = height - ball.radius;
	}
	else if (ball.y - ball.radius <= 0) {
		ball.velY = ball.velY * -.5;
		ball.y = ball.radius;
	}
	if (ball.x + ball.radius >= width) {
		ball.velX = ball.velX * -.5;
		ball.x = width - ball.radius;
	}
	else if (ball.x - ball.radius <= 0) {
		ball.velX = ball.velX * -.5;
		ball.x = ball.radius;
	}

	if (ball.velX > 0)
		ball.velX -= .01;
	else if (ball.velX < 0)
		ball.velX += .01;

	if (ball.velX < .05 && ball.velX > -.05)
		ball.velX = 0;

	ball.velY += .6;
	ball.x += ball.velX;
	ball.y += ball.velY;
	ball.degree += ball.velX;

}