#include "codec_config.hh"

void CodecConfig::SetFlag(Flag flag) {
  flags = flags.value_or(0) | static_cast<int>(flag);
}

void CodecConfig::Apply(AVCodecContext* ctx) const {
  ctx->pix_fmt = format.value_or(ctx->pix_fmt);
  ctx->framerate = framerate.value_or(ctx->framerate);
  ctx->time_base = timebase.value_or(ctx->time_base);
  ctx->width = width.value_or(ctx->width);
  ctx->height = height.value_or(ctx->height);
  ctx->bit_rate = bitrate.value_or(ctx->bit_rate);
  ctx->gop_size = gop_size.value_or(ctx->gop_size);
  ctx->keyint_min = keyint_min.value_or(ctx->keyint_min);
  ctx->max_b_frames = max_b_frames.value_or(ctx->max_b_frames);
  ctx->refs = refs.value_or(ctx->refs);
  ctx->flags = flags.value_or(ctx->flags);
}

void CodecConfig::Register(py::module_& m) {
  auto c = py::class_<CodecConfig>(m, "CodecConfig");

  py::enum_<Flag>(c, "Flag").value("LOW_DELAY", Flag::LOW_DELAY);

  c.def(py::init([] { return CodecConfig{}; }));
  c.def("set_flag", &CodecConfig::SetFlag);

  c.def_readwrite("format", &CodecConfig::format);
  c.def_readwrite("framerate", &CodecConfig::framerate);
  c.def_readwrite("timebase", &CodecConfig::timebase);
  c.def_readwrite("width", &CodecConfig::width);
  c.def_readwrite("height", &CodecConfig::height);
  c.def_readwrite("bitrate", &CodecConfig::bitrate);
  c.def_readwrite("gop_size", &CodecConfig::gop_size);
  c.def_readwrite("keyint_min", &CodecConfig::keyint_min);
  c.def_readwrite("max_b_frames", &CodecConfig::max_b_frames);
  c.def_readwrite("refs", &CodecConfig::refs);
  c.def_readonly("flags", &CodecConfig::flags);
}
