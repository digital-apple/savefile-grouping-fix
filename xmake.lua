set_xmakever("2.8.2")

includes("lib/commonlibsse-ng")

set_project("savefile-grouping-fix")
set_version("1.0.1")
set_license("GPL-3.0")

set_languages("c++23")
set_warnings("allextra", "error")
set_defaultmode("releasedbg")

add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

set_policy("package.requires_lock", true)

add_requires("glaze")

target("savefile-grouping-fix")
    add_deps("commonlibsse-ng")

    add_packages("glaze")

    add_rules("commonlibsse-ng.plugin", {
        name = "savefile-grouping-fix",
        author = "digital-apple",
        description = "fixes custom savefiles not being assigned to their correct group"
    })

    add_files("source/**.cpp")
    add_headerfiles("include/**.h")
    add_includedirs("include", { public = true })
    set_pcxxheader("include/PCH.h")

    after_build(function(target)
        local copy = function(env, ext)
            for _, env in pairs(env:split(";")) do
                if os.exists(env) then
                    local plugins = path.join(env, ext, "SKSE/Plugins")
                    os.mkdir(plugins)
                    os.trycp(target:targetfile(), plugins)
                    os.trycp(target:symbolfile(), plugins)
                end
            end
        end
        if os.getenv("XSE_TES5_MODS_PATH") then
            copy(os.getenv("XSE_TES5_MODS_PATH"), target:name())
        elseif os.getenv("XSE_TES5_GAME_PATH") then
            copy(os.getenv("XSE_TES5_GAME_PATH"), "Data")
        end
    end)