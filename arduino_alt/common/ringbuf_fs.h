#pragma once

#include <Arduino.h>
#include <LittleFS.h>

namespace storage {

class RingbufFS {
  public:
    bool begin(size_t target_bytes);
    bool append(const String &category, const uint8_t *payload, size_t length, const String &ts_iso);
    bool stream_since(const String &category, const String &since_iso, Stream &out, size_t limit = 128);
    void purge_expired(uint32_t retention_days);
    size_t usage_bytes() const;

  private:
    bool ensure_mount();
    String category_path(const String &category) const;
    String file_path(const String &category, const String &ts_iso) const;
    void enforce_quota();

    size_t target_bytes_ = 0;
};

}  // namespace storage

