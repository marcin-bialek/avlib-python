#pragma once

#include <optional>

#include "common.hh"

struct CodecConfig {
  enum class Flag : int {
    LOW_DELAY = AV_CODEC_FLAG_LOW_DELAY,
  };

  std::optional<AVPixelFormat> format;
  std::optional<AVRational> framerate;
  std::optional<AVRational> timebase;
  std::optional<int> width;
  std::optional<int> height;
  std::optional<int> bitrate;
  std::optional<int> gop_size;
  std::optional<int> keyint_min;
  std::optional<int> max_b_frames;
  std::optional<int> refs;
  std::optional<int> flags;

  void SetFlag(Flag flag);
  void Apply(AVCodecContext* ctx) const;

  static void Register(py::module_& m);
};
