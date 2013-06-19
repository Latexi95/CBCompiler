#include "window.h"
#include <assert.h>
#include "error.h"
#include "system.h"

static Window *sInstance = 0;

Window::Window():
	mDisplay(0),
	mEventQueue(0) {
	assert(sInstance == 0);
	sInstance = this;
}

Window::~Window() {
	sInstance = 0;
}

Window *Window::instance() {
	assert(sInstance);
	return sInstance;
}

bool Window::create(int width, int height, Window::WindowMode windowMode) {
	assert(mDisplay == 0 && mEventQueue == 0);

	switch (windowMode) {
		case Windowed:
			al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_OPENGL); break;
		case Resizable:
			al_set_new_bitmap_flags(ALLEGRO_RESIZABLE | ALLEGRO_OPENGL); break;
		case FullScreen:
			al_set_new_bitmap_flags(ALLEGRO_FULLSCREEN | ALLEGRO_OPENGL); break;
	}
	mDisplay = al_create_display(width, height);
	if (mDisplay == 0) {
		error(U"Creating a window failed");

		//If fullscreen, try windowed
		if (windowMode == FullScreen) {
			return create(width, height);
		}
		else {
			return false;
		}
	}

	mEventQueue = al_create_event_queue();
	al_register_event_source(mEventQueue, al_get_display_event_source(mDisplay));

	mBackgroundColor = al_map_rgb(0,0,0);

	activate();
	al_clear_to_color(mBackgroundColor);

	return true;

}

void Window::close() {
	al_destroy_display(mDisplay);
	al_destroy_event_queue(mEventQueue);
	mDisplay = 0;
	mEventQueue = 0;
}

bool Window::isValid() const {
	return mDisplay != 0;
}

bool Window::activate() {
	al_set_target_backbuffer(mDisplay);
	sCurrentTarget = this;
	return true;
}

bool Window::deactivate() {
	sCurrentTarget = 0;
	return true;
}

void Window::drawscreen() {
	ALLEGRO_EVENT e;
	while (al_get_next_event(mEventQueue, &e)) {
		handleEvent(e);
	}
	al_flip_display();
	activate();

	al_clear_to_color(mBackgroundColor);
}

void Window::setBackgroundColor(const ALLEGRO_COLOR &color) {
	mBackgroundColor = color;
}

void Window::handleEvent(const ALLEGRO_EVENT &event) {
	switch (event.type) {
		case ALLEGRO_EVENT_KEY_DOWN:
			//Safe exit
			if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
				closeProgram();
			}
			break;
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			closeProgram(); break;
	}
}
