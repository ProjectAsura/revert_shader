//-------------------------------------------------------------------------------------------------
// File : Reflection.h
// Desc : Reflection Module.
// Copyright(c) Project Asura. All right reserved.
//-------------------------------------------------------------------------------------------------
#pragma once

//-------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------
#include <string>
#include <vector>
#include <map>


namespace a3d {

///////////////////////////////////////////////////////////////////////////////////////////////////
// LAYOUT_TYPE
///////////////////////////////////////////////////////////////////////////////////////////////////
enum LAYOUT_TYPE
{
    LAYOUT_DEFAULT = 0,     // 何も指定しません.
    LAYOUT_COLUMN_MAJOR,    // column_major が HLSLで指定されていることを示し，GLSLでは row_major を指定して出力します.
    LAYOUT_ROW_MAJOR,       // row_major が HLSLで指定されていることを示し， GLSLでは column_major を指定して出力します.
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Signature structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct Signature
{
    std::string     Semantics;       // セマンティクス.
    int             Index;
    int             ArraySize;
    std::string     Mask;
    int             Register;
    std::string     SystemValue;
    std::string     Format;
    std::string     Used;
    std::string     VarName;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Resource structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct Resource
{
    std::string     Name;
    std::string     Type;
    std::string     Format;
    std::string     Dimension;
    std::string     HLSLBind;
    int             Count;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Variable structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct Variable
{
    std::string                 Type;
    std::string                 Name;
    int                         Offset;
    int                         Size;
    int                         Layout; // 行優先か列優先か？
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// ConstantBuffer structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct ConstantBuffer
{
    std::string                 Name;
    std::string                 HLSLBind;
    int                         Size;
    std::vector<Variable>       Variables;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Literal structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct Literal
{
    std::vector<std::string>    Values;
    bool                        HasPoint;   //　浮動小数点を含むかどうか?
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// SwizzleInfo structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct SwizzleInfo
{
    int     Count;          // スウィズルカウント　(.xyz = 3, .zw = 2のようになります).
    char    Pattern[4];     // スウィズルパターン (.xyz の場合は Pattern[0] = x, Pattern[1] = y, Pattern[2] = zのようになります).
    int     Index[4];       // スウィズルインデックス(x=0, y=1, z=2, w=3となります.)
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// Reflection class
///////////////////////////////////////////////////////////////////////////////////////////////////
class Reflection
{
    //=============================================================================================
    // list of friend classes
    //=============================================================================================
    /* NOTHING */

public:
    //=============================================================================================
    // public variables.
    //=============================================================================================
    struct ResourceInfo
    {
        std::string                 Name;
        Resource*                   pResource;
        int                         ArraySize;
        int                         ArrayIndex;
        int                         Register;
        int                         DimValue;
        std::string                 ExpandName;
    };

    struct VarExpandName
    {
        std::string                 Name;
        std::string                 ArrayElement;
        std::string                 Swizzle;
    };

    struct VariableInfo
    {
        Variable*                   pVariable;
        std::vector<std::string>    ArraySize;
        std::vector<int>            ArraySizeVal;
        int                         StartRegister;
        int                         RegisteOffset;
        int                         TypeUsedCount;
        int                         ArrayExpandSize;
        std::vector<std::string>    ArrayIndex;
        std::vector<int>            ArrayIndexVal;
        VarExpandName               ExpandNames;
        ConstantBuffer*             pBuffer;
    };

    struct ConstantBufferInfo
    {
        std::string         Tag;
        std::string         Name;
        int                 SlotCount;
        ConstantBuffer*     pBuffer;
        std::map<std::string, VariableInfo> VariableMap;
    };

    //=============================================================================================
    // public methods.
    //=============================================================================================
    Reflection();
    ~Reflection();

    void Clear();

    void AddResource        (const Resource& value);
    void AddInputSignature  (const Signature& value);
    void AddOutputSignature (const Signature& value);
    void AddConstantBuffer  (const ConstantBuffer& value);

    void Resolve();
    bool QueryName(std::string value, std::string& result);

    const std::vector<std::string>& GetDefConstantBuffer    () const;
    const std::vector<std::string>& GetDefInputSignature    () const;
    const std::vector<std::string>& GetDefInputArgs         () const;
    const std::vector<std::string>& GetDefOutputSignature   () const;
    const std::vector<std::string>& GetDefSamplers          () const;
    const std::vector<std::string>& GetDefTextures          () const;
    const std::vector<std::string>& GetDefBuiltInOutput     () const;

    bool QuerySampler   (const std::string& value, ResourceInfo* pInfo);
    bool QueryTexture   (const std::string& value, ResourceInfo* pInfo);
    bool QueryInput     (const std::string& value, Signature* pInfo);
    bool QueryOutput    (const std::string& value, Signature* pInfo);
    bool QueryBuffer    (const std::string& value, ConstantBufferInfo* pInfo);
    bool HasInput   () const;
    bool HasOutput  () const;
    bool HasTexture () const;
    bool HasSampler () const;
    bool HasBuffer  () const;
    bool HasBuiltinOutput() const;

    std::string GetCastedString(std::string value, const SwizzleInfo& info);

    //static std::string ToGLSLType(std::string type);
    static int         ToElementCount(std::string type);
    static SwizzleInfo ToSwizzleInfo(std::string value);

    bool IsLiteral(std::string type, Literal* pInfo);

private:
    //=============================================================================================
    // private variables.
    //=============================================================================================
    std::vector<Resource>       m_Resources;
    std::vector<Signature>      m_InputSignatures;
    std::vector<Signature>      m_OutputSignatures;
    std::vector<ConstantBuffer> m_ConstantBuffers;

    std::vector<std::string>    m_BuiltInInputDefinitions;
    std::vector<std::string>    m_BuiltInOutputDefinitions;
    std::vector<std::string>    m_InputDefinitions;
    std::vector<std::string>    m_InputArgs;
    std::vector<std::string>    m_OutputDefinitions;
    std::vector<std::string>    m_ConstantBufferDefinitions;
    std::vector<std::string>    m_TextureDefinitions;
    std::vector<std::string>    m_SamplerDefinitions;

    std::map<std::string, Signature>            m_InputDictionary;
    std::map<std::string, Signature>            m_OutputDictionary;
    std::map<std::string, ResourceInfo>         m_TextureDictionary;
    std::map<std::string, ResourceInfo>         m_SamplerDictionary;
    std::map<std::string, ConstantBufferInfo>   m_ConstantBufferDictionary;

    //=============================================================================================
    // private methods.
    //=============================================================================================
    void ResolveInput           ();
    void ResolveOutput          ();
    void ResolveTexture         ();
    void ResolveSampler         ();
    void ResolveConstantBuffer  ();

    bool FindInputName          (const std::string& value, std::string& result);
    bool FindOutputName         (const std::string& value, std::string& result);
    bool FindTextureName        (const std::string& value, std::string& result);
    bool FindSamplerName        (const std::string& value, std::string& result);
    bool FindConstantBufferName (const std::string& value, std::string& result);

    std::string FilterLiteral   (std::string value, const SwizzleInfo& info);
    std::string FilterPrimitive (std::string value, const SwizzleInfo& info);
    std::string FilterSwizzle   (std::string value, const SwizzleInfo& info);

};

} // namespace a3d
