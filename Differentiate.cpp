#include "Differentiate.hpp"

#include <iostream>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "nlohmann/json.hpp"

bool DeltaImage::Private::LoadAndValidateImages(std::string_view FirstPath, std::string_view SecondPath, LoadedImageData& Output)
{
	int32_t OriginalWidth, OriginalHeight, OriginalChannels;
	stbi_uc* First = stbi_load(FirstPath.data(), &OriginalWidth, &OriginalHeight, &OriginalChannels, 0);

	int32_t CompareWidth, CompareHeight, CompareChannels;
	stbi_uc* Second = stbi_load(SecondPath.data(), &CompareWidth, &CompareHeight, &CompareChannels, 0);

	if (!First || !Second)
	{
		std::cout << "Couldn't load images\n";
		return false;
	}

	if ((OriginalWidth != CompareWidth) ||
		(OriginalHeight != CompareHeight) ||
		(OriginalChannels != CompareChannels)
		)
	{
		std::cout << "Dimensions and channels aren't matching\n";
		return false;
	}

	Output.First = First;
	Output.Second = Second;
	Output.Width = OriginalWidth;
	Output.Height = OriginalHeight;
	Output.NumChannels = OriginalChannels;

	return true;
}

void DeltaImage::Private::CompareImages(const LoadedImageData& ImageData, ComparisonData& OutData)
{
	// Remove alpha comparison
	uint32_t NumChannels = ImageData.NumChannels;
	if (NumChannels == 4)
	{
		NumChannels--;
	}

	OutData.Images["ColorDelta"] = Image(ImageData.Width, ImageData.Height, NumChannels);
	Image* ColorImage = &OutData.Images.at("ColorDelta");

	// Example of adding another image
	OutData.Images["Other"] = Image(ImageData.Width, ImageData.Height, NumChannels);

	uint32_t FirstTotalValue = 0, SecondTotalValue = 0, TotalTexelDelta = 0;

	for (int TexelIndex = 0; TexelIndex < ColorImage->Width * ColorImage->Height; TexelIndex++)
	{
		for (int ChannelIndex = 0; ChannelIndex < ColorImage->NumChannels; ChannelIndex++)
		{
			const uint8_t* FirstPtr = ImageData.First + TexelIndex * ImageData.NumChannels + ChannelIndex;
			const uint8_t FirstChannelData = *FirstPtr;

			const uint8_t* SecondPtr = ImageData.Second + TexelIndex * ImageData.NumChannels + ChannelIndex;
			const uint8_t SecondChannelData = *SecondPtr;

			const uint8_t TexelDelta = std::max(FirstChannelData, SecondChannelData) - std::min(FirstChannelData, SecondChannelData);
			ColorImage->Data.at(TexelIndex * ColorImage->NumChannels + ChannelIndex) = 255 - TexelDelta;

			// Example of writing to another image
			OutData.Images["Other"].Data.at(TexelIndex * ColorImage->NumChannels + ChannelIndex) = TexelIndex % ColorImage->Width;

			FirstTotalValue += FirstChannelData;
			SecondTotalValue += SecondChannelData;
			TotalTexelDelta += TexelDelta;
		}
	}

	OutData.NumTexels = ColorImage->NumChannels * ColorImage->Height * ColorImage->Width;
	OutData.FirstTotalValue = FirstTotalValue;
	OutData.SecondTotalValue = SecondTotalValue;
	OutData.TotalTexelDelta = TotalTexelDelta;
}

bool DeltaImage::Private::GenerateOutput(const std::string& Directory, const ComparisonData& CmpData)
{
	nlohmann::json OutJson;
	std::filesystem::create_directories(Directory);

	for (const auto& [Name, Image] : CmpData.Images)
	{
		const int WriteReturn = stbi_write_png(std::string(Directory + "/" + Name + ".png").c_str(), Image.Width, Image.Height, Image.NumChannels, Image.Data.data(), Image.Width * Image.NumChannels);
		if (!WriteReturn)
		{
			std::cout << "Failed to output image\n";
			return false;
		}
		OutJson[Name] = Name + ".png";
	}

	const double TotalDeltaNorm = CmpData.TotalTexelDelta / (255.0 * CmpData.NumTexels);
	const double MatchPercent = (1 - TotalDeltaNorm) * 100.f;

	OutJson["MatchPercent"] = MatchPercent;
	OutJson["AverageDelta"] = (double)CmpData.TotalTexelDelta / CmpData.NumTexels;

	std::ofstream OFile(Directory + "/diff.json", std::ios::trunc);
	if (!OFile.is_open())
	{
		std::cout << "Failed to output diff.json\n";
		return false;
	}

	OFile << OutJson.dump(4);
	OFile.close();

	return true;
}

bool DeltaImage::Differentiate(const std::string& OutputDirectory, std::string_view FirstImage, std::string_view SecondImage)
{
	using namespace Private;

	LoadedImageData ImageData;
	const bool ImagesValid = LoadAndValidateImages(FirstImage, SecondImage, ImageData);
	if (!ImagesValid)
	{
		return false;
	}

	ComparisonData CmpData;
	CompareImages(ImageData, CmpData);

	bool GeneratedOutput = GenerateOutput(OutputDirectory, CmpData);
	if (!GeneratedOutput)
	{
		return false;
	}

	const std::string OriginalImagesDir(OutputDirectory + "/sourceImages");
	std::filesystem::create_directories(OriginalImagesDir);
	std::filesystem::copy(FirstImage, OriginalImagesDir, std::filesystem::copy_options::overwrite_existing);
	std::filesystem::copy(SecondImage, OriginalImagesDir, std::filesystem::copy_options::overwrite_existing);

	return true;
}
