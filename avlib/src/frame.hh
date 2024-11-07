#pragma once

#include "common.hh"

class Frame {
 public:
  enum class Flag : int {
    CORRUPT = AV_FRAME_FLAG_CORRUPT,
    KEY = AV_FRAME_FLAG_KEY,
    DISCARD = AV_FRAME_FLAG_DISCARD,
    INTERLACED = AV_FRAME_FLAG_INTERLACED,
    TOP_FIELD_FIRST = AV_FRAME_FLAG_TOP_FIELD_FIRST,
  };

  explicit Frame();
  explicit Frame(AVPixelFormat format, int width, int height);
  explicit Frame(AVPixelFormat format, const py::array_t<uint8_t>& data);
  Frame(const Frame& other);
  Frame(Frame&& other) noexcept;
  Frame& operator=(const Frame& other);
  Frame& operator=(Frame&& other) noexcept;
  ~Frame() noexcept;

  void SetFlag(Flag flag) noexcept;
  void Unref() const noexcept;
  void MakeWritable() const noexcept;

  AVFrame* operator*() const noexcept;
  AVFrame* operator->() const noexcept;

  static void Register(py::module_& m);

 private:
  AVFrame* handle_{nullptr};
};
