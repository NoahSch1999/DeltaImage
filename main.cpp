#include <iostream>

#include "Differentiate.hpp"
#include "Filter.hpp"

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
			const bool Diffed = DeltaImage::Differentiate(argv[2], FirstImage, SecondImage);

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

			const bool Filtered = DeltaImage::Filter(argv[2], argv[3], Keywords);
			if (!Filtered)
			{
				std::cout << "Failed to filter\n";
				return -1;
			}
		}
	}

	return 0;
}