#pragma once

#include "common.hh"

class Packet {
 public:
  explicit Packet();
  Packet(const Packet& other);
  Packet(Packet&& other) noexcept;
  Packet& operator=(const Packet& other);
  Packet& operator=(Packet&& other) noexcept;
  ~Packet() noexcept;

  void Unref() const noexcept;
  void MakeWritable() const noexcept;

  AVPacket* operator*() const noexcept;
  AVPacket* operator->() const noexcept;

  static void Register(py::module_& m);

 private:
  AVPacket* handle_{nullptr};
};
