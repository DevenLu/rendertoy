#pragma once
#include "Common.h"
#include "Math.h"

#include <string>
#include <unordered_map>


struct TextureSize
{
	ivec2 resolution = ivec2(1, 1);
	vec2 relativeScale = vec2(1, 1);
	std::string scaleRelativeTo = "#window";
	bool useRelativeScale = true;
};

enum class TextureFormat {
	rgba16f,
	r32ui,
	Count,
};

inline const char* const textureFormatToString(TextureFormat fmt) {
	switch (fmt) {
	case TextureFormat::rgba16f: return "rgba16f";
	case TextureFormat::r32ui: return "r32ui";
	default: return nullptr;
	}
}

bool parseTextureFormat(const char* const str, TextureFormat *const res);
TextureFormat parseTextureFormat(const char* const str);

struct TextureDesc {
	TextureDesc()
		: wrapS(true)
		, wrapT(true)
	{}

	enum class Source {
		Load,
		Create,
		Input
	};

	std::string path;
	Source source = Source::Input;
	TextureFormat createFormat = TextureFormat::rgba16f;
	TextureSize size;
	bool wrapS : 1;
	bool wrapT : 1;
};

struct TextureKey {
	u32 width;
	u32 height;
	unsigned int format;	// GLenum

	bool operator==(const TextureKey& other) const {
		return width == other.width && height == other.height && format == other.format;
	}
};

struct CreatedTexture {
	unsigned int texId = 0;			// GLuint
	unsigned int samplerId = 0;		// GLuint
	TextureKey key;

	~CreatedTexture();
};



extern std::unordered_map<std::string, shared_ptr<CreatedTexture>> g_loadedTextures;

shared_ptr<CreatedTexture> loadTexture(const TextureDesc& desc);
shared_ptr<CreatedTexture> createTexture(const TextureDesc& desc, const TextureKey& key);
