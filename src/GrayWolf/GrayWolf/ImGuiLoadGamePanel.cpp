// The window's lifecycle, the install scan and the Setup section; the export itself is in
// ImGuiLoadGamePanel.Disk.cpp.

#include "stdafx.h"

#include "ImGuiLoadGamePanel.h"
#include "ImGuiDialogs.h"
#include "LauncherIcons.h"
#include "DiskGame.h"
#include "SettingsManager.h"

#include "imgui.h"
#include "imgui_internal.h"   // ImGuiComboFlags_CustomPreview

LoadGamePanel::LoadGamePanel()
{
    // Last session's choices first; they are what the user is most likely to want again.
    SetBuf(GameBuf, sizeof(GameBuf), SettingsManager::GetSetting("game_folder", ""));
    SetBuf(DiskOutBuf, sizeof(DiskOutBuf), AbsoluteFolder(SettingsManager::GetSetting("disk_out", "exported_files")));
    const std::string Kinds = "," + SettingsManager::GetSetting("disk_kinds", "image") + ",";
    for (const DiskGame::KindInfo& K : DiskGame::Kinds)
        DiskKindFlags.push_back(Kinds.find(std::string(",") + K.Name + ",") != std::string::npos);
    SetBuf(DiskNameFilterBuf, sizeof(DiskNameFilterBuf), SettingsManager::GetSetting("disk_name_filter", ""));
    SetBuf(DiskZoneFilterBuf, sizeof(DiskZoneFilterBuf), SettingsManager::GetSetting("disk_zone_filter", ""));
    Status = "no game loaded";
}

LoadGamePanel::~LoadGamePanel()
{
    if (Worker.joinable()) Worker.join();
}

void LoadGamePanel::Persist()
{
    SettingsManager::SetSetting("game_folder", GameBuf);
    SettingsManager::SetSetting("disk_out", DiskOutBuf);
    std::string Kinds;
    for (size_t i = 0; i < DiskKindFlags.size(); i++)
        if (DiskKindFlags[i]) Kinds += (Kinds.empty() ? "" : ",") + std::string(DiskGame::Kinds[i].Name);
    SettingsManager::SetSetting("disk_kinds", Kinds);
    SettingsManager::SetSetting("disk_name_filter", DiskNameFilterBuf);
    SettingsManager::SetSetting("disk_zone_filter", DiskZoneFilterBuf);
    // the ticked zones, per game; not before the list was read, which would clear a saved one
    if (Disk != nullptr && (DiskZonesFor + "|").rfind(std::string(Disk->Id()) + "|" + GameBuf + "|", 0) == 0)
    {
        std::string Zones;
        for (size_t i = 0; i < DiskZoneNames.size(); i++)
            if (DiskZoneFlags[i]) Zones += (Zones.empty() ? "" : ",") + DiskZoneNames[i];
        SettingsManager::SetSetting(std::string("disk_zones_") + Disk->Id(), Zones);
    }
    // SetSetting already flushes the whole settings file on every call.
}

void LoadGamePanel::DoScan()
{
    auto Found = GameInstalls::Detect();

    std::lock_guard<std::mutex> Lock(Mutex);
    Installs = std::move(Found);
    InstallIndex = -1;
    ScannedOnce = true;
    Status = Installs.empty()
        ? "no game detected - set the folder manually"
        : (std::to_string(Installs.size()) + " game(s) detected");
}

void LoadGamePanel::ApplyInstall(const GameInstalls::Install& I)
{
    SetBuf(GameBuf, sizeof(GameBuf), I.Path);
    InstallDiskId = I.DiskId;
    InstallPath = I.Path;
    DiskChosenFor.clear();
}

void LoadGamePanel::StartWork(const char* Label, void (LoadGamePanel::*Fn)())
{
    if (Busy.load()) return;
    if (Worker.joinable()) Worker.join();

    BusyLabel = Label;
    Busy.store(true);
    Done.store(false);
    Worker = std::thread([this, Fn] { (this->*Fn)(); Busy.store(false); Done.store(true); });
}

void LoadGamePanel::JoinIfDone()
{
    if (Done.load() && !Busy.load() && Worker.joinable())
    {
        Worker.join();
        Done.store(false);
    }
}

void LoadGamePanel::Draw(ID3D11Device* Dev)
{
    JoinIfDone();
    std::string Picked;
    if (UiAsync::Picked("loadgame.folder", Picked)) { SetBuf(GameBuf, sizeof(GameBuf), Picked); Persist(); }
    if (UiAsync::Picked("loadgame.output", Picked)) { SetBuf(DiskOutBuf, sizeof(DiskOutBuf), Picked); Persist(); }

    // One automatic scan on the first frame, on the worker so the window still paints: the sweep
    // takes about two seconds.
    if (!ScannedOnce && !AutoScanStarted)
    {
        AutoScanStarted = true;
        StartWork("scan", &LoadGamePanel::DoScan);
    }

    ImGui::Begin("Load Game");

    const bool Busyish = Busy.load();
    ImGui::BeginDisabled(Busyish);

    if (ImGui::CollapsingHeader("Setup", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextUnformatted("Detected installs");
        ImGui::SetNextItemWidth(-90);
        {
            std::lock_guard<std::mutex> Lock(Mutex);
            // where a game was installed from: its launcher's own icon (LauncherIcons), then the label
            const float Line = ImGui::GetTextLineHeight();
            const auto Row = [&](const GameInstalls::Install& I) {
                if (ID3D11ShaderResourceView* Icon = LauncherIcons::For(Dev, I.Source))
                    ImGui::Image((ImTextureID)(intptr_t)Icon, ImVec2(Line, Line));
                else
                    ImGui::Dummy(ImVec2(Line, Line));
                ImGui::SameLine(0.0f, Line * 0.5f);
                ImGui::TextUnformatted(I.Label.c_str());
            };
            const bool Picked = InstallIndex >= 0 && InstallIndex < (int)Installs.size();
            const char* InstallPreview = !ScannedOnce ? "(not scanned yet)"
                : Installs.empty() ? "(none found)"
                : Picked ? "" : "select...";

            if (ImGui::BeginCombo("##installs", InstallPreview, Picked ? ImGuiComboFlags_CustomPreview : 0))
            {
                for (int i = 0; i < (int)Installs.size(); i++)
                {
                    const auto& I = Installs[i];
                    // one ID per row, whatever the label: two installs of one game share a title
                    ImGui::PushID(i);
                    const bool Clicked = ImGui::Selectable("##install", InstallIndex == i, ImGuiSelectableFlags_AllowOverlap, ImVec2(0, Line));
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s  ·  %s", I.Source.c_str(), I.Path.c_str());
                    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());
                    Row(I);
                    if (Clicked)
                    {
                        InstallIndex = i;
                        ApplyInstall(I);
                        Persist();
                    }
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            if (Picked && ImGui::BeginComboPreview())
            {
                Row(Installs[InstallIndex]);
                ImGui::EndComboPreview();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Rescan")) StartWork("scan", &LoadGamePanel::DoScan);

        ImGui::TextUnformatted("Game folder");
        ImGui::SetNextItemWidth(-90);
        ImGui::InputText("##game", GameBuf, sizeof(GameBuf));
        // saved, and the folder's game looked for, once the field is left: not on every key
        GameEditing = ImGui::IsItemActive();
        if (ImGui::IsItemDeactivatedAfterEdit()) Persist();
        ImGui::SameLine();
        if (ImGui::Button("Browse##game")) UiAsync::PickFolder("loadgame.folder", "Select the game install folder", GameBuf);
        DrawReadWith();
    }
    else if (DiskChosenFor != GameBuf) ChooseDiskGame();

    if (Disk != nullptr)
    {
        DrawDiskSection();
        ImGui::EndDisabled();  // Busyish
        DrawDiskProgress();
    }
    else
    {
        if (Detecting)
            ImGui::TextDisabled("looking for the game's files...");
        else if (GameBuf[0] != 0)
            ImGui::TextWrapped("No disk reader recognises this folder: pick one under \"Read with\".");
        ImGui::EndDisabled();  // Busyish
        std::lock_guard<std::mutex> Lock(Mutex);
        if (!Status.empty()) ImGui::TextWrapped("%s", Status.c_str());
    }

    if (ImGui::CollapsingHeader("Log"))
    {
        std::lock_guard<std::mutex> Lock(Mutex);
        ImGui::BeginChild("log", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (const auto& Line : Log)
            ImGui::TextUnformatted(Line.c_str());
        if (Busyish) ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();
    }

    ImGui::End();
}
