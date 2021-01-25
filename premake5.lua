-- premake5.lua
workspace "Dark2"
	configurations { "Debug", "Release" }

project "Dark2"
	kind "ConsoleApp"
	language "C++"
	architecture "x86"
	
	--targetdir "bin/%{cfg.buildcfg}"
	targetdir "bin/"
	debugdir "bin/"
	
	location "build"

	buildoptions "/std:c++latest"

	filter { "configurations:Debug" }
		-- We want debug symbols in our debug config
		defines { "DEBUG" }
		symbols "On"
		links {
			"zlibstaticd"
		}
	
	filter { "configurations:Release" }
		-- Release should be optimized
		defines { "NDEBUG" }
		optimize "On"
		links {
			"zlibstatic"
		}
	
	filter {}

	defines { "_CRT_SECURE_NO_WARNINGS" }
	
	files {
		"dep/**",
		"src/**",
		"lib/**",
		"include/**",
	}

	excludes {
		"lib/graveyard/*"
	}

	includedirs {
		"dep",
		"src",
		"lib",
		"include",
		"C:/New folder/",
		"C:/New folder/glm",
		"C:/New folder/glfw-3.3/include",
		"C:/New folder/zlib-1.2.11"
	}
	
	libdirs {
		"C:/New folder/glfw-3.3-32/src/%{cfg.longname}",
		"C:/New folder/zlib-1.2.11-32/%{cfg.longname}"
	}

	links {
		"glfw3"
	--	"zlibstatic"
	}