/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "DB2FileLoader.h"
#include "gtest/gtest.h"

#include <cstring>
#include <vector>

// In-memory DB2FileSource so the WDC4 header parser can be exercised without
// touching the filesystem. The CASC-backed extractor (Task 0.4) provides the
// equivalent runtime source for real client .db2 files.
namespace
{
    class MemoryDB2Source : public DB2FileSource
    {
    public:
        explicit MemoryDB2Source(std::vector<uint8> data) : _data(std::move(data)), _pos(0) { }

        bool IsOpen() const override { return true; }

        bool Read(void* buffer, std::size_t numBytes) override
        {
            if (_pos + numBytes > _data.size())
                return false;

            if (numBytes)
                std::memcpy(buffer, _data.data() + _pos, numBytes);

            _pos += numBytes;
            return true;
        }

        int64 GetPosition() const override { return int64(_pos); }
        bool SetPosition(int64 position) override { _pos = std::size_t(position); return true; }
        int64 GetFileSize() const override { return int64(_data.size()); }
        char const* GetFileName() const override { return "memory.db2"; }

        DB2EncryptedSectionHandling HandleEncryptedSection(DB2SectionHeader const& /*sectionHeader*/) const override
        {
            return DB2EncryptedSectionHandling::Skip;
        }

    private:
        std::vector<uint8> _data;
        std::size_t _pos;
    };

    // Builds the on-disk byte image of a minimal WDC4 header. The struct is
    // packed (#pragma pack(push, 1)) so its memory layout matches the file
    // layout the loader reads.
    std::vector<uint8> MakeHeaderBlob(DB2Header const& header)
    {
        std::vector<uint8> blob(sizeof(DB2Header));
        std::memcpy(blob.data(), &header, sizeof(DB2Header));
        return blob;
    }

    DB2Header MakeMinimalValidHeader()
    {
        DB2Header header{};
        header.Signature = 0x34434457;  // 'WDC4' (little-endian on disk)
        header.RecordCount = 3;
        header.FieldCount = 0;          // no in-file field metadata to read
        header.TotalFieldCount = 7;
        header.TableHash = 0xDEADBEEF;
        header.LayoutHash = 0x12345678;
        header.Flags = 0;              // regular (non-sparse) record layout
        header.SectionCount = 0;       // no section headers follow
        header.ColumnMetaSize = 0;     // no column metadata follows
        header.ParentLookupCount = 0;
        header.MinId = 0;
        header.MaxId = 0;
        return header;
    }
}

// Header parser smoke/contract test: feed a hand-built WDC4 header and confirm
// the loader accepts it and reports the expected record/field counts and hashes.
TEST(DB2FileLoaderTest, ParsesWdc4HeaderRecordAndFieldCounts)
{
    MemoryDB2Source source(MakeHeaderBlob(MakeMinimalValidHeader()));

    DB2FileLoader loader;
    ASSERT_NO_THROW(loader.LoadHeaders(&source, nullptr));

    EXPECT_EQ(loader.GetRecordCount(), 3u);
    EXPECT_EQ(loader.GetCols(), 7u);
    EXPECT_EQ(loader.GetTableHash(), 0xDEADBEEFu);
    EXPECT_EQ(loader.GetLayoutHash(), 0x12345678u);
    EXPECT_EQ(loader.GetRecordCopyCount(), 0u);
}

TEST(DB2FileLoaderTest, RejectsWrongSignature)
{
    DB2Header header = MakeMinimalValidHeader();
    header.Signature = 0x32434457;  // 'WDC2' - not supported

    MemoryDB2Source source(MakeHeaderBlob(header));

    DB2FileLoader loader;
    EXPECT_THROW(loader.LoadHeaders(&source, nullptr), DB2FileLoadException);
}

// Full record/field *data* decoding (bit-packed columns, pallet/common-data
// compression, sparse catalogs, string tables) cannot be validated with a
// hand-built fixture confidently. That coverage comes from extracting real
// client DB2 files via the CASC extractor in Task 0.4.
TEST(DB2FileLoaderTest, DISABLED_DecodesRealClientRecords)
{
    GTEST_SKIP() << "Pending real client DB2 fixtures from Task 0.4 (CASC extraction).";
}
