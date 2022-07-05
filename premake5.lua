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
		includedirs { "D:\\Development\\libs\\glew-2.1.0\\include",
					  "D:\\Development\\libs\\glm",
					  "D:\\Development\\libs\\glfw-3.3.2\\include" }
		libdirs { "D:\\Development\\libs\\glew-2.1.0\\lib\\Release\\x64",
				  "D:\\Development\\libs\\glfw-3.3.2\\lib-vc2019" }
   		links { "opengl32.lib", "glfw3.lib", "glew32s.lib" }
