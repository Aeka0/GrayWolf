// Games/T8/CliCommands.Bo4Strings.cpp - Black Ops 4's string commands: bo4-export-localize-disk,
// bo4-export-rawstrings-disk, bo4-decrypt.
// NOMINMAX before <windows.h>: std::max/std::min below would be mangled by its macros.
#define NOMINMAX

#include <stdio.h>
#include <cmath>
#include <memory>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <vector>
#include <iterator>
#include <string>
#include <format>

#include "CliOptions.h"
#include "CliCommands.h"
#include "CliSupport.h"

#include "WraithX.h"
#include "CoDAssets.h"
#include "FastFile.h"

#include "GameBlackOps4.h"
#include "SABSupport.h"
#include "CascFileSystem.h"
#include "CoDFileHandle.h"
#include "DBGameFiles.h"

int RunBo4ExportLocalizeDisk(const CliOptions& Options)
{
    std::ifstream In(Options.Bo4ExportLocalizeDiskIn, std::ios::binary);
    if (!In)
    {
        CliReport(Options, "result=error\nreason=could not open input file\n");
        return 5;
    }
    std::vector<uint8_t> Zone((std::istreambuf_iterator<char>(In)), std::istreambuf_iterator<char>());

    std::string Report;
    bool Ok = false;
    try
    {
        Ok = GameBlackOps4::ExportDiskLocalize(Zone, Options.Bo4ExportLocalizeDiskOut, Report);
    }
    catch (const std::exception& Ex)
    {
        Report += std::format("dump_error={}\n", Ex.what());
    }
    catch (...)
    {
        Report += "dump_error=unhandled exception\n";
    }

    CliReport(Options, std::format("result={}\n{}", Ok ? "ok" : "error", Report.c_str()));
    return Ok ? 0 : 3;
}

int RunBo4ExportRawStringsDisk(const CliOptions& Options)
{
    std::ifstream In(Options.Bo4ExportRawStringsDiskIn, std::ios::binary);
    if (!In)
    {
        CliReport(Options, "result=error\nreason=could not open input file\n");
        return 5;
    }
    std::vector<uint8_t> Zone((std::istreambuf_iterator<char>(In)), std::istreambuf_iterator<char>());

    std::string Report;
    bool Ok = false;
    try
    {
        Ok = GameBlackOps4::ExportDiskRawStrings(Zone, Options.Bo4ExportRawStringsDiskOut, Report);
    }
    catch (const std::exception& Ex)
    {
        Report += std::format("dump_error={}\n", Ex.what());
    }
    catch (...)
    {
        Report += "dump_error=unhandled exception\n";
    }

    CliReport(Options, std::format("result={}\n{}", Ok ? "ok" : "error", Report.c_str()));
    return Ok ? 0 : 3;
}


// bo4-decrypt --hex|--disk-hex <bytes>: one string, hex-encoded as
// [EncryptionID][StringSize][ciphertext][NUL], through BO4's string cipher.
int RunBo4Decrypt(const CliOptions& Options)
{
    if (!Options.Bo4DecryptHex.empty())
    {
        std::string HexReport;
        bool HexOk = false;
        try
        {
            std::vector<uint8_t> Bytes;
            for (size_t i = 0; i + 1 < Options.Bo4DecryptHex.size(); i += 2)
            {
                Bytes.push_back((uint8_t)std::strtoul(Options.Bo4DecryptHex.substr(i, 2).c_str(), nullptr, 16));
            }
            if (Bytes.size() < 3)
            {
                HexReport += "decrypt_error=need at least [EncryptionID][StringSize][data...]\n";
            }
            else
            {
                const uint8_t EncryptionID = Bytes[0];
                const uint8_t StringSize = Bytes[1];
                auto Buf = Bytes.data() + 2;
                auto Text = GameBlackOps4::DecryptString(Buf, StringSize, EncryptionID, 0);
                HexReport += std::format(
                    "encryption_id={} (case {})\nstring_size={}\ntext={}\n",
                    EncryptionID, (uint8_t)(EncryptionID + 128), StringSize, Text.c_str());
                HexOk = true;
            }
        }
        catch (const std::exception& Ex)
        {
            HexReport += std::format("decrypt_error={}\n", Ex.what());
        }
        catch (...)
        {
            HexReport += "decrypt_error=unhandled exception\n";
        }

        CliReport(Options, std::format(
            "result={}\nsource=offline\n{}",
            HexOk ? "ok" : "error",
            HexReport.c_str()));

        WraithX::ShutdownAPI();
        return HexOk ? 0 : 3;
    }

    // The on-disk cipher (GameBlackOps4::DecryptDiskString).
    if (!Options.Bo4DecryptDiskHex.empty())
    {
        std::string HexReport;
        bool HexOk = false;
        try
        {
            std::vector<uint8_t> Bytes;
            for (size_t i = 0; i + 1 < Options.Bo4DecryptDiskHex.size(); i += 2)
            {
                Bytes.push_back((uint8_t)std::strtoul(Options.Bo4DecryptDiskHex.substr(i, 2).c_str(), nullptr, 16));
            }
            if (Bytes.size() < 3)
            {
                HexReport += "decrypt_error=need at least [EncryptionID][StringSize][data...]\n";
            }
            else
            {
                const uint8_t EncryptionID = Bytes[0];
                const uint8_t StringSize = Bytes[1];
                std::string Text;
                if (GameBlackOps4::DecryptDiskString(Bytes.data() + 2, (uint8_t)(StringSize - 1), EncryptionID, Text))
                {
                    HexReport += std::format("encryption_id={}\nstring_size={}\ntext={}\n",
                        EncryptionID, StringSize, Text.c_str());
                    HexOk = true;
                }
                else
                {
                    HexReport += std::format("decrypt_error=unknown encryption_id {}\n", EncryptionID);
                }
            }
        }
        catch (const std::exception& Ex)
        {
            HexReport += std::format("decrypt_error={}\n", Ex.what());
        }
        catch (...)
        {
            HexReport += "decrypt_error=unhandled exception\n";
        }

        CliReport(Options, std::format(
            "result={}\nsource=offline\n{}",
            HexOk ? "ok" : "error",
            HexReport.c_str()));

        WraithX::ShutdownAPI();
        return HexOk ? 0 : 3;
    }

    CliReport(Options, "result=error\nreason=bo4-decrypt needs --hex or --disk-hex\n");
    return 2;
}
