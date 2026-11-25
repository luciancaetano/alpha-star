#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string>

namespace alpha::platform {

// Pulls audio out of a file a little at a time, as interleaved floats at the
// file's own rate and channel count.
class Decoder {
 public:
  virtual ~Decoder() = default;
  virtual int rate() const = 0;
  virtual int channels() const = 0;
  // Fills up to interleaved.size() / channels() frames; returns how many (0 = end).
  virtual std::size_t read(std::span<float> interleaved) = 0;
  virtual void rewind() = 0;
};

// By extension: .ogg (Vorbis), .mp3, .wav. Throws core::Error naming the
// file and what is supported.
std::unique_ptr<Decoder> open_decoder(const std::string& path);

}  // namespace alpha::platform
