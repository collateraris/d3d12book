#include <config_sys/Config.h>

#include <cassert>

using namespace dx12demo::core;

void Config::Load()
{
    bool flag = (mDocument.LoadFile(mFileName.c_str()) == tinyxml2::XML_SUCCESS);
    assert(flag);
    (void)flag;
}

void Config::Save()
{
    bool flag = (mDocument.SaveFile(mFileName.c_str()) == tinyxml2::XML_SUCCESS);
    assert(flag);
    (void)flag;
}

