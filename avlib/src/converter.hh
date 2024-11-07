#pragma once

#include "common.hh"
#include "frame.hh"

class Converter {
 public:
  explicit Converter(AVPixelFormat format, int width, int height);
  Converter(const Converter& other) = delete;
  Converter(Converter&& other) noexcept;
  Converter& operator=(const Converter& other) = delete;
  Converter& operator=(Converter&& other) noexcept;
  ~Converter() noexcept;

  void Convert(const Frame& src, Frame& dst);
  void Convert(const Frame& src, uint8_t* const dst_data[],
               const int dst_stride[]);
  void Convert(const Frame& src, void* dst_data, int dst_stride);
  Frame Convert(const Frame& src);

  static void Register(py::module_& m);

 private:
  SwsContext* ctx_{nullptr};
  AVPixelFormat format_;
  int width_;
  int height_;
};
