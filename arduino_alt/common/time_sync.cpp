#include "time_sync.h"

#include <sys/time.h>
#include <time.h>

namespace time_sync {

namespace {
int64_t offset = 0;
}

void begin() {
    offset = 0;
}

void update_from_epoch_ms(uint64_t epoch_ms) {
    offset = static_cast<int64_t>(epoch_ms) - static_cast<int64_t>(millis());
    struct timeval tv;
    tv.tv_sec = epoch_ms / 1000;
    tv.tv_usec = (epoch_ms % 1000) * 1000;
    settimeofday(&tv, nullptr);
}

uint64_t now_epoch_ms() {
    return static_cast<uint64_t>(millis() + offset);
}

String now_iso() {
    time_t sec = now_epoch_ms() / 1000;
    struct tm tm;
    gmtime_r(&sec, &tm);
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return String(buffer);
}

int64_t offset_ms() {
    return offset;
}

}  // namespace time_sync

