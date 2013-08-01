/*
 * Copyright 2011 André Hentschel
 * Copyright 2013 Mislav Blažević
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "apphelp.h"
#include "winternl.h"

#include "wine/debug.h"

#define STATUS_SUCCESS ((NTSTATUS)0)

WINE_DEFAULT_DEBUG_CHANNEL(apphelp);

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    TRACE("%p, %u, %p\n", hinst, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hinst );
            break;
    }
    return TRUE;
}

BOOL WINAPI ApphelpCheckInstallShieldPackage( void* ptr, LPCWSTR path )
{
    FIXME("stub: %p %s\n", ptr, debugstr_w(path));
    return TRUE;
}

BOOL WINAPI ApphelpCheckMsiPackage( void* ptr, LPCWSTR path )
{
    FIXME("stub: %p %s\n", ptr, debugstr_w(path));
    return TRUE;
}

PDB WINAPI SdbCreateDatabase( LPCWSTR path, PATH_TYPE type )
{
    FIXME("stub: %s %u\n", debugstr_w(path), type);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

BOOL WINAPI ApphelpCheckShellObject( REFCLSID clsid, BOOL shim, ULONGLONG *flags )
{
    TRACE("(%s, %d, %p)\n", debugstr_guid(clsid), shim, flags);
    if (flags) *flags = 0;
    return TRUE;
}

/**************************************************************************
 *        SdbCloseDatabase                [APPHELP.@]
 *
 * Closes specified database and frees its memory
 *
 * PARAMS
 *  db      [I] Handle to the shim database
 *
 * RETURNS
 *  This function does not return a value.
 */
void WINAPI SdbCloseDatabase(PDB db)
{
    NtClose(db->file);
    HeapFree(GetProcessHeap(), 0, db->data);
    HeapFree(GetProcessHeap(), 0, db);
}

/**************************************************************************
 *        SdbTagToString                [APPHELP.@]
 *
 * Converts specified tag into a string
 *
 * PARAMS
 *  tag      [I] The tag which will be converted to a string
 *
 * RETURNS
 *  Success: Pointer to the string matching specified tag
 *  Failure: L"InvalidTag" string
 */
LPCWSTR WINAPI SdbTagToString(TAG tag)
{
    /* lookup tables for tags in range 0x1 -> 0xFF | TYPE */
    static const WCHAR table[9][0x31][25] = {
    {   /* TAG_TYPE_NULL */
        {'I','N','C','L','U','D','E',0},
        {'G','E','N','E','R','A','L',0},
        {'M','A','T','C','H','_','L','O','G','I','C','_','N','O','T',0},
        {'A','P','P','L','Y','_','A','L','L','_','S','H','I','M','S',0},
        {'U','S','E','_','S','E','R','V','I','C','E','_','P','A','C','K','_','F','I','L','E','S',0},
        {'M','I','T','I','G','A','T','I','O','N','_','O','S',0},
        {'B','L','O','C','K','_','U','P','G','R','A','D','E',0},
        {'I','N','C','L','U','D','E','E','X','C','L','U','D','E','D','L','L',0},
        {'R','A','C','_','E','V','E','N','T','_','O','F','F',0},
        {'T','E','L','E','M','E','T','R','Y','_','O','F','F',0},
        {'S','H','I','M','_','E','N','G','I','N','E','_','O','F','F',0},
        {'L','A','Y','E','R','_','P','R','O','P','A','G','A','T','I','O','N','_','O','F','F',0},
        {'R','E','I','N','S','T','A','L','L','_','U','P','G','R','A','D','E',0}
    },
    {   /* TAG_TYPE_BYTE */
        {'I','n','v','a','l','i','d','T','a','g',0}
    },
    {   /* TAG_TYPE_WORD */
        {'M','A','T','C','H','_','M','O','D','E',0}
    },
    {   /* TAG_TYPE_DWORD */
        {'S','I','Z','E',0},
        {'O','F','F','S','E','T',0},
        {'C','H','E','C','K','S','U','M',0},
        {'S','H','I','M','_','T','A','G','I','D',0},
        {'P','A','T','C','H','_','T','A','G','I','D',0},
        {'M','O','D','U','L','E','_','T','Y','P','E',0},
        {'V','E','R','D','A','T','E','H','I',0},
        {'V','E','R','D','A','T','E','L','O',0},
        {'V','E','R','F','I','L','E','O','S',0},
        {'V','E','R','F','I','L','E','T','Y','P','E',0},
        {'P','E','_','C','H','E','C','K','S','U','M',0},
        {'P','R','E','V','O','S','M','A','J','O','R','V','E','R',0},
        {'P','R','E','V','O','S','M','I','N','O','R','V','E','R',0},
        {'P','R','E','V','O','S','P','L','A','T','F','O','R','M','I','D',0},
        {'P','R','E','V','O','S','B','U','I','L','D','N','O',0},
        {'P','R','O','B','L','E','M','S','E','V','E','R','I','T','Y',0},
        {'L','A','N','G','I','D',0},
        {'V','E','R','_','L','A','N','G','U','A','G','E',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'E','N','G','I','N','E',0},
        {'H','T','M','L','H','E','L','P','I','D',0},
        {'I','N','D','E','X','_','F','L','A','G','S',0},
        {'F','L','A','G','S',0},
        {'D','A','T','A','_','V','A','L','U','E','T','Y','P','E',0},
        {'D','A','T','A','_','D','W','O','R','D',0},
        {'L','A','Y','E','R','_','T','A','G','I','D',0},
        {'M','S','I','_','T','R','A','N','S','F','O','R','M','_','T','A','G','I','D',0},
        {'L','I','N','K','E','R','_','V','E','R','S','I','O','N',0},
        {'L','I','N','K','_','D','A','T','E',0},
        {'U','P','T','O','_','L','I','N','K','_','D','A','T','E',0},
        {'O','S','_','S','E','R','V','I','C','E','_','P','A','C','K',0},
        {'F','L','A','G','_','T','A','G','I','D',0},
        {'R','U','N','T','I','M','E','_','P','L','A','T','F','O','R','M',0},
        {'O','S','_','S','K','U',0},
        {'O','S','_','P','L','A','T','F','O','R','M',0},
        {'A','P','P','_','N','A','M','E','_','R','C','_','I','D',0},
        {'V','E','N','D','O','R','_','N','A','M','E','_','R','C','_','I','D',0},
        {'S','U','M','M','A','R','Y','_','M','S','G','_','R','C','_','I','D',0},
        {'V','I','S','T','A','_','S','K','U',0},
        {'D','E','S','C','R','I','P','T','I','O','N','_','R','C','_','I','D',0},
        {'P','A','R','A','M','E','T','E','R','1','_','R','C','_','I','D',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'C','O','N','T','E','X','T','_','T','A','G','I','D',0},
        {'E','X','E','_','W','R','A','P','P','E','R',0}
    },
    {   /* TAG_TYPE_QWORD */
        {'T','I','M','E',0},
        {'B','I','N','_','F','I','L','E','_','V','E','R','S','I','O','N',0},
        {'B','I','N','_','P','R','O','D','U','C','T','_','V','E','R','S','I','O','N',0},
        {'M','O','D','T','I','M','E',0},
        {'F','L','A','G','_','M','A','S','K','_','K','E','R','N','E','L',0},
        {'U','P','T','O','_','B','I','N','_','P','R','O','D','U','C','T','_','V','E','R','S','I','O','N',0},
        {'D','A','T','A','_','Q','W','O','R','D',0},
        {'F','L','A','G','_','M','A','S','K','_','U','S','E','R',0},
        {'F','L','A','G','S','_','N','T','V','D','M','1',0},
        {'F','L','A','G','S','_','N','T','V','D','M','2',0},
        {'F','L','A','G','S','_','N','T','V','D','M','3',0},
        {'F','L','A','G','_','M','A','S','K','_','S','H','E','L','L',0},
        {'U','P','T','O','_','B','I','N','_','F','I','L','E','_','V','E','R','S','I','O','N',0},
        {'F','L','A','G','_','M','A','S','K','_','F','U','S','I','O','N',0},
        {'F','L','A','G','_','P','R','O','C','E','S','S','P','A','R','A','M',0},
        {'F','L','A','G','_','L','U','A',0},
        {'F','L','A','G','_','I','N','S','T','A','L','L',0}
    },
    {   /* TAG_TYPE_STRINGREF */
        {'N','A','M','E',0},
        {'D','E','S','C','R','I','P','T','I','O','N',0},
        {'M','O','D','U','L','E',0},
        {'A','P','I',0},
        {'V','E','N','D','O','R',0},
        {'A','P','P','_','N','A','M','E',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'C','O','M','M','A','N','D','_','L','I','N','E',0},
        {'C','O','M','P','A','N','Y','_','N','A','M','E',0},
        {'D','L','L','F','I','L','E',0},
        {'W','I','L','D','C','A','R','D','_','N','A','M','E',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'P','R','O','D','U','C','T','_','N','A','M','E',0},
        {'P','R','O','D','U','C','T','_','V','E','R','S','I','O','N',0},
        {'F','I','L','E','_','D','E','S','C','R','I','P','T','I','O','N',0},
        {'F','I','L','E','_','V','E','R','S','I','O','N',0},
        {'O','R','I','G','I','N','A','L','_','F','I','L','E','N','A','M','E',0},
        {'I','N','T','E','R','N','A','L','_','N','A','M','E',0},
        {'L','E','G','A','L','_','C','O','P','Y','R','I','G','H','T',0},
        {'1','6','B','I','T','_','D','E','S','C','R','I','P','T','I','O','N',0},
        {'A','P','P','H','E','L','P','_','D','E','T','A','I','L','S',0},
        {'L','I','N','K','_','U','R','L',0},
        {'L','I','N','K','_','T','E','X','T',0},
        {'A','P','P','H','E','L','P','_','T','I','T','L','E',0},
        {'A','P','P','H','E','L','P','_','C','O','N','T','A','C','T',0},
        {'S','X','S','_','M','A','N','I','F','E','S','T',0},
        {'D','A','T','A','_','S','T','R','I','N','G',0},
        {'M','S','I','_','T','R','A','N','S','F','O','R','M','_','F','I','L','E',0},
        {'1','6','B','I','T','_','M','O','D','U','L','E','_','N','A','M','E',0},
        {'L','A','Y','E','R','_','D','I','S','P','L','A','Y','N','A','M','E',0},
        {'C','O','M','P','I','L','E','R','_','V','E','R','S','I','O','N',0},
        {'A','C','T','I','O','N','_','T','Y','P','E',0},
        {'E','X','P','O','R','T','_','N','A','M','E',0}
    },
    {   /* TAG_TYPE_LIST */
        {'D','A','T','A','B','A','S','E',0},
        {'L','I','B','R','A','R','Y',0},
        {'I','N','E','X','C','L','U','D','E',0},
        {'S','H','I','M',0},
        {'P','A','T','C','H',0},
        {'A','P','P',0},
        {'E','X','E',0},
        {'M','A','T','C','H','I','N','G','_','F','I','L','E',0},
        {'S','H','I','M','_','R','E','F',0},
        {'P','A','T','C','H','_','R','E','F',0},
        {'L','A','Y','E','R',0},
        {'F','I','L','E',0},
        {'A','P','P','H','E','L','P',0},
        {'L','I','N','K',0},
        {'D','A','T','A',0},
        {'M','S','I','_','T','R','A','N','S','F','O','R','M',0},
        {'M','S','I','_','T','R','A','N','S','F','O','R','M','_','R','E','F',0},
        {'M','S','I','_','P','A','C','K','A','G','E',0},
        {'F','L','A','G',0},
        {'M','S','I','_','C','U','S','T','O','M','_','A','C','T','I','O','N',0},
        {'F','L','A','G','_','R','E','F',0},
        {'A','C','T','I','O','N',0},
        {'L','O','O','K','U','P',0},
        {'C','O','N','T','E','X','T',0},
        {'C','O','N','T','E','X','T','_','R','E','F',0}
    },
    {   /* TAG_TYPE_STRING */
        {'I','n','v','a','l','i','d','T','a','g',0}
    },
    {   /* TAG_TYPE_BINARY */
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'P','A','T','C','H','_','B','I','T','S',0},
        {'F','I','L','E','_','B','I','T','S',0},
        {'E','X','E','_','I','D',0},
        {'D','A','T','A','_','B','I','T','S',0},
        {'M','S','I','_','P','A','C','K','A','G','E','_','I','D',0},
        {'D','A','T','A','B','A','S','E','_','I','D',0},
        {'C','O','N','T','E','X','T','_','P','L','A','T','F','O','R','M','_','I','D',0},
        {'C','O','N','T','E','X','T','_','B','R','A','N','C','H','_','I','D',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'I','n','v','a','l','i','d','T','a','g',0},
        {'F','I','X','_','I','D',0},
        {'A','P','P','_','I','D',0}
    }
    };

    /* sizes of tables in above array (# strings per type) */
    static const WORD limits[9] = {
        /* switch off TYPE_* nibble of last tag for each type */
        TAG_REINSTALL_UPGRADE & 0xFF,
        1,
        TAG_MATCH_MODE & 0xFF,
        TAG_EXE_WRAPPER & 0xFF,
        TAG_FLAG_INSTALL & 0xFF,
        TAG_EXPORT_NAME & 0xFF,
        TAG_CONTEXT_REF & 0xFF,
        1,
        TAG_APP_ID & 0xFF
    };

    /* lookup tables for tags in range 0x800 + (0x1 -> 0xFF) | TYPE */
    static const WCHAR table2[9][3][17] = {
    { {'I','n','v','a','l','i','d','T','a','g',0} }, /* TAG_TYPE_NULL */
    { {'I','n','v','a','l','i','d','T','a','g',0} }, /* TAG_TYPE_BYTE */
    {
        {'T','A','G',0}, /* TAG_TYPE_WORD */
        {'I','N','D','E','X','_','T','A','G',0},
        {'I','N','D','E','X','_','K','E','Y',0}
    },
    { {'T','A','G','I','D',0} }, /* TAG_TYPE_DWORD */
    { {'I','n','v','a','l','i','d','T','a','g',0} }, /* TAG_TYPE_QWORD */
    { {'I','n','v','a','l','i','d','T','a','g',0} }, /* TAG_TYPE_STRINGREF */
    {
        {'S','T','R','I','N','G','T','A','B','L','E',0}, /* TAG_TYPE_LIST */
        {'I','N','D','E','X','E','S',0},
        {'I','N','D','E','X',0}
    },
    { {'S','T','R','I','N','G','T','A','B','L','E','_','I','T','E','M',0}, }, /* TAG_TYPE_STRING */
    { {'I','N','D','E','X','_','B','I','T','S',0} } /* TAG_TYPE_BINARY */
    };

    /* sizes of tables in above array, hardcoded for simplicity */
    static const WORD limits2[9] = { 0, 0, 3, 1, 0, 0, 3, 1, 1 };

    static const WCHAR null[] = {'N','U','L','L',0};
    static const WCHAR invalid[] = {'I','n','v','a','l','i','d','T','a','g',0};

    BOOL switch_table; /* should we use table2 and limits2? */
    WORD index, type_index;

    /* special case: null tag */
    if (tag == TAG_NULL)
        return null;

    /* tags with only type mask or no type mask are invalid */
    if ((tag & ~TAG_TYPE_MASK) == 0 || (tag & TAG_TYPE_MASK) == 0)
        return invalid;

    /* some valid tags are in range 0x800 + (0x1 -> 0xF) | TYPE */
    if ((tag & 0xF00) == 0x800)
        switch_table = TRUE;
    else if ((tag & 0xF00) == 0)
        switch_table = FALSE;
    else return invalid;

    /* index of table in array is type nibble */
    type_index = (tag >> 12) - 1;

    /* index of string in table is low byte */
    index = (tag & 0xFF) - 1;

    /* bound check */
    if (type_index >= 9 || index >= (switch_table ? limits2[type_index] : limits[type_index]))
        return invalid;

    /* tag is valid */
    return switch_table ? table2[type_index][index] : table[type_index][index];
}

static BOOL WINAPI SdbSafeAdd(DWORD a, DWORD b, PDWORD c)
{
    if ((a + b) >= a)
    {
        *c = a + b;
        return TRUE;
    }
    return FALSE;
}

static BOOL WINAPI SdbReadData(PDB db, PVOID dest, DWORD offset, DWORD num)
{
    DWORD size;

    if (!SdbSafeAdd(offset, num, &size))
    {
        TRACE("Failed to add %u and %u, overflow!", offset, num);
        return FALSE;
    }

    if (db->size < size)
    {
        TRACE("Cannot read %u bytes starting at %u from database of size %u, overflow!",
              num, offset, db->size);
        return FALSE;
    }

    memcpy(dest, db->data + offset, num);
    return TRUE;
}

/**************************************************************************
 *        SdbGetTagFromTagID                [APPHELP.@]
 *
 * Searches shim database for the tag associated with specified tagid
 *
 * PARAMS
 *  db      [I] Handle to the shim database
 *  tagid   [I] The TAGID associated with the the tag
 *
 * RETURNS
 *  Success: The tag associated with specified tagid
 *  Failure: TAG_NULL
 */
TAG WINAPI SdbGetTagFromTagID(PDB db, TAGID tagid)
{
    TAG data;

    /* A tag associated with tagid is first 2 bytes tagid bytes offset from beginning */
    if (!SdbReadData(db, &data, tagid, 2))
    {
        TRACE("Failed to read tag at tagid %u\n", tagid);
        return TAG_NULL;
    }

    return data;
}

/**************************************************************************
 *        SdbGetFirstChild                [APPHELP.@]
 *
 * Searches shim database for a child of specified parent tag
 *
 * PARAMS
 *  db       [I] Handle to the shim database
 *  parent   [I] TAGID of parent
 *
 * RETURNS
 *  Success: TAGID associated with child tag
 *  Failure: TAGID_NULL
 */
TAGID WINAPI SdbGetFirstChild(PDB db, TAGID parent)
{
    /* if we are at beginning of database */
    if (parent == TAGID_ROOT)
    {
        /* header only database: no tags */
        if (db->size <= _TAGID_ROOT)
            return TAGID_NULL;
        /* return *real* root tagid */
        else return _TAGID_ROOT;
    }

    /* only list tag can have children */
    if ((SdbGetTagFromTagID(db, parent) & TAG_TYPE_MASK) != TAG_TYPE_LIST)
        return TAGID_NULL;

    /* first child is sizeof(TAG) + sizeof(DWORD) bytes after beginning of list */
    return parent + 6;
}

static DWORD WINAPI SdbGetTagSize(PDB db, TAGID tagid)
{
    /* sizes of data types with fixed size + sizeof(TAG) */
    static const SIZE_T sizes[6] = {
        0 + 2, 1 + 2,
        2 + 2, 4 + 2,
        8 + 2, 4 + 2
    };
    WORD type;
    DWORD size;

    type = SdbGetTagFromTagID(db, tagid) & TAG_TYPE_MASK;

    if (type == TAG_NULL)
    {
        TRACE("Invalid tagid\n");
        return 0;
    }

    if (type <= TAG_TYPE_STRINGREF)
        return sizes[(type >> 12) - 1];

    /* tag with dynamic size (e.g. list): must read size */
    if (!SdbReadData(db, &size, tagid + 2, 4))
    {
        TRACE("Failed to read size of tag!\n");
        return 0;
    }

    /* add 4 because of size DWORD */
    return size + 2 + 4;
}

/**************************************************************************
 *        SdbGetNextChild                [APPHELP.@]
 *
 * Searches shim database for next child of specified parent tag
 *
 * PARAMS
 *  db          [I] Handle to the shim database
 *  parent      [I] TAGID of parent
 *  prev_child  [I] TAGID of previous child
 *
 * RETURNS
 *  Success: TAGID associated with next child tag
 *  Failure: TAGID_NULL
 */
TAGID WINAPI SdbGetNextChild(PDB db, TAGID parent, TAGID prev_child)
{
    TAGID next_child;
    DWORD prev_child_size, parent_size;

    prev_child_size = SdbGetTagSize(db, prev_child);
    if (prev_child_size == 0)
    {
        TRACE("Failed to read child tag size\n");
        return TAGID_NULL;
    }

    next_child = prev_child + prev_child_size;
    if (next_child >= db->size)
    {
        TRACE("Next child is beyond end of database, overflow!\n");
        return TAGID_NULL;
    }

    if (parent == TAGID_ROOT)
        return next_child;

    parent_size = SdbGetTagSize(db, parent);
    if (parent_size == 0)
    {
        TRACE("Failed to read parent tag size\n");
        return TAGID_NULL;
    }

    if (next_child >= parent + parent_size + 6)
    {
        TRACE("Specified parent has no more children\n");
        return TAGID_NULL;
    }

    return next_child;
}

static PDB WINAPI SdbAlloc(void)
{
    PDB db;

    db = (PDB)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DB));

    if (db)
        db->file = INVALID_HANDLE_VALUE;

    return db;
}

/**************************************************************************
 *        SdbOpenDatabase                [APPHELP.@]
 *
 * Opens specified shim database file
 *
 * PARAMS
 *  path      [I] Path to the shim database
 *  type      [I] Type of path. Either DOS_PATH or NT_PATH
 *
 * RETURNS
 *  Success: Handle to the shim database
 *  Failure: NULL handle
 */
PDB WINAPI SdbOpenDatabase(LPCWSTR path, PATH_TYPE type)
{
    NTSTATUS status;
    IO_STATUS_BLOCK io;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    PDB db;
    BYTE header[12];
    TAGID iter; /* iterator, used to locate stringtable */

    TRACE("%s, 0x%x\n", debugstr_w(path), type);

    db = SdbAlloc();
    if (!db)
    {
        TRACE("Failed to allocate memory for shim database\n");
        return NULL;
    }

    if (type == DOS_PATH)
    {
        if (!RtlDosPathNameToNtPathName_U(path, &str, NULL, NULL))
            return NULL;
    }
    else
        RtlInitUnicodeString(&str, path);

    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtCreateFile(&db->file, FILE_GENERIC_READ,
                          &attr, &io, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ,
                          FILE_OPEN, FILE_NON_DIRECTORY_FILE, NULL, 0);

    if (type == DOS_PATH)
        RtlFreeUnicodeString(&str);

    if (status != STATUS_SUCCESS)
    {
        SdbCloseDatabase(db);
        TRACE("Failed to open shim database file\n");
        return NULL;
    }

    db->size = GetFileSize(db->file, NULL);
    db->data = HeapAlloc(GetProcessHeap(), 0, db->size);
    ReadFile(db->file, db->data, db->size, NULL, NULL);

    if (!SdbReadData(db, &header, 0, 12))
    {
        SdbCloseDatabase(db);
        TRACE("Failed to read shim database header\n");
        return NULL;
    }

    if (memcmp(&header[8], "sdbf", 4) != 0)
    {
        SdbCloseDatabase(db);
        TRACE("Shim database header is invalid\n");
        return NULL;
    }

    if (*(DWORD*)&header[0] != (DWORD)2)
    {
        SdbCloseDatabase(db);
        TRACE("Invalid shim database version\n");
        return NULL;
    }

    /* Find stringtable, if one exists */
    iter = SdbGetFirstChild(db, TAGID_ROOT);
    while (iter < db->size)
    {
        if (SdbGetTagFromTagID(db, iter) == TAG_STRINGTABLE)
        {
            db->stringtable = iter;
            break;
        }
        iter = SdbGetNextChild(db, TAGID_ROOT, iter);

        /* Prevent infinite loop in case of malformed file */
        if (iter == TAGID_NULL)
            break;
    }

    return db;
}

static LPWSTR WINAPI SdbGetString(PDB db, TAGID tagid, PDWORD size)
{
    TAG tag;
    TAGID offset;

    tag = SdbGetTagFromTagID(db, tagid);
    if (!tag)
        return NULL;

    if ((tag & TAG_TYPE_MASK) == TAG_TYPE_STRINGREF)
    {
        if (db->stringtable == TAGID_NULL)
        {
            TRACE("stringref is invalid because there is no stringtable\n");
            return NULL;
        }

        /* TAG_TYPE_STRINGREF contains offset of string relative to stringtable */
        if (!SdbReadData(db, &tagid, tagid + sizeof(TAG), sizeof(TAGID)))
            return NULL;

        offset = db->stringtable + tagid + 6;
    }
    else if ((tag & TAG_TYPE_MASK) == TAG_TYPE_STRING)
    {
        offset = tagid + 6;
    }
    else
    {
        TRACE("Tag 0x%u at tagid %u is neither a string or reference to string\n", tag, tagid);
        return NULL;
    }

    /* Optionally read string size */
    if (size && !SdbReadData(db, size, tagid + 2, 4))
    {
        TRACE("Failed to read size of string!\n");
        return FALSE;
    }

    return (LPWSTR)(&db->data[offset]);
}

/**************************************************************************
 *        SdbReadStringTag                [APPHELP.@]
 *
 * Searches shim database for string associated with specified tagid
 * and copies string into a bugger
 *
 * PARAMS
 *  db          [I] Handle to the shim database
 *  tagid       [I] TAGID of string or stringref associated with the string
 *  buffer      [O] Buffer in which string will be copied
 *  size        [I] Number of characters to copy
 *
 * RETURNS
 *  TRUE if string was successfully copied to the buffer
 *  FALSE if string was not copied to the buffer
 *
 * NOTES
 *  If size parameter is less than number of characters in string, this
 *  function shall fail and no data shall be copied
 */
BOOL WINAPI SdbReadStringTag(PDB db, TAGID tagid, LPWSTR buffer, DWORD size)
{
    LPWSTR string;
    DWORD string_size;

    string = SdbGetString(db, tagid, &string_size);
    if (!string)
        return FALSE;

    if (size * sizeof(WCHAR) < string_size)
    {
        TRACE("Buffer of size %u is too small for string of size %u\n",
              size * sizeof(WCHAR), string_size);
        return FALSE;
    }

    memcpy(buffer, string, string_size);
    return TRUE;
}
