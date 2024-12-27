workspace "Octopus"
    configurations { "debug", "release" }
    startproject "Octopus"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Octopus"
    language "C"
    location "src"
    kind "ConsoleApp"

    warnings "Extra"
    architecture "x64"
    toolset "clang"

    includedirs {
        "src",
    }

    files {
        "src/**.h",
        "src/**.c",
    }

    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
    targetdir ("bin/" .. outputdir)
    targetprefix ""

    filter "configurations:debug"
        symbols "On"
        defines { "DEBUG" }

    filter { "action:not vs*", "configurations:debug" }
    	buildoptions { "-fsanitize=address" }
    	linkoptions { "-fsanitize=address" }

    filter "configurations:release"
        optimize "On"
        defines { "NDEBUG" }

    filter "system:linux"
        links {
            "m", "X11", "GL", "GLX"
        }

    -- This is dumb, but needed to get everything to work on my windows machine
    filter { "system:windows", "action:*gmake*", "configurations:debug" }
        linkoptions { "-g" }

    filter "system:windows"
        systemversion "latest"
    
        links {
            "gdi32", "kernel32", "user32", "opengl32"
        }

