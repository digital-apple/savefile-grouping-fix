#include "Hooks.h"

#include "Serialization.h"

namespace
{
    struct SaveHeader
    {
        std::uint32_t saveNumber;
        std::string   playerName;
        std::string   location;
    };

    std::optional<SaveHeader> ReadSaveHeader(const char* a_fileName)
    {
        const auto utility = RE::BSWin32SaveDataSystemUtility::GetSingleton();

        if (!utility) { return std::nullopt; }

        char path[0x104]{};

        if (utility->PrepareFileSavePath(a_fileName, path, false, false) != 0) { return std::nullopt; }

        std::ifstream file(path, std::ios::binary);

        if (!file) { logger::warn("ReadSaveHeader :: Could not open '{}'", path); return std::nullopt; }

        const auto read_u32 = [&]() { std::uint32_t value{}; file.read(reinterpret_cast<char*>(&value), sizeof(value)); return value; };
        const auto read_wstring = [&]() {
            std::uint16_t length{};
            file.read(reinterpret_cast<char*>(&length), sizeof(length));
            std::string value(length, '\0');
            file.read(value.data(), length);
            return value;
        };

        char magic[13]{};
        file.read(magic, sizeof(magic));

        if (std::string_view{ magic, sizeof(magic) } != "TESV_SAVEGAME"sv) { return std::nullopt; }

        read_u32();  // header size
        read_u32();  // version

        SaveHeader header{};
        header.saveNumber = read_u32();
        header.playerName = read_wstring();
        read_u32();  // player level
        header.location = read_wstring();

        if (!file) { return std::nullopt; }

        return header;
    }

    /*
        * The game only fills an entry's save number, character name and location while parsing an engine-formatted file name.
          For custom file names that parse fails and those fields stay at their defaults (save 1, no name, no location).
        * Fill them from the savefile header instead, so restored entries show their own label in the Journal Menu.
    */
    void RestoreEntry(RE::BSWin32SaveDataSystemUtility::Entry* a_entry, const char* a_path, std::string_view a_hook)
    {
        const auto character_id = Serialization::GetSingleton()->GetID(a_path);

        if (character_id == 0x0) { return; }

        const auto parsed_by_game = a_entry->fileNameValid;

        a_entry->characterID = character_id;
        a_entry->fileNameValid = true;

        if (!parsed_by_game) {
            if (const auto header = ReadSaveHeader(a_path)) {
                a_entry->saveNumber = header->saveNumber;
                a_entry->characterName = header->playerName.c_str();
                a_entry->locationName = header->location.c_str();
            }
        }

        logger::info("{} :: Restored character ID: '{:X}' for file: '{}'", a_hook, character_id, a_path);
    }
}

namespace Addresses
{
    /*
        * Normally, using a coc or related console command to start a new save file does not reset the character ID that is currently loaded in memory,
          which could lead to problems since that newly created character now shares the same ID as the one that was loaded prior to it.
        * To deal with that, we force the game to generate a new ID for characters created through similar means.
    */

    struct CenterOnCell
    {
        static bool thunk(RE::PlayerCharacter* a_player, const char* a_cellName, RE::TESObjectCELL* a_cell)
        {
            const auto ui_manager = RE::UI::GetSingleton();

            if (ui_manager && ui_manager->IsMenuOpen(RE::MainMenu::MENU_NAME)) {
                RE::BGSSaveLoadManager::GetSingleton()->GenerateCharacterID();
            }

            return func(a_player, a_cellName, a_cell);
        }
        static inline REL::Relocation<decltype(thunk)> func;
    };

    struct CenterOnExterior
    {
        static bool thunk(RE::PlayerCharacter* a_player, const char* a_cellName, RE::TESObjectCELL* a_cell)
        {
            const auto ui_manager = RE::UI::GetSingleton();

            if (ui_manager && ui_manager->IsMenuOpen(RE::MainMenu::MENU_NAME)) {
                RE::BGSSaveLoadManager::GetSingleton()->GenerateCharacterID();
            }

            return func(a_player, a_cellName, a_cell);
        }
        static inline REL::Relocation<decltype(thunk)> func;
    };

    struct CenterOnWorld
    {
        static bool thunk(RE::PlayerCharacter* a_player, const char* a_cellName, RE::TESObjectCELL* a_cell)
        {
            const auto ui_manager = RE::UI::GetSingleton();

            if (ui_manager && ui_manager->IsMenuOpen(RE::MainMenu::MENU_NAME)) {
                RE::BGSSaveLoadManager::GetSingleton()->GenerateCharacterID();
            }

            return func(a_player, a_cellName, a_cell);
        }
        static inline REL::Relocation<decltype(thunk)> func;
    };

    struct SaveFileReaderA
    {
        static void thunk(RE::BSWin32SaveDataSystemUtility::Entry* a_entry, const char* a_path)
        {
            func(a_entry, a_path);

            RestoreEntry(a_entry, a_path, "Addresses::SaveFileReaderA"sv);
        }
        static inline REL::Relocation<decltype(thunk)> func;
    };

    struct SaveFileReaderB
    {
        static void thunk(RE::BSWin32SaveDataSystemUtility::Entry* a_entry, const char* a_path)
        {
            func(a_entry, a_path);

            RestoreEntry(a_entry, a_path, "Addresses::SaveFileReaderB"sv);
        }
        static inline REL::Relocation<decltype(thunk)> func;
    };

    struct SaveGameHandler
    {
        static void thunk(RE::BGSSaveLoadManager* a_manager, std::uint64_t a_2, const char* a_path, bool a_temporary, std::uint32_t a_type)
        {
            func(a_manager, a_2, a_path, a_temporary, a_type);

            /*
                * 0x2 is used by the game to identify the current save type as a "full save"
                * We should ignore other save types because it is unlikely for a savefile created through external methods to have a different type.
            */

            if (a_type != 0x2) { return; };

            Serialization::GetSingleton()->Save(a_path);
        }
        static inline REL::Relocation<decltype(thunk)> func;
    };

    void Hook()
    {
        REL::Relocation<std::uintptr_t> center_on_cell{ RELOCATION_ID(22398, 22873), 0x5B };
        stl::write_thunk_call<CenterOnCell>(center_on_cell.address());

        logger::info("Addresses :: Hooked CenterOnCell");

        REL::Relocation<std::uintptr_t> center_on_exterior{ RELOCATION_ID(22400, 22875), REL::Relocate(0x118, 0x109) };
        stl::write_thunk_call<CenterOnExterior>(center_on_exterior.address());

        logger::info("Addresses :: Hooked CenterOnExterior");

        REL::Relocation<std::uintptr_t> center_on_world{ RELOCATION_ID(22401, 22876), REL::Relocate(0x108, 0x104) };
        stl::write_thunk_call<CenterOnWorld>(center_on_world.address());

        logger::info("Addresses :: Hooked CenterOnWorld");

        REL::Relocation save_reader{ RELOCATION_ID(34872, REL::Module::get().version().patch() >= 1130 ? 442580 : 35783), REL::Relocate(0xB4, REL::Module::get().version().patch() >= 1130 ? 0x1B5 : 0x179) };
        stl::write_thunk_call<SaveFileReaderA>(save_reader.address());

        logger::info("Addresses :: Hooked SavefileReaderA");

        stl::write_vfunc<RE::BSWin32SaveDataSystemUtility::Entry, 0x0, SaveFileReaderB>();

        logger::info("Addresses :: Hooked SavefileReaderB");

        REL::Relocation save_game{ RELOCATION_ID(34818, 35727), REL::Relocate(0x112, 0x1CE) };
        stl::write_thunk_call<SaveGameHandler>(save_game.address());

        logger::info("Addresses :: Hooked SaveGameHandler");
    }
}
