/*
 * Copyright 2012 Detlef Riekenberg
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

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>
#include <initguid.h>
#include <exdisp.h>
#include <shlobj.h>
#include <urlmon.h>
#include <appcompatapi.h>
#include <winbase.h>
#include "../apphelp.h"

#include "wine/test.h"

static HMODULE hdll;
static BOOL (WINAPI *pApphelpCheckShellObject)(REFCLSID, BOOL, ULONGLONG *);
static LPCWSTR (WINAPI *pSdbTagToString)(TAG);
static PDB (WINAPI *pSdbOpenDatabase)(LPCWSTR, PATH_TYPE);
static void (WINAPI *pSdbCloseDatabase)(PDB);
static TAG (WINAPI *pSdbGetTagFromTagID)(PDB, TAGID);
static DWORD (WINAPI *pSdbReadDWORDTag)(PDB, TAGID, DWORD);
static QWORD (WINAPI *pSdbReadQWORDTag)(PDB, TAGID, QWORD);
static TAGID (WINAPI *pSdbGetFirstChild)(PDB, TAGID);
static TAGID (WINAPI *pSdbGetNextChild)(PDB, TAGID, TAGID);
static LPWSTR (WINAPI *pSdbGetStringTagPtr)(PDB, TAGID);
static BOOL (WINAPI *pSdbReadStringTag)(PDB, TAGID, LPWSTR, DWORD);

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

DEFINE_GUID(test_Microsoft_Browser_Architecture, 0xa5e46e3a, 0x8849, 0x11d1, 0x9d, 0x8c, 0x00, 0xc0, 0x4f, 0xc9, 0x9d, 0x61);
DEFINE_GUID(CLSID_MenuBand, 0x5b4dae26, 0xb807, 0x11d0, 0x98, 0x15, 0x00, 0xc0, 0x4f, 0xd9, 0x19, 0x72);
DEFINE_GUID(test_UserAssist, 0xdd313e04, 0xfeff, 0x11d1, 0x8e, 0xcd, 0x00, 0x00, 0xf8, 0x7a, 0x47, 0x0c);

static const CLSID * objects[] = {
    &GUID_NULL,
    /* used by IE */
    &test_Microsoft_Browser_Architecture,
    &CLSID_MenuBand,
    &CLSID_ShellLink,
    &CLSID_ShellWindows,
    &CLSID_InternetSecurityManager,
    &test_UserAssist,
    NULL,};

static const char *debugstr_guid(REFIID riid)
{
    static char buf[50];

    sprintf(buf, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
}

static void test_ApphelpCheckShellObject(void)
{
    ULONGLONG flags;
    BOOL res;
    int i;

    if (!pApphelpCheckShellObject)
    {
        win_skip("ApphelpCheckShellObject not available\n");
        return;
    }

    for (i = 0; objects[i]; i++)
    {
        flags = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pApphelpCheckShellObject(objects[i], FALSE, &flags);
        ok(res && (flags == 0), "%s 0: got %d and 0x%x%08x with 0x%x (expected TRUE and 0)\n",
            debugstr_guid(objects[i]), res, (ULONG)(flags >> 32), (ULONG)flags, GetLastError());

        flags = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pApphelpCheckShellObject(objects[i], TRUE, &flags);
        ok(res && (flags == 0), "%s 1: got %d and 0x%x%08x with 0x%x (expected TRUE and 0)\n",
            debugstr_guid(objects[i]), res, (ULONG)(flags >> 32), (ULONG)flags, GetLastError());

    }

    /* NULL as pointer to flags is checked */
    SetLastError(0xdeadbeef);
    res = pApphelpCheckShellObject(&GUID_NULL, FALSE, NULL);
    ok(res, "%s 0: got %d with 0x%x (expected != FALSE)\n", debugstr_guid(&GUID_NULL), res, GetLastError());

    /* NULL as CLSID* crash on Windows */
    if (0)
    {
        flags = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pApphelpCheckShellObject(NULL, FALSE, &flags);
        trace("NULL as CLSID*: got %d and 0x%x%08x with 0x%x\n", res, (ULONG)(flags >> 32), (ULONG)flags, GetLastError());
    }
}

static void test_SdbTagToString(void)
{
    WORD i;
    static const TAG invalid_values[] = {
        1, TAG_TYPE_WORD, TAG_TYPE_MASK,
        TAG_TYPE_DWORD | 0xFF,
        TAG_TYPE_DWORD | (0x800 + 0xEE),
        0x900, 0xFFFF, 0xDEAD, 0xBEEF
    };
    static const WCHAR invalid[] = {'I','n','v','a','l','i','d','T','a','g',0};
    LPCWSTR ret;

    for (i = 0; i < 9; ++i)
    {
        ret = pSdbTagToString(invalid_values[i]);
        ok(lstrcmpW(ret, invalid) == 0, "unexpected string %s, should be %s\n",
           wine_dbgstr_w(ret), wine_dbgstr_w(invalid));
    }
}

static void Write(HANDLE file, LPCVOID buffer, DWORD size)
{
    WriteFile(file, buffer, size, NULL, NULL);
}

static void test_Sdb(void)
{
    static const WCHAR path[] = {'t','e','m','p',0};
    static const WCHAR tag_size_string[] = {'S','I','Z','E',0};
    static const WCHAR tag_flag_lua_string[] = {'F','L','A','G','_','L','U','A',0};
    static const TAG tags[5] = {
        TAG_SIZE, TAG_FLAG_LUA, TAG_NAME,
        TAG_STRINGTABLE, TAG_STRINGTABLE_ITEM
    };
    WCHAR buffer[6] = {0};
    PDB db;
    HANDLE file; /* temp file created for testing purpose */
    DWORD two = 2, one = 1; /* version magic */
    DWORD value = 5;
    QWORD value2 = 0xDEADBEEF;
    TAG tag;
    DWORD path_size = 5 * sizeof(WCHAR); /* size of path variable */
    TAGID tagid, stringref = 6, stringtable = path_size + 6;
    LPCWSTR string;

    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if (file == INVALID_HANDLE_VALUE)
    {
        ok(FALSE, "could not perform test because temp file could not be created\n");
        return;
    }

    Write(file, &two, 4);
    Write(file, &one, 4);
    Write(file, "sdbf", 4);
    Write(file, &tags[0], 2);
    Write(file, &value, 4);
    Write(file, &tags[1], 2);
    Write(file, &value2, 8);
    Write(file, &tags[2], 2);
    Write(file, &stringref, 4);
    Write(file, &tags[3], 2);
    Write(file, &stringtable, 4);
    Write(file, &tags[4], 2);
    Write(file, &path_size, 4);
    Write(file, path, path_size);
    CloseHandle(file);

    db = pSdbOpenDatabase(path, DOS_PATH);
    ok(db != NULL, "unexpected NULL handle\n");

    tagid = pSdbGetFirstChild(db, TAGID_ROOT);
    ok(tagid == _TAGID_ROOT, "unexpected tagid %u, expected %u\n", tagid, _TAGID_ROOT);

    tag = pSdbGetTagFromTagID(db, tagid);
    ok(tag == TAG_SIZE, "unexpected tag 0x%x, expected 0x%x\n", tag, TAG_SIZE);

    string = pSdbTagToString(tag);
    ok(lstrcmpW(string, tag_size_string) == 0, "unexpected string %s, expected %s\n",
       wine_dbgstr_w(string), wine_dbgstr_w(tag_size_string));

    value = pSdbReadDWORDTag(db, tagid, 0);
    ok(value == 5, "unexpected value %u, expected 5\n", value);

    tagid = pSdbGetNextChild(db, TAGID_ROOT, tagid);
    ok(tagid == _TAGID_ROOT + sizeof(TAG) + sizeof(DWORD), "unexpected tagid %u, expected %u\n",
       tagid, _TAGID_ROOT + sizeof(TAG) + sizeof(DWORD));

    tag = pSdbGetTagFromTagID(db, tagid);
    ok (tag == TAG_FLAG_LUA, "unexpected tag 0x%x, expected 0x%x\n", tag, TAG_FLAG_LUA);

    string = pSdbTagToString(tag);
    ok(lstrcmpW(string, tag_flag_lua_string) == 0, "unexpected string %s, expected %s\n",
       wine_dbgstr_w(string), wine_dbgstr_w(tag_flag_lua_string));

    value2 = pSdbReadQWORDTag(db, tagid, 0);
    ok(value2 == 0xDEADBEEF, "unexpected value 0x%llx, expected 0x%x\n", value2, 0xDEADBEEF);

    tagid = pSdbGetNextChild(db, TAGID_ROOT, tagid);
    string = pSdbGetStringTagPtr(db, tagid);
    ok (string && (lstrcmpW(string, path) == 0), "unexpected string %s, expected %s\n",
        wine_dbgstr_w(string), wine_dbgstr_w(path));

    tagid = pSdbGetNextChild(db, TAGID_ROOT, tagid);
    tagid = pSdbGetFirstChild(db, tagid);

    string = pSdbGetStringTagPtr(db, tagid);
    ok (string && (lstrcmpW(string, path) == 0), "unexpected string %s, expected %s\n",
        wine_dbgstr_w(string), wine_dbgstr_w(path));

    ok (pSdbReadStringTag(db, tagid, buffer, 6), "failed to write string to buffer\n");
    ok (!pSdbReadStringTag(db, tagid, buffer, 3), "string was written to buffer, but failure was expected");

    pSdbCloseDatabase(db);
    DeleteFileW(path);
}

START_TEST(apphelp)
{

    hdll = LoadLibrary("apphelp.dll");
    if (!hdll) {
        win_skip("apphelp.dll not available\n");
        return;
    }
    pApphelpCheckShellObject = (void *) GetProcAddress(hdll, "ApphelpCheckShellObject");
    pSdbTagToString = (void *) GetProcAddress(hdll, "SdbTagToString");
    pSdbOpenDatabase = (void *) GetProcAddress(hdll, "SdbOpenDatabase");
    pSdbCloseDatabase = (void *) GetProcAddress(hdll, "SdbCloseDatabase");
    pSdbGetTagFromTagID = (void *) GetProcAddress(hdll, "SdbGetTagFromTagID");
    pSdbGetFirstChild = (void *) GetProcAddress(hdll, "SdbGetFirstChild");
    pSdbReadDWORDTag = (void *) GetProcAddress(hdll, "SdbReadDWORDTag");
    pSdbReadQWORDTag = (void *) GetProcAddress(hdll, "SdbReadQWORDTag");
    pSdbGetNextChild = (void *) GetProcAddress(hdll, "SdbGetNextChild");
    pSdbGetStringTagPtr = (void *) GetProcAddress(hdll, "SdbGetStringTagPtr");
    pSdbReadStringTag = (void *) GetProcAddress(hdll, "SdbReadStringTag");

    test_Sdb();
    test_SdbTagToString();
    test_ApphelpCheckShellObject();
}
