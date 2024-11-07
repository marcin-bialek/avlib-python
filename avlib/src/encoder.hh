#pragma once

#include "codec_config.hh"
#include "common.hh"
#include "frame.hh"
#include "packet.hh"

class Encoder {
 public:
  explicit Encoder(const AVCodec* codec, const AVStream* stream = nullptr);
  explicit Encoder(std::string_view codec, const AVStream* stream = nullptr);
  explicit Encoder(const AVCodec* codec, const CodecConfig& config,
                   const AVStream* stream = nullptr);
  explicit Encoder(std::string_view codec, const CodecConfig& config,
                   const AVStream* stream = nullptr);
  Encoder(const Encoder& other) = delete;
  Encoder(Encoder&& other) noexcept;
  Encoder& operator=(const Encoder& other) = delete;
  Encoder& operator=(Encoder&& other) noexcept;
  ~Encoder() noexcept;

  void SetOption(std::string_view name, std::string_view value, int flags = 0);
  void Send(const Frame& frame);
  bool Receive(Packet& packet);
  std::optional<Packet> Receive();
  void Flush();

  AVCodecContext* operator*() const noexcept;
  AVCodecContext* operator->() const noexcept;

  struct Iterator {
    using iterator_category = std::input_iterator_tag;
    using difference_type = void;
    using value_type = Packet;
    using pointer = void;
    using reference = void;

    explicit Iterator(Encoder* decoder = nullptr);
    Iterator(const Iterator& other) = delete;
    Iterator(Iterator&& other) noexcept;
    Iterator& operator=(const Iterator& other) = delete;
    Iterator& operator=(Iterator&& other) noexcept;
    Packet operator*();
    Iterator& operator++();
    bool operator==(const Iterator& other) const noexcept;
    bool operator!=(const Iterator& other) const noexcept;

   private:
    Encoder* encoder_{nullptr};
    std::optional<Packet> packet_;
  };

  Iterator begin() noexcept;
  Iterator end() noexcept;

  static void Register(py::module_& m);

 private:
  AVCodecContext* ctx_{nullptr};
};
