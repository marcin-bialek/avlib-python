#pragma once

#include <optional>
#include <string>

#include "common.hh"
#include "decoder.hh"
#include "packet.hh"

class Demuxer {
 public:
  explicit Demuxer(std::string_view filename);
  Demuxer(const Demuxer& other) = delete;
  Demuxer(Demuxer&& other) noexcept;
  Demuxer& operator=(const Demuxer& other) = delete;
  Demuxer& operator=(Demuxer&& other) noexcept;
  ~Demuxer() noexcept;

  const AVStream* FindBestStream(AVMediaType type) const;
  bool Read(Packet& packet, const AVStream* stream = nullptr);
  std::optional<Packet> Read(const AVStream* stream = nullptr);

  AVFormatContext* operator*() const noexcept;
  AVFormatContext* operator->() const noexcept;

  static void Register(py::module_& m);

 private:
  AVFormatContext* ctx_{nullptr};
};
