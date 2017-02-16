#include "Texture.h"
#include "StringUtil.h"

#define NOMINMAX
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tinyexr.h>
#include <FreeImage.h>
#include <gli/gli.hpp>

std::unordered_map<std::string, shared_ptr<CreatedTexture>> g_loadedTextures;

CreatedTexture::~CreatedTexture()
{
	if (texId != 0) glDeleteTextures(1, &texId);
	if (samplerId != 0) glDeleteSamplers(1, &samplerId);
}

shared_ptr<CreatedTexture> loadTextureExr(const TextureDesc& desc)
{
	int ret;
	const char* err;

	// 1. Read EXR version.
	EXRVersion exr_version;

	ret = ParseEXRVersionFromFile(&exr_version, desc.path.c_str());
	if (ret != 0) {
		fprintf(stderr, "Invalid EXR file: %s\n", desc.path.c_str());
		return nullptr;
	}

	if (exr_version.multipart) {
		// must be multipart flag is false.
		printf("Multipart EXR not supported");
		return nullptr;
	}

	// 2. Read EXR header
	EXRHeader exr_header;
	InitEXRHeader(&exr_header);

	ret = ParseEXRHeaderFromFile(&exr_header, &exr_version, desc.path.c_str(), &err);
	if (ret != 0) {
		fprintf(stderr, "Parse EXR err: %s\n", err);
		return nullptr;
	}

	EXRImage exr_image;
	InitEXRImage(&exr_image);

	for (int i = 0; i < exr_header.num_channels; ++i) {
		exr_header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;
	}

	ret = LoadEXRImageFromFile(&exr_image, &exr_header, desc.path.c_str(), &err);
	if (ret != 0) {
		fprintf(stderr, "Load EXR err: %s\n", err);
		return nullptr;
	}

	short* out_rgba = nullptr;

	{
		// RGBA
		int idxR = -1;
		int idxG = -1;
		int idxB = -1;
		int idxA = -1;
		for (int c = 0; c < exr_header.num_channels; c++) {
			if (strcmp(exr_header.channels[c].name, "R") == 0) {
				idxR = c;
			}
			else if (strcmp(exr_header.channels[c].name, "G") == 0) {
				idxG = c;
			}
			else if (strcmp(exr_header.channels[c].name, "B") == 0) {
				idxB = c;
			}
			else if (strcmp(exr_header.channels[c].name, "A") == 0) {
				idxA = c;
			}
		}

		size_t imgSizeBytes = 4 * sizeof(short) * static_cast<size_t>(exr_image.width) * static_cast<size_t>(exr_image.height);

		out_rgba = reinterpret_cast<short *>(malloc(imgSizeBytes));
		memset(out_rgba, 0, imgSizeBytes);

		auto loadChannel = [&](int chIdx, int compIdx) {
			for (int y = 0; y < exr_image.height; ++y) {
				for (int x = 0; x < exr_image.width; ++x) {
					out_rgba[4 * (y * exr_image.width + x) + compIdx] =
						reinterpret_cast<short**>(exr_image.images)[chIdx][((exr_image.height - y - 1) * exr_image.width + x)];
				}
			}
		};

		if (idxR != -1) loadChannel(idxR, 0);
		if (idxG != -1) loadChannel(idxG, 1);
		if (idxB != -1) loadChannel(idxB, 2);

		if (idxA != -1) loadChannel(idxA, 3);
		else {
			const short one = 15 << 10;
			for (int i = 0; i < exr_image.width * exr_image.height; i++) {
				out_rgba[4 * i + 3] = one;
			}
		}
	}

	shared_ptr<CreatedTexture> res = createTexture(
		desc,
		TextureKey{ u32(exr_image.width), u32(exr_image.height), GL_RGBA16F }
	);

	glBindTexture(GL_TEXTURE_2D, res->texId);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, exr_image.width, exr_image.height, GL_RGBA, GL_HALF_FLOAT, out_rgba);

	FreeEXRHeader(&exr_header);
	FreeEXRImage(&exr_image);

	return res;
}

shared_ptr<CreatedTexture> loadTextureGli(const TextureDesc& desc)
{
	gli::texture image = gli::load(desc.path.c_str());
	if (image.empty() || image.target() != gli::TARGET_2D) {
		return nullptr;
	}

	// TODO: is this needed?
	//image = gli::flip(image);

	gli::gl GL(gli::gl::PROFILE_GL33);
	gli::gl::format const format = GL.translate(image.format(), image.swizzles());
	GLenum target = GL.translate(image.target());

	const bool compressed = gli::is_compressed(image.format());

	GLuint TextureName = 0;
	glGenTextures(1, &TextureName);
	glBindTexture(target, TextureName);
	glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(image.levels() - 1));
	glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, &format.Swizzles[0]);

	auto extent = image.extent();
	glTexStorage2D(target, static_cast<GLint>(image.levels()), format.Internal, extent.x, extent.y);

	for (std::size_t Level = 0; Level < image.levels(); ++Level)
	{
		glm::tvec3<GLsizei> levelExtent(image.extent(Level));
		if (compressed) {
			glCompressedTexSubImage2D(
				target, static_cast<GLint>(Level), 0, 0, levelExtent.x, levelExtent.y,
				format.Internal, static_cast<GLsizei>(image.size(Level)), image.data(0, 0, Level));
		} else {
			glTexSubImage2D(
				target, static_cast<GLint>(Level), 0, 0, levelExtent.x, levelExtent.y,
				format.External, format.Type, image.data(0, 0, Level));
		}
	}

	GLuint samplerId;
	glGenSamplers(1, &samplerId);
	glSamplerParameteri(samplerId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri(samplerId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(samplerId, GL_TEXTURE_MAX_LOD, static_cast<GLint>(image.levels() - 1));

	auto tex = std::make_shared<CreatedTexture>();
	tex->key = TextureKey{ u32(extent.x), u32(extent.y), (unsigned int)(format.Internal) };
	tex->texId = TextureName;
	tex->samplerId = samplerId;

	return tex;
}

static FIBITMAP* LoadFIBITMAP(const std::string& path) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

	fif = FreeImage_GetFileType(path.c_str());

	if (FIF_UNKNOWN == fif) {
		// No signature? Try to guess the file format from the file extension.
		fif = FreeImage_GetFIFFromFilename(path.c_str());
	}

	if ((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
		FIBITMAP *dib = FreeImage_Load(fif, path.c_str());
		return dib;
	}

	return nullptr;
}

shared_ptr<CreatedTexture> loadTextureFreeimage(const TextureDesc& desc)
{
	static bool freeimageInitialized = (FreeImage_Initialise(), true);
	auto dib = shared_ptr<FIBITMAP>(LoadFIBITMAP(desc.path), FreeImage_Unload);

	if (dib == nullptr) {
		// error("FreeImage returned a null bitmap");
		return nullptr;
	}

	if (FreeImage_GetPalette(dib.get())) {
		// Convert to a bitmap
		const bool transparent = FreeImage_GetTransparencyCount(dib.get()) > 0;
		auto convFn = transparent ? &FreeImage_ConvertTo32Bits : &FreeImage_ConvertTo24Bits;
		dib = shared_ptr<FIBITMAP>(convFn(dib.get()), FreeImage_Unload);
	}

	const FREE_IMAGE_TYPE itype = FreeImage_GetImageType(dib.get());

	shared_ptr<CreatedTexture> result = nullptr;

	u32 width = FreeImage_GetWidth(dib.get());
	u32 height = FreeImage_GetHeight(dib.get());

	if (FIT_BITMAP == itype) {
		uint bits = FreeImage_GetBPP(dib.get());
		if (bits != 24 && bits != 32) {
			dib = shared_ptr<FIBITMAP>(FreeImage_ConvertTo24Bits(dib.get()), FreeImage_Unload);
			bits = 24;
		}

		const u32 scanLineBytes = FreeImage_GetPitch(dib.get());
		result = createTexture(
			desc,
			TextureKey{ u32(width), u32(height), uint(24 == bits ? GL_SRGB8 : GL_SRGB8_ALPHA8) }
		);

		glBindTexture(GL_TEXTURE_2D, result->texId);
		const u8* const imgData = FreeImage_GetBits(dib.get());
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, uint(24 == bits ? GL_BGR : GL_BGRA), GL_UNSIGNED_BYTE, (const void*)imgData);
	} else if (FIT_RGBF == itype) {
		const u32 scanLineBytes = FreeImage_GetPitch(dib.get());
		result = createTexture(
			desc,
			TextureKey{ u32(width), u32(height), GL_RGB32F }
		);

		glBindTexture(GL_TEXTURE_2D, result->texId);
		const u8* const imgData = FreeImage_GetBits(dib.get());
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, (const void*)imgData);
	} else {
		// TODO
	}

	return result;
}

shared_ptr<CreatedTexture> loadTexture(const TextureDesc& desc) {
	{
		auto found = g_loadedTextures.find(desc.path);
		if (found != g_loadedTextures.end()) {
			return found->second;
		}
	}

	shared_ptr<CreatedTexture> result;

	if (ends_with(to_lower(desc.path), ".exr")) {
		result = loadTextureExr(desc);
	} else if (ends_with(to_lower(desc.path), ".dds") || ends_with(to_lower(desc.path), ".ktx")) {
		result = loadTextureGli(desc);
	} else {
		result = loadTextureFreeimage(desc);
	}

	g_loadedTextures[desc.path] = result;
	return result;
}

shared_ptr<CreatedTexture> createTexture(const TextureDesc& desc, const TextureKey& key)
{
	GLuint tex1;
	glGenTextures(1, &tex1);
	glBindTexture(GL_TEXTURE_2D, tex1);
	glTexStorage2D(GL_TEXTURE_2D, 1u, key.format, key.width, key.height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	GLuint samplerId;
	glGenSamplers(1, &samplerId);
	glSamplerParameteri(samplerId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri(samplerId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	auto tex = std::make_shared<CreatedTexture>();
	tex->key = key;
	tex->texId = tex1;
	tex->samplerId = samplerId;
	return tex;
}
