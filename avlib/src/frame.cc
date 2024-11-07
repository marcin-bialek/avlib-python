#include "frame.hh"

Frame::Frame() : handle_{av_frame_alloc()} {}

Frame::Frame(AVPixelFormat format, int width, int height) : Frame{} {
  handle_->format = format;
  handle_->width = width;
  handle_->height = height;
  CheckError(av_frame_get_buffer(handle_, 0));
}

Frame::Frame(AVPixelFormat format, const py::array_t<uint8_t>& data)
    : Frame{format, data.shape(1), data.shape(0)} {
  switch (format) {
    case AV_PIX_FMT_RGBA:
    case AV_PIX_FMT_ARGB:
    case AV_PIX_FMT_BGRA:
    case AV_PIX_FMT_ABGR:
      for (std::size_t y = 0, rp = 0; y < data.shape(0); ++y) {
        auto cp = rp;
        for (std::size_t x = 0; x < data.shape(1); ++x) {
          for (std::size_t z = 0; z < data.shape(2); ++z) {
            handle_->data[0][cp++] = data.at(y, x, z);
          }
        }
        rp += handle_->linesize[0];
      }
      break;
    default:
      Throw("unsupported format");
  }
}

Frame::Frame(const Frame& other) : handle_{av_frame_clone(other.handle_)} {}

Frame::Frame(Frame&& other) noexcept : handle_{av_frame_alloc()} {
  av_frame_move_ref(handle_, other.handle_);
}

Frame& Frame::operator=(const Frame& other) {
  av_frame_unref(handle_);
  av_frame_ref(handle_, other.handle_);
  return *this;
}

Frame& Frame::operator=(Frame&& other) noexcept {
  av_frame_unref(handle_);
  av_frame_move_ref(handle_, other.handle_);
  return *this;
}

Frame::~Frame() noexcept {
  av_frame_free(&handle_);
}

void Frame::SetFlag(Flag flag) noexcept {
  handle_->flags |= static_cast<int>(flag);
}

void Frame::Unref() const noexcept {
  av_frame_unref(handle_);
}

void Frame::MakeWritable() const noexcept {
  av_frame_make_writable(handle_);
}

AVFrame* Frame::operator*() const noexcept {
  return handle_;
}

AVFrame* Frame::operator->() const noexcept {
  return handle_;
}

void Frame::Register(py::module_& m) {
  auto c = py::class_<Frame>(m, "Frame");

  py::enum_<Flag>(c, "Flag")
      .value("CORRUPT", Flag::CORRUPT)
      .value("KEY", Flag::KEY)
      .value("DISCARD", Flag::DISCARD)
      .value("INTERLACED", Flag::INTERLACED)
      .value("TOP_FIELD_FIRST", Flag::TOP_FIELD_FIRST);

  c.def(py::init<>());
  c.def(py::init<>([](AVPixelFormat format, std::pair<int, int> size) {
    return Frame{format, size.first, size.second};
  }));
  c.def(py::init<AVPixelFormat, const py::array_t<uint8_t>&>());

  c.def("set_flag", &Frame::SetFlag);
  c.def("unref", &Frame::Unref);
  c.def("make_writable", &Frame::MakeWritable);
  c.def_property_readonly("width",
                          [](const Frame& frame) { return frame->width; });
  c.def_property_readonly("height",
                          [](const Frame& frame) { return frame->height; });
  c.def_property_readonly("format", [](const Frame& frame) {
    return static_cast<AVPixelFormat>(frame->format);
  });
  c.def_property(
      "pts", [](const Frame& frame) { return frame->pts; },
      [](const Frame& frame, int64_t v) { frame->pts = v; });
  c.def_property(
      "pict_type", [](const Frame& frame) { return frame->pict_type; },
      [](const Frame& frame, AVPictureType v) { frame->pict_type = v; });

  c.def("planes", [](const Frame& frame) {
    std::vector<py::array_t<uint8_t>> arrays;
    switch (static_cast<AVPixelFormat>(frame->format)) {
      case AV_PIX_FMT_RGBA:
      case AV_PIX_FMT_ARGB:
      case AV_PIX_FMT_BGRA:
      case AV_PIX_FMT_ABGR:
        arrays.emplace_back(
            std::vector<ssize_t>{frame->height, frame->width, 4},
            std::vector<ssize_t>{frame->linesize[0], 4, 1}, frame->data[0]);
        break;
      case AV_PIX_FMT_YUV420P:
        arrays.emplace_back(std::vector<ssize_t>{frame->height, frame->width},
                            std::vector<ssize_t>{frame->linesize[0], 1},
                            frame->data[0]);
        arrays.emplace_back(
            std::vector<ssize_t>{frame->height / 2, frame->width / 2},
            std::vector<ssize_t>{frame->linesize[1], 1}, frame->data[1]);
        arrays.emplace_back(
            std::vector<ssize_t>{frame->height / 2, frame->width / 2},
            std::vector<ssize_t>{frame->linesize[2], 1}, frame->data[2]);
        break;
      default:
        Throw("unsupported format");
    }
    return arrays;
  });
}
