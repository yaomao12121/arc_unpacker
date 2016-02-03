#include "dec/kaguya/link6_archive_decoder.h"
#include "algo/locale.h"
#include "algo/range.h"

using namespace au;
using namespace au::dec::kaguya;

static const auto magic = "LINK6"_b;

namespace
{
    struct ArchiveEntryImpl final : dec::ArchiveEntry
    {
        size_t offset;
        size_t size;
    };
}

bool Link6ArchiveDecoder::is_recognized_impl(io::File &input_file) const
{
    return input_file.stream.seek(0).read(magic.size()) == magic;
}

std::unique_ptr<dec::ArchiveMeta> Link6ArchiveDecoder::read_meta_impl(
    const Logger &logger, io::File &input_file) const
{
    input_file.stream.seek(magic.size() + 2);
    const auto name_size = input_file.stream.read<u8>();
    const auto name = input_file.stream.read(name_size);

    auto meta = std::make_unique<ArchiveMeta>();
    while (true)
    {
        const auto entry_offset = input_file.stream.tell();
        const auto entry_size = input_file.stream.read_le<u32>();
        if (!entry_size)
            break;

        // ?
        input_file.stream.skip(9);

        const auto entry_name_size = input_file.stream.read_le<u16>();
        const auto entry_name = algo::utf16_to_utf8(
            input_file.stream.read(entry_name_size)).str();

        auto entry = std::make_unique<ArchiveEntryImpl>();
        entry->path = entry_name;
        entry->offset = input_file.stream.tell();
        entry->size = entry_size - (entry->offset - entry_offset);
        meta->entries.push_back(std::move(entry));

        input_file.stream.seek(entry_offset + entry_size);
    }

    return meta;
}

std::unique_ptr<io::File> Link6ArchiveDecoder::read_file_impl(
    const Logger &logger,
    io::File &input_file,
    const dec::ArchiveMeta &m,
    const dec::ArchiveEntry &e) const
{
    const auto entry = static_cast<const ArchiveEntryImpl*>(&e);
    return std::make_unique<io::File>(
        entry->path,
        input_file.stream.seek(entry->offset).read(entry->size));
}

std::vector<std::string> Link6ArchiveDecoder::get_linked_formats() const
{
    return {"kaguya/ap", "kaguya/ap0"};
}

static auto _ = dec::register_decoder<Link6ArchiveDecoder>("kaguya/link6");