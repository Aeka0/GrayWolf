#include "stdafx.h"

#include "IWDSupport.h"

#include "Strings.h"
#include "SettingsManager.h"
#include "FileSystems.h"

#include "CoDAssets.h"
#include "CoDIWITranslator.h"

#include <miniz/miniz.h>

bool IWDSupport::ParseIWD(const std::string& FilePath)
{
    mz_zip_archive ZipArchive;
    std::memset(&ZipArchive, 0, sizeof(ZipArchive));

    if (!mz_zip_reader_init_file(&ZipArchive, FilePath.c_str(), 0))
    {
        return false;
    }

    auto FileCount = (uint32_t)mz_zip_reader_get_num_files(&ZipArchive);

    for (uint32_t i = 0; i < FileCount; i++)
    {
        mz_zip_archive_file_stat FileInfo;
        if (mz_zip_reader_file_stat(&ZipArchive, i, &FileInfo))
        {
            auto EntryName = std::string(FileInfo.m_filename);

            if (EntryName.ends_with(".iwi"))
            {
                auto LoadedImage = new CoDImage_t();
                LoadedImage->AssetName = FileSystems::GetFileNameWithoutExtension(EntryName);
                LoadedImage->AssetPointer = 0;
                LoadedImage->Width = 0;
                LoadedImage->Height = 0;
                LoadedImage->AssetStatus = WraithAssetStatus::Loaded;
                LoadedImage->IsFileEntry = true;

                CoDAssets::GameAssets->LoadedAssets.push_back(LoadedImage);
            }
        }
    }

    mz_zip_reader_end(&ZipArchive);

    return true;
}

std::unique_ptr<XImageDDS> IWDSupport::ReadImageFile(const CoDImage_t* Image)
{
    uint32_t ResultSize = 0;
    // Hash the name for the id
    auto ImageData = CoDAssets::GamePackageCache->ExtractPackageObject(CoDAssets::GamePackageCache->HashPackageID(Image->AssetName), ResultSize);

    if (ImageData != nullptr)
    {
        auto Result = CoDIWITranslator::TranslateIWI(ImageData, ResultSize);

        if (Result != nullptr && (SettingsManager::GetSetting("patchnormals", "true") == "true"))
        {
            if (Image->AssetName.ends_with("_n") || Image->AssetName.ends_with("_nml") || Image->AssetName.ends_with("_n2") || Image->AssetName.ends_with("_n3"))
            {
                Result->ImagePatchType = ImagePatch::Normal_Bumpmap;
            }
        }

        return Result;
    }

    return nullptr;
}