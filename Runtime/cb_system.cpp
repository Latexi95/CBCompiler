#include <stdio.h>
#include <time.h>
#include <iostream>
#include <locale>
#include <stdint.h>
#include "window.h"
#include "error.h"
#include "systeminterface.h"

extern "C" char *CBF_CB_Allocate(int size) {
	return new char[size];
}

extern "C" void CBF_CB_Free(char *mem) {
	delete [] mem;
}

extern "C" int CBF_intF(float f) {
	return int(f + 0.5f);
}

extern "C" int CBF_intS(CBString s) {
	return LString(s).toInt();
}

extern "C" float CBF_floatI(int i) {
	return float(i);
}

extern "C" float CBF_floatS(CBString s) {
	return LString(s).toFloat();
}


extern "C" void CBF_printI(int i) {
	printf("%i\n", i);
}

extern "C" void CBF_printF(float f) {
	printf("%f\n", f);
}

extern "C" void CBF_printS(CBString s) {
#ifdef _WIN32 //sizeof(wchar_t) == 2
	std::wcout << LString(s).toWString() << std::endl;
#else //wchar_t == char32_t
	std::cout << LString(s).toUtf8() << std::endl;
#endif
}

extern "C" void CBF_print() {
	printf("\n");
}

extern "C" int CBF_timer() {
	return sys::timeInMSec();
}

extern "C" void CBF_end() {
	sys::closeProgram();
}

extern "C" int CBF_fps() {
	return Window::instance()->fps();
}
