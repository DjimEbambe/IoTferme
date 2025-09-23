#include "ringbuf_fs.h"

#include <algorithm>
#include <vector>
#include <time.h>

#include "debug_log.h"
#include "payload.h"

namespace storage {

namespace {

time_t make_time_utc(int year, int month, int day) {
    struct tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_isdst = 0;
    return mktime(&tm);
}

}  // namespace

bool RingbufFS::ensure_mount() {
    static bool mounted = false;
    if (!mounted) {
        mounted = LittleFS.begin(true);
        if (!mounted) {
            LOGE("LittleFS mount failed");
            return false;
        }
    }
    return true;
}

bool RingbufFS::begin(size_t target_bytes) {
    target_bytes_ = target_bytes;
    return ensure_mount();
}

String RingbufFS::category_path(const String &category) const {
    String cat = category;
    cat.replace("..", "");
    if (!cat.startsWith("/")) {
        cat = "/" + cat;
    }
    return cat;
}

String RingbufFS::file_path(const String &category, const String &ts_iso) const {
    String timestamp = ts_iso.length() ? ts_iso : payload::iso_timestamp_now();
    String day = timestamp.substring(0, 10);
    day.replace("-", "");
    if (day.length() != 8) {
        day = "00000000";
    }
    String path = category_path(category);
    path += "/";
    path += day;
    path += ".log";
    return path;
}

bool RingbufFS::append(const String &category, const uint8_t *payload, size_t length, const String &ts_iso) {
    if (!ensure_mount()) {
        return false;
    }
    String path = file_path(category, ts_iso);
    String dir = path.substring(0, path.lastIndexOf('/'));
    if (!LittleFS.exists(dir)) {
        LittleFS.mkdir(dir);
    }
    File file = LittleFS.open(path, FILE_APPEND);
    if (!file) {
        return false;
    }
    String ts = ts_iso.length() ? ts_iso : payload::iso_timestamp_now();
    file.print(ts);
    file.print(' ');
    file.write(payload, length);
    file.write('\n');
    file.close();
    enforce_quota();
    return true;
}

bool RingbufFS::stream_since(const String &category, const String &since_iso, Stream &out, size_t limit) {
    if (!ensure_mount()) {
        return false;
    }
    String cutoff = since_iso.substring(0, 10);
    cutoff.replace("-", "");
    if (cutoff.length() != 8) {
        cutoff = "00000000";
    }
    String base = category_path(category);
    File dir = LittleFS.open(base);
    if (!dir) {
        return false;
    }
    std::vector<String> files;
    for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
        if (!f.isDirectory()) {
            files.emplace_back(f.name());
        }
        f.close();
    }
    dir.close();
    std::sort(files.begin(), files.end());
    size_t emitted = 0;
    for (const auto &name : files) {
        if (emitted >= limit) {
            break;
        }
        String filename = name;
        int slash = filename.lastIndexOf('/');
        if (slash >= 0) {
            filename.remove(0, slash + 1);
        }
        String day = filename.substring(0, 8);
        if (day < cutoff) {
            continue;
        }
        File file = LittleFS.open(name, FILE_READ);
        if (!file) {
            continue;
        }
        while (file.available() && emitted < limit) {
            String line = file.readStringUntil('\n');
            out.println(line);
            emitted++;
        }
        file.close();
    }
    return emitted > 0;
}

void RingbufFS::purge_expired(uint32_t retention_days) {
    if (!ensure_mount()) {
        return;
    }
    time_t now = time(nullptr);
    time_t cutoff = now - (retention_days * 86400UL);
    File root = LittleFS.open("/");
    if (!root) {
        return;
    }
    for (File dir = root.openNextFile(); dir; dir = root.openNextFile()) {
        if (!dir.isDirectory()) {
            dir.close();
            continue;
        }
        std::vector<String> files;
        for (File file = dir.openNextFile(); file; file = dir.openNextFile()) {
            files.emplace_back(file.name());
            file.close();
        }
        for (const auto &name : files) {
            String filename = name;
            int slash = filename.lastIndexOf('/');
            if (slash >= 0) {
                filename.remove(0, slash + 1);
            }
            if (filename.length() < 8) {
                continue;
            }
            int year = filename.substring(0, 4).toInt();
            int month = filename.substring(4, 6).toInt();
            int day = filename.substring(6, 8).toInt();
            time_t file_ts = make_time_utc(year, month, day);
            if (file_ts < cutoff) {
                LittleFS.remove(name);
            }
        }
        dir.close();
    }
}

size_t RingbufFS::usage_bytes() const {
    if (!LittleFS.begin(true)) {
        return 0;
    }
    return LittleFS.usedBytes();
}

void RingbufFS::enforce_quota() {
    if (target_bytes_ == 0) {
        return;
    }
    size_t used = LittleFS.usedBytes();
    if (used <= target_bytes_) {
        return;
    }
    struct FileInfo {
        String name;
        time_t ts;
    };
    std::vector<FileInfo> files;
    File root = LittleFS.open("/");
    if (!root) {
        return;
    }
    for (File dir = root.openNextFile(); dir; dir = root.openNextFile()) {
        if (!dir.isDirectory()) {
            dir.close();
            continue;
        }
        for (File file = dir.openNextFile(); file; file = dir.openNextFile()) {
            String filename = file.name();
            int slash = filename.lastIndexOf('/');
            String day = filename.substring(slash + 1, slash + 9);
            int year = day.substring(0, 4).toInt();
            int month = day.substring(4, 6).toInt();
            int day_num = day.substring(6, 8).toInt();
            files.push_back({filename, make_time_utc(year, month, day_num)});
            file.close();
        }
        dir.close();
    }
    std::sort(files.begin(), files.end(), [](const FileInfo &a, const FileInfo &b) { return a.ts < b.ts; });
    for (const auto &info : files) {
        if (LittleFS.remove(info.name)) {
            used = LittleFS.usedBytes();
            if (used <= target_bytes_) {
                break;
            }
        }
    }
}

}  // namespace storage

