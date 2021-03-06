#ifndef SYSTEM_H
#define SYSTEM_H
#include <ctime>
namespace sys {
	double timeInSec();
	clock_t timeInMSec();
	void closeProgram();
	bool errorMessagesEnabled();
}
#endif // SYSTEM_H
