#pragma once
#include <string>
#include <vector>
#include <filesystem>

class Utils
{
public:
    // return a list of files with the specified extension 
    static std::vector<std::string> GetFiles(const std::string& path_to_dir, const std::string& extension = ".bank")
    {
        namespace fs = std::filesystem;

        if (fs::is_directory(path_to_dir))
        {
            std::vector<std::string> file_names;

            for (const auto& entry : fs::recursive_directory_iterator(path_to_dir))
            {
                // note: in native windows file systems, make the comparison for extension case-insensitive
                if (entry.is_regular_file() && entry.path().extension() == extension)
                    file_names.push_back(entry.path().string());
            }

            return file_names;
        }

        else return {}; // not a directory; return an empty vector
    }
};