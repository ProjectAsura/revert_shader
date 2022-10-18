//-------------------------------------------------------------------------------------------------
// File : Reflection.cpp
// Desc : Reflection Module.
// Copyright(c) Project Asura. All right reserved.
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------
#include "Reflection.h"
#include "StringHelper.h"
#include <cassert>


namespace {

///////////////////////////////////////////////////////////////////////////////////////////////////
// ArrayInfo structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct ArrayInfo
{
    std::string         Name;
    std::vector<int>    Index;
};

//-------------------------------------------------------------------------------------------------
//      同一名があるかどうかチェックします.
//-------------------------------------------------------------------------------------------------
int Find(std::string name, const std::vector<ArrayInfo>& info)
{
    for(size_t i=0; i<info.size(); ++i)
    {
        if (info[i].Name == name)
        { return static_cast<int>(i); }
    }

    return -1;
}

//-------------------------------------------------------------------------------------------------
//      配列サイズを一次元に展開します.
//-------------------------------------------------------------------------------------------------
int ExpandArraySize(const std::vector<int>& arraySize)
{
    if (arraySize.size() == 0)
    { return 1; }

    auto result = arraySize[0];
    for(size_t i=1; i<arraySize.size(); ++i)
    { result *= arraySize[i]; }

    return result;
}

//-------------------------------------------------------------------------------------------------
//      一次元に展開された配列番号から多次元への配列番号に変換します.
//-------------------------------------------------------------------------------------------------
std::vector<int> CalcArrayElement(int value, const std::vector<int>& arraySize)
{
    std::vector<int> result;
    if (arraySize.size() == 0)
    {
        return result;
    }

    result.resize(arraySize.size());

    auto moveup = 0;
    for(int i=static_cast<int>(arraySize.size() - 1); i>=0; i--)
    {
        if(i == arraySize.size() - 1)
        {
            result[i] = value % arraySize[i];
            moveup = arraySize[i];
        }
        else
        {
            result[i] = value / moveup;
            moveup *= arraySize[i];
        }
    }

    return result;
}

//-------------------------------------------------------------------------------------------------
//      int型に変換します.
//-------------------------------------------------------------------------------------------------
std::vector<int> ToInt(const std::vector<std::string>& value)
{
    std::vector<int> result;
    result.resize(value.size());

    for(size_t i=0; i<value.size(); ++i)
    { result[i] = std::stoi(value[i]); }

    return result;
}

//-------------------------------------------------------------------------------------------------
//      std::string型に変換します.
//-------------------------------------------------------------------------------------------------
std::vector<std::string> ToString(const std::vector<int>& value)
{
    std::vector<std::string> result;
    result.resize(value.size());

    for(size_t i=0; i<value.size(); ++i)
    { result[i] = std::to_string(value[i]); }

    return result;
}

//-------------------------------------------------------------------------------------------------
//      配列表記に変換します
//-------------------------------------------------------------------------------------------------
std::string ToArrayElementString(int value, const std::vector<int>& arraySize)
{
    std::string result;

    auto expandIndex = CalcArrayElement(value, arraySize);
    for(size_t i=0; i<expandIndex.size(); ++i)
    { result += StringHelper::Format("[%d]", expandIndex[i]); }

    return result;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// BuiltInType structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct BuiltInType
{
    std::string HLSLType;       // HLSL形式です.
    std::string GLSLType;       // GLSL形式です.
    int         SlotCount;      // 定数レジスタの使用数です.
    int         ElementCount;   // 要素数です.
    bool        Primitve;
};

// 変換テーブルです.(ベクトル・スカラーOnly).
BuiltInType g_BuiltInTypes[] = {
    { "half"        , "float"   , 1,    1  },
    { "half1"       , "float"   , 1,    1  },
    { "half2"       , "vec2"    , 1,    2  },
    { "half3"       , "vec3"    , 1,    3  },
    { "half4"       , "vec4"    , 1,    4  },
    { "float1"      , "float"   , 1,    1  },
    { "float2"      , "vec2"    , 1,    2  },
    { "float3"      , "vec3"    , 1,    3  },
    { "float4"      , "vec4"    , 1,    4  },
    { "double1"     , "double"  , 1,    1  },
    { "double2"     , "dvec2"   , 1,    2  },
    { "double3"     , "dvec3"   , 1,    3  },
    { "double4"     , "dvec4"   , 1,    4  },
    { "bool1"       , "bool"    , 1,    1  },
    { "bool2"       , "bvec2"   , 1,    2  },
    { "bool3"       , "bvec3"   , 1,    3  },
    { "bool4"       , "bvec4"   , 1,    4  },
    { "int1"        , "int"     , 1,    1  },
    { "int2"        , "ivec2"   , 1,    2  }, 
    { "int3"        , "ivec3"   , 1,    3  },
    { "int4"        , "ivec4"   , 1,    4  },
    { "uint1"       , "uint"    , 1,    1  },
    { "uint2"       , "uvec2"   , 1,    2  },
    { "uint3"       , "uvec3"   , 1,    3  },
    { "uint4"       , "uvec4"   , 1,    4  },
    { "float2x1"    , "vec2"    , 1,    2  },
    { "float2x2"    , "mat2x2"  , 2,    4  },
    { "float2x3"    , "mat2x3"  , 3,    6  },
    { "float2x4"    , "mat2x4"  , 4,    8  },
    { "float3x1"    , "vec3"    , 1,    3  },
    { "float3x2"    , "mat3x2"  , 2,    6  },
    { "float3x3"    , "mat3"    , 3,    9  },
    { "float3x4"    , "mat3x4"  , 4,    12 },
    { "float4x1"    , "vec4"    , 1,    4  },
    { "float4x2"    , "mat4x2"  , 2,    8  },
    { "float4x3"    , "mat4x3"  , 3,    12 },
    { "float4x4"    , "mat4"    , 4,    16 },
    { "double2x1"   , "dvec2"   , 1,    2  },
    { "double2x2"   , "dmat2"   , 2,    4  },
    { "double2x3"   , "dmat2x3" , 3,    6  },
    { "double2x4"   , "dmat2x4" , 4,    8  },
    { "double3x1"   , "dvec3"   , 1,    3  },
    { "double3x2"   , "dmat3x2" , 2,    6  },
    { "double3x3"   , "dmat3"   , 3,    9  },
    { "double3x4"   , "dmat3x4" , 4,    12 },
    { "double4x1"   , "dvec4"   , 1,    4  },
    { "double4x2"   , "dmat4x2" , 2,    8  },
    { "double4x3"   , "dmat4x3" , 3,    12 },
    { "double4x4"   , "dmat4"   , 4,    16 }
};

} // namespace

namespace a3d {

///////////////////////////////////////////////////////////////////////////////////////////////////
// Reflection class
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//      コンストラクタです.
//-------------------------------------------------------------------------------------------------
Reflection::Reflection()
{ /* DO_NOTHING */ }

//-------------------------------------------------------------------------------------------------
//      デストラクタです.
//-------------------------------------------------------------------------------------------------
Reflection::~Reflection()
{ /* DO_NOTHING */ }

//-------------------------------------------------------------------------------------------------
//      メモリを解放します.
//-------------------------------------------------------------------------------------------------
void Reflection::Clear()
{
    m_InputDictionary.clear();
    m_OutputDictionary.clear();
    m_TextureDictionary.clear();
    m_SamplerDictionary.clear();
    m_InputDefinitions.clear();
    m_InputDefinitions.shrink_to_fit();
    m_OutputDefinitions.clear();
    m_OutputDefinitions.shrink_to_fit();
    m_ConstantBufferDefinitions.clear();
    m_ConstantBufferDefinitions.shrink_to_fit();
    m_TextureDefinitions.clear();
    m_TextureDefinitions.shrink_to_fit();
    m_SamplerDefinitions.clear();
    m_SamplerDefinitions.shrink_to_fit();
    m_Resources.clear();
    m_Resources.shrink_to_fit();
    m_InputSignatures.clear();
    m_InputSignatures.shrink_to_fit();
    m_OutputSignatures.clear();
    m_OutputSignatures.shrink_to_fit();
    m_ConstantBuffers.clear();
    m_ConstantBuffers.shrink_to_fit();
}

//-------------------------------------------------------------------------------------------------
//      リソースを追加します.
//-------------------------------------------------------------------------------------------------
void Reflection::AddResource(const Resource& value)
{ m_Resources.push_back(value); }

//-------------------------------------------------------------------------------------------------
//      入力シグニチャを追加します.
//-------------------------------------------------------------------------------------------------
void Reflection::AddInputSignature(const Signature& value)
{ m_InputSignatures.push_back(value); }

//-------------------------------------------------------------------------------------------------
//      出力シグニチャを追加します.
//-------------------------------------------------------------------------------------------------
void Reflection::AddOutputSignature(const Signature& value)
{ m_OutputSignatures.push_back(value); }

//-------------------------------------------------------------------------------------------------
//      定数バッファを追加します.
//-------------------------------------------------------------------------------------------------
void Reflection::AddConstantBuffer(const ConstantBuffer& value)
{ m_ConstantBuffers.push_back(value); }

//-------------------------------------------------------------------------------------------------
//      構造体を追加します.
//-------------------------------------------------------------------------------------------------
void Reflection::AddStructure(const Structure& value)
{
    // 登録済みかどうかチェック.
    for(auto& itr : m_Structures)
    {
        if (itr.Name == value.Name)
        { return; }
    }

    m_Structures.push_back(value);
}

//-------------------------------------------------------------------------------------------------
//      入力引数を追加します.
//-------------------------------------------------------------------------------------------------
void Reflection::AddInputArgs(const std::string& value)
{ m_InputArgs.push_back(value); }

void Reflection::AddUavStructPair(const std::string& uav, const std::string& structure)
{
    if (m_UavStructureDictionary.find(uav) != m_UavStructureDictionary.end())
    { return; }

    m_UavStructureDictionary[uav] = structure;
}

//-------------------------------------------------------------------------------------------------
//      マッピングを解決します.
//-------------------------------------------------------------------------------------------------
void Reflection::Resolve()
{
    ResolveInput();
    ResolveOutput();
    ResolveTexture();
    ResolveSampler();
    ResolveStructure();
    ResolveUav();
    ResolveConstantBuffer();
}

//-------------------------------------------------------------------------------------------------
//      名前を問い合わせします.
//-------------------------------------------------------------------------------------------------
bool Reflection::QueryName(std::string value, std::string& result)
{
    if (FindInputName(value, result))
    { return true; }

    if (FindOutputName(value, result))
    { return true; }

    if (FindTextureName(value, result))
    { return true; }

    if (FindSamplerName(value, result))
    { return true; }

    if (FindUavName(value, result))
    { return true; }

    if (FindConstantBufferName(value, result))
    { return true; }

    return false;
}

//-------------------------------------------------------------------------------------------------
//      定数バッファの定義を取得します.
//-------------------------------------------------------------------------------------------------
const std::vector<std::string>& Reflection::GetDefConstantBuffer() const
{ return m_ConstantBufferDefinitions; }

//-------------------------------------------------------------------------------------------------
//      入力シグニチャの定義を取得します.
//-------------------------------------------------------------------------------------------------
const std::vector<std::string>& Reflection::GetDefInputSignature() const
{ return m_InputDefinitions; }

//-------------------------------------------------------------------------------------------------
//      入力引数の定義を取得します.
//-------------------------------------------------------------------------------------------------
const std::vector<std::string>& Reflection::GetDefInputArgs() const
{ return m_InputArgs; }

//-------------------------------------------------------------------------------------------------
//      出力シグニチャの定義を取得します.
//-------------------------------------------------------------------------------------------------
const std::vector<std::string>& Reflection::GetDefOutputSignature() const
{ return m_OutputDefinitions; }

//-------------------------------------------------------------------------------------------------
//      サンプラーの定義を取得します.
//-------------------------------------------------------------------------------------------------
const std::vector<std::string>& Reflection::GetDefSamplers() const
{ return m_SamplerDefinitions; }

//-------------------------------------------------------------------------------------------------
//      テクスチャの定義を取得します.
//-------------------------------------------------------------------------------------------------
const std::vector<std::string>& Reflection::GetDefTextures() const
{ return m_TextureDefinitions; }

//-------------------------------------------------------------------------------------------------
//      gl_PerVertexの定義を取得します.
//-------------------------------------------------------------------------------------------------
const std::vector<std::string>& Reflection::GetDefBuiltInOutput() const
{ return m_BuiltInOutputDefinitions; }

//-------------------------------------------------------------------------------------------------
//      構造体定義を取得します.
//-------------------------------------------------------------------------------------------------
const std::vector<std::string>& Reflection::GetDefStructures() const
{ return m_StructureDefinitions; }

//-------------------------------------------------------------------------------------------------
//      UAV定義を取得します.
//-------------------------------------------------------------------------------------------------
const std::vector<std::string>& Reflection::GetDefUavs() const
{ return m_UavDefinitions; }

//-------------------------------------------------------------------------------------------------
//      入力シグニチャを解決します.
//-------------------------------------------------------------------------------------------------
void Reflection::ResolveInput()
{
    std::vector<ArrayInfo> info = {};

    // 配列データとして書き込めるようにまとめる.
    for(size_t i=0; i<m_InputSignatures.size(); ++i)
    {
        auto index = Find(m_InputSignatures[i].Semantics, info);
        if (index == -1)
        {
            ArrayInfo item = {};
            item.Name  = m_InputSignatures[i].Semantics;
            item.Index.push_back(static_cast<int>(i));

            info.push_back(item);
        }
        else
        {
            info[index].Index.push_back(static_cast<int>(i));
        }
    }

    for(auto& itr : info)
    {
        // 辞書を作成.
        for(size_t i=0; i<itr.Index.size(); ++i)
        {
            auto& input = m_InputSignatures[itr.Index[i]];
            input.ArraySize = static_cast<int>(itr.Index.size());
            auto name = StringHelper::Format("v%d", input.Register);

            if (m_InputDictionary.find(name) == m_InputDictionary.end())
            { m_InputDictionary[name] = input; }

            name = StringHelper::Format("v[%d]", input.Register);
            if (m_InputDictionary.find(name) == m_InputDictionary.end())
            { m_InputDictionary[name] = input; }
        }

        // 定義コードの生成.
        const auto& input =  m_InputSignatures[itr.Index[0]];
        auto hlslType = input.Format + std::to_string(input.Mask.length());

        if (input.SystemValue == "NONE" || input.SystemValue == "POS")
        {
            auto code = StringHelper::Format("%s %s", hlslType.c_str(), input.VarName.c_str());

            if (itr.Index.size() > 1)
            { code += "[" + std::to_string(itr.Index.size()) + "]"; }

            code += " : ";
            code += input.Semantics;
            
            code += ";\n";

            m_InputDefinitions.push_back(code);
        }
        else if (input.SystemValue == "VERTID")
        {
            std::string code = "uint vertexId : SV_VertexID";
            m_InputArgs.push_back(code);
        }
        else if (input.SystemValue == "INSTID")
        {
            std::string code = "uint instanceId : SV_InstanceID";
            m_InputArgs.push_back(code);
        }
        else
        {
            auto code = StringHelper::Format("%s %s", hlslType.c_str(), input.VarName.c_str());
            code += " : " ;
            code += input.Semantics;
            m_InputArgs.push_back(code);
        }
    }

    m_InputSignatures.shrink_to_fit();
    m_InputDefinitions.shrink_to_fit();
}

//-------------------------------------------------------------------------------------------------
//      出力シグニチャを解決します.
//-------------------------------------------------------------------------------------------------
void Reflection::ResolveOutput()
{
    std::vector<ArrayInfo> info = {};

    // 配列データとして書き込めるようにまとめる.
    for(size_t i=0; i<m_OutputSignatures.size(); ++i)
    {
        auto index = Find(m_OutputSignatures[i].Semantics, info);
        if (index == -1)
        {
            ArrayInfo item = {};
            item.Name  = m_OutputSignatures[i].Semantics;
            item.Index.push_back(static_cast<int>(i));

            info.push_back(item);
        }
        else
        {
            info[index].Index.push_back(static_cast<int>(i));
        }
    }

    for(auto& itr : info)
    {
        // 辞書を作成.
        for(size_t i=0; i<itr.Index.size(); ++i)
        {
            auto& output = m_OutputSignatures[itr.Index[i]];
            auto name = StringHelper::Format("o%d", output.Register);
            output.ArraySize = static_cast<int>(itr.Index.size());

            if (m_OutputDictionary.find(name) == m_OutputDictionary.end())
            { m_OutputDictionary[name] = output; }

            name = StringHelper::Format("o[%d]", output.Register);
            if (m_OutputDictionary.find(name) == m_OutputDictionary.end())
            { m_OutputDictionary[name] = output; }
        }

        // 定義コードの生成.
        {
            const auto& output  = m_OutputSignatures[itr.Index[0]];
            auto hlslType = output.Format + std::to_string(output.Mask.length());

            auto code = StringHelper::Format("%s %s", hlslType.c_str(), output.VarName.c_str());

            if (itr.Index.size() > 1)
            { code += "[" + std::to_string(itr.Index.size()) + "]"; }

            code += " : ";
            code += output.Semantics;
            
            code += ";\n";

            m_OutputDefinitions.push_back(code);
        }
    }

    m_OutputSignatures.shrink_to_fit();
    m_OutputDefinitions.shrink_to_fit();
}

//-------------------------------------------------------------------------------------------------
//      テクスチャを解決します.
//-------------------------------------------------------------------------------------------------
void Reflection::ResolveTexture()
{
    for(size_t i=0; i<m_Resources.size(); ++i)
    {
        auto pRes = &m_Resources[i];
        if (pRes->Type != "texture")
        { continue; }

        // レジスタ番号を取得.
        int reg = -1;
        sscanf_s(pRes->HLSLBind.c_str(), "t%d", &reg);

        ResourceInfo item = {};
        item.Name       = pRes->Name;
        item.pResource  = pRes;
        item.ArraySize  = pRes->Count;
        item.ArrayIndex = 0;
        item.Register   = reg;
        item.ExpandName = pRes->Name;

        std::string type;

        // 次元数を決定.
        if (pRes->Dimension == "1d")
        {
            item.DimValue = 1;
            type = "Texture1D";
        }
        else if (pRes->Dimension == "2d")
        {
            item.DimValue = 2;
            type = "Texture2D";
        }
        else if (pRes->Dimension == "3d")
        {
            item.DimValue = 3;
            type = "Texture3D";
        }
        else if (pRes->Dimension == "cube")
        {
            item.DimValue = 3;
            type = "TextureCube";
        }

        if (pRes->Format != "float4")
        {
            type += "<";
            type += pRes->Format;
            type += ">";
        }

        // レジスタマップを作成 (t0 <--> 変数名 の対応付け).
        if (pRes->Count > 1)
        {
            for(auto i=0; i<pRes->Count; ++i)
            {
                auto bind = StringHelper::Format("t%d", i);
                item.ArrayIndex = static_cast<int>(i);
                item.Register   = reg + i;
                item.ExpandName = StringHelper::Format("%s[%d]", pRes->Name.c_str(), i);

                m_TextureDictionary[bind] = item;
            }

            auto code = StringHelper::Format("%s %s[%d] : register(t%d);\n", type.c_str(), pRes->Name.c_str(), pRes->Count, reg);
            m_TextureDefinitions.push_back(code);
        }
        else
        {
            m_TextureDictionary[pRes->HLSLBind] = item;
            auto code = StringHelper::Format("%s %s : register(t%d);\n", type.c_str(), pRes->Name.c_str(), reg);
            m_TextureDefinitions.push_back(code);
        }
    }
}

//-------------------------------------------------------------------------------------------------
//      サンプラーを解決します.
//-------------------------------------------------------------------------------------------------
void Reflection::ResolveSampler()
{
    auto samplerCount = 0;
    for(size_t i=0; i<m_Resources.size(); ++i)
    {
        auto pRes = &m_Resources[i];
        if (pRes->Type != "sampler" || pRes->Type == "sampler_c")
        { continue; }

        samplerCount++;

        // レジスタ番号を取得.
        int reg = -1;
        sscanf_s(pRes->HLSLBind.c_str(), "s%d", &reg);

        ResourceInfo item = {};
        item.Name       = pRes->Name;
        item.pResource  = pRes;
        item.ArraySize  = pRes->Count;
        item.ArrayIndex = 0;
        item.Register   = reg;
        item.ExpandName = pRes->Name;

        std::string type = "SamplerState";

        // テクスチャ座標の次元数を求める.
        if (pRes->Dimension == "NA")
        {
            bool find = false;

            auto textureCount = 0;
            for(size_t j=0; j<m_Resources.size(); ++j)
            {
                if (m_Resources[j].Type == "texture")
                { textureCount++; }

                if (textureCount == samplerCount)
                {
                    item.pResource->Dimension = m_Resources[j].Dimension;
                    find = true;
                    break;
                }
            }

            // 対応するものが見つからなかった場合.
            if (!find)
            {
                // 最初に見つかったテクスチャを使用するとみなす.
                for(size_t j=0; j<m_Resources.size(); ++j)
                {
                    if (m_Resources[j].Type == "texture")
                    {
                        item.pResource->Dimension = m_Resources[j].Dimension;
                        find = true;
                        break;
                    }
                }

                // テクスチャそのものが存在しない場合, 2次元扱いとする.
                if (!find)
                {
                    item.pResource->Dimension = "2d";
                }
            }
        }

        if (item.pResource->Dimension == "1d")
        {
            item.DimValue = 1;
        }
        else if (item.pResource->Dimension == "2d")
        {
            item.DimValue = 2;
        }
        else if (item.pResource->Dimension == "3d")
        {
            item.DimValue = 3;
        }
        else if (item.pResource->Dimension == "cube")
        {
            item.DimValue = 3;
        }

        if (pRes->Type == "sampler_c")
        {
            type = "SamplerComparisonState";
        }

        // レジスタマップを作成 (s0 <--> 変数名 の対応付け).
        if (pRes->Count > 1)
        {
            for(auto i=0; i<pRes->Count; ++i)
            {
                auto bind = StringHelper::Format("s%d", i);
                item.ArrayIndex = static_cast<int>(i);
                item.Register   = reg + i;
                item.ExpandName = StringHelper::Format("%s[%d]", pRes->Name.c_str(), i);

                m_SamplerDictionary[bind] = item;
            }

            auto code = StringHelper::Format("%s %s[%d] : register(s%d);\n",
                            type.c_str(),
                            pRes->Name.c_str(),
                            pRes->Count,
                            reg);

            m_SamplerDefinitions.push_back(code);
        }
        else
        {
            m_SamplerDictionary[pRes->HLSLBind] = item;
            auto code = StringHelper::Format("%s %s : register(s%d);\n",
                            type.c_str(),
                            pRes->Name.c_str(),
                            reg);

            m_SamplerDefinitions.push_back(code);
        }
    }
}

//-------------------------------------------------------------------------------------------------
//      定数バッファを解決します.
//-------------------------------------------------------------------------------------------------
void Reflection::ResolveConstantBuffer()
{
    std::string tab = "    ";

    // バインディングを解決しておく.
    for(size_t i=0; i<m_Resources.size(); ++i)
    {
        auto& res = m_Resources[i];
        if (res.Type != "cbuffer")
        { continue; }

        for(size_t j=0; j<m_ConstantBuffers.size(); ++j)
        {
            if (res.Name == m_ConstantBuffers[j].Name)
            {
                m_ConstantBuffers[j].HLSLBind = res.HLSLBind;
                break;
            }
        }
    }

    for(size_t b=0; b<m_ConstantBuffers.size(); ++b)
    {
        auto size = 0;
        auto& cb = m_ConstantBuffers[b];

        std::map<std::string, VariableInfo> dic;

        // レジスタ番号を取得.
        int registerIdx = 0;
        sscanf_s(cb.HLSLBind.c_str(), "cb%d", &registerIdx);

        // 定義コード
        std::string code = StringHelper::Format("cbuffer %s : register(b%d) \n{\n", cb.Name.c_str(), registerIdx);

        for(size_t idx=0; idx<cb.Variables.size(); ++idx)
        {
            auto& var = cb.Variables[idx];

            // 最終的に設定されたものをサイズとする.
            size = var.Offset + var.Size;

            // 定数バッファ内の配列番号とスウィズルオフセットを求める.
            auto slot    = var.Offset / 16;
            auto offset  = (var.Offset % 16) / 4;
            auto element = var.Size / 4;

            std::string swz[4] = { "x", "y", "z", "w" };

            VariableInfo info = {};
            info.pBuffer            = &m_ConstantBuffers[b];
            info.pVariable          = &m_ConstantBuffers[b].Variables[idx];
            info.ArraySize          = StringHelper::SplitArrayElement(var.Name);
            info.ArraySizeVal       = ToInt(info.ArraySize);
            info.StartRegister      = slot;
            info.RegisteOffset      = offset;
            info.TypeUsedCount      = ToElementCount(var.Type);
            info.ArrayExpandSize    = ExpandArraySize(info.ArraySizeVal);

            // 配列要素を取り除いて名前だけにしておく.
            std::string stripName = var.Name;
            {
                auto posL = stripName.find("[");
                if(posL != std::string::npos)
                { stripName = stripName.substr(0, posL); }
            }

            auto usedCount = info.TypeUsedCount;
            // mat4みたいなやつはvar[0].xyzzみたいなアクセスなので展開対象.
            if (info.TypeUsedCount > 4)
            {
                usedCount = info.TypeUsedCount / 4;
                info.ArrayExpandSize *= usedCount;
                info.ArraySize.push_back(std::to_string(usedCount));
                info.ArraySizeVal.push_back(usedCount);
            }

            auto usedSize   = 0;
            auto usedOffset = offset;
            for(auto s=0; s<info.ArrayExpandSize; ++s)
            {
                // 変数名を展開する.
                VarExpandName item = {};
                item.Name           = stripName;
                item.ArrayElement   = ToArrayElementString(s, info.ArraySizeVal);
                info.ArrayIndex     = StringHelper::SplitArrayElement(item.ArrayElement);
                info.ArrayIndexVal  = ToInt(info.ArrayIndex);

                // オフセットスウィズルを求める.
                if (usedOffset != 0)
                {
                    if (info.TypeUsedCount <= 3)
                    {
                        item.Swizzle = ".";
                        for(auto i=0; i<info.TypeUsedCount; ++i)
                        {
                            item.Swizzle += swz[i + usedOffset];
                        }
                    }
                }

                //// 配列番号を修正.
                //auto slotIndex = slot + (usedSize / 16);

                // 配列番号を算出.
                auto slotIndex = slot + s;

                info.StartRegister = slotIndex;
                info.RegisteOffset = (s == 0) ? usedOffset : 0;
                info.ExpandNames   = item;

                //if (usedCount % 2 != 0)
                //{
                //    usedSize += (usedCount + 1) * 4;
                //}
                //else
                //{
                //    usedSize += usedCount * 4;
                //}
                //usedOffset = (usedSize % 16) / 4;

                // 検索キーを作成.
                auto key = StringHelper::Format("%s[%d]", cb.HLSLBind.c_str(), slotIndex) + item.Swizzle;

                if (dic.find(key) == dic.end())
                { dic[key] = info; }
            }

            //// 行列の場合は，Direct3Dの演算結果と同じになるように行優先にしておく.
            //// ( "vec = vec * mat"形式を行優先とみなす.)
            std::string layout = "";
            //if (var.Type == "float4x4" ||
            //    var.Type == "float4x3" ||
            //    var.Type == "float4x2" ||
            //    var.Type == "float3x3" ||
            //    var.Type == "float3x2" ||
            //    var.Type == "float2x2")
            //{
            //    //layout = "column_major ";
            //}

            //auto spaceCount = 25 - glslType.length() - layout.length();
            auto spaceCount = 25 - var.Type.length() - layout.length();
            std::string space = " ";
            for(auto i=1; i<spaceCount; ++i)
            { space += " "; }


            code += tab + layout + var.Type + space + var.Name + ";\n";
        }

        // 定数バッファサイズを決定.
        cb.Size = size;
        
        // 検索マップを生成.
        if (m_ConstantBufferDictionary.find(cb.HLSLBind) == m_ConstantBufferDictionary.end())
        {
            ConstantBufferInfo item = {};
            item.Tag            = cb.Name;
            item.Name           = StringHelper::ToLower(cb.Name);
            item.SlotCount      = size / 16;
            item.pBuffer        = &m_ConstantBuffers[b];
            item.VariableMap    = dic;

            m_ConstantBufferDictionary[cb.HLSLBind] = item;
        }

        // 定義を閉じる.
        code += "};\n";

        // 定義コードを追加.
        m_ConstantBufferDefinitions.push_back(code);
    }
}

void Reflection::ResolveStructure()
{
    for(size_t i=0; i<m_Structures.size(); ++i)
    {
        auto& st = m_Structures[i];

        std::string code = "struct ";
        code += st.Name;
        code += "{\n";

        for(size_t j=0; j<st.Members.size(); ++j)
        {
            auto& m = st.Members[j];
            code += StringHelper::Format("%s %s;\n", m.Type, m.Name);
        }

        code += "};\n";

        m_StructureDefinitions.push_back(code);
    }
}

//-------------------------------------------------------------------------------------------------
//      UAVを解決します.
//-------------------------------------------------------------------------------------------------
void Reflection::ResolveUav()
{
    for(size_t i=0; i<m_Resources.size(); ++i)
    {
        auto pRes = &m_Resources[i];
        if (pRes->Type != "UAV")
        { continue; }

        // レジスタ番号を取得.
        int reg = -1;
        sscanf_s(pRes->HLSLBind.c_str(), "u%d", &reg);

        ResourceInfo item = {};
        item.Name       = pRes->Name;
        item.pResource  = pRes;
        item.ArraySize  = pRes->Count;
        item.ArrayIndex = 0;
        item.Register   = reg;
        item.ExpandName = pRes->Name;

        std::string type;

        // 次元数を決定.
        if (pRes->Dimension == "1d")
        {
            item.DimValue = 1;
            type = "RWTexture1D";
        }
        else if (pRes->Dimension == "2d")
        {
            item.DimValue = 2;
            type = "RWTexture2D";
        }
        else if (pRes->Dimension == "3d")
        {
            item.DimValue = 3;
            type = "RWTexture3D";
        }
        else if (pRes->Dimension == "cube")
        {
            item.DimValue = 3;
            type = "RWTextureCube";
        }
        else if (pRes->Dimension == "r/w")
        {
            if (pRes->Format == "struct")
            {
                item.DimValue = 1;
                type = "RWStructuredBuffer";

                std::string structType;
                if (FindUavStructureName(pRes->Name, structType))
                {
                    type += "<";
                    type += structType;
                    type += ">";
                }
            }
            else
            {
                // TODO : Implement.
            }
        }

        if (pRes->Format != "struct")
        {
            type += "<";
            type += pRes->Format;
            type += ">";
        }

        // レジスタマップを作成 (u0 <--> 変数名 の対応付け).
        if (pRes->Count > 1)
        {
            for(auto i=0; i<pRes->Count; ++i)
            {
                auto bind = StringHelper::Format("u%d", i);
                item.ArrayIndex = static_cast<int>(i);
                item.Register   = reg + i;
                item.ExpandName = StringHelper::Format("%s[%d]", pRes->Name.c_str(), i);

                m_UavDictionary[bind] = item;
            }

            auto code = StringHelper::Format("%s %s[%d] : register(u%d);\n", type.c_str(), pRes->Name.c_str(), pRes->Count, reg);
            m_UavDefinitions.push_back(code);
        }
        else
        {
            m_UavDictionary[pRes->HLSLBind] = item;
            auto code = StringHelper::Format("%s %s : register(u%d);\n", type.c_str(), pRes->Name.c_str(), reg);
            m_UavDefinitions.push_back(code);
        }
    }
}

//-------------------------------------------------------------------------------------------------
//      入力定義を問い合わせします.
//-------------------------------------------------------------------------------------------------
bool Reflection::QueryInput(const std::string& value, Signature* pInfo)
{
    if (m_InputDictionary.find(value) == m_InputDictionary.end())
    { return false; }

    *pInfo = m_InputDictionary[value];
    return true;
}

//-------------------------------------------------------------------------------------------------
//      出力定義を問い合わせします.
//-------------------------------------------------------------------------------------------------
bool Reflection::QueryOutput(const std::string& value, Signature* pInfo)
{
    if (m_OutputDictionary.find(value) == m_OutputDictionary.end())
    { return false; }

    *pInfo = m_OutputDictionary[value];
    return true;
}

//-------------------------------------------------------------------------------------------------
//      サンプラー定義を問い合わせします.
//-------------------------------------------------------------------------------------------------
bool Reflection::QuerySampler(const std::string& value, ResourceInfo* pInfo)
{
    if (m_SamplerDictionary.find(value) == m_SamplerDictionary.end())
    { return false; }

    *pInfo = m_SamplerDictionary[value];
    return true;
}

//-------------------------------------------------------------------------------------------------
//      テクスチャ定義を問い合わせします.
//-------------------------------------------------------------------------------------------------
bool Reflection::QueryTexture(const std::string& value, ResourceInfo* pInfo)
{
    if(m_TextureDictionary.find(value) == m_TextureDictionary.end())
    { return false; }

    *pInfo = m_TextureDictionary[value];
    return true;
}

//-------------------------------------------------------------------------------------------------
//      定数バッファ定義を問い合わせします.
//-------------------------------------------------------------------------------------------------
bool Reflection::QueryBuffer(const std::string& value, ConstantBufferInfo* pInfo)
{
    if (m_ConstantBufferDictionary.find(value) == m_ConstantBufferDictionary.end())
    { return false; }

    *pInfo = m_ConstantBufferDictionary[value];
    return true;
}

//-------------------------------------------------------------------------------------------------
//      構造体定義を問い合わせします.
//-------------------------------------------------------------------------------------------------
bool Reflection::QueryStructure(const std::string& value, Structure* pInfo)
{
    if (m_StructureDictionary.find(value) == m_StructureDictionary.end())
    { return false; }

    *pInfo = m_StructureDictionary[value];
    return true;
}

//-------------------------------------------------------------------------------------------------
//      UAV定義を問い合わせします.
//-------------------------------------------------------------------------------------------------
bool Reflection::QueryUav(const std::string& value, ResourceInfo* pInfo)
{
    if (m_UavDictionary.find(value) == m_UavDictionary.end())
    { return false; }

    *pInfo = m_UavDictionary[value];
    return true;
}

//-------------------------------------------------------------------------------------------------
//      入力データを持つかどうかチェックします.
//-------------------------------------------------------------------------------------------------
bool Reflection::HasInput() const
{ return !m_InputDictionary.empty(); }

//-------------------------------------------------------------------------------------------------
//      出力データを持つかどうかチェックします.
//-------------------------------------------------------------------------------------------------
bool Reflection::HasOutput() const
{ return !m_OutputDictionary.empty(); }

//-------------------------------------------------------------------------------------------------
//      テクスチャデータを持つかどうかチェックします.
//-------------------------------------------------------------------------------------------------
bool Reflection::HasTexture() const
{ return !m_TextureDictionary.empty(); }

//-------------------------------------------------------------------------------------------------
//      サンプラーデータを持つかどうかチェックします.
//-------------------------------------------------------------------------------------------------
bool Reflection::HasSampler() const
{ return !m_SamplerDictionary.empty(); }

//-------------------------------------------------------------------------------------------------
//      バッファデータを持つかどうかチェックします.
//-------------------------------------------------------------------------------------------------
bool Reflection::HasBuffer() const
{ return !m_ConstantBufferDictionary.empty(); }

//-------------------------------------------------------------------------------------------------
//      gl_PerVertexの定義を持つかどうかチェックします.
//-------------------------------------------------------------------------------------------------
bool Reflection::HasBuiltinOutput() const
{ return !m_BuiltInOutputDefinitions.empty(); }

bool Reflection::HasStructure() const
{ return !m_StructureDefinitions.empty(); }

bool Reflection::HasUav() const
{ return !m_UavDefinitions.empty(); }

//-------------------------------------------------------------------------------------------------
//      入力シグニチャを検索します.
//-------------------------------------------------------------------------------------------------
bool Reflection::FindInputName(const std::string& value, std::string& result)
{
    std::string sign;
    std::string temp = value;

    // 符号を取り除いておく.
    auto pos = temp.find("-");
    if (pos == 0)
    {
        sign = "-";
        temp = temp.substr(1);
    }

    // 絶対値符号を取り除く.
    auto abs1 = temp.find("|");
    auto abs2 = temp.rfind("|");
    if (abs1 != std::string::npos
     && abs2 != std::string::npos 
     && abs1 != abs2)
    {
        temp = temp.substr(abs1 + 1, abs2 - abs1 - 1);
    }

    auto name = StringHelper::GetWithSwizzle(temp, 0);

    // 演算子があるかどうかチェックする.
    auto arr = StringHelper::SplitArrayElement(name);
    if (!arr.empty())
    {
        auto hasOp = StringHelper::Contain(arr[0], "+");

        if (hasOp)
        {
            auto ops = StringHelper::Split(arr[0], "+");
        }
    }

    auto func = [&](const std::string& name) 
    {
        if (m_InputDictionary.find(name) != m_InputDictionary.end())
        {
            auto& def = m_InputDictionary[name];
            result = sign + "input." + def.VarName;
            if (def.ArraySize > 1)
            {
                result += "[" + std::to_string(def.Index) + "]";
            }

            auto swz = StringHelper::GetSwizzle(temp);
            if (swz.length() - 1 == def.Mask.length())
            {
                char buf1[6] = {};
                char buf2[6] = {};
                strcpy_s(buf1, swz.c_str());
                strcpy_s(buf2, def.Mask.c_str());

                auto matchCount = 0;
                for(auto i=0; i<swz.length() - 1; ++i)
                {
                    if (buf1[i + 1] == buf2[i])
                    { matchCount++; }
                }

                if (matchCount != def.Mask.length())
                {
                    result += swz;
                }
            }
            else
            {
                result += swz;
            }

            return true;
        }

        bool hit = false;

        if (name == "vGSInstanceId")
        {
            result = "gsInstanceId";
            hit = true;
        }
        else if (name == "vOutputControlPointID")
        {
            result = "controlPointId";
            hit = true;
        }
        else if (name == "vThreadID")
        {
            result = "dispatchId";
            hit = true;
        }
        else if (name == "vThreadGroupID")
        {
            result = "groupId";
            hit = true;
        }
        else if (name == "vThreadIDInGroup")
        {
            result = "groupThreadId";
            hit = true;
        }
        else if (name == "vThreadIDInGroupFlattened")
        {
            result = "groupIndex";
            hit = true;
        }

        if (hit)
        {
            auto swz = StringHelper::GetSwizzle(temp);
            result += swz;
            return true;
        }

        return false;
    };

    if (func(name))
    { return true; }

    return false;
}

//-------------------------------------------------------------------------------------------------
//      出力シグニチャを検索します.
//-------------------------------------------------------------------------------------------------
bool Reflection::FindOutputName(const std::string& value, std::string& result)
{
    std::string sign;
    std::string temp = value;

    // 符号を取り除いておく.
    auto pos = temp.find("-");
    if (pos == 0)
    {
        sign = "-";
        temp = temp.substr(1);
    }

    // 絶対値符号を取り除く.
    auto abs1 = temp.find("|");
    auto abs2 = temp.rfind("|");
    if (abs1 != std::string::npos
     && abs2 != std::string::npos 
     && abs1 != abs2)
    {
        temp = temp.substr(abs1 + 1, abs2 - abs1 - 1);
    }

    auto name = StringHelper::GetWithSwizzle(temp, 0);

    auto func = [&](const std::string& name)
    {
        if (m_OutputDictionary.find(name) != m_OutputDictionary.end())
        {
            auto& def = m_OutputDictionary[name];
            result = sign + "output." + def.VarName;
            if (def.ArraySize > 1)
            {
                result += "[" + std::to_string(def.Index) + "]";
            }

            auto swz = StringHelper::GetSwizzle(temp);
            if (swz.length() - 1 == def.Mask.length())
            {
                char buf1[6] = {};
                char buf2[6] = {};
                strcpy_s(buf1, swz.c_str());
                strcpy_s(buf2, def.Mask.c_str());

                auto matchCount = 0;
                for(auto i=0; i<swz.length() - 1; ++i)
                {
                    if (buf1[i + 1] == buf2[i])
                    { matchCount++; }
                }

                if (matchCount != def.Mask.length())
                {
                    result += swz;
                }
            }
            else
            {
                result += swz;
            }
            return true;
        }

        return false;
    };

    if (func(name))
    { return true; }

    return false;
}

//-------------------------------------------------------------------------------------------------
//      テクスチャを検索します.
//-------------------------------------------------------------------------------------------------
bool Reflection::FindTextureName(const std::string& value, std::string& result)
{
    if (m_TextureDictionary.find(value) != m_TextureDictionary.end())
    {
        auto& def = m_TextureDictionary[value];
        result = def.ExpandName;
        return true;
    }

    return false;
}

//-------------------------------------------------------------------------------------------------
//      サンプラーを検索します.
//-------------------------------------------------------------------------------------------------
bool Reflection::FindSamplerName(const std::string& value, std::string& result)
{
    if (m_SamplerDictionary.find(value) != m_SamplerDictionary.end())
    {
        auto& def = m_SamplerDictionary[value];
        result = def.ExpandName;
        return true;
    }

    return false;
}

//-------------------------------------------------------------------------------------------------
//      UAVを検索します.
//-------------------------------------------------------------------------------------------------
bool Reflection::FindUavName(const std::string& value, std::string& result)
{
    if (m_UavDictionary.find(value) != m_UavDictionary.end())
    {
        auto& def = m_UavDictionary[value];
        result = def.ExpandName;
        return true;
    }

    return false;
}

//-------------------------------------------------------------------------------------------------
//      UAV名に対応する構造体名を取得します.
//-------------------------------------------------------------------------------------------------
bool Reflection::FindUavStructureName(const std::string& value, std::string& result)
{
    if (m_UavStructureDictionary.find(value) != m_UavStructureDictionary.end())
    {
        result = m_UavStructureDictionary[value];
        return true;
    }

    return false;
}

//-------------------------------------------------------------------------------------------------
//      定数バッファを検索します.
//-------------------------------------------------------------------------------------------------
bool Reflection::FindConstantBufferName(const std::string& value, std::string& result)
{
    std::string temp = value;

    // 符号を取り除く.
    std::string sign;
    auto pos3 = temp.find("-");
    if (pos3 == 0)
    {
        sign = "-";
        temp = temp.substr(pos3 + 1);
    }

    // 絶対値記号を取り除く.
    auto abs1 = temp.find("|");
    auto abs2 = temp.rfind("|");
    if (abs1 != std::string::npos && abs2 != std::string::npos && abs1 != abs2)
    {
        temp = temp.substr(abs1 + 1, abs2 - abs1 - 1);
    }

    // 基本定数バッファはcb0[1]みたいな感じなので，必ず配列形式.
    auto pos1 = temp.find("[");
    auto pos2 = temp.find("]");
    if (pos2 == std::string::npos || pos1 == std::string::npos)
    { return false; }

    auto name = temp.substr(0, pos1);   
    if (m_ConstantBufferDictionary.find(name) != m_ConstantBufferDictionary.end())
    {
        auto& cb = m_ConstantBufferDictionary[name];

        // スウィズル付きがあるかどうかチェック.
        if (cb.VariableMap.find(temp) != cb.VariableMap.end())
        {
            auto& var = cb.VariableMap[temp];
            //result = sign + cb.Name + ".";
            result = sign;
            result += var.ExpandNames.Name;
            result += var.ExpandNames.ArrayElement;

            if (var.ExpandNames.Swizzle.empty())
            {
                auto count = StringHelper::GetSwizzleCount(temp);

                // スウィズルの妥当性をチェック.
                if (count > 0)
                {
                    // スウィズル取得.
                    auto swizzle = StringHelper::GetSwizzle(temp);

                    char buf[6];
                    strcpy_s(buf, swizzle.c_str());

                    bool err = false;

                    // 1スロット当たりの要素数を求める.
                    auto elementCount = var.TypeUsedCount % 4;
                    elementCount = (elementCount == 0) ? 4 : elementCount;

                    // 各スウィズルをチェック.
                    for(auto i=1; i<count; ++i)
                    {
                        auto dif = 0;

                        // 距離差分を求める.
                        if (buf[i] == 'x')
                        { dif = 0; }
                        else if (buf[i] == 'y')
                        { dif = 1; }
                        else if (buf[i] == 'z')
                        { dif = 2; }
                        else if (buf[i] == 'w')
                        { dif = 3; }

                        // 要素数を超えるものがあれば補正が必要.
                        if (dif >= elementCount)
                        {
                            err = true;
                            break;
                        }
                    }

                    if (err)
                    {
                        // スウィズル補正が必要な場合.
                        std::string swz[4] = {"x", "y", "z", "w"};
                        result += ".";

                        // 1文字ずつ直す.
                        for(auto i=0; i<count; ++i)
                        {
                            auto dif = 0;
                            if (buf[i] == 'x')
                            { dif = 0; }
                            else if (buf[i] == 'y')
                            { dif = 1; }
                            else if (buf[i] == 'z')
                            { dif = 2; }
                            else if (buf[i] == 'w')
                            { dif = 3; }

                            // 要素数を超えないようにインデックスをループさせる.
                            auto idx = dif % elementCount;

                            // スウィズル追加.
                            result += swz[idx];
                        }
                    }
                    else
                    {
                        // スウィズル補正が必要ない場合はそのままくっつける.
                        result += swizzle;
                    }
                }
            }

            return true;
        }

        auto varName = temp.substr(0, pos2 + 1);
        if (cb.VariableMap.find(varName) != cb.VariableMap.end())
        {
            auto& var = cb.VariableMap[varName];
            //result = sign + cb.Name + ".";
            result = sign;
            result += var.ExpandNames.Name;
            result += var.ExpandNames.ArrayElement;

            if (var.ExpandNames.Swizzle.empty())
            {
                auto count = StringHelper::GetSwizzleCount(temp);

                // スウィズルの妥当性をチェック.
                if (count > 0)
                {
                    // スウィズル取得.
                    auto swizzle = StringHelper::GetSwizzle(temp);

                    char buf[6];
                    strcpy_s(buf, swizzle.c_str());

                    bool err = false;

                    // 1スロット当たりの要素数を求める.
                    auto elementCount = var.TypeUsedCount % 4;
                    elementCount = (elementCount == 0) ? 4 : elementCount;

                    // 各スウィズルをチェック.
                    for(auto i=1; i<count; ++i)
                    {
                        auto dif = 0;

                        // 距離差分を求める.
                        if (buf[i] == 'x')
                        { dif = 0; }
                        else if (buf[i] == 'y')
                        { dif = 1; }
                        else if (buf[i] == 'z')
                        { dif = 2; }
                        else if (buf[i] == 'w')
                        { dif = 3; }

                        // 要素数を超えるものがあれば補正が必要.
                        if (dif >= elementCount)
                        {
                            err = true;
                            break;
                        }
                    }

                    if (err)
                    {
                        // スウィズル補正が必要な場合.
                        std::string swz[4] = {"x", "y", "z", "w"};
                        result += ".";

                        // 1文字ずつ直す.
                        for(auto i=0; i<count; ++i)
                        {
                            auto dif = 0;
                            if (buf[i] == 'x')
                            { dif = 0; }
                            else if (buf[i] == 'y')
                            { dif = 1; }
                            else if (buf[i] == 'z')
                            { dif = 2; }
                            else if (buf[i] == 'w')
                            { dif = 3; }

                            // 要素数を超えないようにインデックスをループさせる.
                            auto idx = dif % elementCount;
                            assert(0 <= idx && idx <= 3);

                            // スウィズル追加.
                            result += swz[idx];
                        }
                    }
                    else
                    {
                        // スウィズル補正が必要ない場合はそのままくっつける.
                        result += swizzle;
                    }
                }
            }

            return true;
        }
    }

    return false;
}

//-------------------------------------------------------------------------------------------------
//      要素数に変換します.
//-------------------------------------------------------------------------------------------------
int Reflection::ToElementCount(std::string type)
{
    auto count = sizeof(g_BuiltInTypes) / sizeof(g_BuiltInTypes[0]);
    for(size_t i=0; i<count; ++i)
    {
        if (type == g_BuiltInTypes[i].HLSLType)
        { return g_BuiltInTypes[i].ElementCount; }
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
//      キャストが必要な場合にキャストした文字列を取得します.
//-------------------------------------------------------------------------------------------------
std::string Reflection::GetCastedString(std::string value, const SwizzleInfo& info)
{
    // ユニフォームバッファ名になっているのでドットがつかないやつは除外.
    auto pos = value.find("l(");
    if (pos != std::string::npos)
    { return FilterLiteral(value, info); }

    pos = value.find("float");
    if (pos != std::string::npos)
    { return FilterPrimitive(value, info); }

    pos = value.find(".");
    if (pos == std::string::npos)
    { return value; }

    std::string sign;
    std::string temp = value;
    pos = temp.find("-");

    // 符号があれば取り除いておく.
    if (pos == 0)
    {
        sign = "-";
        temp = temp.substr(pos + 1);
    }

    bool hasAbs = false;
    // 絶対値符号を取り除く.
    auto abs1 = temp.find("|");
    auto abs2 = temp.rfind("|");
    if (abs1 != std::string::npos
     && abs2 != std::string::npos 
     && abs1 != abs2)
    {
        temp = temp.substr(abs1 + 1, abs2 - abs1 - 1);
        hasAbs = true;
    }

    abs1 = temp.find("abs(");
    abs2 = temp.rfind(")");
    if (abs1 != std::string::npos
     && abs2 != std::string::npos 
     && abs1 != abs2)
    {
        temp = temp.substr(abs1 + 4, abs2 - abs1 - 4);
        hasAbs = true;
    }

    auto cbName  = temp.substr(0, pos);
    auto varName = temp.substr(pos + 1);

    // 配列は取り除いて，名前だけにしておく.
    pos = varName.find("[");
    if (pos != std::string::npos)
    { varName = varName.substr(0, pos); }

    for(auto& cb : m_ConstantBuffers)
    {
        if (cb.Name != cbName )
        { continue; }

        for(auto& var : cb.Variables)
        {
            if (var.Name != varName)
            { continue; }

            auto elements = ToElementCount(var.Type);

            // 一致していれば，何もせずに返す.
            if (elements == info.Count)
            { return value; }
            else if (elements == 1) // 要素１のやつはスウィズル出来ないので，キャストする.
            {
                auto hlslType = var.Type + std::to_string(info.Count);
                auto ret = sign + hlslType + "(" + value + ")";
                if (hasAbs)
                { ret = "abs(" + ret + ")"; }
                return ret;
            }
        }
    }

    return FilterSwizzle(value, info);
}

//-------------------------------------------------------------------------------------------------
//      リテラルを適切なデータ型に変換します.
//-------------------------------------------------------------------------------------------------
std::string Reflection::FilterLiteral(std::string value, const SwizzleInfo& info)
{
    auto pos = value.find("l(");
    if (pos == std::string::npos)
    { return value; }

    std::string result;

    auto line = StringHelper::Replace(value, "l", "");
    line = StringHelper::Replace(line, "(", "");
    line = StringHelper::Replace(line, ")", "");

    auto args = StringHelper::Split(line, ",");
    for(size_t i=0; i<args.size(); ++i)
    { args[i] = StringHelper::Replace(args[i], " ", ""); }

    if (info.Count > 1)
    { result = StringHelper::Format("float%d(", info.Count); }

    for(size_t i=0; i<info.Count; ++i)
    {
        if (i != 0)
        { result += ", "; }

        result += args[info.Index[i]];
    }

    if (info.Count > 1)
    { result += ")"; }

    return result;
}

//-------------------------------------------------------------------------------------------------
//      適切なデータ型に変換します.
//-------------------------------------------------------------------------------------------------
std::string Reflection::FilterPrimitive(std::string value, const SwizzleInfo& info)
{
    auto pos = value.find("float");
    if (pos == std::string::npos)
    { return value; }

    auto pos2 = value.find("(");
    if (pos2 == std::string::npos)
    { return value; }

    if (pos2 - pos == 4)
    { return value; }

    auto line = value.substr(pos2);
    line = StringHelper::Replace(line, "(", "");
    line = StringHelper::Replace(line, ")", "");
    line = StringHelper::Replace(line, ", ", " ");

    auto args = StringHelper::Split(line, " ");
    assert(args.size() >= info.Count); // 基本は次数下げのはず.

    if (info.Count == 1)
    { return args[info.Index[0]]; }

    std::string result = StringHelper::Format("float%d(", info.Count);
    for(auto i=0; i<info.Count; ++i)
    {
        if (i != 0)
        { result += ", "; }

        result += args[info.Index[i]];
    }
    result += ")";

    return result;
}

//-------------------------------------------------------------------------------------------------
//      スウィズルを適切に変換します.
//-------------------------------------------------------------------------------------------------
std::string Reflection::FilterSwizzle(std::string value, const SwizzleInfo& info)
{
    if (info.Count == 0)
    { return value; }

    return StringHelper::GetWithSwizzleEx(value, info.Count, info.Index);
}

//-------------------------------------------------------------------------------------------------
//      リテラルかどうか判定します.
//-------------------------------------------------------------------------------------------------
bool Reflection::IsLiteral(std::string value, Literal* pInfo)
{
    char buf[1024];
    strcpy_s(buf, value.c_str());

    auto pos1 = value.find("(");
    auto pos2 = value.find(")");
    if (pos1 == std::string::npos && pos2 == std::string::npos)
    {
        // asciiコードで 0-9 であるか判定している.
        if (0x30 <= buf[0] && buf[0] <= 0x39)
        {
            return true;
        }
    }

    auto pos3 = value.find("vec");
    if (pos3 != std::string::npos && (pos3 == 0 || pos3 == 1))
    {
        auto line = value.substr(pos1 + 1, pos2 - pos1 - 1);
        line = StringHelper::Replace(line, ", ", " ");
        auto values = StringHelper::Split(line, " ");
        if (pInfo != nullptr)
        { 
            (*pInfo).Values = values;
            auto count = 0;
            for(size_t i=0; i<values.size(); ++i)
            {
                auto pos4 = values[i].find(".");
                if (pos4 != std::string::npos)
                { count++; }
            }

            (*pInfo).HasPoint = (count == values.size());
        }

        return true;
    }

    return false;
}

//-------------------------------------------------------------------------------------------------
//      スウィズル情報に変換します.
//-------------------------------------------------------------------------------------------------
SwizzleInfo Reflection::ToSwizzleInfo(std::string value)
{
    auto swizzle = StringHelper::GetSwizzle(value);
    auto count   = StringHelper::GetSwizzleCount(value); 

    char buf[6];
    strcpy_s(buf, swizzle.c_str());

    SwizzleInfo ret = {};
    ret.Count = count;
    memset(ret.Index, -1, sizeof(ret.Index));

    for(auto i=0; i<ret.Count; ++i)
    {
        ret.Pattern[i] = buf[i + 1];

        if (ret.Pattern[i] == 'x')
        { ret.Index[i] = 0; }
        else if (ret.Pattern[i] == 'y')
        { ret.Index[i] = 1; }
        else if (ret.Pattern[i] == 'z')
        { ret.Index[i] = 2; }
        else if (ret.Pattern[i] == 'w')
        { ret.Index[i] = 3; }
    }

    return ret;
}

} // namespace a3d
