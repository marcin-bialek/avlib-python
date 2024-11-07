#include "demuxer.hh"

#include <utility>

#include "common.hh"

Demuxer::Demuxer(std::string_view filename) {
  CheckError(avformat_open_input(&ctx_, filename.data(), nullptr, nullptr));
  CheckError(avformat_find_stream_info(ctx_, nullptr));
}

Demuxer::Demuxer(Demuxer&& other) noexcept
    : ctx_{std::exchange(other.ctx_, nullptr)} {}

Demuxer& Demuxer::operator=(Demuxer&& other) noexcept {
  avformat_close_input(&ctx_);
  ctx_ = std::exchange(other.ctx_, nullptr);
  return *this;
}

Demuxer::~Demuxer() noexcept {
  avformat_close_input(&ctx_);
}

const AVStream* Demuxer::FindBestStream(AVMediaType type) const {
  auto idx = av_find_best_stream(ctx_, type, -1, -1, nullptr, 0);
  if (idx < 0) {
    return nullptr;
  }
  return ctx_->streams[idx];
}

bool Demuxer::Read(Packet& packet, const AVStream* stream) {
  while (0 <= av_read_frame(ctx_, *packet)) {
    if (!stream || packet->stream_index == stream->index) {
      return true;
    }
    packet.Unref();
  }
  return false;
}

std::optional<Packet> Demuxer::Read(const AVStream* stream) {
  Packet packet{};
  if (Read(packet, stream)) {
    return packet;
  }
  return std::nullopt;
}

AVFormatContext* Demuxer::operator*() const noexcept {
  return ctx_;
}

AVFormatContext* Demuxer::operator->() const noexcept {
  return ctx_;
}

void Demuxer::Register(py::module_& m) {
  auto c = py::class_<Demuxer>(m, "Demuxer");
  c.def(py::init<const std::string&>(), py::arg("filename"));
  c.def("find_best_stream", &Demuxer::FindBestStream,
        py::return_value_policy::reference_internal, py::arg("type"));
  c.def(
      "read",
      static_cast<bool (Demuxer::*)(Packet&, const AVStream*)>(&Demuxer::Read),
      py::arg("packet"), py::arg("stream") = py::none{});
  c.def("read",
        static_cast<std::optional<Packet> (Demuxer::*)(const AVStream*)>(
            &Demuxer::Read),
        py::arg("stream") = py::none{});
}
