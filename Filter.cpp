#include "Filter.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "nlohmann/json.hpp"

bool DeltaImage::Filter(const std::string& InputDirectory, const std::string& OutputDirectory, const std::vector<std::string_view>& Keywords)
{
	std::filesystem::create_directories(OutputDirectory);

	std::ifstream IFile(InputDirectory + "/diff.json");
	if (!IFile.is_open())
	{
		std::cout << "No diff.json file found at the specified path\n";
		return false;
	}

	nlohmann::json LoadedJson;
	IFile >> LoadedJson;
	IFile.close();

	nlohmann::json OutJson;
	for (const std::string_view& Keyword : Keywords)
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
					std::filesystem::copy(InputDirectory + "/" + Value, OutputDirectory + "/" + Value, std::filesystem::copy_options::overwrite_existing);
				}
			}
		}
	}

	std::ofstream OFile(OutputDirectory + "/filtered.json", std::ios::trunc);
	OFile << OutJson.dump(4);
	OFile.close();
}
