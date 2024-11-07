#include "converter.hh"

#include <utility>

Converter::Converter(AVPixelFormat format, int width, int height)
    : format_{format}, width_{width}, height_{height} {}

Converter::Converter(Converter&& other) noexcept
    : ctx_{std::exchange(other.ctx_, nullptr)},
      format_{other.format_},
      width_{other.width_},
      height_{other.height_} {}

Converter& Converter::operator=(Converter&& other) noexcept {
  sws_freeContext(ctx_);
  ctx_ = std::exchange(other.ctx_, nullptr);
  format_ = other.format_;
  width_ = other.width_;
  height_ = other.height_;
  return *this;
}

Converter::~Converter() noexcept {
  sws_freeContext(ctx_);
}

void Converter::Convert(const Frame& src, Frame& dst) {
  Convert(src, dst->data, dst->linesize);
}

void Converter::Convert(const Frame& src, uint8_t* const dst_data[],
                        const int dst_stride[]) {
  ctx_ = sws_getCachedContext(
      ctx_, src->width, src->height, static_cast<AVPixelFormat>(src->format),
      width_, height_, format_, SWS_BICUBIC, nullptr, nullptr, nullptr);
  sws_scale(ctx_, src->data, src->linesize, 0, src->height, dst_data,
            dst_stride);
}

void Converter::Convert(const Frame& src, void* dst_data, int dst_stride) {
  uint8_t* data[] = {static_cast<uint8_t*>(dst_data)};
  int stride[] = {dst_stride};
  Convert(src, data, stride);
}

Frame Converter::Convert(const Frame& src) {
  Frame frame{format_, width_, height_};
  Convert(src, frame);
  return frame;
}

void Converter::Register(py::module_& m) {
  auto c = py::class_<Converter>(m, "Converter");

  c.def(py::init([](AVPixelFormat format, std::pair<int, int> size) {
          return Converter{format, size.first, size.second};
        }),
        py::arg("format"), py::arg("size"));

  c.def("convert",
        static_cast<void (Converter::*)(const Frame&, Frame&)>(
            &Converter::Convert),
        py::arg("src"), py::arg("dst"));
  c.def("convert",
        static_cast<Frame (Converter::*)(const Frame&)>(&Converter::Convert),
        py::arg("src"));
}
