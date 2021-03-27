#include "base.hpp"
// Stackoverflow: "Is there an alternative sleep function in C to milliseconds?"
// Slaap voor ms milliseconden.
int sleep_ms(long ms) {
    timespec ts;
    int res;

    if (ms < 0) {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = ms/1000;
    ts.tv_nsec = (ms % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return ms;
}