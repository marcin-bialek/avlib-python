#include "generator.hh"

#include <iostream>

Generator::Generator(std::string_view filename, int width, int height,
                     int batch_size)
    : filename_{filename},
      random_{5, 50},
      batch_size_{batch_size},
      width_{width},
      height_{height} {
  file_converter_.emplace(AV_PIX_FMT_YUV420P, width, height);
  rgba_converter_.emplace(AV_PIX_FMT_RGBA, width, height);
  config_.bitrate = 5'000'000;
  config_.width = width;
  config_.height = height;
  config_.format = AV_PIX_FMT_YUV420P;
  config_.framerate = AVRational{1, 1};
  config_.timebase = AVRational{1, 1};
  config_.gop_size = 1'000'000;
  config_.keyint_min = 1'000'000;
  config_.max_b_frames = 0;
  config_.refs = 1;
  config_.SetFlag(CodecConfig::Flag::LOW_DELAY);
  Reset();
}

void Generator::Reset() {
  file_demuxer_.emplace(filename_);
  stream_ = file_demuxer_->FindBestStream(AVMEDIA_TYPE_VIDEO);
  file_decoder_.emplace("h264_cuvid", stream_);
  encoder_.emplace("h264_nvenc", config_);
  encoder_->SetOption("zerolatency", "1");
  encoder_->SetOption("delay", "0");
  decoder_.emplace("h264_cuvid", config_);
  x_frames_.clear();
  y_frames_.clear();
  pts_ = 0;
}

Generator::Batch Generator::GenerateBatch() {
  auto stride = width_ * 4;
  auto x_buffer = std::make_unique<uint8_t[]>(batch_size_ * height_ * stride);
  auto y_buffer = std::make_unique<uint8_t[]>(batch_size_ * height_ * stride);
  std::size_t p = 0;
  for (int i = 0; i < batch_size_; ++i) {
    auto pair = GeneratePair();
    if (!pair) {
      return {std::nullopt, std::nullopt};
    }
    auto [x, y] = *pair;
    std::size_t q = 0;
    std::size_t r = 0;
    for (int j = 0; j < height_; ++j) {
      std::memcpy(&x_buffer[p], &x->data[0][q], stride);
      std::memcpy(&y_buffer[p], &y->data[0][r], stride);
      p += stride;
      q += x->linesize[0];
      r += y->linesize[0];
    }
  }

  auto x_ptr = x_buffer.release();
  py::capsule x_capsule{
      x_ptr, [](void* data) { delete[] static_cast<uint8_t*>(data); }};
  py::array_t<uint8_t> x_array{{batch_size_, height_, width_, 4},
                               {height_ * width_ * 4, width_ * 4, 4, 1},
                               x_ptr,
                               x_capsule};

  auto y_ptr = y_buffer.release();
  py::capsule y_capsule{
      y_ptr, [](void* data) { delete[] static_cast<uint8_t*>(data); }};
  py::array_t<uint8_t> y_array{{batch_size_, height_, width_, 4},
                               {height_ * width_ * 4, width_ * 4, 4, 1},
                               y_ptr,
                               y_capsule};

  return {x_array, y_array};
}

bool Generator::GenerateGroup() {
  std::vector<Packet> packets{};
  auto n = random_();
  auto p = std::max<std::size_t>(1, n / 5);
  bool has_key = false;
  bool first = true;
  for (bool more = true; more;) {
    auto packet = encoder_->Receive();
    if (packet.has_value()) {
      if (has_key || (*packet)->flags & AV_PKT_FLAG_KEY) {
        packets.push_back(std::move(*packet));
        has_key = true;
      }
    } else if (n <= packets.size()) {
      more = false;
    } else {
      auto frame = file_decoder_->Receive();
      if (frame.has_value()) {
        auto f = file_converter_->Convert(*frame);
        f->pts = pts_++;
        if (first) {
          first = false;
          f->pict_type = AV_PICTURE_TYPE_I;
          encoder_->SetOption("forced-idr", "1");
        } else {
          f->pict_type = AV_PICTURE_TYPE_P;
        }
        encoder_->Send(f);
        y_frames_.push_back(std::move(f));
      } else {
        auto file_packet = file_demuxer_->Read(stream_);
        if (file_packet.has_value()) {
          file_decoder_->Send(*file_packet);
        } else {
          return false;
        }
      }
    }
  }
  n = packets.size();
  for (std::size_t i = 0; i < n; ++i) {
    if ((i < (n - p - 2)) || ((n - 2) <= i)) {
      decoder_->Decode(x_frames_, packets[i]);
    }
  }
  return true;
}

std::optional<std::pair<Frame, Frame>> Generator::GeneratePair() {
  std::optional<std::size_t> x_i;
  std::optional<std::size_t> y_i;
  while (!x_i) {
    if (!GenerateGroup()) {
      return std::nullopt;
    }
    for (std::size_t i = 1; i < x_frames_.size(); ++i) {
      if (x_frames_[i - 1]->pts + 1 < x_frames_[i]->pts) {
        x_i = i;
        break;
      }
    }
  }
  for (std::size_t i = 0; i < y_frames_.size(); ++i) {
    if (y_frames_[i]->pts == x_frames_[*x_i]->pts) {
      y_i = i;
      break;
    }
  }
  if (!x_i || !y_i) {
    Throw("could not find proper y frame");
  }
  auto x = rgba_converter_->Convert(x_frames_[*x_i]);
  auto y = rgba_converter_->Convert(y_frames_[*y_i]);
  x_frames_.erase(x_frames_.begin(), x_frames_.begin() + *x_i + 1);
  y_frames_.erase(y_frames_.begin(), y_frames_.begin() + *y_i + 1);
  return std::pair{std::move(x), std::move(y)};
}

void Generator::Register(py::module_& m) {
  auto c = py::class_<Generator>(m, "Generator");

  c.def(py::init([](std::string_view filename, std::pair<int, int> size,
                    int batch_size) {
          return Generator{filename, size.first, size.second, batch_size};
        }),
        py::arg("filename"), py::arg("frame_size") = std::pair{1280, 720},
        py::arg("batch_size") = 32);

  c.def("reset", &Generator::Reset);
  c.def("generate_batch", &Generator::GenerateBatch);
}
