//-------------------------------------------------------------------------------------------------
// File : AsmParser.h
// Desc : HLSL Assembly Parser Module.
// Copyright(c) Project Asura. All right reserved.
//-------------------------------------------------------------------------------------------------
#pragma once

//-------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------
#include "Tokenizer.h"
#include <string>
#include <vector>
#include <map>
#include "Reflection.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
// SHADER_TYPE enum
///////////////////////////////////////////////////////////////////////////////////////////////////
enum SHADER_TYPE
{
    SHADER_TYPE_VERTEX      = 0x0,
    SHADER_TYPE_PIXEL       = 0x1,
    SHADER_TYPE_GEOMETRY    = 0x2,
    SHADER_TYPE_DOMAIN      = 0x3,
    SHADER_TYPE_HULL        = 0x4,
    SHADER_TYPE_COMPUTE     = 0x5
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// AsmParser class
///////////////////////////////////////////////////////////////////////////////////////////////////
class AsmParser
{
    //=============================================================================================
    // list of friend classes and methods.
    //=============================================================================================
    /* NOTHING */

public:
    //=============================================================================================
    // public variables.
    //=============================================================================================
    struct Argument
    {
        std::string Input;      // asm file path.
        std::string Output;     // hlsl file path.
        std::string EntryPoint; // entry point name.
    };

    //=============================================================================================
    // public methods.
    //=============================================================================================
    AsmParser();
    ~AsmParser();

    bool Convert(const Argument& args);

private:
    //=============================================================================================
    // private variables.
    //=============================================================================================
    char*                       m_pBuffer       = nullptr;
    size_t                      m_BufferSize    = 0;
    Tokenizer                   m_Tokenizer;
    Argument                    m_Argument;
    a3d::Reflection             m_Reflection;
    std::string                 m_ShaderProfile;
    std::vector<std::string>    m_Instructions;
    SHADER_TYPE                 m_ShaderType    = SHADER_TYPE_VERTEX;

    bool m_BufferSection        = false;
    bool m_ResourceSection      = false;
    bool m_InputSection         = false;;
    bool m_OutputSection        = false;
    bool m_HasGetResourceInfo   = false;

    //=============================================================================================
    // private methods.
    //=============================================================================================
    bool LoadAsm(const char* filename);
    void ParseAsm();
    void ParseInstruction();

    std::string GetOperand();
    std::string GetOperand(const a3d::SwizzleInfo& info);
    std::string GetArgs();
    a3d::SwizzleInfo Get1(std::string& op0);
    a3d::SwizzleInfo Get2(std::string& op0, std::string& op1);
    a3d::SwizzleInfo Get3(std::string& op0, std::string& op1, std::string& op2);
    a3d::SwizzleInfo Get4(std::string& op0, std::string& op1, std::string& op2, std::string& op3);
    void GetSample0(std::string& dst, std::string& texture, std::string& sampler, std::string& texcoord );
    void GetSample1(std::string& dst, std::string& texture, std::string& sampler, std::string& texcoord, std::string& arg1);
    void GetSample2(std::string& dst, std::string& texture, std::string& sampler, std::string& texcoord, std::string& arg1, std::string& arg2);
    void GetSampleOffset0(std::string& dst, std::string& texture, std::string& sampler, std::string& texcoord, std::string& offset);
    void GetSampleOffset1(std::string& dst, std::string& texture, std::string& sampler, std::string& texcoord, std::string& offset, std::string& arg1);
    void GetSampleOffset2(std::string& dst, std::string& texture, std::string& sampler, std::string& texcoord, std::string& offset, std::string& arg1, std::string& arg2);
    void GetSampleIndexable0(std::string& dst, std::string& texture, std::string& sampler, std::string& texcoord);
    void GetSampleIndexable1(std::string& dst, std::string& texture, std::string& sampler, std::string& texcoord, std::string& arg1);
    void GetSampleIndexable2(std::string& dst, std::string& texture, std::string& sampler, std::string& texcoord, std::string& args1, std::string& arg2);
    void GetLoad(std::string& dest, std::string& texture, std::string& texcoord);
    void GetLoadOffset(std::string& dest, std::string& texture, std::string& texcoord, std::string& offset);
    void GetResInfo(std::string& dest, std::string& texture, std::string& mipLevel);
    void PushDp(int count, bool sat);
    void PushCmd2(std::string tag, bool sat); // A = tag( B );
    void PushCmd3(std::string tag, bool sat); // A = tag( B, C );
    void PushCmd4(std::string tag, bool sat); // A = tag( B, C, D );
    void PushOp2(std::string tag, bool sat);  // A = B tag C;      <ex> A = B + C;
    void PushOp3(std::string tag1, std::string tag2, bool sat); // A = B tag1 C tag2 D  <ex> A = B + C * D;
    void PushCmp(std::string op, bool integer);
    void PushConvFromFloat(std::string tag, bool sat);
    void PushConvToFloat(std::string tag, bool sat);
    void PushLogicOp(std::string op);
    void PushShiftOp(std::string op);
    void PushMov(bool sat);
    void PushMovc(bool sat);
    std::string FilterSat(std::string value, bool sat);
    bool FindTag(std::string tag);  // 先頭からの部分一致であるので注意. 完全一致は m_Tokenizer.Compare()を使用する.
    bool ContainTag(std::string tag);
    bool Parse();
    void GenerateCode(std::string& sourceCode);
    bool WriteCode(const std::string& sourceCode);
};
