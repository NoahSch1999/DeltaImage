#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "nlohmann/json.hpp"

class Image
{
private:
	void CopyOp(const Image& Other)
	{
		Data = Other.Data;
		NumChannels = Other.NumChannels;
		Width = Other.Width;
		Height = Other.Height;
	}

	void MoveOp(Image& Other)
	{
		Data = std::move(Other.Data);
		NumChannels = Other.NumChannels;
		Width = Other.Width;
		Height = Other.Height;
	}

public:
	std::vector<uint8_t> Data;
	uint32_t NumChannels;
	uint32_t Width;
	uint32_t Height;

	Image() = default;

	Image(uint32_t InWidth, uint32_t InHeight, uint32_t InNumChannels)
	{
		this->Init(InWidth, InHeight, InNumChannels);
	}

	void Init(uint32_t InWidth, uint32_t InHeight, uint32_t InNumChannels)
	{
		const uint32_t DataSize = InWidth * InHeight * InNumChannels;
		Data.resize(DataSize);
		Width = InWidth;
		Height = InHeight;
		NumChannels = InNumChannels;
	}

	Image(const Image& Other)
	{
		this->CopyOp(Other);
	}

	Image(Image&& Other) noexcept
	{
		this->MoveOp(Other);
	}

	Image& operator=(const Image& Other)
	{
		this->CopyOp(Other);
		return *this;
	}

	Image& operator=(Image&& Other) noexcept
	{
		if (this != &Other)
		{
			this->MoveOp(Other);
		}
		return *this;
	}
};

struct LoadedImageData
{
	uint8_t* First;
	uint8_t* Second;
	uint32_t NumChannels;
	uint32_t Width;
	uint32_t Height;
};

bool LoadAndValidateImages(std::string_view FirstPath, std::string_view SecondPath, LoadedImageData& Output)
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

struct ComparisonData
{
	uint32_t FirstTotalValue = 0;
	uint32_t SecondTotalValue = 0;
	uint32_t TotalTexelDelta = 0;
	uint32_t NumTexels;
	std::unordered_map<std::string, Image> Images;
};

void CompareImages(const LoadedImageData& ImageData, ComparisonData& OutData)
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

bool GenerateOutput(const std::string& Directory, const ComparisonData& CmpData)
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

bool Differentiate(const std::string& OutputDirectory, std::string_view FirstImage, std::string_view SecondImage)
{
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

bool Filter(std::string_view InputDirectory, std::string_view OutputDirectory, const std::span<std::string_view>& Keywords)
{
	std::filesystem::create_directories(OutputDirectory);

	const std::string InputDirectoryStr(InputDirectory);
	const std::string OutputDirectoryStr(OutputDirectory);

	std::ifstream IFile(InputDirectoryStr + "/diff.json");
	if (!IFile.is_open())
	{
		std::cout << "No diff.json file found at the specified path\n";
		return false;
	}

	nlohmann::json LoadedJson;
	IFile >> LoadedJson;
	IFile.close();

	nlohmann::json OutJson;
	for (std::string_view& Keyword : Keywords)
	{
		auto Iter = LoadedJson.find(Keyword);
		if (Iter != LoadedJson.end())
		{
			OutJson[Keyword] = Iter.value();

			// Handle image copying
			if (Iter->is_string())
			{
				std::string Value(Iter->get<std::string>());
				if (Value.ends_with(".png"))
				{
					std::filesystem::copy(InputDirectoryStr + "/" + Value, OutputDirectoryStr + "/" + Value, std::filesystem::copy_options::overwrite_existing);
				}
			}
		}
	}

	std::ofstream OFile(OutputDirectoryStr + "/filtered.json", std::ios::trunc);
	OFile << OutJson.dump(4);
	OFile.close();
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cout << "Invalid arguments\n";
		return -1;
	}

	/*
	0 - exe
	1 - Diff
	2 - Output directory
	3 - First image
	4 - Second image
	*/
	std::string_view ToolCommand(argv[1]);
	if (ToolCommand == "Diff")
	{
		if (argc == 5)
		{
			std::string_view FirstImage = argv[3];
			std::string_view SecondImage = argv[4];
			const bool Diffed = Differentiate(argv[2], FirstImage, SecondImage);

			if (!Diffed)
			{
				std::cout << "Failed to diff\n";
				return -1;
			}
		}
	}
	/*
	0 - exe
	1 - Filter
	2 - Input directory
	3 - Output directory
	4...n - Keywords
	*/
	else if (ToolCommand == "Filter")
	{
		if (argc > 4)
		{
			std::vector<std::string_view> Keywords;
			Keywords.resize(argc - 4);
			for (int i = 0; i < Keywords.size(); i++)
			{
				Keywords.at(i) = argv[4 + i];
			}

			const bool Filtered = Filter(argv[2], argv[3], Keywords);
			if (!Filtered)
			{
				std::cout << "Failed to filter\n";
				return -1;
			}
		}
	}

	return 0;
}