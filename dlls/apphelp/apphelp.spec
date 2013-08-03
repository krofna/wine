@ stub AllowPermLayer
@ stub ApphelpCheckExe
@ stdcall ApphelpCheckInstallShieldPackage(ptr wstr)
@ stdcall ApphelpCheckMsiPackage(ptr wstr)
@ stub ApphelpCheckRunApp
@ stub ApphelpCheckRunAppEx
@ stdcall ApphelpCheckShellObject(ptr long ptr)
@ stub ApphelpCreateAppcompatData
@ stub ApphelpFixMsiPackage
@ stub ApphelpFixMsiPackageExe
@ stub ApphelpFreeFileAttributes
@ stub ApphelpGetFileAttributes
@ stub ApphelpGetMsiProperties
@ stub ApphelpGetNTVDMInfo
@ stub ApphelpParseModuleData
@ stub ApphelpQueryModuleData
@ stub ApphelpQueryModuleDataEx
@ stub ApphelpUpdateCacheEntry
@ stub GetPermLayers
@ stub SdbAddLayerTagRefToQuery
@ stub SdbApphelpNotify
@ stub SdbApphelpNotifyExSdbApphelpNotifyEx
@ stdcall SdbBeginWriteListTag(ptr long)
@ stub SdbBuildCompatEnvVariables
@ stub SdbCloseApphelpInformation
@ stdcall SdbCloseDatabase(ptr)
@ stdcall SdbCloseDatabaseWrite(ptr)
@ stub SdbCloseLocalDatabase
@ stub SdbCommitIndexes
@ stdcall SdbCreateDatabase(wstr long)
@ stub SdbCreateHelpCenterURL
@ stub SdbCreateMsiTransformFile
@ stub SdbDeclareIndex
@ stub SdbDumpSearchPathPartCaches
@ stub SdbEnumMsiTransforms
@ stub SdbEscapeApphelpURL
@ stub SdbFindFirstDWORDIndexedTag
@ stub SdbFindFirstMsiPackage
@ stub SdbFindFirstMsiPackage_Str
@ stub SdbFindFirstNamedTag
@ stub SdbFindFirstStringIndexedTag
@ stub SdbFindFirstTag
@ stub SdbFindFirstTagRef
@ stub SdbFindNextDWORDIndexedTag
@ stub SdbFindNextMsiPackage
@ stub SdbFindNextStringIndexedTag
@ stub SdbFindNextTag
@ stub SdbFindNextTagRef
@ stub SdbFreeDatabaseInformation
@ stub SdbFreeFileInfo
@ stub SdbFreeFlagInfo
@ stub SdbGetAppCompatDataSize
@ stub SdbGetAppPatchDir
@ stdcall SdbGetBinaryTagData(ptr long)
@ stub SdbGetDatabaseID
@ stub SdbGetDatabaseInformation
@ stub SdbGetDatabaseInformationByName
@ stub SdbGetDatabaseMatch
@ stub SdbGetDatabaseVersion
@ stub SdbGetDllPath
@ stub SdbGetEntryFlags
@ stdcall SdbGetFileAttributes(wstr ptr ptr)
@ stub SdbGetFileImageType
@ stub SdbGetFileImageTypeEx
@ stub SdbGetFileInfo
@ stdcall SdbGetFirstChild(ptr long)
@ stub SdbGetIndex
@ stub SdbGetItemFromItemRef
@ stub SdbGetLayerName
@ stub SdbGetLayerTagRef
@ stub SdbGetLocalPDB
@ stub SdbGetMatchingExe
@ stub SdbGetMsiPackageInformation
@ stub SdbGetNamedLayer
@ stdcall SdbGetNextChild(ptr long long)
@ stub SdbGetNthUserSdb
@ stub SdbGetPermLayerKeys
@ stub SdbGetShowDebugInfoOption
@ stub SdbGetShowDebugInfoOptionValue
@ stub SdbGetStandardDatabaseGUID
@ stdcall SdbGetStringTagPtr(ptr long)
@ stdcall SdbGetTagFromTagID(ptr long)
@ stub SdbGrabMatchingInfo
@ stub SdbGrabMatchingInfoEx
@ stub SdbGUIDFromString
@ stub SdbGUIDToString
@ stub SdbInitDatabase
@ stub SdbInitDatabaseEx
@ stub SdbIsNullGUID
@ stub SdbIsStandardDatabase
@ stub SdbIsTagrefFromLocalDB
@ stub SdbIsTagrefFromMainDB
@ stub SdbLoadString
@ stub SdbMakeIndexKeyFromString
@ stub SdbOpenApphelpDetailsDatabase
@ stub SdbOpenApphelpDetailsDatabaseSP
@ stub SdbOpenApphelpInformation
@ stub SdbOpenApphelpInformationByID
@ stdcall SdbOpenApphelpResourceFile(wstr)
@ stdcall SdbOpenDatabase(wstr long)
@ stub SdbOpenDbFromGuid
@ stub SdbOpenLocalDatabase
@ stub SdbPackAppCompatData
@ stub SdbQueryApphelpInformation
@ stub SdbQueryBlockUpgrade
@ stub SdbQueryContext
@ stub SdbQueryData
@ stub SdbQueryDataEx
@ stub SdbQueryDataExTagID
@ stub SdbQueryFlagInfo
@ stub SdbQueryName
@ stub SdbQueryReinstallUpgrade
@ stub SdbReadApphelpData
@ stub SdbReadApphelpDetailsData
@ stdcall SdbReadBinaryTag(ptr long ptr long)
@ stub SdbReadBYTETag
@ stdcall SdbReadDWORDTag(ptr long long)
@ stub SdbReadDWORDTagRef
@ stub SdbReadEntryInformation
@ stub SdbReadMsiTransformInfo
@ stub SdbReadPatchBits
@ stdcall SdbReadQWORDTag(ptr long int64)
@ stub SdbReadQWORDTagRef
@ stdcall SdbReadStringTag(ptr long wstr long)
@ stub SdbReadStringTagRef
@ stub SdbReadWORDTagRef
@ stub SdbRegisterDatabase
@ stub SdbReleaseDatabase
@ stub SdbReleaseMatchingExe
@ stub SdbResolveDatabase
@ stub SdbSetApphelpDebugParameters
@ stub SdbSetEntryFlags
@ stub SdbSetImageType
@ stub SdbSetPermLayerKeys
@ stub SdbShowApphelpDialog
@ stub SdbShowApphelpFromQuery
@ stub SdbStartIndexing
@ stub SdbStopIndexing
@ stub SdbStringDuplicate
@ stub SdbStringReplace
@ stub SdbStringReplaceArray
@ stub SdbTagIDToTagRef
@ stdcall SdbTagToString(long)
@ stub SdbUnregisterDatabase
@ stub SdbWriteBinaryTag
@ stdcall SdbWriteBinaryTagFromFile(ptr long wstr)
@ stub SdbWriteBYTETag
@ stdcall SdbWriteDWORDTag(ptr long long)
@ stdcall SdbWriteNULLTag(ptr long)
@ stdcall SdbWriteQWORDTag(ptr long int64)
@ stub SdbWriteStringRefTag
@ stdcall SdbWriteStringTag(ptr long wstr)
@ stub SdbWriteStringTagDirect
@ stdcall SdbWriteWORDTag(ptr long long)
@ stub SE_DllLoaded
@ stub SE_DllUnloaded
@ stub SE_GetHookAPIs
@ stub SE_GetMaxShimCount
@ stub SE_GetProcAddressLoad
@ stub SE_GetShimCount
@ stub SE_InstallAfterInit
@ stub SE_InstallBeforeInit
@ stub SE_IsShimDll
@ stub SE_LdrEntryRemoved
@ stub SE_ProcessDying
@ stub SetPermLayers
@ stub ShimDbgPrint
@ stub ShimDumpCache
@ stub ShimFlushCache
