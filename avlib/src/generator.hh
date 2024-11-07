#pragma once

#include <optional>
#include <random>
#include <tuple>

#include "converter.hh"
#include "decoder.hh"
#include "demuxer.hh"
#include "encoder.hh"

class Generator {
 public:
  using Batch = std::pair<std::optional<py::array_t<uint8_t>>,
                          std::optional<py::array_t<uint8_t>>>;

  explicit Generator(std::string_view filename, int width, int height,
                     int batch_size = 32);

  void Reset();
  Batch GenerateBatch();

  static void Register(py::module_& m);

 private:
  std::string filename_;
  Random<std::size_t> random_;
  CodecConfig config_;
  std::optional<Converter> file_converter_;
  std::optional<Converter> rgba_converter_;
  std::optional<Demuxer> file_demuxer_;
  std::optional<Decoder> file_decoder_;
  std::optional<Encoder> encoder_;
  std::optional<Decoder> decoder_;
  std::vector<Frame> x_frames_;
  std::vector<Frame> y_frames_;
  const AVStream* stream_;
  int64_t pts_;
  int batch_size_;
  int width_;
  int height_;

  bool GenerateGroup();
  std::optional<std::pair<Frame, Frame>> GeneratePair();
};
