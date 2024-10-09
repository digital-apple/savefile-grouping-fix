#pragma once

class Serialization
{
public:
    static auto GetSingleton() -> Serialization*;

    bool GetSaveFileDirectory();
    auto GetID(const std::string_view& a_path) -> std::uint32_t;
    bool Find(const std::string_view& a_path);

    void Save(const std::string_view& a_path);
    void Load();
    void Clean();
private:
    Serialization() = default;
    Serialization(const Serialization&) = delete;
    Serialization(Serialization&&) = delete;

    ~Serialization() = default;

    Serialization& operator=(const Serialization&) = delete;
    Serialization& operator=(Serialization&&) = delete;

    std::optional<std::filesystem::path> save_directory;
    std::string_view JSON_PATH = "Data\\SKSE\\Plugins\\savefile-grouping-fix.json"sv;

    /*
        * Case-insensitive hashing and comparison. 
        * We need it because the game treats the first character of a savefile in uppercase for displaying purposes, 
          and since we are using the savefile name as the key in our map, we must handle such case differences.
    */

    struct CIHash 
    {
        auto operator()(const std::string& a_key) const -> std::size_t
        {
            std::string key = a_key;
            std::transform(key.begin(), key.end(), key.begin(), [](unsigned char x) { return static_cast<unsigned char>(std::tolower(x)); });

            return std::hash<std::string>()(key);
        }
    };

    struct CIEqual
    {
        bool operator()(const std::string& a_lhs, const std::string& a_rhs) const
        {
            return std::equal(a_lhs.begin(), a_lhs.end(), a_rhs.begin(), a_rhs.end(), [](unsigned char x, unsigned char y) {
                return std::tolower(x) == std::tolower(y);
            });
        }
    };

    std::unordered_map<std::string, std::uint32_t, CIHash, CIEqual> save_files;
};
