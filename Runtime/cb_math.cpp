#include "common.h"
#include "mathinterface.h"

using namespace math;

float CBF_sin(float f) {
	return sin(toRad(f));
}

float CBF_cos(float f) {
	return cos(toRad(f));
}

float CBF_tan(float f) {
	return tan(toRad(f));
}

float CBF_asin(float f) {
	return toDeg(asin(f));
}

float CBF_acos(float f) {
	return toDeg(acos(f));
}

float CBF_atan(float f) {
	return toDeg(atan(f));
}

float CBF_abs(float f) {
	return fabs(f);
}

int CBF_abs(int i) {
	return abs(i);
}

float CBF_sqrt(float f) {
	return sqrtf(f);
}

float CBF_distance(float x1, float y1, float x2, float y2) {
	return distance(x1, y1, x2, y2);
}

float CBF_max(float a, float b) {
	return max(a, b);
}

int CBF_max(int a, int b) {
	return max(a, b);
}

float CBF_min(float a, float b) {
	return min(a, b);
}

int CBF_min(int a, int b) {
	return min(a, b);
}

float CBF_wrapAngle(float a) {
	return wrapAngle(a);
}

float CBF_rnd(float max) {
	return rnd(max);
}

float CBF_rnd(float min, float max) {
	return rnd(min, max);
}

int CBF_rand(int max) {
	return rand(max);
}

int CBF_rand(int min, int max) {
	return rand(min, max);
}

void CBF_randomize(int seed) {
	randomize(seed);
}

float CBF_roundUp(float f) {
	return ceilf(f);
}

float CBF_roundDown(float f) {
	return floorf(f);
}

float CBF_getAngle(float x1, float y1, float x2, float y2) {
	return getAngle(x1, y1, x2, y2);
}

float CBF_log(float v) {
	return logf(v);
}

float CBF_log10(float v) {
	return log10f(v);
}
