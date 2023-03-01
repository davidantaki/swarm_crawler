#ifndef _FAKE_THREAD_H_
#define _FAKE_THREAD_H_

#include "utils/log.h"
#include <stdint.h>

class FakeThread {
public:
  FakeThread() {}
  ~FakeThread() {}

  virtual bool init() = 0;
  virtual bool loopHook() = 0;
  bool loop();

private:
  uint32_t lastLoop = 0;
  static constexpr uint32_t LOOP_DELAY_MS = 1000;
};

#endif // _FAKE_THREAD_H_