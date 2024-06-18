/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 15 мая 2024 г.
 *
 * lsp-dsp-units is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-dsp-units is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-dsp-units. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_DSP_UNITS_SHARED_CATALOG_H_
#define LSP_PLUG_IN_DSP_UNITS_SHARED_CATALOG_H_

#include <lsp-plug.in/dsp-units/version.h>

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/ipc/SharedMem.h>
#include <lsp-plug.in/ipc/SharedMutex.h>
#include <lsp-plug.in/runtime/LSPString.h>

namespace lsp
{
    namespace dspu
    {
        /**
         * Shared catalog for registering resources
         */
        class LSP_DSP_UNITS_PUBLIC Catalog
        {
            public:
                static constexpr size_t NAME_BYTES      = 64;
                static constexpr size_t ID_BYTES        = 64;

                typedef struct Record
                {
                    uint32_t        index;                  // Record index
                    uint32_t        magic;                  // Record type
                    uint32_t        version;                // Version of the record
                    LSPString       name;                   // Name of the record
                    LSPString       id;                     // Shared segment identifier of the record
                } Record;

            protected:
                typedef struct sh_header_t
                {
                    uint32_t            nMagic;             // Magic number
                    uint32_t            nVersion;           // Version of the catalog
                    uint32_t            nSize;              // Number of records
                    uint32_t            nAllocated;         // Number of allocated records
                    volatile uint32_t   nChanges;           // Number of changes
                } sh_header_t;

                typedef struct sh_record_t
                {
                    uint32_t            nMagic;             // Record type
                    uint32_t            nHash;              // Name hash
                    uint32_t            nVersion;           // Version of the record
                    uint32_t            nReserved;          // Reserved data
                    char                sName[NAME_BYTES];  // Unique name of the record
                    char                sId[ID_BYTES];      // The identifier of associated shared segment
                } sh_record_t;

            protected:
                mutable ipc::SharedMutex    hMutex;         // Shared mutex for access
                ipc::SharedMem              hMem;           // Shared memory descriptor
                sh_header_t                *pHeader;        // Header of the shared buffer
                sh_record_t                *vRecords;       // Records stored in catalog
                uint32_t                    nChanges;       // Number of changes

            protected:
                static bool         str_equals(const char *var, size_t var_len, const char *fixed, size_t fixed_len);
                static bool         str_copy(char *fixed, size_t fixed_len, const char *var, size_t var_len);
                static uint32_t     str_hash(const char *var, size_t len);
                static void         commit_record(Record *dst, Record *src);

                static status_t     fill_record(Record *dst, const sh_record_t *src);

                status_t            create_catalog(const LSPString *name, size_t entries);
                status_t            open_catalog(const LSPString *name);
                ssize_t             find_empty() const;
                ssize_t             find_by_name(uint32_t hash, const char *name, size_t len) const;
                void                mark_changed();

            public:
                Catalog();
                Catalog(const Catalog &) = delete;
                Catalog(Catalog &&) = delete;
                ~Catalog();

                Catalog & operator = (const Catalog &) = delete;
                Catalog & operator = (Catalog &&) = delete;

            public:
                /**
                 * Open or create shared catalog
                 * @param id the name of the catalog
                 * @param entries number of entries if catalog is created
                 * @return status of operation
                 */
                status_t        open(const char *id, size_t entries);

                /**
                 * Open or create shared catalog
                 * @param id the name of the catalog
                 * @param entries number of entries if catalog is created
                 * @return status of operation
                 */
                status_t        open(const LSPString *id, size_t entries);

                /**
                 * Close catalog
                 * @return status of operation
                 */
                status_t        close();

                /**
                 * Perform lock-free check that there are pending changes available in the catalog
                 * and mark catalog as synchronized.
                 * @return true if there are pending changes available in the catalog
                 */
                bool            sync();

                /**
                 * Perform lock-free check that there are pending changes available in the catalog
                 * @return true if there are pending changes available in the catalog
                 */
                bool            changed() const;

                /**
                 * Return the capacity of catalog
                 * @return capacity of catalog
                 */
                size_t          capacity() const;

                /**
                 * Return number of allocated items
                 * @return number of allocated items
                 */
                size_t          size() const;

                /**
                 * Ensure that catalog is opened
                 * @return true if catalog is opened
                 */
                bool            opened() const;

            public:
                /**
                 * Create catalog record. If record already exists, it will be replaced.
                 * @param magic record type
                 * @param name unique record name (UTF-8 encoded string)
                 * @param id associated shared segment identifier (UTF-8 encoded string)
                 * @return record index or negative error code
                 */
                ssize_t         publish(uint32_t magic, const char *name, const char *id);

                /**
                 * Create catalog record. If record already exists, it will be replaced.
                 * @param magic record type
                 * @param name unique record name
                 * @param id associated shared segment identifier
                 * @param lock lock the record flag
                 * @return record index or negative error code
                 */
                ssize_t         publish(uint32_t magic, const LSPString *name, const LSPString *id);

                /**
                 * Read record from catalog
                 * @param record ponter to store result
                 * @param index index of the record
                 * @return status of operation
                 */
                status_t        get(Record *record, uint32_t index) const;

                /**
                 * Read record from catalog by unique name
                 * @param record ponter to store result
                 * @param name unique name of the record (UTF-8 encoded string)
                 * @return status of operation
                 */
                status_t        get(Record *record, const char *name) const;

                /**
                 * Read record from catalog by unique name
                 * @param record ponter to store result
                 * @param name unique name of the record
                 * @return status of operation
                 */
                status_t        get(Record *record, const LSPString *name) const;

                /**
                 * Erase record with specified index and version
                 * @param index index of the record
                 * @param version version of the record to erase
                 * @return status of operation
                 */
                status_t        revoke(size_t index, uint32_t version);

                /**
                 * Enumerate all records in catalog with specific type
                 * @param result collection to store result, caller is responsible for deleting items
                 * @param magic record type
                 * @return status of operation
                 */
                status_t        enumerate(lltl::parray<Record> *result, uint32_t magic = 0);

            public:
                /**
                 * Cleanup the result returned by the enumerate() calls
                 * @param items collection containing result
                 */
                static void     cleanup(lltl::parray<Record> *items);
        };

    } /* namespace dspu */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_DSP_UNITS_SHARED_CATALOG_H_ */
