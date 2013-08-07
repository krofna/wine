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
#include "winver.h"
#include "imagehlp.h"
#include "winternl.h"
#include "shlwapi.h"

#include "wine/debug.h"
#include "wine/unicode.h"

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

static void WINAPI SdbFlush(PDB db)
{
    WriteFile(db->file, db->data, db->write_iter, NULL, NULL);
}

/**************************************************************************
 *        SdbCloseDatabaseWrite                [APPHELP.@]
 *
 * Closes specified database and writes data to file
 *
 * PARAMS
 *  db      [I] Handle to the shim database
 *
 * RETURNS
 *  This function does not return a value.
 */
void WINAPI SdbCloseDatabaseWrite(PDB db)
{
    SdbFlush(db);
    SdbCloseDatabase(db);
}

static void WINAPI SdbWrite(PDB db, LPCVOID data, DWORD size)
{
    if (db->write_iter + size > db->size)
    {
        /* Round to powers of two to prevent too many reallocations */
        while (db->size < db->write_iter + size) db->size <<= 1;
        db->data = HeapReAlloc(GetProcessHeap(), 0, db->data, db->size);
    }

    memcpy(db->data + db->write_iter, data, size);
    db->write_iter += size;
}

static PDB WINAPI SdbAlloc(void);

/**************************************************************************
 *        SdbCreateDatabase                [APPHELP.@]
 *
 * Creates new shim database file
 *
 * PARAMS
 *  path      [I] Path to the new shim database
 *  type      [I] Type of path. Either DOS_PATH or NT_PATH
 *
 * RETURNS
 *  Success: Handle to the newly created shim database
 *  Failure: NULL handle
 *
 * NOTES
 *  If a file already exists on specified path, that file shall be overwritten.
 *  Use SdbCloseDatabasWrite to close the database opened with this function.
 */
PDB WINAPI SdbCreateDatabase(LPCWSTR path, PATH_TYPE type)
{
    static const DWORD version_major = 2, version_minor = 1;
    static const char* magic = "sdbf";
    NTSTATUS status;
    IO_STATUS_BLOCK io;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    PDB db;

    TRACE("%s %u\n", debugstr_w(path), type);

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

    status = NtCreateFile(&db->file, FILE_GENERIC_WRITE,
                          &attr, &io, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ,
                          FILE_SUPERSEDE, FILE_NON_DIRECTORY_FILE, NULL, 0);

    if (type == DOS_PATH)
        RtlFreeUnicodeString(&str);

    if (status != STATUS_SUCCESS)
    {
        SdbCloseDatabase(db);
        TRACE("Failed to create shim database file\n");
        return NULL;
    }

    db->data = HeapAlloc(GetProcessHeap(), 0, 16);
    db->size = 16;

    SdbWrite(db, &version_major, 4);
    SdbWrite(db, &version_minor, 4);
    SdbWrite(db, magic, 4);

    return db;
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
        TRACE("Failed to add %u and %u, overflow!\n", offset, num);
        return FALSE;
    }

    if (db->size < size)
    {
        TRACE("Cannot read %u bytes starting at %u from database of size %u, overflow!\n",
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
 *  tagid   [I] The TAGID of the tag
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
 *  Success: TAGID of child tag
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
 *  Success: TAGID of next child tag
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

/**************************************************************************
 *        SdbFindNextTag                [APPHELP.@]
 *
 * Searches shim database for a next tag which matches prev_child
 * within parent's domain
 *
 * PARAMS
 *  db          [I] Handle to the shim database
 *  parent      [I] TAGID of parent
 *  prev_child  [I] TAGID of previous match
 *
 * RETURNS
 *  Success: TAGID of next match
 *  Failure: TAGID_NULL
 */
TAGID WINAPI SdbFindNextTag(PDB db, TAGID parent, TAGID prev_child)
{
    TAG tag = SdbGetTagFromTagID(db, prev_child);
    TAGID iter = SdbGetNextChild(db, parent, prev_child);

    while (iter != TAGID_NULL)
    {
        if (SdbGetTagFromTagID(db, iter) == tag)
            return iter;
        iter = SdbGetNextChild(db, parent, iter);
    }

    return TAGID_NULL;
}

/**************************************************************************
 *        SdbFindFirstTag                [APPHELP.@]
 *
 * Searches shim database for a tag within specified domain
 *
 * PARAMS
 *  db          [I] Handle to the shim database
 *  parent      [I] TAGID of parent
 *  tag         [I] TAG to be located
 *
 * RETURNS
 *  Success: TAGID of first matching tag
 *  Failure: TAGID_NULL
 */
TAGID WINAPI SdbFindFirstTag(PDB db, TAGID parent, TAG tag)
{
    TAGID iter = SdbGetFirstChild(db, parent);

    while (iter != TAGID_NULL)
    {
        if (SdbGetTagFromTagID(db, iter) == tag)
            return iter;
        iter = SdbGetNextChild(db, parent, iter);
    }

    return TAGID_NULL;
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

    db->stringtable = SdbFindFirstTag(db, TAGID_ROOT, TAG_STRINGTABLE);

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

static BOOL WINAPI SdbCheckTagType(TAG tag, WORD type)
{
    if ((tag & TAG_TYPE_MASK) != type)
        return FALSE;
    return TRUE;
}

static BOOL WINAPI SdbCheckTagIDType(PDB db, TAGID tagid, WORD type)
{
    TAG tag = SdbGetTagFromTagID(db, tagid);
    if (tag == TAG_NULL)
        return FALSE;
    return SdbCheckTagType(tag, type);
}

/**************************************************************************
 *        SdbReadDWORDTag                [APPHELP.@]
 *
 * Reads DWORD value at specified tagid
 *
 * PARAMS
 *  db          [I] Handle to the shim database
 *  tagid       [I] TAGID of DWORD value
 *  ret         [I] Default return value in case function fails
 *
 * RETURNS
 *  Success: DWORD value at specified tagid
 *  Failure: ret
 */
DWORD WINAPI SdbReadDWORDTag(PDB db, TAGID tagid, DWORD ret)
{
    if (!SdbCheckTagIDType(db, tagid, TAG_TYPE_DWORD))
    {
        TRACE("Tag associated with tagid %u is not a DWORD\n", tagid);
        return ret;
    }

    if (!SdbReadData(db, &ret, tagid + 2, sizeof(DWORD)))
    {
        TRACE("Failed to read DWORD tag at tagid %u\n", tagid);
        return ret;
    }

    return ret;
}

/**************************************************************************
 *        SdbReadQWORDTag                [APPHELP.@]
 *
 * Reads QWORD value at specified tagid
 *
 * PARAMS
 *  db          [I] Handle to the shim database
 *  tagid       [I] TAGID of QWORD value
 *  ret         [I] Default return value in case function fails
 *
 * RETURNS
 *  Success: QWORD value at specified tagid
 *  Failure: ret
 */
QWORD WINAPI SdbReadQWORDTag(PDB db, TAGID tagid, QWORD ret)
{
    if (!SdbCheckTagIDType(db, tagid, TAG_TYPE_QWORD))
    {
        TRACE("Tag associated with tagid %u is not a QWORD\n", tagid);
        return ret;
    }

    if (!SdbReadData(db, &ret, tagid + 2, sizeof(QWORD)))
    {
        TRACE("Failed to read QWORD tag at tagid %u\n", tagid);
        return ret;
    }

    return ret;
}

/**************************************************************************
 *        SdbGetBinaryTagData                 [APPHELP.@]
 *
 * Retrieves binary data at specified tagid
 *
 * PARAMS
 *  db          [I] Handle to the shim database
 *  tagid       [I] TAGID of binary data
 *
 * RETURNS
 *  Success: Pointer to binary data at specified tagid
 *  Failure: NULL
 */
PVOID WINAPI SdbGetBinaryTagData(PDB db, TAGID tagid)
{
    if (!SdbCheckTagIDType(db, tagid, TAG_TYPE_BINARY))
    {
        TRACE("The tag associated with tagid %u is not of type BINARY\n", tagid);
        return NULL;
    }

    return &db->data[tagid + 6];
}

/**************************************************************************
 *        SdbGetStringTagPtr                [APPHELP.@]
 *
 * Searches shim database for string associated with specified tagid
 *
 * PARAMS
 *  db          [I] Handle to the shim database
 *  tagid       [I] TAGID of string or stringref associated with the string
 *
 * RETURNS
 *  Success: Pointer to string associated with specified tagid
 *  Failure: NULL
 */
LPWSTR WINAPI SdbGetStringTagPtr(PDB db, TAGID tagid)
{
    return SdbGetString(db, tagid, NULL);
}

/**************************************************************************
 *        SdbWriteDWORDTag                [APPHELP.@]
 *
 * Writes a DWORD entry to the specified shim database
 *
 * PARAMS
 *  db        [I] Handle to the shim database
 *  tag       [I] A tag for the entry
 *  data      [I] DWORD entry which will be written to the database
 *
 * RETURNS
 *  TRUE if data was successfully written
 *  FALSE otherwise
 */
BOOL WINAPI SdbWriteDWORDTag(PDB db, TAG tag, DWORD data)
{
    if (!SdbCheckTagType(tag, TAG_TYPE_DWORD))
        return FALSE;

    SdbWrite(db, &tag, 2);
    SdbWrite(db, &data, 4);
    return TRUE;
}

/**************************************************************************
 *        SdbWriteQWORDTag                [APPHELP.@]
 *
 * Writes a DWORD entry to the specified shim database
 *
 * PARAMS
 *  db        [I] Handle to the shim database
 *  tag       [I] A tag for the entry
 *  data      [I] QWORD entry which will be written to the database
 *
 * RETURNS
 *  TRUE if data was successfully written
 *  FALSE otherwise
 */
BOOL WINAPI SdbWriteQWORDTag(PDB db, TAG tag, QWORD data)
{
    if (!SdbCheckTagType(tag, TAG_TYPE_QWORD))
        return FALSE;

    SdbWrite(db, &tag, 2);
    SdbWrite(db, &data, 8);
    return TRUE;
}

/**************************************************************************
 *        SdbWriteStringTag                [APPHELP.@]
 *
 * Writes a wide string entry to the specified shim database
 *
 * PARAMS
 *  db        [I] Handle to the shim database
 *  tag       [I] A tag for the entry
 *  string    [I] Wide string entry which will be written to the database
 *
 * RETURNS
 *  TRUE if data was successfully written
 *  FALSE otherwise
 */
BOOL WINAPI SdbWriteStringTag(PDB db, TAG tag, LPCWSTR string)
{
    DWORD size;

    if (!SdbCheckTagType(tag, TAG_TYPE_STRING))
        return FALSE;

    size = (lstrlenW(string) + 1) * sizeof(WCHAR);
    SdbWrite(db, &tag, 2);
    SdbWrite(db, &size, 4);
    SdbWrite(db, string, size);
    return TRUE;
}

/**************************************************************************
 *        SdbWriteWORDTag                [APPHELP.@]
 *
 * Writes a WORD entry to the specified shim database
 *
 * PARAMS
 *  db        [I] Handle to the shim database
 *  tag       [I] A tag for the entry
 *  data      [I] WORD entry which will be written to the database
 *
 * RETURNS
 *  TRUE if data was successfully written
 *  FALSE otherwise
 */
BOOL WINAPI SdbWriteWORDTag(PDB db, TAG tag, WORD data)
{
    if (!SdbCheckTagType(tag, TAG_TYPE_WORD))
        return FALSE;

    SdbWrite(db, &tag, 2);
    SdbWrite(db, &data, 2);
    return TRUE;
}

/**************************************************************************
 *        SdbWriteNULLTag                [APPHELP.@]
 *
 * Writes a tag-only (NULL) entry to the specified shim database
 *
 * PARAMS
 *  db        [I] Handle to the shim database
 *  tag       [I] A tag for the entry
 *
 * RETURNS
 *  TRUE if data was successfully written
 *  FALSE otherwise
 */
BOOL WINAPI SdbWriteNULLTag(PDB db, TAG tag)
{
    if (!SdbCheckTagType(tag, TAG_TYPE_NULL))
        return FALSE;

    SdbWrite(db, &tag, 2);
    return TRUE;
}

static PVOID WINAPI SdbOpenMemMappedFile(LPCWSTR path, PHANDLE file, PHANDLE mapping, PDWORD size)
{
    PVOID view;

    *file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (*file == INVALID_HANDLE_VALUE)
    {
        TRACE("Failed to open file %s\n", debugstr_w(path));
        return NULL;
    }

    *mapping = CreateFileMappingW(*file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (*mapping == INVALID_HANDLE_VALUE)
    {
        TRACE("Failed to create mapping for file\n");
        return NULL;
    }

    *size = GetFileSize(*file, NULL);
    view = MapViewOfFile(*mapping, FILE_MAP_READ, 0, 0, *size);
    if (!view)
    {
        TRACE("Failed to map view of file\n");
        return NULL;
    }

    return view;
}

/**************************************************************************
 *        SdbWriteBinaryTag                [APPHELP.@]
 *
 * Writes data the specified shim database
 *
 * PARAMS
 *  db        [I] Handle to the shim database
 *  tag       [I] A tag for the entry
 *  data      [I] Pointer to data
 *  size      [I] Number of bytes to write
 *
 * RETURNS
 *  TRUE if data was successfully written
 *  FALSE otherwise
 */
BOOL WINAPI SdbWriteBinaryTag(PDB db, TAG tag, PBYTE data, DWORD size)
{
    if (!SdbCheckTagType(tag, TAG_TYPE_BINARY))
        return FALSE;

    SdbWrite(db, &tag, 2);
    SdbWrite(db, &size, 4);
    SdbWrite(db, data, size);
    return TRUE;
}

/**************************************************************************
 *        SdbWriteBinaryTagFromFile                [APPHELP.@]
 *
 * Writes data from a file to the specified shim database
 *
 * PARAMS
 *  db        [I] Handle to the shim database
 *  tag       [I] A tag for the entry
 *  path      [I] Path of the input file
 *
 * RETURNS
 *  TRUE if data was successfully written
 *  FALSE otherwise
 */
BOOL WINAPI SdbWriteBinaryTagFromFile(PDB db, TAG tag, LPCWSTR path)
{
    HANDLE file, mapping;
    DWORD size;
    LPVOID view;

    if (!SdbCheckTagType(tag, TAG_TYPE_BINARY))
        return FALSE;

    view = SdbOpenMemMappedFile(path, &file, &mapping, &size);
    if (!view)
        return FALSE;

    SdbWriteBinaryTag(db, tag, view, size);

    CloseHandle(mapping);
    CloseHandle(file);
    return TRUE;
}

static void WINAPI SdbSetDWORDAttr(PATTRINFO attr, TAG tag, DWORD value)
{
    attr->type = tag;
    attr->flags = ATTRIBUTE_AVAILABLE;
    attr->dwattr = value;
}

static void WINAPI SdbSetStringAttr(PATTRINFO attr, TAG tag, WCHAR *string)
{
    if (!string)
    {
        attr->flags = ATTRIBUTE_FAILED;
        return;
    }

    attr->type = tag;
    attr->flags = ATTRIBUTE_AVAILABLE;
    attr->lpattr = string;
}

static void WINAPI SdbSetAttrFail(PATTRINFO attr)
{
    attr->flags = ATTRIBUTE_FAILED;
}

static WCHAR* WINAPI SdbGetStringAttr(LPWSTR translation, LPCWSTR attr, PVOID file_info)
{
    DWORD size = 0;
    PVOID buffer;
    WCHAR value[128] = {0};

    if (!file_info)
        return NULL;

    snprintfW(value, 128, translation, attr);
    if (VerQueryValueW(file_info, value, &buffer, &size) && size != 0)
        return (WCHAR*)buffer;

    return NULL;
}

/**************************************************************************
 *        SdbFreeFileAttributes                [APPHELP.@]
 *
 * Frees attribute data allocated by SdbGetFileAttributes
 *
 * PARAMS
 *  attr_info  [I] Pointer to array of ATTRINFO which will be deallocated
 *
 * RETURNS
 *  This function always returns TRUE
 */
BOOL WINAPI SdbFreeFileAttributes(PATTRINFO *attr_info)
{
    WORD i;
    for (i = 0; i < 28; i++)
        if ((attr_info[i]->type & TAG_TYPE_MASK) == TAG_TYPE_STRINGREF)
            HeapFree(GetProcessHeap(), 0, attr_info[i]->lpattr);
    HeapFree(GetProcessHeap(), 0, attr_info);
    return TRUE;
}

/**************************************************************************
 *        SdbGetFileAttributes                [APPHELP.@]
 *
 * Retrieves attribute data shim database requires to match a file with
 * database entry
 *
 * PARAMS
 *  path       [I] Path to the file
 *  attr_info  [O] Pointer to array of ATTRINFO. Contains attribute data
 *  attr_count [O] Number of attributes in attr_info
 *
 * RETURNS
 *  TRUE attribute data was successfully retrieved
 *  FALSE otherwise
 *
 * NOTES
 *  You must free the attr_info allocated by this function by calling
 *  SdbFreeFileAttributes
 */
BOOL WINAPI SdbGetFileAttributes(LPCWSTR path, PATTRINFO *attr_info, LPDWORD attr_count)
{
    static const WCHAR str_tinfo[] = {'\\','V','a','r','F','i','l','e','I','n','f','o','\\','T','r','a','n','s','l','a','t','i','o','n',0};
    static const WCHAR str_trans[] = {'\\','S','t','r','i','n','g','F','i','l','e','I','n','f','o','\\','%','0','4','x','%','0','4','x','\\','%','%','s',0};
    static const WCHAR str_CompanyName[] = {'C','o','m','p','a','n','y','N','a','m','e',0};
    static const WCHAR str_FileDescription[] = {'F','i','l','e','D','e','s','c','r','i','p','t','i','o','n',0};
    static const WCHAR str_FileVersion[] = {'F','i','l','e','V','e','r','s','i','o','n',0};
    static const WCHAR str_InternalName[] = {'I','n','t','e','r','n','a','l','N','a','m','e',0};
    static const WCHAR str_LegalCopyright[] = {'L','e','g','a','l','C','o','p','y','r','i','g','h','t',0};
    static const WCHAR str_OriginalFilename[] = {'O','r','i','g','i','n','a','l','F','i','l','e','n','a','m','e',0};
    static const WCHAR str_ProductName[] = {'P','r','o','d','u','c','t','N','a','m','e',0};
    static const WCHAR str_ProductVersion[] = {'P','r','o','d','u','c','t','V','e','r','s','i','o','n',0};

    PIMAGE_NT_HEADERS headers;
    HANDLE file;
    HANDLE mapping;
    PVOID view, file_info = 0;
    DWORD file_size, info_size, page_size;
    DWORD headersum, checksum;
    WCHAR translation[128] = {0};

    struct LANGANDCODEPAGE {
        WORD language;
        WORD code_page;
    } *lang_page;

    TRACE("%s %p %p\n", debugstr_w(path), attr_info, attr_count);

    view = SdbOpenMemMappedFile(path, &file, &mapping, &file_size);
    if (!view)
    {
        TRACE("failed to open file\n");
        return FALSE;
    }

    file_size = GetFileSize(file, NULL);
    headers = CheckSumMappedFile(view, file_size, &headersum, &checksum);
    if (!headers)
    {
        TRACE("failed to get file header size\n");
        return FALSE;
    }

    info_size = GetFileVersionInfoSizeW(path, NULL);
    if (info_size != 0)
    {
        file_info = HeapAlloc(GetProcessHeap(), 0, info_size);
        GetFileVersionInfoW(path, 0, info_size, file_info);
        VerQueryValueW(file_info, str_tinfo, (LPVOID)&lang_page, &page_size);
        snprintfW(translation, 128, str_trans, lang_page->language, lang_page->code_page);
    }

    *attr_info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 28 * sizeof(ATTRINFO));

    SdbSetDWORDAttr(attr_info[0], TAG_SIZE, file_size);

    SdbSetAttrFail(attr_info[1]); /* TAG_CHECKSUM */
    SdbSetAttrFail(attr_info[2]); /* TAG_BIN_FILE_VERSION */
    SdbSetAttrFail(attr_info[3]); /* TAG_BIN_PRODUCT_VERSION - same as above? */

    SdbSetStringAttr(attr_info[4], TAG_PRODUCT_VERSION, SdbGetStringAttr(translation, str_ProductVersion, file_info));
    SdbSetStringAttr(attr_info[5], TAG_FILE_DESCRIPTION, SdbGetStringAttr(translation, str_FileDescription, file_info));
    SdbSetStringAttr(attr_info[6], TAG_COMPANY_NAME, SdbGetStringAttr(translation, str_CompanyName, file_info));
    SdbSetStringAttr(attr_info[7], TAG_PRODUCT_NAME, SdbGetStringAttr(translation, str_ProductName, file_info));
    SdbSetStringAttr(attr_info[8], TAG_FILE_VERSION, SdbGetStringAttr(translation, str_FileVersion, file_info));
    SdbSetStringAttr(attr_info[9], TAG_ORIGINAL_FILENAME, SdbGetStringAttr(translation, str_OriginalFilename, file_info));
    SdbSetStringAttr(attr_info[10], TAG_INTERNAL_NAME, SdbGetStringAttr(translation, str_InternalName, file_info));
    SdbSetStringAttr(attr_info[11], TAG_LEGAL_COPYRIGHT, SdbGetStringAttr(translation, str_LegalCopyright, file_info));

    SdbSetAttrFail(attr_info[12]); /* TAG_VERDATEHI - always 0? */
    SdbSetAttrFail(attr_info[13]); /* TAG_VERDATELO - always 0? */

    /* http://msdn.microsoft.com/en-us/library/windows/desktop/ms680339(v=vs.85).aspx */
    SdbSetAttrFail(attr_info[14]); /* TAG_VERFILEOS - 0x000, 0x4, 0x40004, 0x40000, 0x10004, 0x10001*/
    SdbSetAttrFail(attr_info[15]); /* TAG_VERFILETYPE 0x1(exe?), 0x2(dll?), 0x3(special dll?), 0x4(service?) */
    SdbSetAttrFail(attr_info[16]); /* TAG_MODULE_TYPE (1: WIN16?) (3: WIN32?) (WIN64?), Win32VersionValue? */

    SdbSetDWORDAttr(attr_info[17], TAG_PE_CHECKSUM, headers->OptionalHeader.CheckSum);

    SdbSetAttrFail(attr_info[18]); /* TAG_LINKER_VERSION - doesn't seem to match NT header data : check */
    SdbSetAttrFail(attr_info[19]); /* TAG_NULL, ATTRIBUTE_FAILED */
    SdbSetAttrFail(attr_info[20]); /* TAG_NULL, ATTRIBUTE_FAILED */
    SdbSetAttrFail(attr_info[21]); /* TAG_UPTO_BIN_FILE_VERSION */
    SdbSetAttrFail(attr_info[22]); /* TAG_UPTO_BIN_PRODUCT_VERSION: same as above? */

    SdbSetDWORDAttr(attr_info[23], TAG_LINK_DATE, headers->FileHeader.TimeDateStamp);
    SdbSetDWORDAttr(attr_info[24], TAG_UPTO_LINK_DATE, headers->FileHeader.TimeDateStamp);

    SdbSetAttrFail(attr_info[25]); /* TAG_EXPORT_NAME - often file name? */
    /* http://msdn.microsoft.com/en-us/library/windows/desktop/dd318693(v=vs.85).aspx */
    /* http://msdn.microsoft.com/en-us/library/windows/desktop/aa381019(v=vs.85).aspx */
    SdbSetAttrFail(attr_info[26]); /* TAG_VER_LANGUAGE 0x0: neutral, 0x409: eng (US), Russisch (Rusland) 0x419, Svenska (Sverige) 0x41d, eng (AUS) 0x800, Svenska (Sverige) [0x41d]*/
    SdbSetAttrFail(attr_info[27]); /* TAG_EXE_WRAPPER - boolean */

    HeapFree(GetProcessHeap(), 0, file_info);
    *attr_count = 28;
    return TRUE;
}

/**************************************************************************
 *        SdbReadBinaryTag                [APPHELP.@]
 *
 * Reads binary data at specified tagid
 *
 * PARAMS
 *  db          [I] Handle to the shim database
 *  tagid       [I] TAGID of binary data
 *  buffer      [O] Buffer in which data will be copied
 *  size        [I] Size of the buffer
 *
 * RETURNS
 *  TRUE if data was successfully written
 *  FALSE otherwise
 */
BOOL WINAPI SdbReadBinaryTag(PDB db, TAGID tagid, PBYTE buffer, DWORD size)
{
    if (!SdbCheckTagIDType(db, tagid, TAG_TYPE_BINARY))
        return FALSE;

    /* TODO: Error checking */
    return SdbReadData(db, buffer, tagid, size);
}

/**************************************************************************
 *        SdbBeginWriteListTag                [APPHELP.@]
 *
 * Writes a list tag to specified database
 *
 * PARAMS
 *  db          [I] Handle to the shim database
 *  tag         [I] TAG for the list
 *
 * RETURNS
 *  Success: TAGID of the newly created list
 *  Failure: TAGID_NULL
 *
 * NOTES
 * All subsequent SdbWrite* functions shall write to newly created list
 * untill TAGID of that list is passed to SdbEndWriteListTag
 */
TAGID WINAPI SdbBeginWriteListTag(PDB db, TAG tag)
{
    TAGID list_id;

    if (!SdbCheckTagType(tag, TAG_TYPE_LIST))
        return TAGID_NULL;

    list_id = db->write_iter;
    SdbWrite(db, &tag, 2);
    db->write_iter += 4; /* reserve some memory for storing list size */
    return list_id;
}

/**************************************************************************
 *        SdbEndWriteListTag                [APPHELP.@]
 *
 * Marks end of the specified list
 *
 * PARAMS
 *  db          [I] Handle to the shim database
 *  tagid       [I] TAGID of the list
 *
 * RETURNS
 *  TRUE if end of list was successfully marked
 *  FALSE otherwise
 */
BOOL WINAPI SdbEndWriteListTag(PDB db, TAGID tagid)
{
    if (!SdbCheckTagIDType(db, tagid, TAG_TYPE_LIST))
        return FALSE;

    *(DWORD*)&db->data[tagid + 2] = db->write_iter - tagid - 2;
    return TRUE;
}

/**************************************************************************
 *        SdbWriteStringRefTag                [APPHELP.@]
 *
 * Writes a stringref tag to specified database
 *
 * PARAMS
 *  db          [I] Handle to the shim database
 *  tag         [I] TAG which will be written
 *  tagid       [I] TAGID of the string tag refers to
 *
 * RETURNS
 *  TRUE if tag was successfully written
 *  FALSE otherwise
 *
 * NOTES
 *  Reference (tagid) is not checked for validity.
 */
BOOL WINAPI SdbWriteStringRefTag(PDB db, TAG tag, TAGID tagid)
{
    if (!SdbCheckTagType(tag, TAG_TYPE_STRINGREF))
        return FALSE;

    SdbWrite(db, &tag, 2);
    SdbWrite(db, &tagid, 4);
    return TRUE;
}

static BOOL WINAPI SdbFileExists(LPCWSTR path)
{
    DWORD attr = GetFileAttributesW(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

/**************************************************************************
 *        SdbGetMatchingExe                [APPHELP.@]
 *
 * Queries database for a specified exe
 *
 * PARAMS
 *  hsdb        [I] Handle to the shim database
 *  path        [I] Path to executable for which we query database
 *  module_name [I] Unused
 *  env         [I] Unused
 *  flags       [I] Unused
 *  result      [O] Pointer to structure in which query result shall be stored
 *
 * RETURNS
 *  TRUE if function succeeded
 *  FALSE otherwise
 */
BOOL WINAPI SdbGetMatchingExe(HSDB hsdb, LPCWSTR path, LPCWSTR module_name,
                              LPCWSTR env, DWORD flags, PSDBQUERYRESULT result)
{
    static const WCHAR fmt[] = {'%','s','%','s',0};
    BOOL ok;
    TAGID database, iter, attr;
    ATTRINFO attribs[28];
    DWORD attr_count;
    LPWSTR file_name;
    LPWSTR dir_path;
    WCHAR buffer[256];
    PDB db;

    db = hsdb->db;

    /* Extract file name */
    file_name = StrChrW(path, '\\') + 1;

    /* Extract directory path */
    memcpy(dir_path, path, (size_t)(file_name - path) * sizeof(WCHAR));

    /* Get information about executable required to match it with database entry */
    if (!SdbGetFileAttributes(path, &attribs, &attr_count))
        return FALSE;

    /* DATABASE is list TAG which contains all executables */
    database = SdbFindFirstTag(db, TAGID_ROOT, TAG_DATABASE);
    if (database == TAGID_NULL)
        return FALSE;

    /* EXE is list TAG which contains data required to match executable */
    iter = SdbFindFirstTag(db, database, TAG_EXE);

    /* Search for entry in database */
    while (iter != TAGID_NULL)
    {
        /* Check if exe name matches */
        attr = SdbFindFirstTag(db, iter, TAG_NAME);
        if (lstrcmpW(SdbGetStringTagPtr(db, attr), file_name) == 0)
        {
            /* Assume that entry is found (in case there are no "matching files") */
            ok = TRUE;

            /* Check if all "matching files" exist */
            /* TODO: check size/checksum as well */
            for (attr = SdbFindFirstTag(db, attr, TAG_MATCHING_FILE);
                 attr != TAGID_NULL; attr = SdbFindNextTag(db, iter, attr))
            {
                snprintfW(buffer, 256, fmt, dir_path, SdbGetStringTagPtr(db, attr));
                if (!SdbFileExists(buffer))
                    ok = FALSE;
            }

            /* Found it! */
            if (ok)
            {
                /* TODO: fill result data */
                /* TODO: there may be multiple matches */
                return TRUE;
            }
        }

        /* Continue iterating */
        iter = SdbFindNextTag(db, database, TAG_EXE);
    }

    /* Exe not found */
    return FALSE;
}

/**************************************************************************
 *        SdbGetAppPatchDir                [APPHELP.@]
 *
 * Retrieves AppPatch directory
 *
 * PARAMS
 *  db      [I] Handle to the shim database
 *  path    [O] Pointer to memory in which path shall be written
 *  size    [I] Size of the buffer in characters
 *
 * RETURNS
 *  This function does not return a value
 */
void WINAPI SdbGetAppPatchDir(HSDB db, LPWSTR path, DWORD size)
{
    static const WCHAR default_dir[] = {'C',':','\\','w','i','n','d','o','w','s','\\','A','p','p','P','a','t','c','h',0};
    DWORD string_size;

    /* In case function fails, path holds empty string */
    if (size > 0)
        *path = 0;

    if (!db)
    {
        string_size = (lstrlenW(default_dir) + 1) * sizeof(WCHAR);
        if (size >= string_size)
            StrCpyW(path, default_dir);
    }
    else
    {
        /* fixme */
    }
}

/**************************************************************************
 *        SdbOpenApphelpResourceFile                [APPHELP.@]
 *
 * Loads Apphelp resource dll
 *
 * PARAMS
 *  resource_file   [I] Path to the resource file
 *
 * RETURNS
 *  Success: Handle to the resource dll
 *  Failure: NULL
 *
 * NOTES
 *  If resource_file is NULL, default resource dll shall be loaded
 */
HMODULE WINAPI SdbOpenApphelpResourceFile(LPCWSTR resource_file)
{
    static const WCHAR default_dll[] = {'\\','e','n','-','U','S','\\','A','c','R','e','s','.','d','l','l','.','m','u','i',0};
    WCHAR buffer[128];

    if (!resource_file)
    {
        SdbGetAppPatchDir(NULL, buffer, 128);
        memcpy(buffer + lstrlenW(buffer), default_dll, (lstrlenW(default_dll) + 1) * sizeof(WCHAR));
    }

    return (HMODULE)LoadLibraryW(resource_file ? resource_file : buffer);
}

/**************************************************************************
 *        SdbLoadString                [APPHELP.@]
 *
 * Loads string from Apphelp resource dll
 *
 * PARAMS
 *  dll     [I] Handle to the resource dll
 *  id      [I] Identifier of the string
 *  buffer  [O] Buffer in which string shall be written
 *  size    [I] Size of the buffer
 *
 * RETURNS
 *  Success: Number of bytes copied
 *  Failure: 0
 *
 * NOTES
 *  If size is less than string size, only size characters shall be copied.
 *  If size is 0, buffer shall receive pointer to string and function shall
 *  return size of the string.
 */
DWORD WINAPI SdbLoadString(HMODULE dll, DWORD id, LPWSTR buffer, DWORD size)
{
    return LoadStringW(dll, id, buffer, size);
}

/**************************************************************************
 *        SdbInitDatabase                [APPHELP.@]
 *
 * Opens specified shim database file
 *
 * PARAMS
 *  flags   [I] Specifies type of path or predefined database
 *  path    [I] Path to the shim database file
 *
 * RETURNS
 *  Success: Handle to the opened shim database
 *  Failure: NULL
 *
 * NOTES
 *  Handle returned by this function may only be used by functions which
 *  take HSDB param thus differing it from SdbOpenDatabase.
 */
HSDB WINAPI SdbInitDatabase(DWORD flags, LPCWSTR path)
{
    static const WCHAR shim[] = {'s','y','s','m','a','i','n','.','s','d','b',0};
    static const WCHAR msi[] = {'m','s','i','m','a','i','n','.','s','d','b',0};
    static const WCHAR drivers[] = {0}; /* this one makes no sense in wine */
    LPCWSTR name;
    WCHAR buffer[128];
    HSDB sdb;

    sdb = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SDB));

    /* Check for predefined databases */
    if (((flags & HID_DATABASE_TYPE_MASK) == HID_DATABASE_TYPE_MASK) && path == NULL)
    {
        switch (flags & HID_DATABASE_TYPE_MASK)
        {
            case SDB_DATABASE_MAIN_SHIM: name = shim; break;
            case SDB_DATABASE_MAIN_MSI: name = msi; break;
            case SDB_DATABASE_MAIN_DRIVERS: name = drivers; break;
        }
        SdbGetAppPatchDir(NULL, buffer, 128);
        memcpy(buffer + lstrlenW(buffer), name, (lstrlenW(name) + 1) * sizeof(WCHAR));
    }

    sdb->db = SdbOpenDatabase(path ? path : buffer, flags & 0xF);
    return sdb;
}

/**************************************************************************
 *        SdbReleaseDatabase                [APPHELP.@]
 *
 * Closes shim database opened by SdbInitDatabase
 *
 * PARAMS
 *  hsdb   [I] Handle to the shim database
 */
void WINAPI SdbReleaseDatabase(HSDB hsdb)
{
    SdbCloseDatabase(hsdb->db);
    HeapFree(GetProcessHeap(), 0, hsdb);
}
