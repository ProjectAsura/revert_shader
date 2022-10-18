//-------------------------------------------------------------------------------------------------
// File : StringHelper.cpp
// Desc : String Helper.
// Copyright(c) Project Asura. All right reserved.
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------
#include "StringHelper.h"
#include <algorithm>
#include <cstdarg>
#include <regex>


///////////////////////////////////////////////////////////////////////////////////////////////////
// StringHelper class
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//      文字列を置換します.
//-------------------------------------------------------------------------------------------------
std::string StringHelper::Replace
(
    const std::string&  input,
    std::string         pattern,
    std::string         replace)
{
    std::string result = input;
    auto pos = result.find( pattern );

    while( pos != std::string::npos )
    {
        result.replace( pos, pattern.length(), replace );
        pos = result.find( pattern, pos + replace.length() );
    }

    return result;
}

//-------------------------------------------------------------------------------------------------
//      文字列を置換します.
//-------------------------------------------------------------------------------------------------
std::wstring StringHelper::Replace
(
    const std::wstring&  input,
    std::wstring         pattern,
    std::wstring         replace)
{
    std::wstring result = input;
    auto pos = result.find( pattern );

    while( pos != std::wstring::npos )
    {
        result.replace( pos, pattern.length(), replace );
        pos = result.find( pattern, pos + replace.length() );
    }

    return result;
}

//-------------------------------------------------------------------------------------------------
//      小文字に変換します.
//-------------------------------------------------------------------------------------------------
std::string StringHelper::ToLower(std::string value)
{
    std::string result = value;
    std::transform( result.begin(), result.end(), result.begin(), tolower );
    return result;
}

//-------------------------------------------------------------------------------------------------
//      小文字に変換します.
//-------------------------------------------------------------------------------------------------
std::wstring StringHelper::ToLower(std::wstring value)
{
    std::wstring result = value;
    std::transform( result.begin(), result.end(), result.begin(), tolower );
    return result;
}

//-------------------------------------------------------------------------------------------------
//      大文字に変換します.
//-------------------------------------------------------------------------------------------------
std::string StringHelper::ToUpper(std::string value)
{
    std::string result = value;
    std::transform( result.begin(), result.end(), result.begin(), toupper );
    return result;
}

//-------------------------------------------------------------------------------------------------
//      大文字に変換します.
//-------------------------------------------------------------------------------------------------
std::wstring StringHelper::ToUpper(std::wstring value)
{
    std::wstring result = value;
    std::transform( result.begin(), result.end(), result.begin(), toupper );
    return result;
}

//-------------------------------------------------------------------------------------------------
//      部分文字列に分割します.
//-------------------------------------------------------------------------------------------------
std::vector<std::string> StringHelper::Split(const std::string& input, std::string split)
{
    std::vector<std::string> result;
    auto temp = input;
    auto pos = temp.find(split);

    while( pos != std::string::npos )
    {
        auto item = temp.substr(0, pos);
        if (item != split && !item.empty())
        { result.push_back(item); }
        temp = temp.substr(pos + 1);
        pos = temp.find(split);
    }

    if (!temp.empty())
    { result.push_back(temp); }

    return result;
}

//-------------------------------------------------------------------------------------------------
//      部分文字列に分割します.
//-------------------------------------------------------------------------------------------------
std::vector<std::wstring> StringHelper::Split(const std::wstring& input, std::wstring split)
{
    std::vector<std::wstring> result;
    auto temp = input;
    auto pos = temp.find(split);

    while( pos != std::wstring::npos )
    {
        auto item = temp.substr(0, pos);
        if (item != split && !item.empty())
        { result.push_back(item); }
        temp = temp.substr(pos + 1);
        pos = temp.find(split);
    }

    if (!temp.empty())
    { result.push_back(temp); }

    return result;
}

//-------------------------------------------------------------------------------------------------
//      文字列を含むかどうかチェックします.
//-------------------------------------------------------------------------------------------------
int StringHelper::Contain(const std::string& input, std::string value)
{
    int count = 0;
    auto temp = input;
    auto pos = temp.find(value);

    while (pos != std::string::npos)
    {
        count++;
        temp = temp.substr(pos + 1);
        pos = temp.find(value, pos);
    }

    return count;
}

//-------------------------------------------------------------------------------------------------
//      文字列を含むかどうかチェックします.
//-------------------------------------------------------------------------------------------------
int StringHelper::Contain(const std::wstring& input, std::wstring value)
{ 
    int count = 0;
    auto temp = input;
    auto pos = temp.find(value);

    while (pos != std::wstring::npos)
    {
        count++;
        temp = temp.substr(pos + 1);
        pos = temp.find(value, pos);
    }

    return count;
}

//-------------------------------------------------------------------------------------------------
//      整形します.
//-------------------------------------------------------------------------------------------------
std::string StringHelper::Format(const char* format, ...)
{
    char buf[4096];

    va_list arg;

    va_start( arg, format );
    vsprintf_s( buf, format, arg );
    va_end( arg );

    return buf;
}

//-------------------------------------------------------------------------------------------------
//      整形します.
//-------------------------------------------------------------------------------------------------
std::wstring StringHelper::Format(const wchar_t* format, ...)
{
    wchar_t buf[4096];

    va_list arg;

    va_start( arg, format );
    vswprintf_s( buf, format, arg );
    va_end( arg );

    return buf;
}

//-------------------------------------------------------------------------------------------------
//      スウィズル数を取得します.
//-------------------------------------------------------------------------------------------------
int StringHelper::GetSwizzleCount(std::string value)
{
    std::string temp = value;
    // 即値は除外.
    auto pos = temp.find("float");
    if (pos != std::string::npos)
    { return 0; }

    pos = temp.find("uint");
    if (pos != std::string::npos)
    { return 0; }

    pos = temp.find("int");
    if (pos != std::string::npos)
    { return 0; }

    // リテラルも除外
    pos = temp.find("l(");
    if (pos != std::string::npos)
    { return 0; }

    // 絶対値記号は取り除いておく.
    auto abs1 = temp.find("abs(");
    auto abs2 = temp.find(")");
    if (abs1 != std::string::npos 
     && abs2 != std::string::npos
     && abs1 != abs2)
    {
        temp = temp.substr(abs1 + 4, abs2 - abs1 - 4);
    }

    pos = temp.rfind(".");
    if (pos == std::string::npos)
    { return 0; }

    auto swz = temp.substr(pos);
    auto cnt = swz.size();
    if (cnt > 5 || cnt <= 1)
    { return 0; }

    char plt[4] =  { 'x', 'y', 'z', 'w' };
    auto ret = 0;

    // 文字の妥当性をチェック.
    for(auto i=1; i<cnt; ++i)
    {
        for(auto j=0; j<4; ++j)
        {
            if (plt[j] == swz[i])
            {
                ret++;
                break;
            }
        }
    }

    // スウィズルパターンにマッチすればカウントを返却.
    if (ret == cnt - 1)
    { return ret;}

    // xyzw以外の文字が含まれている場合はスウィズルではないので，0を返却.
    return 0;
}

//-------------------------------------------------------------------------------------------------
//      スウィズル数を取得します.
//-------------------------------------------------------------------------------------------------
int StringHelper::GetSwizzleCount(std::wstring value)
{
    std::wstring temp = value;

    // 即値は除外.
    auto pos = temp.find(L"float");
    if (pos != std::wstring::npos)
    { return 0; }

    pos = temp.find(L"uint");
    if (pos != std::wstring::npos)
    { return 0; }

    pos = temp.find(L"int");
    if (pos != std::wstring::npos)
    { return 0; }

    // リテラルも除外
    pos = temp.find(L"l(");
    if (pos != std::wstring::npos)
    { return 0; }

    // 絶対値記号は取り除いておく.
    auto abs1 = temp.find(L"abs(");
    auto abs2 = temp.find(L")");
    if (abs1 != std::wstring::npos 
     && abs2 != std::wstring::npos
     && abs1 != abs2)
    {
        temp = temp.substr(abs1 + 4, abs2 - abs1 - 4);
    }

    pos = temp.rfind(L".");
    if (pos == std::wstring::npos)
    { return 0; }

    auto swz = value.substr(pos);
    auto cnt = swz.size();
    if (cnt > 5 || cnt <= 1)
    { return 0; }

    wchar_t plt[4] =  { L'x', L'y', L'z', L'w' };
    auto ret = 0;

    // 文字の妥当性をチェック.
    for(auto i=1; i<cnt; ++i)
    {
        for(auto j=0; j<4; ++j)
        {
            if (plt[j] == swz[i])
            {
                ret++;
                break;
            }
        }
    }

    // スウィズルパターンにマッチすればカウントを返却.
    if (ret == cnt - 1)
    { return ret;}

    // xyzw以外の文字が含まれている場合はスウィズルではないので，0を返却.
    return 0;
}

//-------------------------------------------------------------------------------------------------
//      スウィズルを取得します.
//-------------------------------------------------------------------------------------------------
std::string StringHelper::GetSwizzle(std::string value, int count)
{
    std::string temp = value;

    // 即値は除外.
    auto pos = temp.find("float");
    if (pos != std::string::npos)
    { return std::string(); }

    pos = temp.find("uint");
    if (pos != std::string::npos)
    { return std::string(); }

    pos = temp.find("int");
    if (pos != std::string::npos)
    { return std::string(); }

    // リテラルも除外
    pos = temp.find("l(");
    if (pos != std::string::npos)
    { return std::string(); }

    // 絶対値記号は取り除いておく.
    auto abs1 = temp.find("abs(");
    auto abs2 = temp.find(")");
    if (abs1 != std::string::npos 
     && abs2 != std::string::npos
     && abs1 != abs2)
    {
        temp = temp.substr(abs1 + 4, abs2 - abs1 - 4);
    }

    pos = temp.rfind(".");
    if (pos == std::string::npos)
    { return std::string(); }

    if (count < 0)
    { count = 0; }  
    if (count > 4)
    { count = 4; }

    auto swz = temp.substr(pos);
    auto cnt = swz.size();
    if (cnt > 5 || cnt <= 1)
    { return std::string(); }

    if (cnt > count + 1)
    { cnt = count + 1; }

    char plt[4] =  { 'x', 'y', 'z', 'w' };
    auto ret = 0;

    // 文字の妥当性をチェック.
    for(auto i=1; i<cnt; ++i)
    {
        for(auto j=0; j<4; ++j)
        {
            if (plt[j] == swz[i])
            {
                ret++;
                break;
            }
        }
    }

    // スウィズルパターンにマッチすればカウントを返却.
    if (ret == cnt - 1)
    { return temp.substr(pos, cnt); }

    // xyzw以外の文字が含まれている場合はスウィズルではないので，0を返却.
    return std::string();
}

//-------------------------------------------------------------------------------------------------
//      スウィズルを取得します.
//-------------------------------------------------------------------------------------------------
std::wstring StringHelper::GetSwizzle(std::wstring value, int count)
{
    std::wstring temp = value;

    auto pos = temp.find(L"float");
    if (pos != std::wstring::npos)
    { return std::wstring(); }

    pos = temp.find(L"uint");
    if (pos != std::wstring::npos)
    { return std::wstring(); }

    pos = temp.find(L"int");
    if (pos != std::wstring::npos)
    { return std::wstring(); }

    pos = temp.find(L"l(");
    if (pos != std::wstring::npos)
    { return std::wstring(); }

    // 絶対値記号は取り除いておく.
    auto abs1 = temp.find(L"abs(");
    auto abs2 = temp.find(L")");
    if (abs1 != std::wstring::npos 
     && abs2 != std::wstring::npos
     && abs1 != abs2)
    {
        temp = temp.substr(abs1 + 4, abs2 - abs1 - 4);
    }

    pos = temp.rfind(L".");
    if (pos == std::wstring::npos)
    { return std::wstring(); }

    if (count < 0)
    { count = 0; }  
    if (count > 4)
    { count = 4; }

    auto swz = temp.substr(pos);
    auto cnt = swz.size();
    if (cnt > 5 || cnt <= 1)
    { return std::wstring(); }

    wchar_t plt[4] =  { L'x', L'y', L'z', L'w' };
    auto ret = 0;

    if (cnt > count + 1)
    { cnt = count + 1; }

    // 文字の妥当性をチェック.
    for(auto i=1; i<cnt; ++i)
    {
        for(auto j=0; j<4; ++j)
        {
            if (plt[j] == swz[i])
            {
                ret++;
                break;
            }
        }
    }

    // スウィズルパターンにマッチすればカウントを返却.
    if (ret == cnt - 1)
    { return temp.substr(pos, cnt); }

    // xyzw以外の文字が含まれている場合はスウィズルではないので，0を返却.
    return std::wstring();
}

//-------------------------------------------------------------------------------------------------
//      スウィズル数を考慮して文字列を取得します.
//-------------------------------------------------------------------------------------------------
std::string StringHelper::GetWithSwizzle(std::string value, int count)
{
    bool strip = (count == -1);
    std::string temp = value;

    if (count < 0)
    { count = 4; }
    if (count > 4)
    { count = 4; }

    auto pos = temp.find("float");
    if (pos != std::string::npos)
    { return value; }

    pos = temp.find("uint");
    if (pos != std::string::npos)
    { return value; }

    pos = temp.find("int");
    if (pos != std::string::npos)
    { return value; }

    pos = temp.find("l(");
    if (pos != std::string::npos)
    { return value; }

    auto hasAbs = false;
    // 絶対値記号は取り除いておく.
    auto abs1 = temp.find("abs(");
    auto abs2 = temp.find(")");
    if (abs1 != std::string::npos 
     && abs2 != std::string::npos
     && abs1 != abs2)
    {
        temp = temp.substr(abs1 + 4, abs2 - abs1 - 4);
        hasAbs = true;
    }

    pos = temp.rfind(".");
    if (pos == std::string::npos)
    { return value; }

    auto name = temp.substr(0, pos);
    if (count == 0)
    { return name;}

    auto swz = temp.substr(pos);
    auto cnt = swz.size();
    if (cnt > 5)
    { return value; }

    if (cnt > count + 1)
    { cnt = count + 1; }

    char plt[4] =  { 'x', 'y', 'z', 'w' };
    auto ret = 0;

    // 文字の妥当性をチェック.
    for(auto i=1; i<cnt; ++i)
    {
        for(auto j=0; j<4; ++j)
        {
            if (plt[j] == swz[i])
            {
                ret++;
                break;
            }
        }
    }

    if (ret == cnt - 1)
    {
        auto swizzle = swz.substr(0, cnt);

        if (strip && swizzle == ".xyzw" )
        { swizzle = ""; }

        auto result = name + swizzle;
        if (hasAbs)
        { result = "abs(" + result + ")"; }
        return result;
    }

    return value;
}

//-------------------------------------------------------------------------------------------------
//      スウィズル数を考慮して文字列を取得します.
//-------------------------------------------------------------------------------------------------
std::wstring StringHelper::GetWithSwizzle(std::wstring value, int count)
{
    bool strip = (count == -1);
    auto temp = value;

    if (count < 0)
    { count = 4; }
    if (count > 4)
    { count = 4; }

    auto pos = temp.find(L"float");
    if (pos != std::wstring::npos)
    { return value; }

    pos = temp.find(L"uint");
    if (pos != std::wstring::npos)
    { return value; }

    pos = temp.find(L"int");
    if (pos != std::wstring::npos)
    { return value; }

    pos = temp.find(L"l(");
    if (pos != std::wstring::npos)
    { return value; }

    auto hasAbs = false;
    // 絶対値記号は取り除いておく.
    auto abs1 = temp.find(L"abs(");
    auto abs2 = temp.find(L")");
    if (abs1 != std::wstring::npos 
     && abs2 != std::wstring::npos
     && abs1 != abs2)
    {
        temp = temp.substr(abs1 + 4, abs2 - abs1 - 4);
        hasAbs = true;
    }

    pos = temp.rfind(L".");
    if (pos == std::wstring::npos)
    { return value; }

    auto name = temp.substr(0, pos);
    if (count == 0)
    { return name;}

    auto swz = temp.substr(pos);
    auto cnt = swz.size();
    if (cnt > 5)
    { return value; }

    if (cnt > count + 1)
    { cnt = count + 1; }

    wchar_t plt[4] =  { L'x', L'y', L'z', L'w' };
    auto ret = 0;

    // 文字の妥当性をチェック.
    for(auto i=1; i<cnt; ++i)
    {
        for(auto j=0; j<4; ++j)
        {
            if (plt[j] == swz[i])
            {
                ret++;
                break;
            }
        }
    }

    if (ret == cnt - 1)
    {
        auto swizzle = swz.substr(0, cnt);

        if (strip && swizzle == L".xyzw" )
        { swizzle = L""; }

        auto result = name + swizzle;
        if (hasAbs)
        { result = L"abs(" + result + L")"; }
        return result;
    }

    return value;
}


//-------------------------------------------------------------------------------------------------
//      スウィズル数を考慮して文字列を取得します.
//-------------------------------------------------------------------------------------------------
std::string StringHelper::GetWithSwizzleEx(std::string value, int count, const int* pIndex)
{
    bool strip = (count == -1);
    auto temp = value;

    if (count < 0)
    { count = 4; }
    if (count > 4)
    { count = 4; }

    auto pos = temp.find("float");
    if (pos != std::string::npos)
    { return value; }

    pos = temp.find("uint");
    if (pos != std::string::npos)
    { return value; }

    pos = temp.find("int");
    if (pos != std::string::npos)
    { return value; }

    pos = temp.find("l(");
    if (pos != std::string::npos)
    { return value; }

    auto hasAbs = false;
    // 絶対値記号は取り除いておく.
    auto abs1 = temp.find("abs(");
    auto abs2 = temp.find(")");
    if (abs1 != std::string::npos 
     && abs2 != std::string::npos
     && abs1 != abs2)
    {
        temp = temp.substr(abs1 + 4, abs2 - abs1 - 4);
        hasAbs = true;
    }

    pos = temp.rfind(".");
    if (pos == std::string::npos)
    { return value; }

    auto name = temp.substr(0, pos);
    if (count == 0)
    { return name;}

    auto swz = temp.substr(pos);
    auto cnt = swz.size();
    if (cnt > 5)
    { return value; }

    if (cnt > count + 1)
    { cnt = count + 1; }

    char plt[4] =  { 'x', 'y', 'z', 'w' };
    auto ret = 0;

    // 文字の妥当性をチェック.
    for(auto i=1; i<cnt; ++i)
    {
        for(auto j=0; j<4; ++j)
        {
            if (plt[j] == swz[i])
            {
                ret++;
                break;
            }
        }
    }

    if (ret == cnt - 1)
    {
        auto swizzle = std::string(".");

        if (ret > 1)
        {
            for(auto i=0; i<ret; ++i)
            {
                swizzle += swz[pIndex[i] + 1];
            }
        }
        else
        {
            swizzle += swz[1];
        }

        if (strip && swizzle == ".xyzw" )
        { swizzle = ""; }

        auto result = name + swizzle;
        if (hasAbs)
        { result = "abs(" + result + ")"; }
        return result;
    }

    return value;
}

//-------------------------------------------------------------------------------------------------
//      スウィズル数を考慮して文字列を取得します.
//-------------------------------------------------------------------------------------------------
std::wstring StringHelper::GetWithSwizzleEx(std::wstring value, int count, const int* pIndex)
{
    bool strip = (count == -1);
    auto temp = value;

    if (count < 0)
    { count = 4; }
    if (count > 4)
    { count = 4; }

    auto pos = temp.find(L"float");
    if (pos != std::wstring::npos)
    { return value; }

    pos = temp.find(L"uint");
    if (pos != std::wstring::npos)
    { return value; }

    pos = temp.find(L"int");
    if (pos != std::wstring::npos)
    { return value; }

    pos = temp.find(L"l(");
    if (pos != std::wstring::npos)
    { return value; }

    auto hasAbs = false;
    // 絶対値記号は取り除いておく.
    auto abs1 = temp.find(L"abs(");
    auto abs2 = temp.find(L")");
    if (abs1 != std::wstring::npos 
     && abs2 != std::wstring::npos
     && abs1 != abs2)
    {
        temp = temp.substr(abs1 + 4, abs2 - abs1 - 4);
        hasAbs = true;
    }

    pos = temp.rfind(L".");
    if (pos == std::wstring::npos)
    { return value; }

    auto name = temp.substr(0, pos);
    if (count == 0)
    { return name;}

    auto swz = temp.substr(pos);
    auto cnt = swz.size();
    if (cnt > 5)
    { return value; }

    if (cnt > count + 1)
    { cnt = count + 1; }

    wchar_t plt[4] =  { L'x', L'y', L'z', L'w' };
    auto ret = 0;

    // 文字の妥当性をチェック.
    for(auto i=1; i<cnt; ++i)
    {
        for(auto j=0; j<4; ++j)
        {
            if (plt[j] == swz[i])
            {
                ret++;
                break;
            }
        }
    }

    if (ret == cnt - 1)
    {
        auto swizzle = std::wstring(L".");

        if (ret > 1)
        {
            for(auto i=0; i<ret; ++i)
            {
                swizzle += swz[pIndex[i] + 1];
            }
        }
        else
        {
            swizzle += swz[1];
        }

        if (strip && swizzle == L".xyzw" )
        { swizzle = L""; }

        auto result = name + swizzle;
        if (hasAbs)
        { result = L"abs(" + result + L")"; }
        return result;
    }

    return value;
}

//-------------------------------------------------------------------------------------------------
//      配列要素を部分文字列として取得します.
//-------------------------------------------------------------------------------------------------
std::vector<std::string> StringHelper::SplitArrayElement(std::string value)
{
    std::vector<std::string> result;
    std::string temp = value;
    std::string l = "[";
    std::string r = "]";
    auto posL = temp.find(l);
    auto posR = temp.find(r);

    while( posL != std::string::npos && posR != std::string::npos )
    {
        auto item = temp.substr(posL + 1, posR - posL - 1);
        result.push_back(item);
        temp = temp.substr(posR + 1);
        posL = temp.find(l);
        posR = temp.find(r);
    }

    return result;
}

//-------------------------------------------------------------------------------------------------
//      配列要素を部分文字列として取得します.
//-------------------------------------------------------------------------------------------------
std::vector<std::wstring> StringHelper::SplitArrayElement(std::wstring value)
{
    std::vector<std::wstring> result;
    std::wstring temp = value;
    std::wstring l = L"[";
    std::wstring r = L"]";
    auto posL = temp.find(l);
    auto posR = temp.find(r);

    while( posL != std::wstring::npos && posR != std::wstring::npos )
    {
        auto item = temp.substr(posL + 1, posR - posL - 1);
        result.push_back(item);
        temp = temp.substr(posR + 1);
        posL = temp.find(l);
        posR = temp.find(r);
    }

    return result;
}

//-------------------------------------------------------------------------------------------------
//      配列要素を整数として取得します.
//-------------------------------------------------------------------------------------------------
std::vector<int> StringHelper::SplitArrayElementAsInt(std::string value)
{
    std::vector<int> result;
    std::string temp = value;
    std::string l = "[";
    std::string r = "]";
    auto posL = temp.find(l);
    auto posR = temp.find(r);

    while( posL != std::string::npos && posR != std::string::npos )
    {
        auto item = temp.substr(posL + 1, posR - posL - 1);
        result.push_back(std::stoi(item));
        temp = temp.substr(posR + 1);
        posL = temp.find(l);
        posR = temp.find(r);
    }

    return result;
}

//-------------------------------------------------------------------------------------------------
//      配列要素を整数として取得します.
//-------------------------------------------------------------------------------------------------
std::vector<int> StringHelper::SplitArrayElementAsInt(std::wstring value)
{
    std::vector<int> result;
    std::wstring temp = value;
    std::wstring l = L"[";
    std::wstring r = L"]";
    auto posL = temp.find(l);
    auto posR = temp.find(r);

    while( posL != std::wstring::npos && posR != std::wstring::npos )
    {
        auto item = temp.substr(posL + 1, posR - posL - 1);
        result.push_back(std::stoi(item));
        temp = temp.substr(posR + 1);
        posL = temp.find(l);
        posR = temp.find(r);
    }

    return result;
}

//-------------------------------------------------------------------------------------------------
//      変数データを解析します.
//-------------------------------------------------------------------------------------------------
void StringHelper::ParseVariable
(
    const std::string&  value,
    std::string&        type,
    std::string&        name,
    std::vector<int>&   elements
)
{
    auto pos1 = value.find(" ");
    if (pos1 == std::string::npos)
    { return; }

    type = value.substr(0, pos1);

    auto pos2 = value.find("[", pos1);
    if (pos2 == std::string::npos)
    {
        name = value.substr(pos1 + 1);
        elements.clear();
        return;
    }

    name = value.substr(pos1 + 1, pos2);
    elements = SplitArrayElementAsInt(value.substr(pos2 + 1));
}

//-------------------------------------------------------------------------------------------------
//      数値を表す文字列であるかどうかチェックします.
//-------------------------------------------------------------------------------------------------
bool StringHelper::IsValue(const std::string& value)
{
    auto lower = ToLower(value); // 小文字化してしまう.
    std::regex pattern( R"((0x)*[0-9]+\.*[0-9]*[fu]*)" ); // 0x + 数値 + . + 数値 + サフィックス.
    return std::regex_match( lower, pattern );
}

//-------------------------------------------------------------------------------------------------
//      変数を表す文字列であるかどうかチェックします.
//-------------------------------------------------------------------------------------------------
bool StringHelper::IsVariable(const std::string& value)
{
    return !IsValue(value);
}
