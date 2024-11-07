#pragma once

#include <random>
#include <sstream>
#include <stdexcept>

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

namespace py = pybind11;

template <typename... Args>
std::string Format(Args&&... args) {
  std::stringstream str;
  (str << ... << args);
  return str.str();
}

#define Throw(...)          \
  throw std::runtime_error{ \
      Format(__FILE__, ":", __LINE__, " error: ", __VA_ARGS__)};

#define CheckError(ret)            \
  if (ret < 0) {                   \
    char buffer[128];              \
    av_strerror(ret, buffer, 128); \
    Throw(buffer);                 \
  }

template <typename Tp, typename = std::enable_if_t<std::is_integral_v<Tp>>>
class Random {
 public:
  explicit Random(Tp min, Tp max) : dist_{min, max} {}

  Tp operator()() {
    return dist_(engine_);
  }

 private:
  inline static thread_local std::random_device device_{};
  inline static thread_local std::default_random_engine engine_{device_()};
  std::uniform_int_distribution<Tp> dist_;
};
