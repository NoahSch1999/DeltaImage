#include <vector>

namespace DeltaImage
{
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
}