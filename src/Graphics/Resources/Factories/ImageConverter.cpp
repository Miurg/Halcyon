#include "ImageConverter.hpp"

std::vector<unsigned char> ImageConverter::convertToRGBA(const tinygltf::Image& img)
{
	if (img.image.empty() || img.width <= 0 || img.height <= 0)
	{
		return {};
	}

	int components = img.component; // 1, 2, 3 or 4
	int pixelType = img.pixel_type; // TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE

	std::vector<unsigned char> rgbaData;
	rgbaData.resize(img.width * img.height * 4);

	const unsigned char* src = img.image.data();
	unsigned char* dst = rgbaData.data();

	for (int y = 0; y < img.height; y++)
	{
		for (int x = 0; x < img.width; x++)
		{
			int srcIdx = (y * img.width + x) * components;
			int dstIdx = (y * img.width + x) * 4;

			if (components == 1)
			{
				// Grayscale -> RGBA
				dst[dstIdx + 0] = src[srcIdx];
				dst[dstIdx + 1] = src[srcIdx];
				dst[dstIdx + 2] = src[srcIdx];
				dst[dstIdx + 3] = 255;
			}
			else if (components == 2)
			{
				// Grayscale + Alpha -> RGBA
				dst[dstIdx + 0] = src[srcIdx];
				dst[dstIdx + 1] = src[srcIdx];
				dst[dstIdx + 2] = src[srcIdx];
				dst[dstIdx + 3] = src[srcIdx + 1];
			}
			else if (components == 3)
			{
				// RGB -> RGBA
				dst[dstIdx + 0] = src[srcIdx + 0];
				dst[dstIdx + 1] = src[srcIdx + 1];
				dst[dstIdx + 2] = src[srcIdx + 2];
				dst[dstIdx + 3] = 255;
			}
			else if (components == 4)
			{
				// RGBA -> RGBA
				dst[dstIdx + 0] = src[srcIdx + 0];
				dst[dstIdx + 1] = src[srcIdx + 1];
				dst[dstIdx + 2] = src[srcIdx + 2];
				dst[dstIdx + 3] = src[srcIdx + 3];
			}
		}
	}

	return rgbaData;
}
