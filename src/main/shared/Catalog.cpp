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

#include <lsp-plug.in/dsp-units/shared/Catalog.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/endian.h>
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/stdlib/string.h>

namespace lsp
{
    namespace dspu
    {
        static constexpr uint32_t catalog_magic = 0x53434154; // "SCAT"

        Catalog::Catalog()
        {
            pHeader     = NULL;
            vRecords    = NULL;
            nChanges    = 0;
        }

        Catalog::~Catalog()
        {
            close();
        }

        void Catalog::cleanup(lltl::parray<Record> *items)
        {
            if (items == NULL)
                return;

            for (lltl::iterator<Record> it=items->values(); it; ++it)
            {
                Record *r = it.get();
                if (r != NULL)
                    delete r;
            }

            items->flush();
        }


        status_t Catalog::open(const char *id, size_t entries)
        {
            if (pHeader != NULL)
                return STATUS_OPENED;

            LSPString tmp;
            if (!tmp.set_utf8(id))
                return STATUS_NO_MEM;
            return open(&tmp, entries);
        }

        status_t Catalog::open(const LSPString *id, size_t entries)
        {
            if (pHeader != NULL)
                return STATUS_OPENED;

            status_t res = STATUS_OK;
            lsp_finally {
                if (res != STATUS_OK)
                    close();
            };

            // Create shated mutex
            LSPString name;
            if (!name.set(id))
                return res = STATUS_NO_MEM;
            if (!name.append_ascii(".lock"))
                return res = STATUS_NO_MEM;
            if ((res = hMutex.open(&name)) != STATUS_OK)
                return res;

            // Lock the mutex
            if ((res = hMutex.lock()) != STATUS_OK)
                return res;
            lsp_finally {
                hMutex.unlock();
            };

            // Try to create or open new shared memory segment
            if (!name.set(id))
                return res = STATUS_NO_MEM;
            if (!name.append_ascii(".shm"))
                return res = STATUS_NO_MEM;

            res = create_catalog(&name, entries);
            if (res == STATUS_ALREADY_EXISTS)
                res = open_catalog(&name);

            return res;
        }

        status_t Catalog::create_catalog(const LSPString *name, size_t entries)
        {
            // Try to create new shared segment
            const size_t page_size  = system::page_size();
            const size_t hdr_size   = align_size(sizeof(sh_header_t), page_size);
            const size_t recs_size  = align_size(entries * sizeof(sh_record_t), page_size);
            const size_t total_size = hdr_size + recs_size;

            status_t res = hMem.open(name, ipc::SharedMem::SHM_RW | ipc::SharedMem::SHM_CREATE | ipc::SharedMem::SHM_PERSIST, total_size);
            if (res != STATUS_OK)
                return res;

            // OK, now map it
            if ((res = hMem.map(0, total_size)) != STATUS_OK)
                return res;

            uint8_t *ptr            = reinterpret_cast<uint8_t *>(hMem.data());
            if (ptr == NULL)
                return STATUS_UNKNOWN_ERR;

            // Initialize catalog
            pHeader                 = advance_ptr_bytes<sh_header_t>(ptr, hdr_size);
            vRecords                = advance_ptr_bytes<sh_record_t>(ptr, recs_size);
            nChanges                = 0;

            pHeader->nMagic         = BE_TO_CPU(catalog_magic);
            pHeader->nVersion       = 1;
            pHeader->nSize          = entries;
            pHeader->nAllocated     = 0;
            pHeader->nChanges       = nChanges;

            // Initialize records
            bzero(vRecords, recs_size);

            return STATUS_OK;
        }

        status_t Catalog::open_catalog(const LSPString *name)
        {
            status_t res = hMem.open(name, ipc::SharedMem::SHM_RW | ipc::SharedMem::SHM_PERSIST, 0);
            if (res != STATUS_OK)
                return res;

            // Map header only
            if ((res = hMem.map(0, sizeof(sh_header_t))) != STATUS_OK)
                return res;
            sh_header_t *hdr        = reinterpret_cast<sh_header_t *>(hMem.data());

            const uint32_t magic    = CPU_TO_BE(hdr->nMagic);
            if (magic != catalog_magic)
                return STATUS_BAD_FORMAT;
            if (hdr->nVersion != 1)
                return STATUS_UNSUPPORTED_FORMAT;

            const size_t page_size  = system::page_size();
            const size_t hdr_size   = align_size(sizeof(sh_header_t), page_size);
            const size_t recs_size  = align_size(hdr->nSize * sizeof(sh_record_t), page_size);
            const size_t total_size = hdr_size + recs_size;

            // Remap header
            if ((res = hMem.map(0, total_size)) != STATUS_OK)
                return res;

            uint8_t *ptr            = reinterpret_cast<uint8_t *>(hMem.data());
            if (ptr == NULL)
                return STATUS_UNKNOWN_ERR;

            // Initialize catalog
            pHeader                 = advance_ptr_bytes<sh_header_t>(ptr, hdr_size);
            vRecords                = advance_ptr_bytes<sh_record_t>(ptr, recs_size);
            nChanges                = pHeader->nChanges;

            return STATUS_OK;
        }

        status_t Catalog::close()
        {
            status_t res    = hMem.close();
            res             = update_status(res, hMutex.close());

            pHeader         = NULL;
            vRecords        = 0;
            nChanges        = 0;

            return res;
        }

        bool Catalog::sync()
        {
            if (pHeader == NULL)
                return false;

            uint32_t changes = pHeader->nChanges;
            if (nChanges == changes)
                return false;

            nChanges        = changes;
            return true;
        }

        bool Catalog::changed() const
        {
            if (pHeader == NULL)
                return false;

            uint32_t changes = pHeader->nChanges;
            return nChanges != changes;
        }

        size_t Catalog::capacity() const
        {
            return (pHeader != NULL) ? pHeader->nSize : 0;
        }

        size_t Catalog::size() const
        {
            return (pHeader != NULL) ? pHeader->nAllocated: 0;
        }

        bool Catalog::opened() const
        {
            return pHeader != NULL;
        }

        ssize_t Catalog::find_empty() const
        {
            size_t count = pHeader->nSize;
            if (pHeader->nAllocated >= count)
                return -STATUS_NO_MEM;

            for (size_t i=0; i<count; ++i)
            {
                const sh_record_t *rec = &vRecords[i];
                if (rec->nMagic == 0)
                {
                    // Validate state of record
                    if (rec->sName[0] != '\0')
                        return -STATUS_CORRUPTED;
                    if (rec->sId[0] != '\0')
                        return -STATUS_CORRUPTED;
                    return i;
                }
            }

            return -STATUS_CORRUPTED;
        }

        ssize_t Catalog::find_by_name(uint32_t hash, const char *name, size_t len) const
        {
            size_t count = pHeader->nSize;
            size_t allocated = pHeader->nAllocated;

            if (allocated >= count)
                return -STATUS_NO_MEM;

            for (size_t i=0, found=0; (i<count) && (found < allocated); ++i)
            {
                const sh_record_t *rec = &vRecords[i];
                if (rec->nMagic == 0)
                    continue;
                ++found;

                if (rec->nHash != hash)
                    continue;
                if (str_equals(name, len, rec->sName, NAME_BYTES))
                    return i;
            }

            return -STATUS_NOT_FOUND;
        }

        void Catalog::mark_changed()
        {
            uint32_t changes    = pHeader->nChanges;
            pHeader->nChanges   = changes + 1;
        }

        ssize_t Catalog::publish(Record *record, uint32_t magic, const char *name, const char *id)
        {
            if (pHeader == NULL)
                return -STATUS_CLOSED;

            // Validate arguments
            if ((name == NULL) || (id == NULL) || (magic == 0))
                return -STATUS_BAD_ARGUMENTS;

            const size_t name_len = strlen(name);
            if (name_len > NAME_BYTES)
                return -STATUS_TOO_BIG;
            else if (name_len < 1)
                return -STATUS_BAD_ARGUMENTS;

            const size_t id_len = strlen(id);
            if (id_len > ID_BYTES)
                return -STATUS_TOO_BIG;
            else if (id_len < 1)
                return -STATUS_BAD_ARGUMENTS;

            const uint32_t hash = str_hash(name, name_len);

            // Lock the mutex
            status_t res = hMutex.lock();
            if (res != STATUS_OK)
                return -res;
            lsp_finally {
                hMutex.unlock();
            };

            // Now we are ready to perform lookup
            ssize_t index       = find_by_name(hash, name, name_len);
            sh_record_t *rec    = &vRecords[index];
            if (index < 0)
            {
                if (index != -STATUS_NOT_FOUND)
                    return index;
                if ((index = find_empty()) < 0)
                    return index;

                // Create new record
                ++pHeader->nAllocated;
                rec                 = &vRecords[index];
                rec->nHash          = hash;
                str_copy(rec->sName, NAME_BYTES, name, name_len);
            }

            // Update other fields
            rec->nMagic         = magic;
            str_copy(rec->sId, ID_BYTES, id, id_len);
            ++rec->nVersion;

            // Mark catalog as changed
            mark_changed();

            // Fill result
            if (record != NULL)
                fill_record(record, rec);

            return index;
        }

        ssize_t Catalog::publish(Record *record, uint32_t magic, const LSPString *name, const LSPString *id)
        {
            if (pHeader == NULL)
                return -STATUS_CLOSED;

            if ((name == NULL) || (id == NULL) || (magic == 0))
                return -STATUS_BAD_ARGUMENTS;

            return publish(record, magic, name->get_utf8(), id->get_utf8());
        }

        status_t Catalog::get(Record *record, uint32_t index) const
        {
            if (pHeader == NULL)
                return STATUS_CLOSED;
            if (index >= pHeader->nSize)
                return STATUS_OVERFLOW;

            // Lock the mutex
            status_t res = hMutex.lock();
            if (res != STATUS_OK)
                return -res;
            lsp_finally {
                hMutex.unlock();
            };

            // Check that record is valid
            const sh_record_t *rec = &vRecords[index];
            if (rec->nMagic == 0)
                return STATUS_NOT_FOUND;

            if (record != NULL)
            {
                Record tmp;
                tmp.index       = index;
                if ((res = fill_record(&tmp, rec)) != STATUS_OK)
                    return res;

                commit_record(record, &tmp);
            }

            return STATUS_OK;
        }

        bool Catalog::validate(const Record *record) const
        {
            if ((record == NULL) || (record->magic == 0))
                return false;
            if (pHeader == NULL)
                return false;
            if (record->index >= pHeader->nSize)
                return false;

            // Lock the mutex
            status_t res = hMutex.lock();
            if (res != STATUS_OK)
                return -res;
            lsp_finally {
                hMutex.unlock();
            };

            // Check that record is valid
            const sh_record_t *rec = &vRecords[record->index];
            if (rec->nMagic != record->magic)
                return false;
            if (rec->nVersion != record->version)
                return false;

            LSPString tmp;
            if (!tmp.set_utf8(rec->sName, strnlen(rec->sName, NAME_BYTES)))
                return false;
            if (!record->name.equals(&tmp))
                return false;

            if (!tmp.set_utf8(rec->sId, strnlen(rec->sId, ID_BYTES)))
                return false;
            if (!record->name.equals(&tmp))
                return false;

            return true;
        }

        status_t Catalog::get(Record *record, const char *name) const
        {
            if (pHeader == NULL)
                return STATUS_CLOSED;

            // Validate arguments
            if (name == NULL)
                return STATUS_BAD_ARGUMENTS;
            const size_t name_len  = strlen(name);
            if (name_len > NAME_BYTES)
                return -STATUS_TOO_BIG;
            else if (name_len < 1)
                return -STATUS_BAD_ARGUMENTS;

            const uint32_t hash = str_hash(name, name_len);

            // Lock the mutex
            status_t res = hMutex.lock();
            if (res != STATUS_OK)
                return -res;
            lsp_finally {
                hMutex.unlock();
            };

            // Find record
            ssize_t index = find_by_name(hash, name, name_len);
            if (index < 0)
                return -index;

            // Check that we need to return value
            if (record != NULL)
            {
                const sh_record_t *rec = &vRecords[index];

                Record tmp;
                tmp.index       = index;
                if ((res = fill_record(&tmp, rec)) != STATUS_OK)
                    return res;

                commit_record(record, &tmp);
            }

            return STATUS_OK;
        }

        status_t Catalog::get(Record *record, const LSPString *name) const
        {
            if (pHeader == NULL)
                return STATUS_CLOSED;
            return get(record, name->get_utf8());
        }

        status_t Catalog::revoke(size_t index, uint32_t version)
        {
            if (pHeader == NULL)
                return STATUS_CLOSED;
            if (index >= pHeader->nSize)
                return STATUS_OVERFLOW;

            // Lock the mutex
            status_t res = hMutex.lock();
            if (res != STATUS_OK)
                return -res;
            lsp_finally {
                hMutex.unlock();
            };

            // Check that record is valid
            sh_record_t *rec = &vRecords[index];
            if (rec->nMagic == 0)
                return STATUS_NOT_FOUND;
            if (rec->nVersion != version)
                return STATUS_NOT_FOUND;

            rec->nMagic         = 0;
            rec->nHash          = 0;
            ++rec->nVersion;
            rec->nReserved      = 0;

            bzero(rec->sName, NAME_BYTES);
            bzero(rec->sId, ID_BYTES);

            --pHeader->nAllocated;

            mark_changed();

            return STATUS_OK;
        }

        status_t Catalog::enumerate(lltl::parray<Record> *result, uint32_t magic)
        {
            if (pHeader == NULL)
                return STATUS_CLOSED;
            if (result == NULL)
                return STATUS_BAD_ARGUMENTS;

            // Result
            lltl::parray<Record> items;
            lsp_finally {
                cleanup(&items);
            };

            // Lock the mutex
            status_t res = hMutex.lock();
            if (res != STATUS_OK)
                return -res;
            lsp_finally {
                hMutex.unlock();
            };

            const size_t allocated = pHeader->nAllocated;
            const size_t count = pHeader->nSize;

            for (size_t i=0, found=0; (i<count) && (found < allocated); ++i)
            {
                sh_record_t *rec = &vRecords[i];
                if (rec->nMagic == 0)
                    continue;

                ++found;
                if ((magic != 0) && (magic != rec->nMagic))
                    continue;

                // Allocate new record
                Record *dst = new Record;
                if (dst == NULL)
                    return STATUS_NO_MEM;
                if (!items.add(dst))
                {
                    delete dst;
                    return STATUS_NO_MEM;
                }

                // Fill record
                dst->index      = i;
                if ((res = fill_record(dst, rec)) != STATUS_OK)
                    return res;
            }

            // Commit result
            items.swap(result);

            return STATUS_OK;
        }

        status_t Catalog::fill_record(Record *dst, const sh_record_t *src)
        {
            dst->magic      = src->nMagic;
            dst->version    = src->nVersion;

            size_t name_len = strnlen(src->sName, NAME_BYTES);
            size_t id_len   = strnlen(src->sId, ID_BYTES);

            if (!dst->name.set_utf8(src->sName, name_len))
                return STATUS_NO_MEM;
            if (!dst->id.set_utf8(src->sId, id_len))
                return STATUS_NO_MEM;

            return STATUS_OK;
        }

        bool Catalog::str_equals(const char *var, size_t var_len, const char *fixed, size_t fixed_len)
        {
            if (var_len > fixed_len)
                return false;

            if ((memcmp(var, fixed, var_len)) != 0)
                return false;

            return (var_len < fixed_len) ? fixed[var_len] == '\0' : true;
        }

        bool Catalog::str_copy(char *fixed, size_t fixed_len, const char *var, size_t var_len)
        {
            if (var_len > fixed_len)
                return false;

            memcpy(fixed, var, var_len);
            bzero(&fixed[var_len], fixed_len - var_len);
            return true;
        }

        uint32_t Catalog::str_hash(const char *var, size_t len)
        {
            uint32_t res = len * 1021;
            for (size_t i=0; i<len; ++i)
            {
                uint64_t tmp    = uint64_t(res) * 97 + var[i];
                res             = uint32_t(tmp) ^ uint32_t(tmp >> 32);
            }
            return res;
        }

        void Catalog::commit_record(Record *dst, Record *src)
        {
            dst->index = src->index;
            dst->magic = src->magic;
            dst->version = src->version;
            dst->name.swap(src->name);
            dst->id.swap(src->id);
        }

    } /* namespace dspu */
} /* namespace lsp */


