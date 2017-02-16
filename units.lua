require "tundra.syntax.glob"
require "tundra.syntax.files"

local glfwDefines = {
	"_GLFW_WIN32", "_GLFW_USE_OPENGL", "_GLFW_WGL"
}

local copy_freeimage_win64 = CopyFile { Source = "src/ext/freeimage/win64/FreeImage.dll", Target = "$(OBJECTDIR)/FreeImage.dll" }

local glfw = StaticLibrary {
	Name = "glfw",
	Includes = { "src/glfw/include" },
	Defines = glfwDefines,
	Sources = {
		Glob {
			Dir = "src/ext/glfw/src",
			Extensions = {".c", ".h"},
			Recursive = false,
		}
	}
}

local imgui = StaticLibrary {
	Name = "imgui",
	Includes = { "src/ext/imgui" },
	Sources = {
		Glob {
			Dir = "src/ext/imgui",
			Extensions = {".cpp", ".h"}
		}
	}
}

local glad = StaticLibrary {
	Name = "glad",
	Includes = { "src/ext/glad/include" },
	Sources = {
		"src/ext/glad/src/glad.c",
		"src/ext/glad/include/glad/glad.h",
	},
}

local tinyexr = StaticLibrary {
	Name = "tinyexr",
	Includes = { "src/ext/tinyexr" },
	Sources = {
		"src/ext/tinyexr/tinyexr.cc"
	},
}

local rendertoy = Program {
	Name = "rendertoy",
	Depends = {
		glfw, imgui, glad, tinyexr,
		{ copy_freeimage_win64; Config = {"win*"} },
	},
	Defines = glfwDefines,
	Includes = {
		"src/ext/glfw/include",
		"src/ext/imgui",
		"src/ext/glad/include",
		"src/ext/rapidjson/include",
		"src/ext/glm/include",
		"src/ext/tinyexr",
		"src/ext/freeimage/include",
		"src/ext/gli",
	},
	Sources = {
		Glob { Dir = "src/rendertoy", Extensions = {".cpp", ".h"} }
	},
	Libs = {
		{
			"user32.lib",
			"shell32.lib",
			"comdlg32.lib",
			"gdi32.lib",
			"opengl32.lib",
			"src/ext/freeimage/win64/FreeImage.lib",
			Config = {"win*"}
		},
	},
}

Default(rendertoy)
