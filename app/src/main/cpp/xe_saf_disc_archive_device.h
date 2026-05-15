/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */
#ifndef AX360E_XE_SAF_DISC_ARCHIVE_DEVICE_H
#define AX360E_XE_SAF_DISC_ARCHIVE_DEVICE_H

#include <memory>
#include <string>

#include "xenia/base/mapped_memory.h"
#include "xenia/vfs/device.h"

#include "third_party/zarchive/include/zarchive/zarchivereader.h"

#include "document_file.h"
namespace xe {
    namespace vfs {

        /*const fourcc_t kZarMagic = make_fourcc((_ZARCHIVE::Footer::kMagic >> 24 & 0xFF),
                                               (_ZARCHIVE::Footer::kMagic >> 16 & 0xFF),
                                               (_ZARCHIVE::Footer::kMagic >> 8 & 0xFF),
                                               (_ZARCHIVE::Footer::kMagic & 0xFF));*/

        class SAF_DiscZarchiveEntry;

        class SAF_DiscZarchiveDevice : public Device {
        public:
            SAF_DiscZarchiveDevice(const std::string_view mount_path,
                                   std::unique_ptr<DocumentFile> path);
            ~SAF_DiscZarchiveDevice() override;

            bool Initialize() override;
            void Dump(StringBuffer* string_buffer) override;
            Entry* ResolvePath(const std::string_view path) override;

            const std::string& name() const override { return name_; }
            uint32_t attributes() const override { return 0; }
            uint32_t component_name_max_length() const override { return 255; }

            uint32_t total_allocation_units() const override { return 128 * 1024; }
            uint32_t available_allocation_units() const override { return 0; }
            uint32_t sectors_per_allocation_unit() const override { return 1; }
            uint32_t bytes_per_sector() const override { return 0x200; }

            ZArchiveReader* reader() const { return reader_.get(); }

        private:
            bool ReadAllEntries(const std::string& path, SAF_DiscZarchiveEntry* node,
                                SAF_DiscZarchiveEntry* parent);

            std::string name_;
            std::unique_ptr<DocumentFile> saf_path_;
            std::unique_ptr<Entry> root_entry_;
            std::unique_ptr<ZArchiveReader> reader_;
        };

    }  // namespace vfs
}  // namespace xe

#endif //AX360E_XE_SAF_DISC_ARCHIVE_DEVICE_H
