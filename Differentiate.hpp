#include <string>
#include <unordered_map>

namespace DeltaImage
{
	class Image;

	namespace Private
	{
		struct LoadedImageData
		{
			uint8_t* First;
			uint8_t* Second;
			uint32_t NumChannels;
			uint32_t Width;
			uint32_t Height;
		};

		bool LoadAndValidateImages(std::string_view FirstPath, std::string_view SecondPath, LoadedImageData& Output);

		struct ComparisonData
		{
			uint32_t FirstTotalValue = 0;
			uint32_t SecondTotalValue = 0;
			uint32_t TotalTexelDelta = 0;
			uint32_t NumTexels;
			std::unordered_map<std::string, Image> Images;
		};

		void CompareImages(const LoadedImageData& ImageData, ComparisonData& OutData);

		bool GenerateOutput(const std::string& Directory, const ComparisonData& CmpData);

	}

	bool Differentiate(const std::string& OutputDirectory, std::string_view FirstImage, std::string_view SecondImage);
}