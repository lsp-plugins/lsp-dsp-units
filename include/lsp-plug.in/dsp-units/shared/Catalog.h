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
                typedef struct Record
                {
                    uint32_t        index;              // Record index
                    uint32_t        magic;              // Record type
                    LSPString       name;               // Name of the record
                    LSPString       id;                 // Shared segment identifier of the record
                } Record;

            protected:
                typedef struct sh_header_t
                {
                    uint32_t            nMagic;         // Magic number
                    uint32_t            nVersion;       // Version of the catalog
                    uint32_t            nSize;          // Number of records
                    uint32_t            nAllocated;     // Number of allocated records
                    volatile uint32_t   nChanges;       // Number of changes
                } sh_header_t;

                typedef struct sh_record_t
                {
                    uint32_t            nMagic;         // Record type
                    uint32_t            nReplacement;   // Replacement index
                    uint32_t            nReferences;    // Number of references
                    uint32_t            nReserved;      // Reserved: padding
                    char                sName[64];      // Unique name of the record
                    char                sId[64];        // The identifier of associated shared segment
                } sh_record_t;

            protected:
                ipc::SharedMutex    hMutex;             // Shared mutex for access
                ipc::SharedMem      hMem;               // Shared memory descriptor
                sh_header_t        *pHeader;            // Header of the shared buffer
                sh_record_t        *vRecords;           // Records stored in catalog
                uint32_t            nChanges;           // Number of changes

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
                 * @return status of operation
                 */
                status_t        open(const char *id);

                /**
                 * Open or create shared catalog
                 * @param id the name of the catalog
                 * @return status of operation
                 */
                status_t        open(const LSPString *id);

                /**
                 * Close catalog
                 * @return status of operation
                 */
                status_t        close();

                /**
                 * Perform lock-free check that there are pending changes available in the catalog
                 * @return true if there are pending changes available in the catalog
                 */
                bool            sync();

            public:
                /**
                 * Create catalog record. If record already exists, it will be replaced.
                 * @param magic record type
                 * @param name unique record name (UTF-8 encoded string)
                 * @param id associated shared segment identifier (UTF-8 encoded string)
                 * @param lock lock the record flag
                 * @return record index or negative error code
                 */
                ssize_t         create(uint32_t magic, const char *name, const char *id, bool lock = true);

                /**
                 * Create catalog record. If record already exists, it will be replaced.
                 * @param magic record type
                 * @param name unique record name
                 * @param id associated shared segment identifier
                 * @param lock lock the record flag
                 * @return record index or negative error code
                 */
                ssize_t         create(uint32_t magic, const LSPString *name, const LSPString *id, bool lock = true);

                /**
                 * Read record from catalog
                 * @param record ponter to store result
                 * @param index index of the record
                 * @return status of operation
                 */
                status_t        get(Record *record, uint32_t index);

                /**
                 * Read record from catalog by unique name
                 * @param record ponter to store result
                 * @param name unique name of the record (UTF-8 encoded string)
                 * @return status of operation
                 */
                status_t        get(Record *record, const char *name);

                /**
                 * Read record from catalog by unique name
                 * @param record ponter to store result
                 * @param name unique name of the record
                 * @return status of operation
                 */
                status_t        get(Record *record, const LSPString *name);

                /**
                 * Enumerate all records in catalog
                 * @param result collection to store result, caller is responsible for deleting items
                 * @return status of operation
                 */
                status_t        enumerate(lltl::parray<Record> *result);

                /**
                 * Enumerate all records in catalog with specific type
                 * @param result collection to store result, caller is responsible for deleting items
                 * @param magic record type
                 * @return status of operation
                 */
                status_t        enumerate(lltl::parray<Record> *result, uint32_t magic);

                /**
                 * Lock record by it's index
                 * @param index the index of the record
                 * @return status of operation
                 */
                status_t        lock(size_t index);

                /**
                 * Lock record by it's unique name
                 * @param name name of the record (UTF-8 encoded string)
                 * @return status of operation
                 */
                status_t        lock(const char *name);

                /**
                 * Lock record by it's unique name
                 * @param name name of the record
                 * @return status of operation
                 */
                status_t        lock(const LSPString *name);

                /**
                 * Unlock record by it's index
                 * @param index the index of the record
                 * @return status of operation
                 */
                status_t        unlock(size_t index);

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
