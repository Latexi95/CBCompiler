#include "image.h"
#include "systeminterface.h"
#include "error.h"
Image *CBF_makeImage(int w, int h) {
	Image *img = new Image(w, h);
	return img;
}

Image *CBF_loadImage(CBString str) {
	Image *img = Image::load(str);
	if (!img && sys::errorMessagesEnabled()) {
		error(LString(U"LoadImage failed!  \"%1\"").arg(str));
	}
	return img;
}

void CBF_drawImage(Image *img, float x, float y) {
	img->draw(x, y);
}

void CBF_maskImage(Image *img, int r, int g, int b) {
	img->mask(al_map_rgb(r, g, b));
}

void CBF_maskImage(Image *img, int r, int g, int b, int a) {
	img->mask(al_map_rgba(r, g, b, a));
}
