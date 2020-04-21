//#include "EmbeddedResource.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

std::string FilePathToSym(std::filesystem::path filepath)
{
    auto sym = filepath.filename().string();
    replace(sym.begin(), sym.end(), '.', '_');
    replace(sym.begin(), sym.end(), '-', '_');
    if (std::isdigit(sym[0]))
    {
        return "_" + sym;
    }
    return sym;
}

auto ParseArg(std::string_view arg)
{
    // If name is resname:filepath use resname or else convert filename into a symbol
    auto                  idx = arg.find('!');
    std::string           resname;
    std::filesystem::path src;
    if (idx == std::string::npos)
    {
        src     = std::filesystem::path(arg);
        resname = src.filename().string();
    }
    else
    {
        if (idx == 0)
        {
            throw std::invalid_argument("Invalid name for resource: " + std::string(arg));
        }
        src     = std::filesystem::path(std::string(arg.begin() + idx + 1, arg.end()));
        resname = std::string_view(arg.data(), idx);
    }

    return std::make_tuple(std::move(resname), std::move(src), FilePathToSym(src));
}
int main(int argc, char** argv) try
{
    if (argc < 3)
    {
        for (int i = 0; i < argc; i++)
        {
            std::cerr << " " << argv[i];
        }

        std::cerr << std::endl
                  << std::endl
                  << "USAGE: %s {sym} {rsrc}..." << std::endl
                  << std::endl
                  << "  Creates {sym}.c from the contents of each {rsrc}\n"
                  << "  Each resource {rsrc} can be specified in the format [name:]filepath" << argv[0];
        return EXIT_FAILURE;
    }

    std::filesystem::path dst{argv[1]};
    create_directories(dst.parent_path());

    std::ofstream ofs{dst.string()};
    ofs << "#include <EmbeddedResource.h>" << std::endl;

    auto colsym = FilePathToSym(dst.stem());

    for (int i = 2; i < argc; i++)
    {
        auto [resname, src, sym] = ParseArg(std::string_view(argv[i]));

        std::ifstream ifs{src, std::ios::binary | std::ios::in};
        if (!ifs.is_open())
        {
            throw std::invalid_argument("Cannot find file" + src.string());
        }
        ofs << "namespace EmbeddedResource::Data::" << colsym << "::" << sym << " {" << std::endl;
        ofs << "std::byte const _ResourceData[] = {" << std::endl;
        uint8_t c;
        ifs.read((char*)&c, sizeof(c));
        for (size_t j = 0; !ifs.eof() ; j++, ifs.read((char*)&c, sizeof(c)))
        {
            if (j > 0)
            {
                ofs << ",";
            }
            ofs << "(std::byte)0x" << std::hex << static_cast<uint32_t>(c) << "u";
            if ((j + 1) % 10 == 0)
            {
                ofs << std::endl;
            }
        }

        ofs << "};" << std::endl;

        ofs << "const std::wstring_view _ResourceName = L\"" << resname << "\";" << std::endl;

        ofs << "EmbeddedResource::Resource GetResource() noexcept {"
            << "    return EmbeddedResource::Resource(_ResourceName, "
            << "std::span<const std::byte>(std::begin(_ResourceData), std::end(_ResourceData)));"
            << "}" << std::endl;

        ofs << "}" << std::endl << std::endl;
    }
    ofs << "namespace EmbeddedResource::Data::" << colsym << " {" << std::endl;

    ofs << "EmbeddedResource::Resource::GetFunc* const  _ResourceTable[] = {" << std::endl;
    for (int i = 2; i < argc; i++)
    {
        std::filesystem::path src{argv[i]};
        auto                  ressym = FilePathToSym(src);
        ofs << ressym << "::GetResource" << ((i + 1 == argc) ? "" : ",");
    }

    ofs << "};" << std::endl;

    ofs << "}" << std::endl << std::endl;

    ofs << "DECLARE_RESOURCE_COLLECTION(" << colsym << ") { return EmbeddedResource::Collection(EmbeddedResource::Data::" << colsym
        << "::_ResourceTable); }" << std::endl;

    ofs.close();
    return EXIT_SUCCESS;
}
catch (std::exception const& ex)
{
    std::cerr << ex.what() << std::endl;
    return -1;
}