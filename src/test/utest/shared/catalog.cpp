/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-units
 * Created on: 17 мая 2024 г.
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

#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/shared/Catalog.h>
#include <lsp-plug.in/test-fw/utest.h>

UTEST_BEGIN("dspu.shared", catalog)
    UTEST_TIMELIMIT(30)

    UTEST_MAIN
    {
        LSPString id;
        dspu::Catalog cat;

        UTEST_ASSERT(id.fmt_utf8("%s-cat", full_name()));

        printf("Testing Catalog single use: %s...\n", id.get_native());

        // Open and close catalog
        UTEST_ASSERT(cat.open(&id, 16) == STATUS_OK);
        UTEST_ASSERT(!cat.changed());
        UTEST_ASSERT(cat.close() == STATUS_OK);

        // Open catalog again
        UTEST_ASSERT(cat.open(&id, 0) == STATUS_OK);
        UTEST_ASSERT(cat.capacity() == 16);
        UTEST_ASSERT(cat.size() == 0);

        UTEST_ASSERT(cat.publish(NULL, 0, "test", "test") < 0);
        UTEST_ASSERT(cat.publish(NULL, 0x11223344, "", "test") < 0);
        UTEST_ASSERT(cat.publish(NULL, 0x11223344, NULL, "test") < 0);
        UTEST_ASSERT(cat.publish(NULL, 0x11223344, "test", "") < 0);
        UTEST_ASSERT(cat.publish(NULL, 0x11223344, "test", NULL) < 0);
        UTEST_ASSERT(cat.publish(NULL, 0x11223344, "test", "test.shm") >= 0);
        UTEST_ASSERT(cat.publish(NULL, 0x11223344, "0123456789012345678901234567890123456789012345678901234567890123456789", "test2.shm") < 0);
        UTEST_ASSERT(cat.publish(NULL, 0x11223344, "test2", "0123456789012345678901234567890123456789012345678901234567890123456789") < 0);
        UTEST_ASSERT(cat.publish(NULL, 0x22334455, "test2", "test2.shm") >= 0);

        UTEST_ASSERT(cat.changed());
        UTEST_ASSERT(cat.sync());
        UTEST_ASSERT(!cat.changed());

        // Check size
        UTEST_ASSERT(cat.capacity() == 16);
        UTEST_ASSERT(cat.size() == 2);
        UTEST_ASSERT(!cat.changed());

        // Find the record
        dspu::Catalog::Record rec1;
        UTEST_ASSERT(cat.get(&rec1, "test") == STATUS_OK);
        UTEST_ASSERT(rec1.magic == 0x11223344);
        UTEST_ASSERT(rec1.name.equals_ascii("test"));
        UTEST_ASSERT(rec1.id.equals_ascii("test.shm"));
        UTEST_ASSERT(!cat.changed());

        // Find the record
        dspu::Catalog::Record rec2;
        UTEST_ASSERT(cat.get(&rec2, "test2") == STATUS_OK);
        UTEST_ASSERT(rec2.magic == 0x22334455);
        UTEST_ASSERT(rec2.name.equals_ascii("test2"));
        UTEST_ASSERT(rec2.id.equals_ascii("test2.shm"));
        UTEST_ASSERT(!cat.changed());

        // Get non-existing record
        dspu::Catalog::Record rec3;
        UTEST_ASSERT(cat.get(&rec3, "test3") == STATUS_NOT_FOUND);
        UTEST_ASSERT(!cat.changed());

        // Update record
        UTEST_ASSERT(cat.publish(NULL, 0x33445566, "test", "another-segment.shm") == STATUS_OK);
        UTEST_ASSERT(cat.changed());
        UTEST_ASSERT(cat.sync());
        UTEST_ASSERT(!cat.changed());

        // Get changed record
        dspu::Catalog::Record rec4;
        UTEST_ASSERT(cat.get(&rec4, rec1.index) == STATUS_OK);
        UTEST_ASSERT(rec4.magic == 0x33445566);
        UTEST_ASSERT(rec4.name.equals_ascii("test"));
        UTEST_ASSERT(rec4.id.equals_ascii("another-segment.shm"));
        UTEST_ASSERT(rec4.version == uint32_t(rec1.version + 1));
        UTEST_ASSERT(!cat.changed());

        // Check size
        UTEST_ASSERT(cat.capacity() == 16);
        UTEST_ASSERT(cat.size() == 2);
        UTEST_ASSERT(!cat.changed());

        // Enumerate records
        lltl::parray<dspu::Catalog::Record> items;
        UTEST_ASSERT(cat.enumerate(&items) == STATUS_OK);
        UTEST_ASSERT(items.size() == 2);
        UTEST_ASSERT(!cat.changed());

        UTEST_ASSERT(cat.enumerate(&items, 0x11223344) == STATUS_OK);
        UTEST_ASSERT(items.size() == 0);
        UTEST_ASSERT(!cat.changed());

        UTEST_ASSERT(cat.enumerate(&items, 0x22334455) == STATUS_OK);
        UTEST_ASSERT(items.size() == 1);
        UTEST_ASSERT(!cat.changed());

        UTEST_ASSERT(cat.enumerate(&items, 0x33445566) == STATUS_OK);
        UTEST_ASSERT(items.size() == 1);
        UTEST_ASSERT(!cat.changed());

        dspu::Catalog::cleanup(&items);

        // Revoke records
        UTEST_ASSERT(cat.revoke(rec1.index, rec1.version) != STATUS_OK);
        UTEST_ASSERT(!cat.changed());

        UTEST_ASSERT(cat.revoke(rec4.index, rec4.version) == STATUS_OK);
        UTEST_ASSERT(cat.changed());
        UTEST_ASSERT(cat.sync());
        UTEST_ASSERT(!cat.changed());

        UTEST_ASSERT(cat.revoke(rec2.index, rec2.version) == STATUS_OK);
        UTEST_ASSERT(cat.changed());
        UTEST_ASSERT(cat.sync());
        UTEST_ASSERT(!cat.changed());

        // Check size
        UTEST_ASSERT(cat.capacity() == 16);
        UTEST_ASSERT(cat.size() == 0);
        UTEST_ASSERT(!cat.changed());

        // Publish record with long name and description and delete it
        UTEST_ASSERT(cat.publish(NULL, 0x12345678, "abcdefghijklmnopabcdefghijklmnopabcdefghijklmnopabcdefghijklmnop", "0123456789012345678901234567890123456789012345678901234567890123") >= 0);
        UTEST_ASSERT(cat.capacity() == 16);
        UTEST_ASSERT(cat.size() == 1);
        UTEST_ASSERT(cat.changed());
        UTEST_ASSERT(cat.sync());
        UTEST_ASSERT(!cat.changed());

        dspu::Catalog::Record rec5;
        UTEST_ASSERT(cat.get(&rec5, "abcdefghijklmnopabcdefghijklmnopabcdefghijklmnopabcdefghijklmnop") == STATUS_OK);
        UTEST_ASSERT(rec5.magic == 0x12345678);
        UTEST_ASSERT(rec5.name.equals_ascii("abcdefghijklmnopabcdefghijklmnopabcdefghijklmnopabcdefghijklmnop"));
        UTEST_ASSERT(rec5.id.equals_ascii("0123456789012345678901234567890123456789012345678901234567890123"));
        UTEST_ASSERT(!cat.changed());

        UTEST_ASSERT(cat.revoke(rec5.index, rec5.version) == STATUS_OK);
        UTEST_ASSERT(cat.changed());
        UTEST_ASSERT(cat.sync());
        UTEST_ASSERT(!cat.changed());

        // Close catalog
        cat.close();
    }
UTEST_END




