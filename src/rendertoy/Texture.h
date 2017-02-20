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
