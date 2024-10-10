#include "Hooks.h"

#include "Serialization.h"

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

            const auto character_id = Serialization::GetSingleton()->GetID(a_path);

            if (character_id == 0x0) { return; }

            a_entry->characterID = character_id;
            a_entry->fileNameValid = true;

            logger::info("Addresses::SaveFileReaderA :: Restored character ID: '{:X}' for file: '{}'", character_id, a_path);
        }
        static inline REL::Relocation<decltype(thunk)> func;
    };

    struct SaveFileReaderB
    {
        static void thunk(RE::BSWin32SaveDataSystemUtility::Entry* a_entry, const char* a_path)
        {
            func(a_entry, a_path);

            const auto character_id = Serialization::GetSingleton()->GetID(a_path);

            if (character_id == 0x0) { return; }

            a_entry->characterID = character_id;
            a_entry->fileNameValid = true;

            logger::info("Addresses::SaveFileReaderB :: Restored character ID: '{:X}' for file: '{}'", character_id, a_path);
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

		REL::Relocation save_reader{ RELOCATION_ID(34872, 442580), REL::Relocate(0xB4, 0x1B5) };
		stl::write_thunk_call<SaveFileReaderA>(save_reader.address());

        logger::info("Addresses :: Hooked SavefileReaderA");

        stl::write_vfunc<RE::BSWin32SaveDataSystemUtility::Entry, 0x0, SaveFileReaderB>();

        logger::info("Addresses :: Hooked SavefileReaderB");

		REL::Relocation save_game{ RELOCATION_ID(34818, 35727), REL::Relocate(0x112, 0x1CE) };
		stl::write_thunk_call<SaveGameHandler>(save_game.address());

        logger::info("Addresses :: Hooked SaveGameHandler");
    }
}
