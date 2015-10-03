#include "fmt/french_bread/p_archive_decoder.h"
#include <algorithm>
#include "fmt/french_bread/ex3_image_decoder.h"
#include "util/range.h"

using namespace au;
using namespace au::fmt::french_bread;

namespace
{
    struct TableEntry final
    {
        std::string name;
        size_t offset;
        size_t size;
    };

    using Table = std::vector<std::unique_ptr<TableEntry>>;
}

static const u32 encryption_key = 0xE3DF59AC;

static std::string read_file_name(io::IO &arc_io, size_t file_id)
{
    std::string file_name = arc_io.read(60).str();
    for (auto i : util::range(60))
        file_name[i] ^= file_id * i * 3 + 0x3D;
    return file_name.substr(0, file_name.find('\0'));
}

static Table read_table(io::IO &arc_io)
{
    size_t file_count = arc_io.read_u32_le() ^ encryption_key;
    Table table;
    for (auto i : util::range(file_count))
    {
        std::unique_ptr<TableEntry> entry(new TableEntry);
        entry->name = read_file_name(arc_io, i);
        entry->offset = arc_io.read_u32_le();
        entry->size = arc_io.read_u32_le() ^ encryption_key;
        table.push_back(std::move(entry));
    }
    return table;
}

static std::unique_ptr<File> read_file(
    io::IO &arc_io, TableEntry &entry, bool encrypted)
{
    std::unique_ptr<File> file(new File);

    arc_io.seek(entry.offset);
    bstr data = arc_io.read(entry.size);

    static const size_t block_size = 0x2172;
    for (auto i : util::range(std::min(block_size + 1, entry.size)))
        data[i] ^= entry.name[i % entry.name.size()] + i + 3;

    file->name = entry.name;
    file->io.write(data);

    return file;
}

struct PArchiveDecoder::Priv final
{
    Ex3ImageDecoder ex3_image_decoder;
};

PArchiveDecoder::PArchiveDecoder() : p(new Priv)
{
    add_decoder(&p->ex3_image_decoder);
}

PArchiveDecoder::~PArchiveDecoder()
{
}

bool PArchiveDecoder::is_recognized_internal(File &arc_file) const
{
    u32 encrypted = arc_file.io.read_u32_le();
    size_t file_count = arc_file.io.read_u32_le() ^ encryption_key;
    if (encrypted != 0 && encrypted != 1)
        return false;
    if (file_count > arc_file.io.size() || file_count * 68 > arc_file.io.size())
        return false;
    for (auto i : util::range(file_count))
    {
        read_file_name(arc_file.io, i);
        size_t offset = arc_file.io.read_u32_le();
        size_t size = arc_file.io.read_u32_le() ^ encryption_key;
        if (offset + size > arc_file.io.size())
            return false;
    }
    return true;
}

void PArchiveDecoder::unpack_internal(File &arc_file, FileSaver &saver) const
{
    bool encrypted = arc_file.io.read_u32_le() == 1;
    Table table = read_table(arc_file.io);
    for (auto &entry : table)
        saver.save(read_file(arc_file.io, *entry, encrypted));
}

static auto dummy = fmt::Registry::add<PArchiveDecoder>("fbread/p");