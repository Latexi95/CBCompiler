#include "referencecounter.h"


ReferenceCounter::operator int() {
	return mCounter;
}


bool ReferenceCounter::increase() {
	/*unsigned char ret;
	asm volatile("lock\n"
				 "incl %0\n"
				 "setne %1"
				 : "=m" (mCounter), "=qm" (ret)
				 : "m" (mCounter)
				 : "memory");*/
	//return atomic_increase(&mCounter);
	return ++mCounter;
}

bool ReferenceCounter::decrease() {
	/*unsigned char ret;
	asm volatile("lock\n"
				 "decl %0\n"
				 "setne %1"
				 : "=m" (mCounter), "=qm" (ret)
				 : "m" (mCounter)
				 : "memory");*/
	//return atomic_decrease(&mCounter);
	return --mCounter;
}
