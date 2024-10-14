#include "Serialization.h"

#include "REX/W32/OLE32.h"
#include "REX/W32/SHELL32.h"

auto Serialization::GetSingleton() -> Serialization*
{
    static Serialization singleton;

    return std::addressof(singleton);
}

bool Serialization::GetSaveFileDirectory()
{
    const auto ini_settings = RE::INISettingCollection::GetSingleton();

    if (!ini_settings) {
        return false;
    }

    wchar_t* buffer = nullptr;
    const auto result = REX::W32::SHGetKnownFolderPath(REX::W32::FOLDERID_Documents, REX::W32::KF_FLAG_DEFAULT, nullptr, std::addressof(buffer));

    std::unique_ptr<wchar_t[], decltype(&REX::W32::CoTaskMemFree)> path_ptr(buffer, REX::W32::CoTaskMemFree);

    if (!path_ptr || result != 0) {
        return false;
    }

    std::filesystem::path path = path_ptr.get();

    path /= "My Games"sv;
    path /= std::filesystem::exists("steam_api64.dll") ? "Skyrim Special Edition" : "Skyrim Special Edition GOG";
    path /= ini_settings->GetSetting("sLocalSavePath:General")->GetString();

    save_directory = path;

    return true;
}

auto Serialization::GetID(const std::string_view& a_path) -> std::uint32_t
{
    return !a_path.empty() && save_files.contains(a_path.data()) ? save_files[a_path.data()] : 0x0;
}

bool Serialization::Find(const std::string_view& a_path)
{
    for (const auto& entry : std::filesystem::directory_iterator(save_directory)) {
        const auto filename = entry.path().filename();

        if (filename == a_path) {
            return true;
        }
    }

    return false;
}

void Serialization::Save(const std::string_view& a_path)
{
    if (a_path.empty()) { return; }

    const auto save_manager = RE::BGSSaveLoadManager::GetSingleton();

    const auto character_id = save_manager ? save_manager->currentCharacterID : 0x0;

    if (character_id == 0x0) { return; }

    save_files.insert_or_assign(std::string(a_path) + ".ess", character_id);

    if (auto error = glz::write_file_json(save_files, JSON_PATH, std::string{})) { logger::error("Serialization::Save :: Error code: {} | Failed to serialize ID & File pair: '{:X}' | '{}'", std::to_underlying(error.ec), character_id, a_path.data()); };
}

void Serialization::Load()
{
    if (auto error = glz::read_file_json(save_files, JSON_PATH, std::string{})) { logger::error("Serialization::Load :: Error code: '{}' | Failed to deserialize data!", std::to_underlying(error.ec)); };
}

void Serialization::Clean()
{
    if (!std::filesystem::exists(save_directory)) { logger::error("Serialization::Clean :: Savefile directory couldn't be found! Canceling cleanup..."); return; }

    for (const auto& [file, id] : save_files) {
        if (!Find(file)) {
            logger::warn("Serialization::Clean :: Could not find ID & File pair: '{:X}' | '{}'! Erasing entry...", id, file);

            save_files.erase(file);
        }
    }
}