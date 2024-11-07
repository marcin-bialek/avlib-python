#include "packet.hh"

Packet::Packet() : handle_{av_packet_alloc()} {
  av_packet_make_refcounted(handle_);
}

Packet::Packet(const Packet& other) : handle_{av_packet_clone(other.handle_)} {}

Packet::Packet(Packet&& other) noexcept : handle_{av_packet_alloc()} {
  av_packet_move_ref(handle_, other.handle_);
}

Packet& Packet::operator=(const Packet& other) {
  av_packet_unref(handle_);
  av_packet_ref(handle_, other.handle_);
  return *this;
}

Packet& Packet::operator=(Packet&& other) noexcept {
  av_packet_unref(handle_);
  av_packet_move_ref(handle_, other.handle_);
  return *this;
}

void Packet::Unref() const noexcept {
  av_packet_unref(handle_);
}

void Packet::MakeWritable() const noexcept {
  av_packet_make_writable(handle_);
}

Packet::~Packet() noexcept {
  av_packet_free(&handle_);
}

AVPacket* Packet::operator*() const noexcept {
  return handle_;
}

AVPacket* Packet::operator->() const noexcept {
  return handle_;
}

void Packet::Register(pybind11::module_& m) {
  auto c = py::class_<Packet>{m, "Packet", py::buffer_protocol{}};
  c.def(py::init<>());
  c.def("unref", &Packet::Unref);
  c.def("make_writable", &Packet::MakeWritable);
  c.def_property_readonly("pts", [](const Packet& p) { return p->pts; });
  c.def_property_readonly("flags", [](const Packet& p) { return p->flags; });
  c.def_buffer([](Packet& p) { return py::buffer_info{p->data, p->size}; });
}
