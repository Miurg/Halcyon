#include "ImageConverter.hpp"

#include <cstddef>

#include "JobsCore/Managers/JobManager.hpp"

std::vector<unsigned char> ImageConverter::convertToRGBA(const tinygltf::Image& img, JobManager& jobManager)
{
	if (img.image.empty() || img.width <= 0 || img.height <= 0)
	{
		return {};
	}

	const int components = img.component;
	if (components < 1 || components > 4)
	{
		return {};
	}

	const std::size_t width = static_cast<std::size_t>(img.width);
	const std::size_t height = static_cast<std::size_t>(img.height);
	const std::size_t pixelCount = width * height;

	if (components == 4)
	{
		return std::vector<unsigned char>(img.image.data(), img.image.data() + pixelCount * 4);
	}

	std::vector<unsigned char> rgbaData(pixelCount * 4);
	const unsigned char* src = img.image.data();
	unsigned char* dst = rgbaData.data();

	jobManager.parallelFor(
	    height,
	    [src, dst, width, components](std::size_t yBegin, std::size_t yEnd)
	    {
		    for (std::size_t y = yBegin; y < yEnd; ++y)
		    {
			    const std::size_t rowPixelBase = y * width;
			    for (std::size_t x = 0; x < width; ++x)
			    {
				    const std::size_t pixelIdx = rowPixelBase + x;
				    const std::size_t srcIdx = pixelIdx * static_cast<std::size_t>(components);
				    const std::size_t dstIdx = pixelIdx * 4;

				    if (components == 1)
				    {
					    dst[dstIdx + 0] = src[srcIdx];
					    dst[dstIdx + 1] = src[srcIdx];
					    dst[dstIdx + 2] = src[srcIdx];
					    dst[dstIdx + 3] = 255;
				    }
				    else if (components == 2)
				    {
					    dst[dstIdx + 0] = src[srcIdx];
					    dst[dstIdx + 1] = src[srcIdx];
					    dst[dstIdx + 2] = src[srcIdx];
					    dst[dstIdx + 3] = src[srcIdx + 1];
				    }
				    else
				    {
					    dst[dstIdx + 0] = src[srcIdx + 0];
					    dst[dstIdx + 1] = src[srcIdx + 1];
					    dst[dstIdx + 2] = src[srcIdx + 2];
					    dst[dstIdx + 3] = 255;
				    }
			    }
		    }
	    },
	    32, "ImageConverter::convertToRGBA");

	return rgbaData;
}
