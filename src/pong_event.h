#ifndef PONG_EVENT_H
#define PONG_EVENT_H

#include <cstdint>

namespace pong
{
  // ===========================================
  // An enumeration of all possible event types.
  // ===========================================
  enum class EventType : uint8_t {
    // ...
  };

  // ==========================================
  struct Event final
  {
    EventType type;
  };
}

#endif
