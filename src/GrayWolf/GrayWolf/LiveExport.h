#pragma once

// One asset the live readers read (the game being read: CoDAssets::Game, its memory a zone served
// by VirtualGameSession), or a model or animation a reader built itself, written in the formats
// the settings ask for. ExportWriters guards these calls for the disk exporters; the file mode
// (PackageFileMode.h) writes its images and sounds through them too.

#include <functional>
#include <memory>
#include <string>

#include "CoDAssetType.h"

class WraithAnim;
class WraithModel;
namespace Materials { struct Sidecar; }   // MaterialDescription.h

enum class ExportGameResult
{
    Success,
    Placeholder,
    UnknownError
};

namespace LiveExport
{
    // The model as the game's reader reads it, nullptr when it does not
    std::unique_ptr<XModel_t> ReadModel(const CoDModel_t* Model);

    // A model into ExportPath. TakeImages, when given, receives each texture (it may rename it;
    // false drops it) instead of it being written to ImagesPath, and the model names them all in
    // ImageRelativePath, without material folders.
    using ImageTaker = std::function<bool(XImage_t& Image)>;
    ExportGameResult ExportModel(const CoDModel_t* Model, const std::string& ExportPath, const std::string& ImagesPath,
                                 const std::string& ImageRelativePath, const std::string& ImageExtension,
                                 const ImageTaker* TakeImages = nullptr);
    ExportGameResult ExportAnim(const CoDAnim_t* Anim, const std::string& ExportPath);
    ExportGameResult ExportSound(const CoDSound_t* Sound, const std::string& ExportPath, const std::string& SoundExtension);
    ExportGameResult ExportImage(const CoDImage_t* Image, const std::string& ExportPath, const std::string& ImageExtension);

    // An animation or a model a reader built itself (from files, not game memory), written exactly
    // as the live exports write theirs. A model's materials are described by its reader
    // (Materials::Sidecar, its .materials.json and the .cast slots).
    ExportGameResult ExportWraithAnim(WraithAnim& Anim, const std::string& ExportPath);
    ExportGameResult ExportWraithModel(std::unique_ptr<WraithModel>& Model, const std::string& ExportPath,
                                       const Materials::Sidecar* Sidecar);

    // Forgets the textures written so far, so the next one is written again (a new export run)
    void ForgetWrittenImages();
}
