workspace "Platformer"
	configurations { "Debug", "Release" }

project "Platformer"
	kind "ConsoleApp"
	language "C++"
	targetdir "bin/%{cfg.buildcfg}"

	files { "src/main.cpp" }

	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:Release"
		optimize "On"

	filter { "system:windows" }
		architecture "x86_64"
		includedirs { "libs\\include" }
		libdirs { "libs\\windows" }
   		links { "opengl32.lib", "glfw3.lib", "glew32s.lib", "assimp.lib" }
