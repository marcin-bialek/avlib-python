#include "codec_config.hh"
#include "converter.hh"
#include "decoder.hh"
#include "demuxer.hh"
#include "encoder.hh"
#include "frame.hh"
#include "generator.hh"
#include "packet.hh"

PYBIND11_MODULE(avlib, m) {
  m.doc() = "ffmpeg bindings";

  py::enum_<AVMediaType>(m, "MediaType").value("VIDEO", AVMEDIA_TYPE_VIDEO);

  py::enum_<AVPixelFormat>(m, "PixelFormat")
      .value("YUV420P", AV_PIX_FMT_YUV420P)
      .value("NV12", AV_PIX_FMT_NV12)
      .value("ARGB", AV_PIX_FMT_ARGB)
      .value("RGBA", AV_PIX_FMT_RGBA)
      .value("ABGR", AV_PIX_FMT_ABGR)
      .value("BGRA", AV_PIX_FMT_BGRA)
      .value("RGB32", AV_PIX_FMT_RGB32)
      .value("BGR32", AV_PIX_FMT_BGR32);

  py::enum_<AVPictureType>(m, "PictureType")
      .value("NONE", AV_PICTURE_TYPE_NONE)
      .value("I", AV_PICTURE_TYPE_I)
      .value("P", AV_PICTURE_TYPE_P)
      .value("B", AV_PICTURE_TYPE_B)
      .value("S", AV_PICTURE_TYPE_S)
      .value("SI", AV_PICTURE_TYPE_SI)
      .value("SP", AV_PICTURE_TYPE_SP)
      .value("BI", AV_PICTURE_TYPE_BI);

  py::class_<AVCodec>(m, "Codec")
      .def_readonly("name", &AVCodec::name)
      .def_readonly("long_name", &AVCodec::long_name)
      .def("__repr__", [](const AVCodec& c) {
        return Format("<avlib.AVCodec ", c.name, ">");
      });

  py::class_<AVStream>(m, "Stream")
      .def_readonly("index", &AVStream::index)
      .def_readonly("time_base", &AVStream::time_base)
      .def_readonly("avg_frame_rate", &AVStream::avg_frame_rate)
      .def("__repr__", [](const AVStream& s) {
        return Format("<avlib.AVStream ", s.index, ">");
      });

  py::class_<AVRational>(m, "Rational")
      .def(py::init([](int num, int den) {
        return AVRational{num, den};
      }))
      .def_readwrite("num", &AVRational::num)
      .def_readwrite("den", &AVRational::den)
      .def("__repr__", [](const AVRational& r) {
        return Format("<avlib.Rational ", r.num, "/", r.den, ">");
      });

  Packet::Register(m);
  Frame::Register(m);
  Demuxer::Register(m);
  CodecConfig::Register(m);
  Encoder::Register(m);
  Decoder::Register(m);
  Converter::Register(m);
  Generator::Register(m);
}
