#include "encoder.hh"

#include <utility>

static const AVCodec* FindEncoderByName(std::string_view name) {
  auto codec = avcodec_find_encoder_by_name(name.data());
  if (codec == nullptr) {
    Throw("could not find encoder ", name);
  }
  return codec;
}

Encoder::Encoder(const AVCodec* codec, const AVStream* stream)
    : Encoder{codec, {}, stream} {}

Encoder::Encoder(std::string_view codec, const AVStream* stream)
    : Encoder{codec, {}, stream} {}

Encoder::Encoder(const AVCodec* codec, const CodecConfig& config,
                 const AVStream* stream)
    : ctx_{avcodec_alloc_context3(codec)} {
  if (stream) {
    CheckError(avcodec_parameters_to_context(ctx_, stream->codecpar));
  }
  config.Apply(ctx_);
  CheckError(avcodec_open2(ctx_, codec, nullptr));
}

Encoder::Encoder(std::string_view codec, const CodecConfig& config,
                 const AVStream* stream)
    : Encoder{FindEncoderByName(codec), config, stream} {}

Encoder::Encoder(Encoder&& other) noexcept
    : ctx_{std::exchange(other.ctx_, nullptr)} {}

Encoder& Encoder::operator=(Encoder&& other) noexcept {
  avcodec_free_context(&ctx_);
  ctx_ = std::exchange(other.ctx_, nullptr);
  return *this;
}

Encoder::~Encoder() noexcept {
  avcodec_free_context(&ctx_);
}

void Encoder::SetOption(std::string_view name, std::string_view value,
                        int flags) {
  CheckError(av_opt_set(ctx_->priv_data, name.data(), value.data(), flags));
}

void Encoder::Send(const Frame& frame) {
  CheckError(avcodec_send_frame(ctx_, *frame));
}

bool Encoder::Receive(Packet& packet) {
  packet.MakeWritable();
  int ret = avcodec_receive_packet(ctx_, *packet);
  if (ret == AVERROR(EAGAIN) || ret == AVERROR(EOF)) {
    return false;
  }
  CheckError(ret);
  return true;
}

std::optional<Packet> Encoder::Receive() {
  Packet packet{};
  if (Receive(packet)) {
    return packet;
  }
  return std::nullopt;
}

void Encoder::Flush() {
  CheckError(avcodec_send_frame(ctx_, nullptr));
}

AVCodecContext* Encoder::operator*() const noexcept {
  return ctx_;
}

AVCodecContext* Encoder::operator->() const noexcept {
  return ctx_;
}

Encoder::Iterator::Iterator(Encoder* encoder) : encoder_{encoder} {
  if (encoder_) {
    packet_ = encoder_->Receive();
  }
}

Encoder::Iterator::Iterator(Iterator&& other) noexcept
    : encoder_{std::exchange(other.encoder_, nullptr)},
      packet_{std::move(other.packet_)} {}

Encoder::Iterator& Encoder::Iterator::operator=(Iterator&& other) noexcept {
  encoder_ = std::exchange(other.encoder_, nullptr);
  packet_ = std::move(other.packet_);
  return *this;
}

Packet Encoder::Iterator::operator*() {
  auto packet = std::move(*packet_);
  packet_.reset();
  return packet;
}

Encoder::Iterator& Encoder::Iterator::operator++() {
  packet_ = encoder_->Receive();
  return *this;
}

bool Encoder::Iterator::operator==(const Iterator& other) const noexcept {
  return packet_.has_value() == other.packet_.has_value();
}

bool Encoder::Iterator::operator!=(const Iterator& other) const noexcept {
  return !(*this == other);
}

Encoder::Iterator Encoder::begin() noexcept {
  return Iterator{this};
}

Encoder::Iterator Encoder::end() noexcept {
  return Iterator{};
}

void Encoder::Register(py::module_& m) {
  auto c = py::class_<Encoder>(m, "Encoder");

  c.def(py::init<std::string_view, const AVStream*>(), py::arg("codec_name"),
        py::arg("stream") = py::none{});
  c.def(py::init<std::string_view, const CodecConfig&, const AVStream*>(),
        py::arg("codec_name"), py::arg("config"),
        py::arg("stream") = py::none{});

  c.def("set_option", &Encoder::SetOption, py::arg("name"), py::arg("value"),
        py::arg("flags") = py::int_{0});
  c.def("send", &Encoder::Send, py::arg("frame"));
  c.def("receive", static_cast<bool (Encoder::*)(Packet&)>(&Encoder::Receive),
        py::arg("packet"));
  c.def("receive",
        static_cast<std::optional<Packet> (Encoder::*)()>(&Encoder::Receive));

  c.def(
      "__iter__",
      [](Encoder& e) { return py::make_iterator(e.begin(), e.end()); },
      py::keep_alive<0, 1>());

  c.def_property_readonly("pix_fmt",
                          [](const Encoder& e) { return e->pix_fmt; });
  c.def_property_readonly("size", [](const Encoder& e) {
    return std::pair{e->width, e->height};
  });
  c.def_property_readonly("delay", [](const Encoder& e) { return e->delay; });
}
