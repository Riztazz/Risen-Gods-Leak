#include "GitRevision.h"
#include "CompilerDefs.h"
#include "revision_data.h"

char const* GitRevision::GetHash()
{
    return _HASH;
}

char const* GitRevision::GetDate()
{
    return _DATE;
}

char const* GitRevision::GetBranch()
{
    return _BRANCH;
}

char const* GitRevision::GetCMakeCommand()
{
    return _CMAKE_COMMAND;
}

char const* GitRevision::GetBuildDirectory()
{
    return _BUILD_DIRECTORY;
}

char const* GitRevision::GetSourceDirectory()
{
    return _SOURCE_DIRECTORY;
}

char const* GitRevision::GetMySQLExecutable()
{
    return _MYSQL_EXECUTABLE;
}

char const* GitRevision::GetFullDatabase()
{
    return _FULL_DATABASE;
}

#define _PACKAGENAME "TrinityCore"
#define _COLORCODE "cffff69b4"

char const* GitRevision::GetFullVersion(bool wowChat/* = false*/)
{
    if (wowChat)
    {
#if PLATFORM == PLATFORM_WINDOWS
# ifdef _WIN64
        return _PACKAGENAME " rev. |" _COLORCODE VER_PRODUCTVERSION_STR " (Win64, " _BUILD_DIRECTIVE ")|r";
# else
        return _PACKAGENAME " rev. |" _COLORCODE VER_PRODUCTVERSION_STR " (Win32, " _BUILD_DIRECTIVE ")|r";
# endif
#else
        return _PACKAGENAME " rev. |" _COLORCODE VER_PRODUCTVERSION_STR " (Unix, " _BUILD_DIRECTIVE ")|r";
#endif
    }
    else
    {
#if PLATFORM == PLATFORM_WINDOWS
# ifdef _WIN64
        return _PACKAGENAME " rev. " VER_PRODUCTVERSION_STR " (Win64, " _BUILD_DIRECTIVE ")";
# else
        return _PACKAGENAME " rev. " VER_PRODUCTVERSION_STR " (Win32, " _BUILD_DIRECTIVE ")";
# endif
#else
        return _PACKAGENAME " rev. " VER_PRODUCTVERSION_STR " (Unix, " _BUILD_DIRECTIVE ")";
#endif
    }
}

char const* GitRevision::GetCompanyNameStr()
{
    return VER_COMPANYNAME_STR;
}

char const* GitRevision::GetLegalCopyrightStr()
{
    return VER_LEGALCOPYRIGHT_STR;
}

char const* GitRevision::GetFileVersionStr()
{
    return VER_FILEVERSION_STR;
}

char const* GitRevision::GetProductVersionStr()
{
    return VER_PRODUCTVERSION_STR;
}

char const* GitRevision::GetBuildDirective()
{
    return _BUILD_DIRECTIVE;
}
