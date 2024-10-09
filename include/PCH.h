#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

#include <glaze/glaze.hpp>

namespace logger = SKSE::log;
using namespace std::literals;

namespace stl
{
    using namespace SKSE::stl;

    template <class T>
    void write_thunk_call(std::uintptr_t a_source)
    {
        auto& trampoline = SKSE::GetTrampoline();
        T::func = trampoline.write_call<5>(a_source, T::thunk);
    }

    template <class F, std::size_t idx, class T>
    void write_vfunc()
    {
        REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[0] };
        T::func = vtbl.write_vfunc(idx, T::thunk);
    }
}
