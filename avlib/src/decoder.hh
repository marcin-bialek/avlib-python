#pragma once

#include <optional>
#include <string>
#include <vector>

#include "codec_config.hh"
#include "common.hh"
#include "frame.hh"
#include "packet.hh"

class Decoder {
 public:
  explicit Decoder(const AVCodec* codec, const AVStream* stream = nullptr);
  explicit Decoder(std::string_view codec, const AVStream* stream = nullptr);
  explicit Decoder(const AVCodec* codec, const CodecConfig& config,
                   const AVStream* stream = nullptr);
  explicit Decoder(std::string_view codec, const CodecConfig& config,
                   const AVStream* stream = nullptr);
  Decoder(const Decoder& other) = delete;
  Decoder(Decoder&& other) noexcept;
  Decoder& operator=(const Decoder& other) = delete;
  Decoder& operator=(Decoder&& other) noexcept;
  ~Decoder() noexcept;

  void SetOption(std::string_view name, std::string_view value, int flags = 0);
  void Send(const Packet& packet);
  bool Receive(Frame& frame);
  std::optional<Frame> Receive();
  void Decode(std::vector<Frame>& frames, const Packet& packet);
  void Decode(std::vector<Frame>& frames, const std::vector<Packet>& packets);
  std::vector<Frame> Decode(const Packet& packet);
  std::vector<Frame> Decode(const std::vector<Packet>& packets);

  AVCodecContext* operator*() const noexcept;
  AVCodecContext* operator->() const noexcept;

  struct Iterator {
    using iterator_category = std::input_iterator_tag;
    using difference_type = void;
    using value_type = Frame;
    using pointer = void;
    using reference = void;

    explicit Iterator(Decoder* decoder = nullptr);
    Iterator(const Iterator& other) = delete;
    Iterator(Iterator&& other) noexcept;
    Iterator& operator=(const Iterator& other) = delete;
    Iterator& operator=(Iterator&& other) noexcept;
    Frame operator*();
    Iterator& operator++();
    bool operator==(const Iterator& other) const noexcept;
    bool operator!=(const Iterator& other) const noexcept;

   private:
    Decoder* decoder_{nullptr};
    std::optional<Frame> frame_;
  };

  Iterator begin() noexcept;
  Iterator end() noexcept;

  static void Register(py::module_& m);

 private:
  AVCodecContext* ctx_{nullptr};
};
