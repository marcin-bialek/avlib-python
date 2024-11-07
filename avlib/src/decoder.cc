#include "decoder.hh"

#include <utility>

#include "common.hh"

static const AVCodec* FindDecoderByName(std::string_view name) {
  auto codec = avcodec_find_decoder_by_name(name.data());
  if (codec == nullptr) {
    Throw("could not find decoder ", name);
  }
  return codec;
}

Decoder::Decoder(const AVCodec* codec, const AVStream* stream)
    : Decoder{codec, {}, stream} {}

Decoder::Decoder(std::string_view codec, const AVStream* stream)
    : Decoder{codec, {}, stream} {}

Decoder::Decoder(const AVCodec* codec, const CodecConfig& config,
                 const AVStream* stream)
    : ctx_{avcodec_alloc_context3(codec)} {
  if (stream) {
    CheckError(avcodec_parameters_to_context(ctx_, stream->codecpar));
  }
  config.Apply(ctx_);
  CheckError(avcodec_open2(ctx_, codec, nullptr));
}

Decoder::Decoder(std::string_view codec, const CodecConfig& config,
                 const AVStream* stream)
    : Decoder{FindDecoderByName(codec), config, stream} {}

Decoder::Decoder(Decoder&& other) noexcept
    : ctx_{std::exchange(other.ctx_, nullptr)} {}

Decoder& Decoder::operator=(Decoder&& other) noexcept {
  avcodec_free_context(&ctx_);
  ctx_ = std::exchange(other.ctx_, nullptr);
  return *this;
}

Decoder::~Decoder() noexcept {
  avcodec_free_context(&ctx_);
}

void Decoder::SetOption(std::string_view name, std::string_view value,
                        int flags) {
  CheckError(av_opt_set(ctx_->priv_data, name.data(), value.data(), flags));
}

void Decoder::Send(const Packet& packet) {
  CheckError(avcodec_send_packet(ctx_, *packet));
}

bool Decoder::Receive(Frame& frame) {
  frame.MakeWritable();
  auto ret = avcodec_receive_frame(ctx_, *frame);
  if (ret == AVERROR(EAGAIN) || ret == AVERROR(EOF)) {
    return false;
  }
  CheckError(ret);
  return true;
}

std::optional<Frame> Decoder::Receive() {
  Frame frame{};
  if (Receive(frame)) {
    return frame;
  }
  return std::nullopt;
}

void Decoder::Decode(std::vector<Frame>& frames, const Packet& packet) {
  Send(packet);
  auto frame = Receive();
  while (frame.has_value()) {
    frames.push_back(std::move(*frame));
    frame = Receive();
  }
}

void Decoder::Decode(std::vector<Frame>& frames,
                     const std::vector<Packet>& packets) {
  for (auto& packet : packets) {
    Decode(frames, packet);
  }
}

std::vector<Frame> Decoder::Decode(const Packet& packet) {
  std::vector<Frame> frames{};
  Decode(frames, packet);
  return frames;
}

std::vector<Frame> Decoder::Decode(const std::vector<Packet>& packets) {
  std::vector<Frame> frames{};
  Decode(frames, packets);
  return frames;
}

AVCodecContext* Decoder::operator*() const noexcept {
  return ctx_;
}

AVCodecContext* Decoder::operator->() const noexcept {
  return ctx_;
}

Decoder::Iterator::Iterator(Decoder* decoder) : decoder_{decoder} {
  if (decoder_) {
    frame_ = decoder_->Receive();
  }
}

Decoder::Iterator::Iterator(Iterator&& other) noexcept
    : decoder_{std::exchange(other.decoder_, nullptr)},
      frame_{std::move(other.frame_)} {}

Decoder::Iterator& Decoder::Iterator::operator=(Iterator&& other) noexcept {
  decoder_ = std::exchange(other.decoder_, nullptr);
  frame_ = std::move(other.frame_);
  return *this;
}

Frame Decoder::Iterator::operator*() {
  auto frame = std::move(*frame_);
  frame_.reset();
  return frame;
}

Decoder::Iterator& Decoder::Iterator::operator++() {
  frame_ = decoder_->Receive();
  return *this;
}

bool Decoder::Iterator::operator==(const Iterator& other) const noexcept {
  return frame_.has_value() == other.frame_.has_value();
}

bool Decoder::Iterator::operator!=(const Iterator& other) const noexcept {
  return !(*this == other);
}

Decoder::Iterator Decoder::begin() noexcept {
  return Iterator{this};
}

Decoder::Iterator Decoder::end() noexcept {
  return Iterator{};
}

void Decoder::Register(py::module_& m) {
  auto c = py::class_<Decoder>(m, "Decoder");

  c.def(py::init<std::string_view, const AVStream*>(), py::arg("codec_name"),
        py::arg("stream") = py::none{});
  c.def(py::init<std::string_view, const CodecConfig&, const AVStream*>(),
        py::arg("codec_name"), py::arg("config"),
        py::arg("stream") = py::none{});

  c.def("set_option", &Decoder::SetOption, py::arg("name"), py::arg("value"),
        py::arg("flags") = py::int_{0});
  c.def("send", &Decoder::Send, py::arg("packet"));
  c.def("receive", static_cast<bool (Decoder::*)(Frame&)>(&Decoder::Receive),
        py::arg("frame"));
  c.def("receive",
        static_cast<std::optional<Frame> (Decoder::*)()>(&Decoder::Receive));
  c.def("decode", static_cast<std::vector<Frame> (Decoder::*)(const Packet&)>(
                      &Decoder::Decode));
  c.def(
      "decode",
      static_cast<std::vector<Frame> (Decoder::*)(const std::vector<Packet>&)>(
          &Decoder::Decode));
  c.def(
      "__iter__",
      [](Decoder& d) { return py::make_iterator(d.begin(), d.end()); },
      py::keep_alive<0, 1>());
  c.def_property_readonly("pix_fmt",
                          [](const Decoder& d) { return d->pix_fmt; });
  c.def_property_readonly("size", [](const Decoder& d) {
    return std::pair{d->width, d->height};
  });
  c.def_property_readonly("delay", [](const Decoder& d) { return d->delay; });
}
