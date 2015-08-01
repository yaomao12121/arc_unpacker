#ifndef AU_FMT_PROPELLER_MPK_ARCHIVE_H
#define AU_FMT_PROPELLER_MPK_ARCHIVE_H
#include "fmt/archive.h"

namespace au {
namespace fmt {
namespace propeller {

    class MpkArchive final : public Archive
    {
    public:
        MpkArchive();
        ~MpkArchive();
    protected:
        bool is_recognized_internal(File &) const override;
        void unpack_internal(File &, FileSaver &) const override;
    private:
        struct Priv;
        std::unique_ptr<Priv> p;
    };

} } }

#endif