#pragma once

#include <RmlUi/Core/FileInterface.h>

namespace alpha::ui {

// RmlUi reads documents, style sheets and templates through the engine's file
// layer (platform/vfs): the whole file is read at Open and closed at once, so
// RmlUi never holds a pack file open and reads from wherever the pack lives.
class PackFiles final : public Rml::FileInterface {
 public:
  Rml::FileHandle Open(const Rml::String& path) override;
  void Close(Rml::FileHandle file) override;
  size_t Read(void* buffer, size_t size, Rml::FileHandle file) override;
  bool Seek(Rml::FileHandle file, long offset, int origin) override;
  size_t Tell(Rml::FileHandle file) override;
  size_t Length(Rml::FileHandle file) override;
};

}  // namespace alpha::ui
