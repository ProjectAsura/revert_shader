//-------------------------------------------------------------------------------------------------
// File : AsmParser.cpp
// Desc : HLSL Assembly Parser Module.
// Copyright(c) Project Asura. All right reserved.
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------
#include "AsmParser.h"
#include "StringHelper.h"
#include <cstdio>
#include <new>
#include <cassert>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>


#ifndef DLOG
#if defined(DEBUG) || defined(_DEBUG)
#define DLOG( x, ... ) fprintf_s( stdout, "[File:%s, Line:%d] " x "\n", __FILE__, __LINE__, ##__VA_ARGS__ )
#else
#define DLOG( x, ... )
#endif
#endif//DLOG

#ifndef ELOG
#define ELOG( x, ... ) fprintf_s( stderr, x "\n", ##__VA_ARGS__)
#endif//ELOG


namespace {


std::string ToVarName(const std::string& name)
{
    std::string result;
    std::string temp = name;

    auto idx = name.find("SV_");
    if (idx != -1)
    {
        temp = name.substr(idx + 3);
    }

    auto count = temp.length();

    if (count >= 1)
    {
        result = std::toupper(temp[0]);
        for(auto i=1; i<count; ++i)
        {
            result += std::tolower(temp[i]);
        }
    }

    return result;
}

} // namespace


///////////////////////////////////////////////////////////////////////////////////////////////////
// AsmParser class
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//      コンストラクタです.
//-------------------------------------------------------------------------------------------------
AsmParser::AsmParser()
: m_pBuffer     (nullptr)
, m_BufferSize  (0)
, m_Tokenizer   ()
{ /* DO_NOTHING */ }

//-------------------------------------------------------------------------------------------------
//      デストラクタです.
//-------------------------------------------------------------------------------------------------
AsmParser::~AsmParser()
{
    if (m_pBuffer != nullptr)
    {
        delete[] m_pBuffer;
        m_pBuffer = nullptr;
    }

    m_Tokenizer.Term();
}

//-------------------------------------------------------------------------------------------------
//      アセンブリファイルをロードします.
//-------------------------------------------------------------------------------------------------
bool AsmParser::LoadAsm(const char* filename)
{
    FILE* pFile;

    // ファイルを開く.
    auto ret = fopen_s( &pFile, filename, "rb" );
    if ( ret != 0 )
    {
        ELOG( "Error : File Open Failed." );
        return false;
    }

    // ファイルサイズを算出.
    auto curpos = ftell(pFile);
    fseek(pFile, 0, SEEK_END);
    auto endpos = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    m_BufferSize = static_cast<size_t>(endpos - curpos);

    // メモリを確保.
    m_pBuffer = new(std::nothrow) char[m_BufferSize + 1]; // null終端させるために +1 している.
    if (m_pBuffer == nullptr)
    {
        ELOG( "Error : Out of memory." );
        fclose(pFile);
        return false;
    }

    // null終端になるようゼロクリア.
    memset(m_pBuffer, 0, sizeof(char) * (m_BufferSize + 1));

    // 一括読み込み.
    fread(m_pBuffer, sizeof(char) * m_BufferSize, 1, pFile);

    // ファイルを閉じる.
    fclose(pFile);

    // 正常終了.
    return true;
}

//-------------------------------------------------------------------------------------------------
//      アセンブリファイルを解析します.
//-------------------------------------------------------------------------------------------------
bool AsmParser::Parse()
{
    if (!LoadAsm(m_Argument.Input.c_str()))
    {
        ELOG( "Error : HLSL Asm File Load Failed. filename = %s", m_Argument.Input.c_str());
        return false;
    }

    if (!m_Tokenizer.Init(4096))
    {
        ELOG( "Error : Tokenizer Initialize Failed." );
        return false;
    }

    m_Tokenizer.SetSeparator(" \t\r\n,");
    m_Tokenizer.SetCutOff("{}():");
    m_Tokenizer.SetBuffer( m_pBuffer );

    std::ifstream file;
    file.open(m_Argument.Input.c_str(), std::ios::in);
    if (!file.is_open())
    { return false; }

    int index = -1;
    char buf[4096];
    memset(buf, 0, sizeof(buf));

    auto instructionCount = 0;
    bool uavInfo = false;
    bool structInfo = false;
    std::string uavName;

    a3d::ConstantBuffer cbDef = {};
    a3d::Structure structDef = {};

    std::string line;
    for(;;)
    {
        std::getline(file, line);
        if (!file)
            break;

        if (file.eof())
            break;

        strcpy_s(buf, line.c_str());

        if (buf[0] == '/' && buf[1] == '/')
        {
            // 改行コードは飛ばす.
            if (strlen(buf) == 2)
            {
                continue; 
            }
            else if (strstr(buf, "=") != nullptr)
            {
                // 定数代入文は無視.
                continue;
            }
            else if (strstr(buf, "Definitions:") != nullptr)
            {
                m_BufferSection = true;
            }
            else if (strstr(buf, "Bindings:") != nullptr)
            {
                m_ResourceSection   = true;
                m_BufferSection     = false;
                m_InputSection      = false;
                m_OutputSection     = false;
            }
            else if (strstr(buf, "signature:") != nullptr)
            {
                if (strstr(buf, "Input") != nullptr)
                {
                    m_ResourceSection   = false;
                    m_BufferSection     = false;
                    m_InputSection      = true;
                    m_OutputSection     = false;
                }
                else if (strstr(buf, "Output") != nullptr)
                {
                    m_ResourceSection   = false;
                    m_BufferSection     = false;
                    m_InputSection      = false;
                    m_OutputSection     = true;
                }
            }
            else if((buf[3] == 'N' && buf[4] == 'a' && buf[5] == 'm' && buf[6] == 'e')
                 || (buf[3] == '-'))
            {
                continue;
            }
            // 定数バッファの定義.
            else if (m_BufferSection)
            {
                auto decl = StringHelper::Replace(line, "//", "");
                if (StringHelper::Contain(decl, "=") >= 1)
                {
                   continue;
                }

                a3d::LAYOUT_TYPE layout = a3d::LAYOUT_DEFAULT;
                if (StringHelper::Contain(decl, "row_major") >= 1)
                {
                    decl = StringHelper::Replace(decl, "row_major", "");
                    layout = a3d::LAYOUT_ROW_MAJOR;
                }
                if (StringHelper::Contain(decl, "column_major") >= 1)
                {
                    decl = StringHelper::Replace(decl, "column_major", "");
                    layout = a3d::LAYOUT_COLUMN_MAJOR;
                }

                decl = StringHelper::Replace(decl, ";", "; "); // 分割のためにスペースを空ける.
                auto args = StringHelper::Split(decl, " ");

                if (StringHelper::Contain(decl, "}") >= 1)
                {
                    if (structInfo)
                    {
                        structDef.Members.shrink_to_fit();
                        structInfo = false;

                        m_Reflection.AddStructure(structDef);

                        continue;
                    }

                    if (uavInfo)
                    {
                        if (structDef.Name == "" && !structDef.Members.empty())
                        {
                            m_Reflection.AddUavStructPair(uavName, structDef.Members.front().Type);
                        }

                        structDef = a3d::Structure();
                        uavName.clear();

                        uavInfo = false;
                        continue;
                    }

                    // 追加登録.
                    cbDef.Variables.shrink_to_fit();
                    m_Reflection.AddConstantBuffer(cbDef);
                }
                else if (StringHelper::Contain(decl, "{") >= 1)
                {
                    continue;
                }
                else if (StringHelper::Contain(decl, "cbuffer") >= 1)
                {
                    cbDef.Name.clear();
                    cbDef.Variables.clear();
                    assert(args.size() == 2);

                    cbDef.Name = StringHelper::Replace(args[1], "$", "");
                }
                else if (StringHelper::Contain(decl, "struct") >= 1 && uavInfo)
                {
                    structDef.Name = args[1];
                    structInfo = true;
                    m_Reflection.AddUavStructPair(uavName, structDef.Name);
                }
                else
                {
                    if (args.size() == 5 && StringHelper::Contain(decl, "Resource bind info for") >= 1) 
                    {
                        uavInfo = true;
                        uavName = args[4];
                        continue;
                    }

                    assert(args.size() >= 6);
                    a3d::Variable varDef = {};
                    varDef.Type     = args[0];
                    varDef.Name     = StringHelper::Replace(args[1], ";", "");
                    varDef.Offset   = std::stoi(args[3]);
                    varDef.Size     = std::stoi(args[5]);
                    varDef.Layout   = layout;

                    if (!uavInfo)
                    { cbDef.Variables.push_back(varDef); }
                    else

                    { structDef.Members.push_back(varDef); }
                }
            }
            // リソースバインディングの定義.
            else if (m_ResourceSection)
            {
                auto decl = StringHelper::Replace(line, "//", "");
                auto item = StringHelper::Split(decl, " ");
                assert(item.size() == 6);

                a3d::Resource def = {};
                def.Name        = StringHelper::Replace(item[0], "$", "");
                def.Type        = item[1];
                def.Format      = item[2];
                def.Dimension   = item[3];
                def.HLSLBind    = item[4];
                def.Count       = std::stoi(item[5]);

                m_Reflection.AddResource(def);
            }
            // 入力定義.
            else if (m_InputSection)
            {
                if (StringHelper::Contain(line, "no Input") >= 1)
                {
                    // 入力データがない場合はすっ飛ばす.
                    continue;
                }

                auto decl = StringHelper::Replace(line, "//", "");
                auto args = StringHelper::Split(decl, " ");
                assert(args.size() >= 6);

                a3d::Signature inputDef = {};
                inputDef.Semantics      = args[0];
                inputDef.Index          = std::stoi(args[1]);
                inputDef.Mask           = args[2];
                inputDef.Register       = std::stoi(args[3]);
                inputDef.SystemValue    = args[4];
                inputDef.Format         = args[5];
                inputDef.Used           = (args.size() == 7) ? args[6] : "";
                inputDef.VarName        = ToVarName(inputDef.Semantics);

                m_Reflection.AddInputSignature(inputDef);
            }
            else if (m_OutputSection)
            {
                if (StringHelper::Contain(line, "no Output") >= 1)
                {
                    // 出力データがない場合はすっ飛ばす.
                    continue;
                }

                auto decl = StringHelper::Replace(line, "//", "");
                auto args = StringHelper::Split(decl, " ");
                assert(args.size() >= 6);

                a3d::Signature outputDef = {};
                outputDef.Semantics      = args[0];
                outputDef.Index          = std::stoi(args[1]);
                outputDef.Mask           = args[2];
                outputDef.Register       = std::stoi(args[3]);
                outputDef.SystemValue    = args[4];
                outputDef.Format         = args[5];
                outputDef.Used           = (args.size() == 7) ? args[6] : "";
                outputDef.VarName        = ToVarName(outputDef.Semantics);

                m_Reflection.AddOutputSignature(outputDef);
            }
        }
        else
        {
            //break;
            m_InputSection  = false;
            m_OutputSection = false;
            m_BufferSection = false;
            m_ResourceSection = false;
            instructionCount++;
        }
    }

    // ファイルを閉じる.
    file.close();

    m_Instructions.clear();
    m_Instructions.reserve(instructionCount);

    // 名前解決.
    m_Reflection.Resolve();

    // アセンブリ命令を解析.
    ParseAsm();

    m_Instructions.shrink_to_fit();

    return true;
}

//-------------------------------------------------------------------------------------------------
//      アセンブリファイルを解析します.
//-------------------------------------------------------------------------------------------------
void AsmParser::ParseAsm()
{
    /* HLSL Shader Compiler 10.1 を対象としています. */

    m_BufferSection     = false;
    m_ResourceSection   = false;
    m_InputSection      = false;
    m_OutputSection     = false;
    m_ShaderType        = SHADER_TYPE_VERTEX;

    bool find = false;

    int instructionCount = 0;

    while (!m_Tokenizer.IsEnd())
    {
        // シェーダプロファイルを取得.
        if (FindTag("vs"))
        {
            m_ShaderProfile = m_Tokenizer.GetAsChar();
            m_ShaderType    = SHADER_TYPE_VERTEX;
            find = true;
        }
        else if (FindTag("ps"))
        {
            m_ShaderProfile = m_Tokenizer.GetAsChar();
            m_ShaderType    = SHADER_TYPE_PIXEL;
            find = true;
        }
        else if (FindTag("gs"))
        {
            m_ShaderProfile = m_Tokenizer.GetAsChar();
            m_ShaderType    = SHADER_TYPE_GEOMETRY;
            find = true;
        }
        else if (FindTag("ds"))
        {
            m_ShaderProfile = m_Tokenizer.GetAsChar();
            m_ShaderType    = SHADER_TYPE_DOMAIN;
            find = true;
        }
        else if (FindTag("hs"))
        {
            m_ShaderProfile = m_Tokenizer.GetAsChar();
            m_ShaderType    = SHADER_TYPE_HULL;
            find = true;
        }
        else if (FindTag("cs"))
        {
            m_ShaderProfile = m_Tokenizer.GetAsChar();
            m_ShaderType    = SHADER_TYPE_COMPUTE;
            find = true;
        }

        // アセンブリ命令を解析.
        if (find)
        {
            if (ParseInstructionSM5())
            { continue; }

            if (ParseInstructionSM4())
            { continue; }

            m_Tokenizer.Next(); // 見つからない場合.
        }
        else
        { m_Tokenizer.Next(); } // プロファイルが出るまではコメント行とみなしてすっ飛ばす.
    }
}

//-------------------------------------------------------------------------------------------------
//      Shader Model 4.0 の命令を解析します.
//-------------------------------------------------------------------------------------------------
bool AsmParser::ParseInstructionSM4()
{
    // MSDN - Shader Model 4 Assembly.
    // https://msdn.microsoft.com/en-us/library/windows/desktop/bb943998(v=vs.85).aspx 参照.

    bool sat = ContainTag("_sat");

    if (FindTag("add"))
    {
        PushOp2("+", sat);
    }
    else if (FindTag("and"))
    {
        PushLogicOp("&");
    }
    else if (m_Tokenizer.Compare("break"))
    {
        PushInstruction("break;\n");
        m_Indent--;
        m_Tokenizer.Next();
    }
    else if (m_Tokenizer.Compare("breakc_z"))
    {
        auto cond = GetOperand();
        std::string cmd = "if (" + cond + " == 0) { break; }\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("breakc_nz"))
    {
        auto cond = GetOperand();
        std::string cmd = "if (" + cond + " != 0) { break; }\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("call"))
    {
        // TODO : Implementation
        auto tag = m_Tokenizer.NextAsChar();
        m_Tokenizer.SkipLine();
    }
    else if (m_Tokenizer.Compare("callc"))
    {
        // TODO : Implementation
        auto tag = m_Tokenizer.NextAsChar();
        m_Tokenizer.SkipLine();
    }
    else if (FindTag("case"))
    {
        std::string val;
        Get1(val);
        std::string cmd = "case " + val + ":\n";
        PushInstruction(cmd);
        m_Indent++;
    }
    else if (FindTag("cut"))
    {
        // TODO : Implementation
        auto tag = m_Tokenizer.NextAsChar();
        m_Tokenizer.SkipLine();
    }
    else if (FindTag("continue"))
    {
        PushInstruction("continue;\n");
        m_Tokenizer.Next();
    }
    else if (FindTag("continuec"))
    {
        // TODO : Implementation
        auto tag = m_Tokenizer.NextAsChar();
        m_Tokenizer.SkipLine();
    }
    else if (FindTag("dcl_constantBuffer"))
    {
        // TODO : Implementation
        std::string cb = m_Tokenizer.NextAsChar();
        std::string pt = m_Tokenizer.NextAsChar();
    }
    else if (FindTag("dcl_globalFlags"))
    {
        // TODO : Implementation
        std::string flag = m_Tokenizer.NextAsChar();
    }
    else if (FindTag("dcl_immediateConstantBuffer"))
    {
        // TODO : Implementation
        std::string value = m_Tokenizer.NextAsChar();
        std::string size  = m_Tokenizer.NextAsChar();
    }
    else if (FindTag("dcl_indexableTemp"))
    {
        std::string reg = m_Tokenizer.NextAsChar();
        std::string cnt = m_Tokenizer.NextAsChar();
        std::string cmd = StringHelper::Format("float%s %s;\n", cnt.c_str(), reg.c_str());
        PushInstruction(cmd);
    }
    else if (FindTag("dcl_indexRange"))
    {
        // TODO : Implementation
        std::string maxM = m_Tokenizer.NextAsChar();
        std::string maxN = m_Tokenizer.NextAsChar();
    }
    else if (FindTag("dcl_inputPrimitive"))
    {
        // TODO : Implementation
        std::string type = m_Tokenizer.NextAsChar();
    }
    else if (FindTag("dcl_input_sv"))
    {
        // TODO : Implementation
        std::string value    = m_Tokenizer.NextAsChar();
        std::string sysValue = m_Tokenizer.NextAsChar();
    }
    else if (FindTag("dcl_input"))
    {
        // TODO : Implementation
        std::string value = m_Tokenizer.NextAsChar();
    }
    else if (FindTag("dcl_maxOutputVertexCount"))
    {
        // TODO : Implementation
        std::string count = m_Tokenizer.NextAsChar();
    }
    else if (FindTag("dcl_output_sgv"))
    {
        // TODO : Implementation
        std::string reg      = m_Tokenizer.NextAsChar();
        std::string sysValue = m_Tokenizer.NextAsChar();
    }
    else if (FindTag("dcl_output_siv"))
    {
        // TODO : Implementation
        std::string reg = m_Tokenizer.NextAsChar();
        std::string sysValue = m_Tokenizer.NextAsChar();
    }
    else if (FindTag("dcl_outputTopology"))
    {
        // TODO : Implementation
        std::string type = m_Tokenizer.NextAsChar();
    }
    else if (FindTag("dcl_output"))
    {
        // TODO : Implementation
        std::string reg = m_Tokenizer.NextAsChar();
    }
    else if (FindTag("dcl_resource"))
    {
        // TODO : Implementation
        std::string reg     = m_Tokenizer.NextAsChar();
        std::string resType = GetArgs();
        std::string retType = m_Tokenizer.NextAsChar();
    }
    else if (FindTag("dcl_sampler"))
    {
        // TODO : Implementation
        std::string reg  = m_Tokenizer.NextAsChar();
        std::string mode = m_Tokenizer.NextAsChar();
        m_Tokenizer.Next();
    }
    else if (FindTag("dcl_temps"))
    {
        auto count = m_Tokenizer.NextAsInt();
        char buf[5];
        for(auto i=0; i<count; ++i)
        {
            sprintf_s(buf, "r%d", i);
            std::string temp = "float4 " + std::string(buf) + ";\n";
            PushInstruction(temp);
        }

        // 空行を入れる.
        m_Instructions.push_back("\n");
    }
    else if (FindTag("default"))
    {
        PushInstruction("default:\n");
        m_Indent++;
        m_Tokenizer.Next();
    }
    else if (FindTag("deriv_rtx"))
    {
        PushCmd2("ddx", sat);
    }
    else if (FindTag("deriv_rty"))
    {
        PushCmd2("ddy", sat);
    }
    else if (FindTag("discard_nz"))
    {
        std::string val;
        Get1(val);
        std::string cmd = "if (" + val + " != 0 ) { discard; }\n";
        PushInstruction(cmd);
    }
    else if (FindTag("discard_z"))
    {
        std::string val;
        Get1(val);
        std::string cmd = "if (" + val + " == 0 ) { discard; }\n";
        PushInstruction(cmd);
    }
    else if (FindTag("div"))
    {
        PushOp2("/", sat);
    }
    else if (FindTag("dp2"))
    {
        PushDp(2, sat);
    }
    else if (FindTag("dp3"))
    {
        PushDp(3, sat);
    }
    else if (FindTag("dp4"))
    {
        PushDp(4, sat);
    }
    else if (FindTag("else"))
    {
        m_Indent--;
        PushInstruction("}\n");
        PushInstruction("else\n");
        PushInstruction("{\n");
        m_Indent++;
        m_Tokenizer.Next();
    }
    else if (FindTag("emit"))
    {
        // TODO : HLSL Implement.
        m_Tokenizer.Next();
    }
    else if (FindTag("emitThenCut"))
    {
        // TODO : HLSL Implement.
        m_Tokenizer.Next();
    }
    else if (FindTag("endif"))
    {
        m_Indent--;
        PushInstruction("}\n");
        m_Tokenizer.Next();
    }
    else if (FindTag("endloop"))
    {
        m_Indent--;
        PushInstruction("}\n");
        m_Tokenizer.Next();
    }
    else if (FindTag("endswitch"))
    {
        m_Indent--;
        PushInstruction("}\n");
        m_Tokenizer.Next();
    }
    else if (FindTag("eq"))
    {
        PushCmp("==", false);
    }
    else if (FindTag("exp"))
    {
        PushCmd2("exp", sat);
    }
    else if (FindTag("frc"))
    {
        PushCmd2("frac", sat);
    }
    else if (FindTag("ftoi"))
    {
        PushConvFromFloat("asint", sat);
    }
    else if (FindTag("ftou"))
    {
        PushConvFromFloat("asuint", sat);
    }
    else if (FindTag("ge"))
    {
        PushCmp(">=", false);
    }
    else if (FindTag("iadd"))
    {
        PushOp2("+", sat);
    }
    else if (FindTag("ieq"))
    {
        PushCmp("==", true);
    }
    else if (FindTag("if_z"))
    {
        std::string val;
        Get1(val);
        std::string cmd = "if (" + FilterSat( val, sat ) + ")\n";
        PushInstruction(cmd);
        PushInstruction("{\n");
        m_Indent++;
    }
    else if (FindTag("if_nz"))
    {
        std::string val;
        Get1(val);
        std::string cmd = "if (" + FilterSat( val, sat ) + " != 0)\n";
        PushInstruction(cmd);
        PushInstruction("{\n");
        m_Indent++;
    }
    else if (FindTag("ige"))
    {
        PushCmp(">=", true);
    }
    else if (m_Tokenizer.Compare("ilt"))
    {
        PushCmp("<", true);
    }
    else if (FindTag("imad"))
    {
        PushOp3("*", "+", sat);
    }
    else if (FindTag("imin"))
    {
        PushCmd3("min", sat);
    }
    else if (FindTag("imul"))
    {
        std::string dstHi;
        std::string dstLo;
        std::string lhs;
        std::string rhs;
        Get4(dstHi, dstLo, lhs, rhs);

        if (dstLo == "null")
        {
            std::string cmd = dstHi + " = " + lhs + " * " + rhs + ";\n";
            PushInstruction(cmd);
        }
        else
        {
            std::string cmd = dstLo + " = " + lhs + " * " + rhs + ";\n";
            PushInstruction(cmd);
        }
    }
    else if (m_Tokenizer.Compare("ine"))
    {
        PushCmp("!=", true);
    }
    else if (m_Tokenizer.Compare("ineg"))
    {
        std::string dst, src;
        Get2(dst, src);

        std::string cmd = dst + " = " + "~" + src + ";\n";
        PushInstruction(cmd);
    }
    else if (FindTag("ishl"))
    {
        PushShiftOp("<<");
    }
    else if (FindTag("ishr"))
    {
        PushShiftOp(">>");
    }
    else if (FindTag("itof"))
    {
        PushConvToFloat("asfloat", sat);
    }
    else if (FindTag("label"))
    {
        // TODO : Implementation
        std::string tag = m_Tokenizer.NextAsChar();
    }
    else if (m_Tokenizer.Compare("ld"))
    {
        std::string dest;
        std::string texture;
        std::string texcoord;
        GetLoad(dest, texture, texcoord);
        std::string cmd = dest + " = " + texture + ".Load(" + texcoord + ");\n";
        PushInstruction(cmd);
    }
    else if (FindTag("ld_aoffimmi"))
    {
        std::string dest;
        std::string texture;
        std::string texcoord;
        std::string offset;
        GetLoadOffset(dest, texture, texcoord, offset);
        std::string cmd = dest + " = " + texture + ".Load(" + texcoord + ", " + offset + ");\n";
        PushInstruction(cmd);
    }
    else if (FindTag("log"))
    {
        PushCmd2("log", sat);
    }
    else if (FindTag("loop"))
    {
        PushInstruction("while(1)\n");
        PushInstruction("{\n");
        m_Indent++;
        m_Tokenizer.Next();
    }
    else if (FindTag("lt"))
    {
        PushCmp("<", false);
    }
    else if (FindTag("mad"))
    {
        PushCmd4("mad", sat);
    }
    else if (FindTag("max"))
    {
        PushCmd3("max", sat);
    }
    else if (FindTag("min"))
    {
        PushCmd3("min", sat);
    }
    else if (m_Tokenizer.Compare("mov"))
    {
        PushMov(sat);
    }
    else if (m_Tokenizer.Compare("movc"))
    {
        PushMovc(sat);
    }
    else if (FindTag("mul"))
    {
        PushOp2("*", sat);
    }
    else if (FindTag("ne"))
    {
        PushCmp("!=", false);
    }
    else if (FindTag("nop"))
    {
        m_Tokenizer.Next();
    }
    else if (FindTag("not"))
    {
        PushCmd2("not", sat);
    }
    else if (FindTag("or"))
    {
        PushLogicOp("|");
    }
    else if (FindTag("resinfo")) // resinfo_uint, resinfo_rcpFloat 共通.
    {
        std::string dest;
        std::string texture;
        std::string mipLevel;
        GetResInfo(dest, texture, mipLevel);

        std::string cmd = dest + " = " + "GetResourceInfo(" + texture + ", " + mipLevel + ");\n";
        PushInstruction(cmd);

        m_HasGetResourceInfo = true;
    }
    else if (m_Tokenizer.Compare("retc_z"))
    {
        std::string op = GetOperand();
        std::string cmd = "if (" + op + ") return;\n";
        PushInstruction(cmd);
        m_Indent++;
    }
    else if (m_Tokenizer.Compare("retc_nz"))
    {
        std::string op = GetOperand();
        std::string cmd = "if (!" + op + ") return;\n";
        PushInstruction(cmd);
        m_Indent++;
    }
    else if (m_Tokenizer.Compare("ret"))
    {
        m_Tokenizer.Next();
    }
    else if (FindTag("round_ne"))
    {
        PushCmd2("round", sat);
    }
    else if (FindTag("round_ni"))
    {
        PushCmd2("floor", sat);
    }
    else if (FindTag("round_pi"))
    {
        PushCmd2("ceil", sat);
    }
    else if (FindTag("round_z"))
    {
        PushCmd2("frac", sat);
    }
    else if (FindTag("rsq"))
    {
        PushCmd2("rsqrt", sat);
    }
    else if (m_Tokenizer.Compare("sample"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        GetSample0(dest, texture, sampler, texcoord);

        std::string cmd = dest + " = " + texture + ".Sample(" + sampler + ", " + texcoord + ");\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_aoffimmi"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string offset;
        GetSampleOffset0(dest, texture, sampler, texcoord, offset);

        std::string cmd = dest + " = " + texture + ".Sample(" + sampler + ", " + texcoord + ", " + offset + ");\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_b"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string lodBias;
        GetSample1( dest, texture, sampler, texcoord, lodBias);

        std::string cmd = dest + " = " + texture + "SampleBias(" + sampler + "," + texcoord + ", " + lodBias + ");\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_b_aoffimmi"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string offset;
        std::string lodBias;
        GetSampleOffset1( dest, texture, sampler, texcoord, offset, lodBias );

        std::string cmd = dest + " = " + texture + ".SampleBias(" + sampler + ", " + texcoord + ", " + lodBias + ", " + offset + ");\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_c"))
    {
        std::string dst;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string refValue;
        GetSample1(dst, texture, sampler, texcoord, refValue);

        std::string cmd = dst + " = " + texture + ".SampleCmp(" + sampler + ", " + texcoord + ", " + refValue + ");\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_c_aoffimmi"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string offset;
        std::string refValue;
        GetSampleOffset1(dest, texture, sampler, texcoord, offset, refValue);

        std::string cmd = dest + " = " + texture + ".SampleCmp(" + sampler + ", " + texcoord + ", " + refValue + ", " + offset + ");\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_c_lz"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string refValue;
        GetSample1( dest, texture, sampler, texcoord, refValue );

        std::string cmd = dest + " = " + texture + ".SampleCmpLevelZero(" + sampler + ", " + texcoord + ", "  + refValue + ");\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_c_lz_aoffimmi"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string offset;
        std::string refValue;
        GetSampleOffset1( dest, texture, sampler, texcoord, offset, refValue );

        std::string cmd = dest + " = " + texture + ".SampleCmpLevelZero(" + sampler + ", " + texcoord + ", " + refValue + ", " + offset + ");\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_d"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string xDerivative;
        std::string yDerivative;
        GetSample2( dest, texture, sampler, texcoord, xDerivative, yDerivative );

        std::string left = texture + ".SampleGrad(" + sampler + ", " + texcoord + ", " + xDerivative + ", " + yDerivative + ")";
        std::string cmd = dest + " = " + FilterSat( left, sat ) + ";\n";
        PushInstruction(cmd);
    }
    else if(m_Tokenizer.Compare("sample_d_aoffimmi"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string offset;
        std::string xDerivative;
        std::string yDerivative;
        GetSampleOffset2(dest, texture, sampler, texcoord, offset, xDerivative, yDerivative);

        std::string left = texture + ".SampleGrad(" + sampler + ", " + texcoord + ", " + xDerivative + ", " + yDerivative + ", " + offset + ")";
        std::string cmd  = dest + " = " + FilterSat( left, sat ) + ";\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_l"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string lod;
        GetSample1(dest, texture, sampler, texcoord, lod);

        std::string left = texture + ".SampleLevel(" + sampler + ", " + texcoord + ", " + lod + ")";
        std::string cmd  = dest + " = " + FilterSat( left, sat ) + ";\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_l_aoffimmi"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string offset;
        std::string lod;
        GetSampleOffset1( dest, texture, sampler, texcoord, offset, lod );

        std::string left = texture + ".SampleLevel(" + sampler + ", " + texcoord + ", " + lod + ", " + offset + ")";
        std::string cmd  = dest + " = " + FilterSat( left, sat ) + ";\n";
        PushInstruction(cmd);
    }
    else if (FindTag("sincos"))
    {
        std::string dstSin = m_Tokenizer.NextAsChar();
        std::string dstCos = m_Tokenizer.NextAsChar();
        std::string src    = GetOperand();

        auto sinInfo = a3d::Reflection::ToSwizzleInfo(dstSin);
        auto cosInfo = a3d::Reflection::ToSwizzleInfo(dstCos);

        dstSin = StringHelper::GetWithSwizzle(dstSin);
        dstCos = StringHelper::GetWithSwizzle(dstCos);
        
        std::string srcSin = m_Reflection.GetCastedString(src, sinInfo);
        std::string srcCos = m_Reflection.GetCastedString(src, cosInfo);

        std::string left1 = "sin(" + srcSin + ")";
        std::string left2 = "cos(" + srcCos + ")";

        std::string cmd1, cmd2;
        if (dstSin != "null")
        {
            cmd1 = dstSin + " = " + FilterSat( left1, sat ) + ";\n";
            PushInstruction(cmd1);
        }
        if (dstCos != "null")
        { 
            cmd2 = dstCos + " = " + FilterSat( left2, sat ) + ";\n";
            PushInstruction(cmd2);
        }
    }
    else if (FindTag("sqrt"))
    {
        PushCmd2("sqrt", sat);
    }
    else if (m_Tokenizer.Compare("switch"))
    {
        std::string val = m_Tokenizer.NextAsChar();
        std::string cmd = "switch(" + val + ") {\n";
        PushInstruction(cmd);
        m_Indent++;
    }
    else if (FindTag("udiv"))
    {
        std::string dstQUOT, dstREM, lhs, rhs;
        Get4(dstQUOT, dstREM, lhs, rhs);

        if (dstREM != "null")
        {
            std::string cmd = dstREM + " = " + lhs + " % " + rhs + ";\n";
            PushInstruction(cmd);
        }
        if (dstQUOT != "null")
        {
            std::string cmd = dstQUOT + " = " + lhs + " / " + rhs + ";\n";
            PushInstruction(cmd);
        }
    }
    else if (FindTag("uge"))
    {
        PushCmp(">=", true);
    }
    else if (FindTag("ult"))
    {
        PushCmp("<", true);
    }
    else if (FindTag("umad"))
    {
        PushOp3("*", "+", sat);
    }
    else if (FindTag("umax"))
    {
        PushCmd3("max", sat);
    }
    else if (FindTag("umin"))
    {
        PushCmd3("min", sat);
    }
    else if (FindTag("umul"))
    {
        PushOp2("*", sat);
    }
    else if (FindTag("ushr"))
    {   
        PushShiftOp(">>");
    }
    else if (FindTag("utof"))
    {
        PushConvToFloat("asfloat", sat);
    }
    else if (FindTag("xor"))
    {
        PushLogicOp("^");
    }
    else
    {
        return false;
    }

    return true;
}

//-------------------------------------------------------------------------------------------------
//      Shader Model 5.0 の命令を解析します.
//-------------------------------------------------------------------------------------------------
bool AsmParser::ParseInstructionSM5()
{
    // MSDN - Shader Model 5 Assembly.
    // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/shader-model-5-assembly--directx-hlsl- 参照.

    bool sat = ContainTag("_sat");

    if (FindTag("atomic_and"))
    {
        std::string dst;
        std::string dstAddress;
        std::string src0;
        Get3(dst, dstAddress, src0);

        std::string cmd = std::string("InterlockedAnd(") + dst + ", " + src0 + ");\n";
        PushInstruction(cmd);
    }
    else if (FindTag("atomic_cmp_store"))
    {
        std::string dst;
        std::string dstAddress;
        std::string src0;
        std::string src1;
        Get4(dst, dstAddress, src0, src1);

        // TODO : Implement.
    }
    else if (FindTag("atomic_iadd"))
    {
        std::string dst;
        std::string dstAddress;
        std::string src0;
        Get3(dst, dstAddress, src0);

        std::string cmd = std::string("InterlockedAdd(") + dst + ", " + src0 + ");\n";
        PushInstruction(cmd);
    }
    else if (FindTag("atomic_imax"))
    {
        std::string dst;
        std::string dstAddress;
        std::string src0;
        Get3(dst, dstAddress, src0);

        std::string cmd = std::string("InterlockedMax(") + dst + ", " + src0 + ");\n";
        PushInstruction(cmd);
    }
    else if (FindTag("atomic_imin"))
    {
        std::string dst;
        std::string dstAddress;
        std::string src0;
        Get3(dst, dstAddress, src0);

        std::string cmd = std::string("InterlockedMin(") + dst + ", " + src0 + ");\n";
        PushInstruction(cmd);
    }
    else if (FindTag("atomic_or"))
    {
        std::string dst;
        std::string dstAddress;
        std::string src0;
        Get3(dst, dstAddress, src0);

        std::string cmd = std::string("InterlockedOr(") + dst + ", " + src0 + ");\n";
        PushInstruction(cmd);
    }
    else if (FindTag("atomic_umax"))
    {
        std::string dst;
        std::string dstAddress;
        std::string src0;
        Get3(dst, dstAddress, src0);

        std::string cmd = std::string("InterlockedMax(") + dst + ", " + src0 + ");\n";
        PushInstruction(cmd);
    }
    else if (FindTag("atomic_umin"))
    {
        std::string dst;
        std::string dstAddress;
        std::string src0;
        Get3(dst, dstAddress, src0);

        std::string cmd = std::string("InterlockedMin(") + dst + ", " + src0 + ");\n";
        PushInstruction(cmd);
    }
    else if (FindTag("atomic_xor"))
    {
        std::string dst;
        std::string dstAddress;
        std::string src0;
        Get3(dst, dstAddress, src0);

        std::string cmd = std::string("InterlockedXor(") + dst + ", " + src0 + ");\n";
        PushInstruction(cmd);
    }
    else if (FindTag("bfi"))
    {
        std::string dst;
        std::string src0;
        std::string src1;
        std::string src2;
        std::string src3;
        Get5(dst, src0, src1, src2, src3);

        // TODO : Implement.
    }
    else if (FindTag("bfrev"))
    {
        PushCmd2("reversebits", sat);
    }
    else if (FindTag("bufinfo"))
    {
        std::string dst;
        std::string srcResource;
        Get2(dst, srcResource);

        std::string cmd = srcResource + ".GetDimensions(" + dst + ");\n";
        PushInstruction(cmd);
    }
    else if (FindTag("countbits"))
    {
        PushCmd2("countbits", sat);
    }
    else if (FindTag("cut_stream"))
    {
        std::string streamIndex = GetOperand();

        // TODO : Implement.
    }
    else if (FindTag("dadd"))
    {
        PushOp2("+", sat);
    }
    else if (FindTag("dcl_function_body"))
    {
        auto label = m_Tokenizer.NextAsChar();
        // TODO : Implement.
    }
    else if (FindTag("dcl_function_table"))
    {
        // TODO : Implement.
        auto table = GetOperand();
        while(m_Tokenizer.Compare("}"))
        {
            m_Tokenizer.Next();
        }
    }
    else if (FindTag("dcl_hs_fork_phase_instance_count"))
    {
        // TODO : Implement.
        while(m_Tokenizer.Compare("}"))
        {
            m_Tokenizer.Next();
        }
    }
    else if (FindTag("dcl_hs_join_phase_instance_count"))
    {
        // TODO : Implement.
        while(m_Tokenizer.Compare("}"))
        {
            m_Tokenizer.Next();
        }
    }
    else if (FindTag("dcl_hs_max_tessfactor"))
    {
        // TODO : Implement.
        auto count = m_Tokenizer.NextAsChar();
    }
    else if (FindTag("dcl_input"))
    {
        m_Tokenizer.Next();
        if (m_Tokenizer.Compare("vForkInstanceID"))
        {
            // TODO : Implement.
        }
        else if (m_Tokenizer.Compare("vGSInstanceID"))
        {
            auto instanceCount = m_Tokenizer.NextAsChar();

            std::string cmd = "uint gsInstanceId : SV_InstanceID";
            m_Reflection.AddInputArgs(cmd);
        }
        else if (m_Tokenizer.Compare("vJoinInstanceID"))
        {
            // TODO : Implement.
        }
        else if (m_Tokenizer.Compare("vOutputControlPointID"))
        {
            std::string cmd = "uint controlPointId : SV_OutputControlPointID";
            m_Reflection.AddInputArgs(cmd);
        }
        else if (FindTag("vThreadID"))
        {
            std::string id = m_Tokenizer.GetAsChar();
            auto info = a3d::Reflection::ToSwizzleInfo(id);

            std::string cmd = StringHelper::Format("uint%d dispatchId : SV_DispatchThreadID", info.Count);
            m_Reflection.AddInputArgs(cmd);
        }
        else if (FindTag("vThreadGroupID"))
        {
            std::string id = m_Tokenizer.GetAsChar();
            auto info = a3d::Reflection::ToSwizzleInfo(id);

            std::string cmd = StringHelper::Format("uint%d groupId : SV_GroupID", info.Count);
            m_Reflection.AddInputArgs(cmd);
        }
        else if (FindTag("vThreadIDInGroup"))
        {
            std::string id = m_Tokenizer.GetAsChar();
            auto info = a3d::Reflection::ToSwizzleInfo(id);

            std::string cmd = StringHelper::Format("uint%d groupThreadId : SV_GroupThreadID", info.Count);
            m_Reflection.AddInputArgs(cmd);
        }
        else if (FindTag("vThreadIDInGroupFlattened"))
        {
            std::string id = m_Tokenizer.GetAsChar();
            auto info = a3d::Reflection::ToSwizzleInfo(id);

            std::string cmd = StringHelper::Format("uint%d groupIndex : SV_GroupIndex", info.Count);
            m_Reflection.AddInputArgs(cmd);
        }
    }
    else if (FindTag("dcl_input_control_point_count"))
    {
        // TODO : Implement.
        while(m_Tokenizer.Compare("}"))
        {
            m_Tokenizer.Next();
        }
    }
    else if (FindTag("dcl_interface"))
    {
        // TODO : Implement.
        auto fp = GetOperand();
        m_Tokenizer.Next(); // =
        while(m_Tokenizer.Compare("}"))
        {
            m_Tokenizer.Next();
        }
    }
    else if (FindTag("dcl_interface_dynamicindexed"))
    {
        // TODO : Implement.
        auto fp = GetOperand();
        m_Tokenizer.Next(); // =
        while(m_Tokenizer.Compare("}"))
        {
            m_Tokenizer.Next();
        }
    }
    else if (FindTag("dcl_output"))
    {
        auto mask = GetOperand();

        // TODO : Implement.
    }
    else if (FindTag("dcl_output_control_point_count"))
    {
        auto count = m_Tokenizer.NextAsChar();
        // TODO : Implement.
    }
    else if (FindTag("dcl_resource_raw"))
    {
        auto uav = GetOperand();
        // TODO : Implement.
    }
    else if (FindTag("dcl_resource_structured"))
    {
        auto uav = GetOperand();
        auto stride = GetOperand();
        // TODO : Implement.
    }
    else if (FindTag("dcl_stream"))
    {
        auto count = m_Tokenizer.NextAsChar(); // m0, m1, m2, m3.
        // TODO : Implement.
    }
    else if (FindTag("dcl_tessellator_domain"))
    {
        auto domain = m_Tokenizer.NextAsChar();
        // TODO : Implement.
    }
    else if (FindTag("dcl_tessellator_output_primitive"))
    {
        auto primitive = m_Tokenizer.NextAsChar();
        // TODO : Implement.
    }
    else if (FindTag("dcl_tessellator_partitioning"))
    {
        auto partition = m_Tokenizer.NextAsChar();
        // TODO : Implement.
    }
    else if (FindTag("dcl_tgsm_raw"))
    {
        auto group     = m_Tokenizer.NextAsChar();
        auto byteCount = m_Tokenizer.NextAsChar();
        // TODO : Implement.
    }
    else if (FindTag("dcl_tgsm_structured"))
    {
        auto group  = m_Tokenizer.NextAsChar();
        auto stride = m_Tokenizer.NextAsChar();
        auto count  = m_Tokenizer.NextAsChar();
        // TODO : Implement.
    }
    else if (FindTag("dcl_thread_group"))
    {
        auto x = m_Tokenizer.NextAsInt();
        auto y = m_Tokenizer.NextAsInt();
        auto z = m_Tokenizer.NextAsInt();

        m_ThreadCountX = uint32_t(x);
        m_ThreadCountY = uint32_t(y);
        m_ThreadCountZ = uint32_t(z);
    }
    else if (FindTag("dcl_uav_raw"))
    {
        auto uav = GetOperand();
        // TODO : Implement.
    }
    else if (FindTag("dcl_uav_structured"))
    {
        auto uav = GetOperand();
        auto stride = m_Tokenizer.NextAsChar();
        // TODO : Implement.
    }
    else if (FindTag("dcl_uav_typed"))
    {
        auto uav       = GetOperand();
        auto dimension = m_Tokenizer.NextAsChar();
        auto type      = m_Tokenizer.NextAsChar();
        // TODO : Implement.
    }
    else if (FindTag("ddiv"))
    {
        PushOp2("/", sat);
    }
    else if (FindTag("deq"))
    {
        PushOp2("==", sat);
    }
    else if (FindTag("deriv_rtx_coarse"))
    {
        PushCmd2("ddx_coarse", sat);
    }
    else if (FindTag("deriv_rtx_fine"))
    {
        PushCmd2("ddx_fine", sat);
    }
    else if (FindTag("deriv_rty_coarse"))
    {
        PushCmd2("ddy_coarse", sat);
    }
    else if (FindTag("deriv_rty_fine"))
    {
        PushCmd2("ddy_fine", sat);
    }
    else if (FindTag("dfma"))
    {
        PushCmd4("fma", sat);
    }
    else if (FindTag("dge"))
    {
        PushOp2(">=", sat);
    }
    else if (FindTag("dlt"))
    {
        PushOp2("<", sat);
    }
    else if (FindTag("dmax"))
    {
        PushCmd2("max", sat);
    }
    else if (FindTag("dmin"))
    {
        PushCmd2("min", sat);
    }
    else if (FindTag("dmov"))
    {
        PushMov(sat);
    }
    else if (FindTag("dmovc"))
    {
        PushMovc(sat);
    }
    else if (FindTag("dmul"))
    {
        PushOp2("*", sat);
    }
    else if (FindTag("dne"))
    {
        PushOp2("!=", sat);
    }
    else if (FindTag("drcp"))
    {
        PushCmd2("rcp", sat);
    }
    else if (FindTag("dtof"))
    {
        PushCmd2("asfloat", sat);
    }
    else if (FindTag("emit_stream"))
    {
        auto streamIndex = m_Tokenizer.NextAsChar();
        // TODO : Implement.
    }
    else if (FindTag("emitThenCut_stream"))
    {
        auto streamIndex = m_Tokenizer.NextAsChar();
        // TODO : Implement.
    }
    else if (FindTag("f16tof32"))
    {
        PushCmd2("f16tof32", sat);
    }
    else if (FindTag("f32tof16"))
    {
        PushCmd2("f32tof16", sat);
    }
    else if (FindTag("fcall"))
    {
        auto fp = GetOperand();
        // TODO : Implement.
    }
    else if (FindTag("firstbit"))
    {
        if (m_Tokenizer.Compare("firstbit_hi"))
        {
            PushCmd2("firstbithigh", sat);
        }
        else if (m_Tokenizer.Compare("firstbit_lo"))
        {
            PushCmd2("firstbitlow", sat);
        }
        else if (m_Tokenizer.Compare("fistbit_shi"))
        {
            PushCmd2("firstbithigh", sat);
        }
    }
    else if (FindTag("ftod"))
    {
        PushCmd2("asdouble", sat);
    }
    else if (FindTag("gather4"))
    {
        if (m_Tokenizer.Compare("gather4_aoffimmi_indexable"))
        {
            std::string dest;
            std::string texture;
            std::string sampler;
            std::string texcoord;
            std::string offset;
            GetSampleOffsetIndexable0(dest, texture, sampler, texcoord, offset);

            std::string cmd = dest + " = " + texture + ".Gather(" + sampler + ", " + texcoord + ", " + offset + ");\n";
            PushInstruction(cmd);
        }
        else if (m_Tokenizer.Compare("gather4_indexable"))
        {
            std::string dest;
            std::string texcoord;
            std::string texture;
            std::string sampler;
            GetSampleIndexable0(dest, texture, sampler, texcoord);

            std::string left = texture + ".Gather(" + sampler + ", " + texcoord + ")";
            std::string cmd = dest + " = " + FilterSat(left, sat) + ";\n";
            PushInstruction(cmd);
        }
    }
    else if (FindTag("gather4_c"))
    {
        if (m_Tokenizer.Compare("gather4_c_aoffimmi_indexable"))
        {
            std::string dest;
            std::string texture;
            std::string sampler;
            std::string texcoord;
            std::string offset;
            std::string refValue;
            GetSampleOffsetIndexable1(dest, texture, sampler, texcoord, offset, refValue);

            std::string cmd = dest + " = " + texture + ".GatherCmp(" + sampler + ", " + texcoord + ", " + refValue + ", " + offset + ");\n";
            PushInstruction(cmd);
        }
        else if (m_Tokenizer.Compare("gather4_c_indexable"))
        {
            std::string dst;
            std::string texture;
            std::string sampler;
            std::string texcoord;
            std::string refValue;
            GetSampleIndexable1(dst, texture, sampler, texcoord, refValue);

            std::string cmd = dst + " = " + texture + ".GatherCmp(" + sampler + ", " + texcoord + ", " + refValue + ");\n";
            PushInstruction(cmd);
        }
    }
    else if (FindTag("gather4_po"))
    {
        if (m_Tokenizer.Compare("gather4_po_aoffimmi_indexable"))
        {
            std::string dest;
            std::string srcAddress;
            std::string srcOffset;
            std::string srcResource;
            std::string srcSampler;
            Get5(dest, srcAddress, srcOffset, srcResource, srcSampler);
            // TODO : Implement.
        }
        else if (m_Tokenizer.Compare("gather4_po_indexable"))
        {
            std::string dest;
            std::string srcAddress;
            std::string srcOffset;
            std::string srcResource;
            std::string srcSampler;
            Get5(dest, srcAddress, srcOffset, srcResource, srcSampler);
            // TODO : Implement.
        }
    }
    else if (FindTag("gather4_po_c"))
    {
        if (m_Tokenizer.Compare("gather4_po_c_aoffimmi_indexable"))
        {
            std::string dest;
            std::string srcAddress;
            std::string srcOffset;
            std::string srcResource;
            std::string srcSampler;
            std::string srcReferenceValue;
            Get6(dest, srcAddress, srcOffset, srcResource, srcSampler, srcReferenceValue);
            // TODO : Implement.
        }
        else if (m_Tokenizer.Compare("gather4_po_c_indexable"))
        {
            std::string dest;
            std::string srcAddress;
            std::string srcOffset;
            std::string srcResource;
            std::string srcSampler;
            std::string srcReferenceValue;
            Get6(dest, srcAddress, srcOffset, srcResource, srcSampler, srcReferenceValue);
            // TODO : Implement.
        }
    }
    else if (FindTag("hs_control_point_phase"))
    {
        // TODO : Implement.
    }
    else if (FindTag("hs_decls"))
    {
        // TODO : Implement.
    }
    else if (FindTag("hs_fork_phase"))
    {
        // TODO : Implement.
    }
    else if (FindTag("hs_join_phase"))
    {
        // TODO : Implement.
    }
    else if (FindTag("ibfe"))
    {
        auto dest = GetOperand();
        auto src0 = GetOperand();
        auto src1 = GetOperand();
        auto src2 = GetOperand();
        // TODO : Implement.
    }
    else if (FindTag("imm_atomic_alloc"))
    {
        std::string dst;
        std::string dstUAV;
        Get2(dst, dstUAV);
        // TODO : Implement.
    }
    else if (FindTag("imm_atomic_and"))
    {
        std::string dst0;
        std::string dst1;
        std::string dstAddress;
        std::string src0;
        Get4(dst0, dst1, dstAddress, src0);
        // TODO : Implement.
    }
    else if (FindTag("imm_atomic_cmp_exch"))
    {
        std::string dst0;
        std::string dst1;
        std::string dstAddress;
        std::string src0;
        std::string src1;
        Get5(dst0, dst1, dstAddress, src0, src1);
        // TODO : Implement.
    }
    else if (FindTag("imm_atomic_consume"))
    {
        std::string dst0;
        std::string dstUAV;
        Get2(dst0, dstUAV);
        // TODO : Implement.
    }
    else if (FindTag("imm_atomic_exch"))
    {
        std::string dst0;
        std::string dst1;
        std::string dstAddress;
        std::string src0;
        Get4(dst0, dst1, dstAddress, src0);
        // TODOl : Implement.
    }
    else if (FindTag("imm_atomic_iadd"))
    {
        std::string dst0;
        std::string dst1;
        std::string dstAddress;
        std::string src0;
        Get4(dst0, dst1, dstAddress, src0);
        // TODO : Implement.
    }
    else if (FindTag("imm_atomic_imax"))
    {
        std::string dst0;
        std::string dst1;
        std::string dstAddress;
        std::string src0;
        Get4(dst0, dst1, dstAddress, src0);
        // TODO : Implement.
    }
    else if (FindTag("imm_atomic_imin"))
    {
        std::string dst0;
        std::string dst1;
        std::string dstAddress;
        std::string src0;
        Get4(dst0, dst1, dstAddress, src0);
        // TODO : Implement.
    }
    else if (FindTag("imm_atomic_or"))
    {
        std::string dst0;
        std::string dst1;
        std::string dstAddress;
        std::string src0;
        Get4(dst0, dst1, dstAddress, src0);
        // TODO : Implement.
    }
    else if (FindTag("imm_atomic_umax"))
    {
        std::string dst0;
        std::string dst1;
        std::string dstAddress;
        std::string src0;
        Get4(dst0, dst1, dstAddress, src0);
        // TODOl : Implement.
    }
    else if (FindTag("imm_atomic_umin"))
    {
        std::string dst0;
        std::string dst1;
        std::string dstAddress;
        std::string src0;
        Get4(dst0, dst1, dstAddress, src0);
        // TODO : Implement.
    }
    else if (FindTag("imm_atomic_xor"))
    {
        std::string dst0;
        std::string dst1;
        std::string dstAddress;
        std::string src0;
        Get4(dst0, dst1, dstAddress, src0);
        // TODO : Implement.
    }
    else if (FindTag("ishl"))
    {
        PushOp2("<<", sat);
    }
    else if (FindTag("ishr"))
    {
        PushOp2(">>", sat);
    }
    else if (FindTag("ld_raw"))
    {
        std::string dst0;
        Get1(dst0);

        auto srcByteOffset = GetOperand();
        auto src0 = GetOperand();

        // TODO : 実装が怪しいので後でチェック.
        std::string cmd = dst0 + " = " + src0 + "[" + srcByteOffset + "];\n";
        PushInstruction(cmd);
    }
    else if (FindTag("ld_structured"))
    {
        std::string dst0;
        Get1(dst0);

        auto srcAddress    = GetOperand();
        auto srcByteOffset = GetOperand();
        auto src0          = GetOperand();

        // TODO : 実装が怪しいので後でチェック.
        std::string cmd = dst0 + " = " + src0 + "[" + srcAddress + "];\n";
        PushInstruction(cmd);
    }
    else if (FindTag("ld_uav_typed"))
    {
        std::string dst0;
        Get1(dst0);

        auto srcAddress = GetOperand();
        auto srcUAV = GetOperand();

        std::string cmd = dst0 + " = " + srcUAV + "[" + srcAddress + "];\n";
        PushInstruction(cmd);
    }
    else if (FindTag("rcp"))
    {
        PushCmd2("rcp", sat);
    }
    else if (m_Tokenizer.Compare("sample_indexable"))
    {
        std::string dest;
        std::string texcoord;
        std::string texture;
        std::string sampler;
        GetSampleIndexable0(dest, texture, sampler, texcoord);

        std::string left = texture + ".Sample(" + sampler + ", " + texcoord + ")";
        std::string cmd = dest + " = " + FilterSat(left, sat) + ";\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_aoffimmi_indexable"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string offset;
        GetSampleOffsetIndexable0(dest, texture, sampler, texcoord, offset);

        std::string cmd = dest + " = " + texture + ".Sample(" + sampler + ", " + texcoord + ", " + offset + ");\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_b"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string lodBias;
        GetSampleIndexable1( dest, texture, sampler, texcoord, lodBias);

        std::string cmd = dest + " = " + texture + "SampleBias(" + sampler + "," + texcoord + ", " + lodBias + ");\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_b_aoffimmi_indexable"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string offset;
        std::string lodBias;
        GetSampleOffsetIndexable1( dest, texture, sampler, texcoord, offset, lodBias );

        std::string cmd = dest + " = " + texture + ".SampleBias(" + sampler + ", " + texcoord + ", " + lodBias + ", " + offset + ");\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_c_indexable"))
    {
        std::string dst;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string refValue;
        GetSampleIndexable1(dst, texture, sampler, texcoord, refValue);

        std::string cmd = dst + " = " + texture + ".SampleCmp(" + sampler + ", " + texcoord + ", " + refValue + ");\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_c_aoffimmi_indexable"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string offset;
        std::string refValue;
        GetSampleOffsetIndexable1(dest, texture, sampler, texcoord, offset, refValue);

        std::string cmd = dest + " = " + texture + ".SampleCmp(" + sampler + ", " + texcoord + ", " + refValue + ", " + offset + ");\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_c_lz_indexable"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string refValue;
        GetSampleIndexable1( dest, texture, sampler, texcoord, refValue );

        std::string cmd = dest + " = " + texture + ".SampleCmpLevelZero(" + sampler + ", " + texcoord + ", "  + refValue + ");\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_c_lz_aoffimmi_indexable"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string offset;
        std::string refValue;
        GetSampleOffsetIndexable1( dest, texture, sampler, texcoord, offset, refValue );

        std::string cmd = dest + " = " + texture + ".SampleCmpLevelZero(" + sampler + ", " + texcoord + ", " + refValue + ", " + offset + ");\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_d_indexable"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string xDerivative;
        std::string yDerivative;
        GetSample2( dest, texture, sampler, texcoord, xDerivative, yDerivative );

        std::string left = texture + ".SampleGrad(" + sampler + ", " + texcoord + ", " + xDerivative + ", " + yDerivative + ")";
        std::string cmd = dest + " = " + FilterSat( left, sat ) + ";\n";
        PushInstruction(cmd);
    }
    else if(m_Tokenizer.Compare("sample_d_aoffimmi_indexable"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string offset;
        std::string xDerivative;
        std::string yDerivative;
        GetSampleOffsetIndexable2(dest, texture, sampler, texcoord, offset, xDerivative, yDerivative);

        std::string left = texture + ".SampleGrad(" + sampler + ", " + texcoord + ", " + xDerivative + ", " + yDerivative + ", " + offset + ")";
        std::string cmd  = dest + " = " + FilterSat( left, sat ) + ";\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_l_indexable"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string lod;
        GetSampleIndexable1(dest, texture, sampler, texcoord, lod);

        std::string left = texture + ".SampleLevel(" + sampler + ", " + texcoord + ", " + lod + ")";
        std::string cmd  = dest + " = " + FilterSat( left, sat ) + ";\n";
        PushInstruction(cmd);
    }
    else if (m_Tokenizer.Compare("sample_l_aoffimmi_indexable"))
    {
        std::string dest;
        std::string texture;
        std::string sampler;
        std::string texcoord;
        std::string offset;
        std::string lod;
        GetSampleOffsetIndexable1( dest, texture, sampler, texcoord, offset, lod );

        std::string left = texture + ".SampleLevel(" + sampler + ", " + texcoord + ", " + lod + ", " + offset + ")";
        std::string cmd  = dest + " = " + FilterSat( left, sat ) + ";\n";
        PushInstruction(cmd);
    }
    else if (FindTag("store_raw"))
    {
        auto dstUAV = GetOperand();

        a3d::Reflection::ResourceInfo info;
        if (m_Reflection.QueryUav(dstUAV, &info))
        {
            dstUAV = info.ExpandName;
        }

        char pat[] = { 'x', 'y', 'z', 'w' };
        a3d::SwizzleInfo swz = {};
        swz.Count = info.DimValue;
        for(auto i=0; i<info.DimValue; ++i)
        {
            swz.Pattern[i] = pat[i];
            swz.Index[i] = i;
        }

        auto dstAddress = GetOperand(swz);
        auto src0       = GetOperand();

        std::string cmd = dstUAV + "[" + dstAddress + "]" + " = " + src0 + ";\n";
        PushInstruction(cmd);
    }
    else if (FindTag("store_structured"))
    {
        auto dstUAV = GetOperand();

        a3d::Reflection::ResourceInfo info;
        if (m_Reflection.QueryUav(dstUAV, &info))
        {
            dstUAV = info.ExpandName;
        }

        char pat[] = { 'x', 'y', 'z', 'w' };
        a3d::SwizzleInfo swz = {};
        swz.Count = info.DimValue;
        for(auto i=0; i<info.DimValue; ++i)
        {
            swz.Pattern[i] = pat[i];
            swz.Index[i] = i;
        }

        auto dstAddress = GetOperand(swz);
        auto src0       = GetOperand();

        std::string cmd = dstUAV + "[" + dstAddress + "]" + " = " + src0 + ";\n";
        PushInstruction(cmd);
    }
    else if (FindTag("store_uav_typed"))
    {
        std::string dstUAV;
        Get1(dstUAV);

        a3d::Reflection::ResourceInfo info;
        if (m_Reflection.QueryUav(dstUAV, &info))
        {
            dstUAV = info.ExpandName;
        }

        char pat[] = { 'x', 'y', 'z', 'w' };
        a3d::SwizzleInfo swz = {};
        swz.Count = info.DimValue;
        for(auto i=0; i<info.DimValue; ++i)
        {
            swz.Pattern[i] = pat[i];
            swz.Index[i] = i;
        }

        auto dstAddress = GetOperand(swz);
        auto src0       = GetOperand();

        std::string cmd = dstUAV + "[" + dstAddress + "]" + " = " + src0 + ";\n";
        PushInstruction(cmd);
    }
    else if (FindTag("swapc"))
    {
        std::string dst0;
        std::string dst1;
        std::string src0;
        std::string src1;
        std::string src2;
        Get5(dst0, dst1, src0, src1, src2);
        // TODO : Implement.
    }
    else if (FindTag("sync"))
    {
        if (m_Tokenizer.Compare("sync_uglobal"))
        {
            // TODO : Implement.
        }
        else if (m_Tokenizer.Compare("sync_uglobal_g"))
        {
            // TODO : Implement.
        }
        else if (m_Tokenizer.Compare("sync_uglobal_g_t"))
        {
            // TODO : Implement.
        }
        else if (m_Tokenizer.Compare("sync_uglobal_t"))
        {
            // TODO : Implement.
        }
        else if (m_Tokenizer.Compare("sync_ugroup"))
        {
            // TODO : Implement.
        }
        else if (m_Tokenizer.Compare("sync_ugroup_g"))
        {
            // TODO : Implement.
        }
        else if (m_Tokenizer.Compare("sync_ugroup_g_t"))
        {
            // TODO : Implement.
        }
        else if (m_Tokenizer.Compare("sync_ugroup_t"))
        {
            // TODO : Implement.
        }
        else if (m_Tokenizer.Compare("sync_g"))
        {
            // TODO : Implement.
        }
        else if (m_Tokenizer.Compare("sync_g_t"))
        {
            // TODO : Implement.
        }
        else if (m_Tokenizer.Compare("sync_t"))
        {
            // TODO : Implement.
        }
    }
    else if (FindTag("uaddc"))
    {
        std::string dst0;
        Get1(dst0);

        auto dst1 = GetOperand();
        auto src0 = GetOperand();
        auto src1 = GetOperand();

        std::string cmd = dst0 + " = " + src0 + " + " + src1 + ";\n";
        PushInstruction(cmd);
    }
    else if (FindTag("ubfe"))
    {
        std::string dst0;
        std::string src0;
        std::string src1;
        std::string src2;
        Get4(dst0, src0, src1, src2);
        // TODO : Implement.
    }
    else if (FindTag("ushr"))
    {
        PushOp2(">>", sat);
    }
    else if (FindTag("usubb"))
    {
        std::string dst0;
        Get1(dst0);

        auto dst1 = GetOperand();
        auto src0 = GetOperand();
        auto src1 = GetOperand();

        // TODO : 実装が怪しいので要確認.
        std::string cmd = dst0 + "= " + src0 + " - " + src1 + ";\n";
        PushInstruction(cmd);
    }
    else
    {
        return false;
    }

    return true;
}

//-------------------------------------------------------------------------------------------------
//      オペランドを取得します.
//-------------------------------------------------------------------------------------------------
std::string AsmParser::GetOperand()
{
    std::string temp;
    m_Tokenizer.Next();
    if (m_Tokenizer.Compare("l"))
    {
        temp += m_Tokenizer.NextAsChar(); // (
        temp += m_Tokenizer.NextAsChar(); // 数値X.

        m_Tokenizer.Next();
        if (!m_Tokenizer.Compare(")"))
        {
            temp += ", ";
            temp += m_Tokenizer.GetAsChar(); // 数値Y
            m_Tokenizer.Next();
        }
        else
        {
            temp += m_Tokenizer.GetAsChar();
            temp = StringHelper::Replace(temp, "(", "");
            temp = StringHelper::Replace(temp, ")", "");
            return temp;
        }

        if (!m_Tokenizer.Compare(")"))
        {
            temp += ", ";
            temp += m_Tokenizer.GetAsChar(); // 数値Z
            m_Tokenizer.Next();
        }
        else
        {
            temp += m_Tokenizer.GetAsChar();
            return std::string("float2") + temp;
        }

        if (!m_Tokenizer.Compare(")"))
        {
            temp += ", ";
            temp += m_Tokenizer.GetAsChar(); // 数値W
            m_Tokenizer.Next();
        }
        else
        {
            temp += m_Tokenizer.GetAsChar();
            return std::string("float3") + temp;
        }
    

        return std::string("float4") + temp + m_Tokenizer.GetAsChar();
    }

    temp = m_Tokenizer.GetAsChar();
    if (strstr(temp.c_str(), "[") != nullptr && strstr(temp.c_str(), "]" ) == nullptr)
    {
        std::string words;

        while(true)
        {
            words += m_Tokenizer.NextAsChar();
            if (strstr(words.c_str(), "]") != nullptr)
            { break; }
        }

        std::string line = temp + words;
        line = StringHelper::Replace(line, "[", "[asuint(");
        line = StringHelper::Replace(line, "]", ")]");
        temp = line;
    }

    std::string ret = temp;
    if (!m_Reflection.QueryName(temp, ret))
    {
        ret = temp;

        // 絶対値記号による修飾があるかどうかチェック.
        auto pos1 = temp.find("|");
        auto pos2 = temp.rfind("|");
        if (pos1 != std::string::npos
         && pos2 != std::string::npos
         && pos1 != pos2)
        { ret = "abs(" + temp.substr(pos1 + 1, pos2 - pos1 - 1) + ")"; }
    }
    else
    {
        // 絶対値記号による修飾があるかどうかチェック.
        auto pos1 = ret.find("|");
        auto pos2 = ret.rfind("|");
        if (pos1 != std::string::npos 
         && pos2 != std::string::npos 
         && pos1 != pos2 )
        { ret = "abs(" + ret + ")"; }
    }

    return ret;
}

//-------------------------------------------------------------------------------------------------
//      スウィズル数を考慮してオペランドを取得します.
//-------------------------------------------------------------------------------------------------
std::string AsmParser::GetOperand(const a3d::SwizzleInfo& info)
{
    auto op = GetOperand();
    return m_Reflection.GetCastedString(op, info);
}

//-------------------------------------------------------------------------------------------------
//      最大4つまでの引数を取得します.
//-------------------------------------------------------------------------------------------------
std::string AsmParser::GetArgs()
{
    m_Tokenizer.Next(); // (
    std::string a = m_Tokenizer.NextAsChar();
    std::string b, c, d;
    m_Tokenizer.Next();
    if (!m_Tokenizer.Compare(")"))
    {
        b = ", ";
        b += m_Tokenizer.GetAsChar();
        m_Tokenizer.Next();

        if (!m_Tokenizer.Compare(")"))
        {
            c = ", ";
            c += m_Tokenizer.GetAsChar();
            m_Tokenizer.Next();

            if (!m_Tokenizer.Compare(")"))
            {
                d = ", ";
                d += m_Tokenizer.GetAsChar();
            }
        }
    }

    return a + b + c + d;
}

//-------------------------------------------------------------------------------------------------
//      命令に使用するオペランドを１つ取得します.
//-------------------------------------------------------------------------------------------------
a3d::SwizzleInfo AsmParser::Get1(std::string& op0)
{
    std::string temp = m_Tokenizer.NextAsChar();
    auto info = a3d::Reflection::ToSwizzleInfo(temp);

    if(!m_Reflection.QueryName(temp, op0))
    { op0 = temp; }

    auto swz = StringHelper::GetSwizzle(op0);
    if (swz == ".xyzw")
    { op0 = StringHelper::GetWithSwizzle(op0, 0); }

    return info;
}

//-------------------------------------------------------------------------------------------------
//      命令に使用するオペランドを出力先のスウィズル数を考慮して2つ取得します.
//-------------------------------------------------------------------------------------------------
a3d::SwizzleInfo AsmParser::Get2(std::string& op0, std::string& op1)
{
    auto info = Get1(op0);
    op1 = GetOperand(info);
    return info;
}

//-------------------------------------------------------------------------------------------------
//      命令に使用するオペランドを出力先のスウィズル数を考慮して3つ取得します.
//-------------------------------------------------------------------------------------------------
a3d::SwizzleInfo AsmParser::Get3(std::string& op0, std::string& op1, std::string& op2)
{
    auto info = Get1(op0);
    op1 = GetOperand(info);
    op2 = GetOperand(info);
    return info;
}

//-------------------------------------------------------------------------------------------------
//      命令に使用するオペランドを出力先のスウィズル数を考慮して4つ取得します.
//-------------------------------------------------------------------------------------------------
a3d::SwizzleInfo AsmParser::Get4(std::string& op0, std::string& op1, std::string& op2, std::string& op3)
{
    auto info = Get1(op0);
    op1 = GetOperand(info);
    op2 = GetOperand(info);
    op3 = GetOperand(info);
    return info;
}

//-------------------------------------------------------------------------------------------------
//      命令に使用するオペランドを出力先のスウィズル数を考慮して5つ取得します.
//-------------------------------------------------------------------------------------------------
a3d::SwizzleInfo AsmParser::Get5(std::string& op0, std::string& op1, std::string& op2, std::string& op3, std::string& op4)
{
    auto info = Get1(op0);
    op1 = GetOperand(info);
    op2 = GetOperand(info);
    op3 = GetOperand(info);
    op4 = GetOperand(info);
    return info;
}

//-------------------------------------------------------------------------------------------------
//      命令に使用するオペランドを出力先のスウィズル数を考慮して6つ取得します.
//-------------------------------------------------------------------------------------------------
a3d::SwizzleInfo AsmParser::Get6(std::string& op0, std::string& op1, std::string& op2, std::string& op3, std::string& op4, std::string& op5)
{
    auto info = Get1(op0);
    op1 = GetOperand(info);
    op2 = GetOperand(info);
    op3 = GetOperand(info);
    op4 = GetOperand(info);
    op5 = GetOperand(info);
    return info;
}

//------------------------------------------------------------------------------------------------
//      サンプル命令のオペランドを取得します.
//------------------------------------------------------------------------------------------------
void AsmParser::GetSample0(std::string& dest, std::string& texture, std::string& sampler, std::string& texcoord)
{
    std::string dst;
    Get1(dst);
    std::string uv  = GetOperand();
    std::string tex = GetOperand();
    std::string smp = m_Tokenizer.NextAsChar();

    std::string texName;
    int cnt;
    {
        auto idx = tex.find(".");
        if (idx != -1)
        { tex = tex.substr(0, idx); }

        a3d::Reflection::ResourceInfo info = {};
        m_Reflection.QueryTexture(tex, &info);

        texName = info.Name;
        cnt = info.DimValue;
        if (info.ArraySize > 1)
        { texName += "[" + std::to_string(info.ArrayIndex) + "]"; }
    }

    a3d::SwizzleInfo swzInfo = {};
    swzInfo.Count = cnt;
    if (cnt >= 1)
    {
        swzInfo.Pattern[0] = 'x';
        swzInfo.Index[0] = 0;
    }
    if (cnt >= 2)
    {
        swzInfo.Pattern[1] = 'y';
        swzInfo.Index[1] = 1;
    }
    if (cnt >= 3)
    {
        swzInfo.Pattern[2] = 'z';
        swzInfo.Index[2] = 2;
    }

    dest    = dst;
    texture = texName;
    sampler = smp;
    texcoord = m_Reflection.GetCastedString(uv, swzInfo);
}

//--------------------------------------------------------------------------------------------------
//      サンプル命令のオペランドを取得します(オプション1個)
//--------------------------------------------------------------------------------------------------
void AsmParser::GetSample1(std::string& dest, std::string& texture, std::string& sampler, std::string& texcoord, std::string& arg1)
{
    GetSample0(dest, texture, sampler, texcoord);
    arg1 = GetOperand();
}

//-------------------------------------------------------------------------------------------------
//      サンプル命令のオペランドを取得します(オプション2個)
//-------------------------------------------------------------------------------------------------
void AsmParser::GetSample2
(
    std::string& dest,
    std::string& texture,
    std::string& sampler,
    std::string& texcoord,
    std::string& arg1,
    std::string& arg2
)
{
    GetSample0(dest, texture, sampler, texcoord);
    arg1 = GetOperand();
    arg2 = GetOperand();
}

//-------------------------------------------------------------------------------------------------
//      サンプルオフセット命令のオペランドを取得します.
//-------------------------------------------------------------------------------------------------
void AsmParser::GetSampleOffset0
(
    std::string& dest,
    std::string& texture,
    std::string& sampler,
    std::string& texcoord,
    std::string& sampleOffset
)
{
    std::string dst;
    std::string offset      = GetArgs();
    Get1(dst);
    std::string uv  = GetOperand();
    std::string tex = GetOperand();
    std::string smp = m_Tokenizer.NextAsChar();

    std::string texName;
    int cnt;
    {
        auto idx = tex.find(".");
        if (idx != -1)
        { tex = tex.substr(0, idx); }

        a3d::Reflection::ResourceInfo info = {};
        m_Reflection.QueryTexture(tex, &info);

        texName = info.Name;
        cnt = info.DimValue;
        if (info.ArraySize > 1)
        { texName += "[" + std::to_string(info.ArrayIndex) + "]"; }
    }

    a3d::SwizzleInfo swzInfo = {};
    swzInfo.Count = cnt;
    if (cnt >= 1)
    {
        swzInfo.Pattern[0] = 'x';
        swzInfo.Index[0] = 0;
    }
    if (cnt >= 2)
    {
        swzInfo.Pattern[1] = 'y';
        swzInfo.Index[1] = 1;
    }
    if (cnt >= 3)
    {
        swzInfo.Pattern[2] = 'z';
        swzInfo.Index[2] = 2;
    }

    offset = "float3(" + offset + ")";
    offset = m_Reflection.GetCastedString(offset, swzInfo);
    offset = StringHelper::Replace(offset, "float", "uint");

    dest         = dst;
    texture      = texName;
    sampler      = smp;
    texcoord     = m_Reflection.GetCastedString(uv, swzInfo);
    sampleOffset = offset;
}

//-------------------------------------------------------------------------------------------------
//      サンプルオフセット命令のオペランドを取得します(オプション1個).
//-------------------------------------------------------------------------------------------------
void AsmParser::GetSampleOffset1
(
    std::string& dest,
    std::string& texture,
    std::string& sampler,
    std::string& texcoord,
    std::string& sampleOffset,
    std::string& arg1
)
{
    GetSampleOffset0(dest, texture, texcoord, sampler, sampleOffset);
    arg1 = GetOperand();
}

//-------------------------------------------------------------------------------------------------
//      サンプルオフセット命令のオペランドを取得します(オプション2個)
//-------------------------------------------------------------------------------------------------
void AsmParser::GetSampleOffset2
(
    std::string& dest,
    std::string& texture,
    std::string& sampler,
    std::string& texcoord,
    std::string& sampleOffset,
    std::string& arg1,
    std::string& arg2
)
{
    GetSampleOffset0(dest, texture, sampler, texcoord, sampleOffset);
    arg1 = GetOperand();
    arg2 = GetOperand();
}

//------------------------------------------------------------------------------------------------
//      サンプル命令のオペランドを取得します.
//------------------------------------------------------------------------------------------------
void AsmParser::GetSampleIndexable0(std::string& dest, std::string& texture, std::string& sampler, std::string& texcoord)
{
    m_Tokenizer.Next(); // "("
    std::string type = m_Tokenizer.NextAsChar();
    m_Tokenizer.Next(); // ")"

    std::string args = GetArgs();
    m_Tokenizer.Next();

    std::string dst;
    Get1(dst);

    std::string uv  = GetOperand();
    std::string tex = GetOperand();
    std::string smp = GetOperand();

    std::string texName;
    int cnt;
    {
        auto idx = tex.find(".");
        if (idx != -1)
        { tex = tex.substr(0, idx); }

        a3d::Reflection::ResourceInfo info = {};
        m_Reflection.QueryTexture(tex, &info);

        texName = info.Name;
        cnt = info.DimValue;
        if (info.ArraySize > 1)
        { texName += "[" + std::to_string(info.ArrayIndex) + "]"; }
    }

    a3d::SwizzleInfo swzInfo = {};
    swzInfo.Count = cnt;
    if (cnt >= 1)
    {
        swzInfo.Pattern[0] = 'x';
        swzInfo.Index[0] = 0;
    }
    if (cnt >= 2)
    {
        swzInfo.Pattern[1] = 'y';
        swzInfo.Index[1] = 1;
    }
    if (cnt >= 3)
    {
        swzInfo.Pattern[2] = 'z';
        swzInfo.Index[2] = 2;
    }

    dest = dst;
    texture = texName;
    sampler = smp;
    texcoord = m_Reflection.GetCastedString(uv, swzInfo);
}

//------------------------------------------------------------------------------------------------
//      サンプル命令のオペランドを取得します.
//------------------------------------------------------------------------------------------------
void AsmParser::GetSampleIndexable1(std::string& dest, std::string& texture, std::string& sampler, std::string& texcoord, std::string& arg1)
{
    GetSampleIndexable0(dest, texture, sampler, texcoord);
    arg1 = GetOperand();
}

//------------------------------------------------------------------------------------------------
//      サンプル命令のオペランドを取得します.
//------------------------------------------------------------------------------------------------
void AsmParser::GetSampleIndexable2(std::string& dest, std::string& texture, std::string& sampler, std::string& texcoord, std::string& arg1, std::string& arg2)
{
    GetSampleIndexable0(dest, texture, sampler, texcoord);
    arg1 = GetOperand();
    arg2 = GetOperand();
}

//-------------------------------------------------------------------------------------------------
//      サンプルオフセット命令のオペランドを取得します.
//-------------------------------------------------------------------------------------------------
void AsmParser::GetSampleOffsetIndexable0(std::string& dest, std::string& texture, std::string& sampler, std::string& texcoord, std::string& sampleOffset)
{
    m_Tokenizer.Next(); // "("
    std::string type = m_Tokenizer.NextAsChar();
    m_Tokenizer.Next(); // ")"

    std::string args = GetArgs();
    m_Tokenizer.Next();


    std::string offset = GetArgs();

    std::string dst;
    Get1(dst);

    std::string uv  = GetOperand();
    std::string tex = GetOperand();
    std::string smp = GetOperand();

    std::string texName;
    int cnt;
    {
        auto idx = tex.find(".");
        if (idx != -1)
        { tex = tex.substr(0, idx); }

        a3d::Reflection::ResourceInfo info = {};
        m_Reflection.QueryTexture(tex, &info);

        texName = info.Name;
        cnt = info.DimValue;
        if (info.ArraySize > 1)
        { texName += "[" + std::to_string(info.ArrayIndex) + "]"; }
    }

    a3d::SwizzleInfo swzInfo = {};
    swzInfo.Count = cnt;
    if (cnt >= 1)
    {
        swzInfo.Pattern[0] = 'x';
        swzInfo.Index[0] = 0;
    }
    if (cnt >= 2)
    {
        swzInfo.Pattern[1] = 'y';
        swzInfo.Index[1] = 1;
    }
    if (cnt >= 3)
    {
        swzInfo.Pattern[2] = 'z';
        swzInfo.Index[2] = 2;
    }

    offset = "float3(" + offset + ")";
    offset = m_Reflection.GetCastedString(offset, swzInfo);
    offset = StringHelper::Replace(offset, "float", "uint");

    dest         = dst;
    texture      = texName;
    sampler      = smp;
    texcoord     = m_Reflection.GetCastedString(uv, swzInfo);
    sampleOffset = offset;
}

//-------------------------------------------------------------------------------------------------
//      サンプルオフセット命令のオペランドを取得します(オプション1個).
//-------------------------------------------------------------------------------------------------
void AsmParser::GetSampleOffsetIndexable1(std::string& dest, std::string& texture, std::string& sampler, std::string& texcoord, std::string& offset, std::string& arg1)
{
    GetSampleOffsetIndexable0(dest, texture, texcoord, sampler, offset);
    arg1 = GetOperand();
}

//-------------------------------------------------------------------------------------------------
//      サンプルオフセット命令のオペランドを取得します(オプション2個).
//-------------------------------------------------------------------------------------------------
void AsmParser::GetSampleOffsetIndexable2(std::string& dest, std::string& texture, std::string& sampler, std::string& texcoord, std::string& offset, std::string& arg1, std::string& arg2)
{
    GetSampleOffsetIndexable0(dest, texture, texcoord, sampler, offset);
    arg1 = GetOperand();
    arg2 = GetOperand();
}

//-------------------------------------------------------------------------------------------------
//      ロード命令のオペランドを取得します.
//-------------------------------------------------------------------------------------------------
void AsmParser::GetLoad(std::string& dest, std::string& texture, std::string& texcoord)
{
    std::string dst;
    Get1(dst);
    std::string srcAddress  = GetOperand();
    std::string srcResource = GetOperand();

    a3d::Reflection::ResourceInfo info = {};
    m_Reflection.QueryTexture(srcResource, &info);

    std::string name = info.Name;
    if (info.ArraySize > 1)
    { name += "[" + std::to_string(info.ArrayIndex) + "]"; }

    dest     = dst;
    texture  = name;
    texcoord = srcAddress;
}

//-------------------------------------------------------------------------------------------------
//      ロードオフセット命令のオペランドを取得します.
//-------------------------------------------------------------------------------------------------
void AsmParser::GetLoadOffset
(
    std::string& dest,
    std::string& texture,
    std::string& texcoord,
    std::string& loadOffset
)
{
    std::string dst;
    std::string offset      = GetArgs();
    Get1(dst);
    std::string srcAddress  = GetOperand();
    std::string srcResource = GetOperand();

    a3d::Reflection::ResourceInfo info = {};
    m_Reflection.QueryTexture(srcResource, &info);

    auto name = info.Name;
    if (info.ArraySize > 1)
    { name += "[" + std::to_string(info.ArrayIndex) + "]"; }

    dest        = dst;
    texture     = name;
    texcoord    = srcAddress;
    loadOffset  = offset;
}

//-------------------------------------------------------------------------------------------------
//      リソース情報取得命令のオペランドを取得します.
//-------------------------------------------------------------------------------------------------
void AsmParser::GetResInfo(std::string& dest, std::string& texture, std::string& mipLevel)
{
    std::string dst;
    Get1(dst);
    std::string srcMipLevel = GetOperand();
    std::string srcResource = GetOperand();

    std::string name;
    {
        auto textureName = StringHelper::GetWithSwizzle(srcResource, 0);
        a3d::Reflection::ResourceInfo info = {};
        m_Reflection.QueryTexture(textureName, &info);

        name = info.Name;
        if (info.ArraySize > 1)
        { name += "[" + std::to_string(info.ArrayIndex) + "]"; }
    }

    dest     = dst;
    texture  = name;
    mipLevel = srcMipLevel;
}

//-------------------------------------------------------------------------------------------------
//      内積命令を追加します.
//-------------------------------------------------------------------------------------------------
void AsmParser::PushDp(int count, bool sat)
{
    // ここはスカラーにベクトル関数の結果を代入するのでGet3を使わない.
    std::string dst, lhs, rhs;
    Get1(dst);

    a3d::SwizzleInfo info = {};
    info.Count = count;

    if (count >= 1)
    {
        info.Pattern[0] = 'x';
        info.Index[0] = 0;
    }

    if (count >= 2)
    {
        info.Pattern[1] = 'y';
        info.Index[1] = 1;
    }

    if (count >= 3)
    {
        info.Pattern[2] = 'z';
        info.Index[2] = 2;
    }

    if (count >= 4)
    {
        info.Pattern[3] = 'w';
        info.Index[3] = 3;
    }

    lhs = GetOperand(info);
    rhs = GetOperand(info);

    std::string left = "dot(" + lhs + ", " + rhs + ")";
    std::string cmd = dst + " = " + FilterSat( left, sat ) + ";\n";

    PushInstruction(cmd);
}

//-------------------------------------------------------------------------------------------------
//      2つ数をとる命令を追加します.
//-------------------------------------------------------------------------------------------------
void AsmParser::PushCmd2(std::string tag, bool sat)
{
    std::string dst, src;
    Get2(dst, src);

    std::string right = tag + "(" + src + ")";
    std::string cmd  = dst + " = " + FilterSat( right, sat ) + ";\n";

    PushInstruction(cmd);
}

//-------------------------------------------------------------------------------------------------
//      3つの数をとる命令を追加します.
//-------------------------------------------------------------------------------------------------
void AsmParser::PushCmd3(std::string tag, bool sat)
{
    std::string dst, lhs, rhs;
    Get3(dst, lhs, rhs);

    std::string right = tag + "(" + lhs + ", " + rhs  + ")";
    std::string cmd  = dst + " = " + FilterSat( right, sat ) + ";\n";

    PushInstruction(cmd);
}

//-------------------------------------------------------------------------------------------------
//      4つの数を命令を追加します.
//-------------------------------------------------------------------------------------------------
void AsmParser::PushCmd4(std::string tag, bool sat)
{
    std::string dst, op1, op2, op3;
    Get4(dst, op1, op2, op3);

    std::string right = tag + "(" + op1 + ", " + op2 + ", " + op3 + ")";
    std::string cmd   = dst + " = " + FilterSat( right, sat ) + ";\n";

    PushInstruction(cmd);
}

//-------------------------------------------------------------------------------------------------
//      2項演算を追加します.
//-------------------------------------------------------------------------------------------------
void AsmParser::PushOp2(std::string tag, bool sat)
{
    std::string dst, lhs, rhs;
    Get3(dst, lhs, rhs);

    // タグ補正.
    if (tag == "+")
    {
        if(rhs.find("-") != std::string::npos)
        {
            tag = "-";
            rhs = StringHelper::Replace(rhs, "-", "");
        }
    }

    std::string right = lhs + " " + tag + " " + rhs;
    std::string cmd   = dst + " = " + FilterSat( right, sat ) + ";\n";

    PushInstruction(cmd);
}

//-------------------------------------------------------------------------------------------------
//      3項演算を追加します.
//-------------------------------------------------------------------------------------------------
void AsmParser::PushOp3(std::string tag1, std::string tag2, bool sat)
{
    std::string dst, op1, op2, op3;
    Get4(dst, op1, op2, op3);

    // タグ補正.
    if (tag1 == "+")
    {
        if(op2.find("-") != std::string::npos)
        {
            tag1 = "-";
            op2 = StringHelper::Replace(op2, "-", "");
        }
    }

    // タグ補正.
    if (tag2 == "+")
    {
        if(op3.find("-") != std::string::npos)
        {
            tag2 = "-";
            op3 = StringHelper::Replace(op3, "-", "");
        }
    }

    std::string right = op1 + " " + tag1 + " " + op2 + " " + tag2 + " " + op3;
    std::string cmd   = dst + " = " + FilterSat( right, sat ) + ";\n";

    PushInstruction(cmd);
}

//-------------------------------------------------------------------------------------------------
//      比較命令を追加します.
//-------------------------------------------------------------------------------------------------
void AsmParser::PushCmp(std::string tag, bool integer)
{
    std::string dst, lhs, rhs;
    auto swzDst = Get1(dst);
    lhs = GetOperand();
    rhs = GetOperand();

    std::string one  = (integer) ? "1" : "1.0";
    std::string zero = (integer) ? "0" : "0.0";
 
    if (swzDst.Count == 1)
    {
        std::string cmd = dst + " = ( " + lhs + " " + tag + " " + rhs + " ) ? " + one + " : " + zero + ";\n"; 
        PushInstruction(cmd);
    }
    else
    {
        auto swzLhs = a3d::Reflection::ToSwizzleInfo(lhs);
        auto swzRhs = a3d::Reflection::ToSwizzleInfo(rhs);

        std::string baseDst = StringHelper::GetWithSwizzle(dst, 0);
        std::string baseLhs = StringHelper::GetWithSwizzle(lhs, 0);
        std::string baseRhs = StringHelper::GetWithSwizzle(rhs, 0);

        bool expandLhs = false;
        bool expandRhs = false;

        std::string leftX, leftY, leftZ, leftW;
        std::string rightX, rightY, rightZ, rightW;

        std::string dstX, dstY, dstZ, dstW;

        if (swzDst.Count >= 1)
        { dstX = baseDst + "." + swzDst.Pattern[0]; }

        if (swzDst.Count >= 2)
        { dstY = baseDst + "." + swzDst.Pattern[1]; }

        if (swzDst.Count >= 3)
        { dstZ = baseDst + "." + swzDst.Pattern[2]; }

        if (swzDst.Count >= 4)
        { dstW = baseDst + "." + swzDst.Pattern[3]; }

        // 要素を展開する.
        if (lhs.find("float") != std::string::npos)
        {
            auto temp = StringHelper::Replace(lhs, ", ", " ");
            temp = StringHelper::Replace(temp, "(", " ");
            temp = StringHelper::Replace(temp, ")", " ");
            std::stringstream stream(temp);
            std::string type;

            std::string args[4];
            stream >> type >> args[0] >> args[1] >> args[2] >> args[3];

            if (swzDst.Count >= 1)
            {
                auto idx = swzDst.Index[0];
                leftX = args[idx];
            }
            if (swzDst.Count >= 2)
            {
                auto idx = swzDst.Index[1];
                leftY = args[idx];
            }
            if (swzDst.Count >= 3)
            {
                auto idx = swzDst.Index[2];
                leftZ = args[idx];
            }
            if (swzDst.Count >= 4)
            {
                auto idx = swzDst.Index[3];
                leftW = args[idx];
            }
        }
        else
        {
            leftX = baseLhs;
            leftY = baseLhs;
            leftZ = baseLhs;
            leftW = baseLhs;

            if (swzDst.Count >= 1)
            {
                auto idx = swzDst.Index[0] % swzLhs.Count;
                leftX += StringHelper::Format( ".%c", swzLhs.Pattern[idx] );
            }
            
            if (swzDst.Count >= 2)
            {
                auto idx = swzDst.Index[1] % swzLhs.Count;
                leftY += StringHelper::Format( ".%c", swzLhs.Pattern[idx] );
            }

            if (swzDst.Count >= 3)
            {
                auto idx = swzDst.Index[2] % swzLhs.Count;
                leftZ += StringHelper::Format( ".%c", swzLhs.Pattern[idx] );
            }

            if (swzDst.Count >= 4)
            {
                auto idx = swzDst.Index[3] % swzLhs.Count;
                leftW += StringHelper::Format( ".%c", swzLhs.Pattern[idx] );
            }
        }

        if (rhs.find("float") != std::string::npos)
        {
            auto temp = StringHelper::Replace(rhs, ", ", " ");
            temp = StringHelper::Replace(temp, "(", " ");
            temp = StringHelper::Replace(temp, ")", " ");
            std::stringstream stream(temp);
            std::string type;

            std::string args[4];
            stream >> type >> args[0] >> args[1] >> args[2] >> args[3];


            if (swzDst.Count >= 1)
            {
                auto idx = swzDst.Index[0];
                rightX = args[idx];
            }
            if (swzDst.Count >= 2)
            {
                auto idx = swzDst.Index[1];
                rightY = args[idx];
            }
            if (swzDst.Count >= 3)
            {
                auto idx = swzDst.Index[2];
                rightZ = args[idx];
            }
            if (swzDst.Count >= 4)
            {
                auto idx = swzDst.Index[3];
                rightW = args[idx];
            }
        }
        else
        {
            rightX = baseRhs;
            rightY = baseRhs;
            rightZ = baseRhs;
            rightW = baseRhs;

            if (swzDst.Count >= 1)
            {
                auto idx = swzDst.Index[0] % swzRhs.Count;
                rightX += StringHelper::Format( ".%c", swzRhs.Pattern[idx] );
            }
            
            if (swzDst.Count >= 2)
            {
                auto idx = swzDst.Index[1] % swzRhs.Count;
                rightY += StringHelper::Format( ".%c", swzRhs.Pattern[idx] );
            }

            if (swzDst.Count >= 3)
            {
                auto idx = swzDst.Index[2] % swzRhs.Count;
                rightZ += StringHelper::Format( ".%c", swzRhs.Pattern[idx] );
            }

            if (swzDst.Count >= 4)
            {
                auto idx = swzDst.Index[3] % swzRhs.Count;
                rightW += StringHelper::Format( ".%c", swzRhs.Pattern[idx] );
            }
        }

        if (swzDst.Count >= 1)
        {
            std::string cmd = dstX + " = ( " + leftX + " " + tag + " " + rightX + " ) ? " + one + " : " + zero + ";\n";
            PushInstruction(cmd);
        }

        if (swzDst.Count >= 2)
        {
            std::string cmd = dstY + " = ( " + leftY + " " + tag + " " + rightY + " ) ? " + one + " : " + zero + ";\n";
            PushInstruction(cmd);
        }

        if (swzDst.Count >= 3)
        {
            std::string cmd = dstZ + " = ( " + leftZ + " " + tag + " " + rightZ + " ) ? " + one + " : " + zero + ";\n";
            PushInstruction(cmd);
        }

        if (swzDst.Count == 4)
        {
            std::string cmd = dstW + " = ( " + leftW + " " + tag + " " + rightW + " ) ? " + one + " : " + zero + ";\n";
            PushInstruction(cmd);
        }
    }
}

//-------------------------------------------------------------------------------------------------
//      ビット論理操作命令を追加します
//-------------------------------------------------------------------------------------------------
void AsmParser::PushLogicOp(std::string op)
{
    /* 論理和, 論理積系のみ シフト演算系は PushShiftOp() を使って下さい. */

    std::string dst, lhs, rhs;
    auto info = Get3(dst, lhs, rhs);

    if (info.Count != 1)
    {
        std::string cmd = "{\n";
        PushInstruction(cmd);
        m_Indent++;

        cmd = "uint" + std::to_string(info.Count) + " lhs_ = asuint(" + lhs + ");\n";
        PushInstruction(cmd);
        cmd = "uint" + std::to_string(info.Count) + " rhs_ = asuint(" + rhs + ");\n";
        PushInstruction(cmd);
        cmd = dst + " = asfloat(lhs_ " + op + " rhs_);\n";
        PushInstruction(cmd);

        m_Indent--;
        cmd = "}\n";
        PushInstruction(cmd);
    }
    else
    {
        std::string cmd = "{\n";
        PushInstruction(cmd);
        m_Indent++;

        if (!StringHelper::IsValue(lhs))
        {
            cmd = std::string("uint") + " lhs_ = asuint(" + lhs + ");\n";
            PushInstruction(cmd);
        }
        else
        {
            // 浮動小数を含まない場合.
            if (lhs.find(".") == std::string::npos)
            { cmd = std::string("uint lhs_ = ") + lhs + ";\n"; }
            else
            { cmd = std::string("uint lhs_ = asuint(") + lhs + ");\n"; }
            PushInstruction(cmd);
        }

        if (!StringHelper::IsValue(rhs))
        {
            cmd = std::string("uint") + " rhs_ = asuint(" + rhs + ");\n";
            PushInstruction(cmd);
        }
        else
        {
            // 浮動小数を含まない場合.
            if (rhs.find(".") == std::string::npos)
            { cmd = std::string("uint rhs_ = ") + rhs + ";\n"; }
            else
            { cmd = std::string("uint rhs_s = asuint(") + rhs + ");\n"; }
            PushInstruction(cmd);
        }

        cmd = "    " + dst + " = asfloat(lhs_ " + op + " rhs_);\n";
        PushInstruction(cmd);

        m_Indent--;
        cmd = "}\n";
        PushInstruction(cmd);

    }
}

//-------------------------------------------------------------------------------------------------
//      シフト演算を追加します.
//-------------------------------------------------------------------------------------------------
void AsmParser::PushShiftOp(std::string op)
{
    std::string dst, lhs, rhs;
    auto info = Get3(dst, lhs, rhs);

    std::string rhsString = rhs;
    if (StringHelper::IsVariable(rhs))
    {
        rhsString = "asuint(" + rhs + ")";
    }

    std::string cmd = dst + " = asuint(" + lhs + ") " + op + " " + rhsString +";\n";
    PushInstruction(cmd);
}

//-------------------------------------------------------------------------------------------------
//      float型から整数型への命令を追加します.
//-------------------------------------------------------------------------------------------------
void AsmParser::PushConvFromFloat(std::string tag, bool sat)
{
    std::string dst, src;
    Get2(dst, src);
    a3d::Literal info;
    std::string right;
    if (m_Reflection.IsLiteral(src, &info))
    {
        if (info.HasPoint)
        {
            std::string cmd = dst + " = " + FilterSat( src, sat ) + ";\n";
            PushInstruction(cmd);
        }
        else
        {
            std::string right = tag + "(" + src + ")";
            std::string cmd = dst + " = " + FilterSat( right, sat ) + ";\n";
            PushInstruction(cmd);
        }
    }
    else
    {
        std::string right = tag + "(" + src + ")";
        std::string cmd = dst + " = " + FilterSat( right, sat ) + ";\n";
        PushInstruction(cmd);
    }
}

//-------------------------------------------------------------------------------------------------
//      整数型からfloat型への命令を追加します.
//-------------------------------------------------------------------------------------------------
void AsmParser::PushConvToFloat(std::string tag, bool sat)
{
    std::string dst, src;
    Get2(dst, src);
    a3d::Literal info;
    std::string right;
    if (m_Reflection.IsLiteral(src, &info))
    {
        if (info.HasPoint)
        {
            std::string right = tag + "(" + src + ")";
            std::string cmd = dst + " = " + FilterSat( right, sat ) + ";\n";
            PushInstruction(cmd);
        }
        else
        {
            std::string cmd = dst + " = " + FilterSat( src, sat ) + ";\n";
            PushInstruction(cmd);
        }
    }
    else
    {
        auto l = StringHelper::Split(dst, ".");
        auto r = StringHelper::Split(src, ".");

        bool add = false;
        if (!l.empty() && !r.empty())
        {
            if (l[0] == r[0])
            {
                add = true;
                std::string cmd = dst + " = " + FilterSat( src, sat ) + ";\n";
                PushInstruction(cmd);
            }
        }

        if (!add)
        {
            std::string right = tag + "(" + src + ")";
            std::string cmd = dst + " = " + FilterSat( right, sat ) + ";\n";
            PushInstruction(cmd);
        }
    }
}

//-------------------------------------------------------------------------------------------------
//      代入命令を追加します.
//-------------------------------------------------------------------------------------------------
void AsmParser::PushMov(bool sat)
{
    std::string dst, src;
    Get2(dst, src);

    std::string cmd = dst + " = " + FilterSat( src, sat ) + ";\n";
    PushInstruction(cmd);
}

//-------------------------------------------------------------------------------------------------
//      比較代入命令を追加します.
//-------------------------------------------------------------------------------------------------
void AsmParser::PushMovc(bool sat)
{
    std::string dst, op0, op1, op2;
    auto swzDst = Get4(dst, op0, op1, op2);

    if (swzDst.Count == 1)
    {
        std::string cmd = dst + " = ( " + op0 + " >= 0 ) ? " + op1 + " : " + op2 + ";\n";
        PushInstruction(cmd);
    }
    else
    {
        auto swzOp0 = a3d::Reflection::ToSwizzleInfo(op0);
        auto swzOp1 = a3d::Reflection::ToSwizzleInfo(op1);
        auto swzOp2 = a3d::Reflection::ToSwizzleInfo(op2);

        auto baseDst = StringHelper::GetWithSwizzle(dst, 0);
        auto baseOp0 = StringHelper::GetWithSwizzle(op0, 0);
        auto baseOp1 = StringHelper::GetWithSwizzle(op1, 0);
        auto baseOp2 = StringHelper::GetWithSwizzle(op2, 0);

        auto pos0 = op0.find("float");
        auto pos1 = op1.find("float");
        auto pos2 = op2.find("float");

        std::string modOp0[4];
        std::string modOp1[4];
        std::string modOp2[4];

        if (pos0 != std::string::npos)
        {
            auto temp = StringHelper::Replace(op0, ", ", " ");
            temp = StringHelper::Replace(temp, "(", " ");
            temp = StringHelper::Replace(temp, ")", " ");
            std::stringstream stream(temp);
            std::string type;
                    
            std::string args[4];
            stream >> type >> args[0] >> args[1] >> args[2] >> args[3];

            for(auto i=0; i<swzDst.Count; ++i)
            { modOp0[i] = args[i]; }
        }
        else
        {
            for(auto i=0; i<swzDst.Count; ++i)
            {
                modOp0[i] = baseOp0 + StringHelper::Format(".%c", swzOp0.Pattern[i]);
            }
        }

        if (pos1 != std::string::npos)
        {
            auto temp = StringHelper::Replace(op1, ", ", " ");
            temp = StringHelper::Replace(temp, "(", " ");
            temp = StringHelper::Replace(temp, ")", " ");
            std::stringstream stream(temp);
            std::string type;
                    
            std::string args[4];
            stream >> type >> args[0] >> args[1] >> args[2] >> args[3];

            for(auto i=0; i<swzDst.Count; ++i)
            { modOp1[i] = args[i]; }
        }
        else
        {
            for(auto i=0; i<swzDst.Count; ++i)
            {
                modOp1[i] = baseOp1 + StringHelper::Format(".%c", swzOp1.Pattern[i]);
            }
        }

        if (pos2 != std::string::npos)
        {
            auto temp = StringHelper::Replace(op2, ", ", " ");
            temp = StringHelper::Replace(temp, "(", " ");
            temp = StringHelper::Replace(temp, ")", " ");
            std::stringstream stream(temp);
            std::string type;
                    
            std::string args[4];
            stream >> type >> args[0] >> args[1] >> args[2] >> args[3];

            for(auto i=0; i<swzDst.Count; ++i)
            { modOp2[i] = args[i]; }
        }
        else
        {
            for(auto i=0; i<swzDst.Count; ++i)
            {
                modOp2[i] = baseOp2 + StringHelper::Format(".%c", swzOp2.Pattern[i]);
            }
        }

        for(auto i=0; i<swzDst.Count; ++i)
        {
            std::string cmd = baseDst + "." + swzDst.Pattern[i] + " = ( " 
                                + modOp0[i] + " > 0 ) ? " 
                                + modOp1[i] + " : "
                                + modOp2[i] + ";\n";
            PushInstruction(cmd);
        }
    }
}

//-------------------------------------------------------------------------------------------------
//      saturate命令を付加します.
//-------------------------------------------------------------------------------------------------
std::string AsmParser::FilterSat(std::string value, bool sat)
{ 
    return (sat) ? "saturate(" + value + ")" : value;
}

//-------------------------------------------------------------------------------------------------
//      指定された部分文字列を含むかどうかチェックします.
//-------------------------------------------------------------------------------------------------
bool AsmParser::FindTag(std::string value)
{
    std::string token = m_Tokenizer.GetAsChar();
    auto pos = token.find(value);
    return (pos != std::string::npos) && (pos == 0);
}

//-------------------------------------------------------------------------------------------------
//      指定された部分文字列を含むかどうかチェックします.
//-------------------------------------------------------------------------------------------------
bool AsmParser::ContainTag(std::string value)
{
    std::string token = m_Tokenizer.GetAsChar();
    auto pos = token.find(value);
    return (pos != std::string::npos);
}

void AsmParser::PushInstruction(const std::string& cmd)
{
    std::string instruction;
    for(auto i=0; i<m_Indent; ++i)
    { instruction += "    "; }
    instruction += cmd;

    m_Instructions.push_back(instruction);
}

//-------------------------------------------------------------------------------------------------
//      HLSLコードを生成します
//-------------------------------------------------------------------------------------------------
void AsmParser::GenerateCode(std::string& sourceCode)
{
    sourceCode += "//-------------------------------------------------------------------------------------------------\n";
    sourceCode += "// <auto-generated>\n";
    sourceCode += "// Changes to this file may cause incorrect behavior and will be lost if the code is regenerated.\n";
    sourceCode += "// </auto-generated>\n"; 
    sourceCode += "//-------------------------------------------------------------------------------------------------\n";
    sourceCode += "\n\n";

    const std::string kShaderTag[] = {
        "VS",
        "PS",
        "GS",
        "DS",
        "HS",
        "CS"
    };

    // 入力データ書き込み.
    if (m_Reflection.HasInput())
    {
        sourceCode += "//-------------------------------------------------------------------------------------------------\n";
        sourceCode += "// Input Definitions.\n";
        sourceCode += "//-------------------------------------------------------------------------------------------------\n";

        sourceCode += StringHelper::Format("struct %sInput\n", kShaderTag[m_ShaderType].c_str());
        sourceCode += "{\n";
        const auto& code = m_Reflection.GetDefInputSignature();
        for( auto& itr : code )
        { sourceCode += StringHelper::Format("    %s", itr.c_str()); }
        sourceCode += "};\n";

        sourceCode += "\n\n";
    }

    // 出力データ書き込み.
    if (m_Reflection.HasOutput())
    {
        sourceCode += "//-------------------------------------------------------------------------------------------------\n";
        sourceCode += "// Output Definitions.\n";
        sourceCode += "//-------------------------------------------------------------------------------------------------\n";

        sourceCode += StringHelper::Format("struct %sOutput\n", kShaderTag[m_ShaderType].c_str());
        sourceCode += "{\n";
        const auto& code = m_Reflection.GetDefOutputSignature();
        for( auto& itr : code )
        { sourceCode += StringHelper::Format("    %s", itr.c_str()); }
        sourceCode += "};\n";

        sourceCode += "\n\n";
    }

    if (m_Reflection.HasStructure())
    {
        sourceCode += "//-------------------------------------------------------------------------------------------------\n";
        sourceCode += "// Structures.\n";
        sourceCode += "//-------------------------------------------------------------------------------------------------\n";

        const auto& code = m_Reflection.GetDefStructures();
        for( auto& itr : code )
        { sourceCode += StringHelper::Format("%s", itr.c_str()); }

        sourceCode += "\n\n";
    }

    // 定数バッファ書き込み.
    if (m_Reflection.HasBuffer())
    {
        sourceCode += "//-------------------------------------------------------------------------------------------------\n";
        sourceCode += "// Constant Buffers.\n";
        sourceCode += "//-------------------------------------------------------------------------------------------------\n";

        const auto& code = m_Reflection.GetDefConstantBuffer();
        for( auto& itr : code )
        { sourceCode += StringHelper::Format("%s", itr.c_str()); }

        sourceCode += "\n\n";
    }

    // テクスチャ書き込み.
    if (m_Reflection.HasTexture())
    {
        sourceCode += "//-------------------------------------------------------------------------------------------------\n";
        sourceCode += "// Textures.\n";
        sourceCode += "//-------------------------------------------------------------------------------------------------\n";

        const auto& code = m_Reflection.GetDefTextures();
        for( auto& itr : code )
        { sourceCode += StringHelper::Format("%s", itr.c_str()); }

        sourceCode += "\n\n";
    }

    // UAV書き込み.
    if (m_Reflection.HasUav())
    {
        sourceCode += "//-------------------------------------------------------------------------------------------------\n";
        sourceCode += "// Unordered Access Views.\n";
        sourceCode += "//-------------------------------------------------------------------------------------------------\n";

        const auto& code = m_Reflection.GetDefUavs();
        for( auto& itr : code )
        { sourceCode += StringHelper::Format("%s", itr.c_str()); }

        sourceCode += "\n\n";
    }

    // サンプラー書き込み.
    if (m_Reflection.HasSampler())
    {
        sourceCode += "//-------------------------------------------------------------------------------------------------\n";
        sourceCode += "// Samplers.\n";
        sourceCode += "//-------------------------------------------------------------------------------------------------\n";

        const auto& code = m_Reflection.GetDefSamplers();
        for( auto& itr : code )
        { sourceCode += StringHelper::Format("%s", itr.c_str()); }

        sourceCode += "\n\n";
    }

    // ラッパー関数定義.
    if (m_HasGetResourceInfo)
    {
        sourceCode += "//-------------------------------------------------------------------------------------------------\n";
        sourceCode += "// Wrapper Functions.\n";
        sourceCode += "//-------------------------------------------------------------------------------------------------\n";

        sourceCode += "float4 GetResourceInfo(Texture1D map, uint mipLevel)\n";
        sourceCode += "{\n";
        sourceCode += "    float width;\n";
        sourceCode += "    float mipCount;\n";
        sourceCode += "    map.GetDimensions(mipLevel, width, mipCount);\n";
        sourceCode += "    return float4(width, 0.0f, 0.0f, mipCount);\n";
        sourceCode += "}\n";

        sourceCode += "float4 GetResourceInfo(Texture1DArray map, uint mipLevel)\n";
        sourceCode += "{\n";
        sourceCode += "    float width;\n";
        sourceCode += "    uint  arraySize;\n";
        sourceCode += "    float mipCount;\n";
        sourceCode += "    map.GetDimensions(mipLevel, width, mipCount);\n";
        sourceCode += "    return float4(width, 0.0f, arraySize, mipCount);\n";
        sourceCode += "}\n";

        sourceCode += "float4 GetResourceInfo(Texture2D map, uint mipLevel)\n";
        sourceCode += "{\n";
        sourceCode += "    float width;\n";
        sourceCode += "    float height;\n";
        sourceCode += "    float mipCount;\n";
        sourceCode += "    map.GetDimensions(mipLevel, width, height, mipCount);\n";
        sourceCode += "    return float4(width, height, 0.0f, mipCount);\n";
        sourceCode += "}\n";

        sourceCode += "float4 GetResourceInfo(Texture2DArray map, uint mipLevel)\n";
        sourceCode += "{\n";
        sourceCode += "    float width;\n";
        sourceCode += "    float height;\n";
        sourceCode += "    uint  arraySize;\n";
        sourceCode += "    float mipCount;\n";
        sourceCode += "    map.GetDimensions(mipLevel, width, height, arraySize, mipCount);\n";
        sourceCode += "    return float4(width, height, arraySize, mipCount);\n";
        sourceCode += "}\n";

        sourceCode += "float4 GetResourceInfo(Texture3D map, uint mipLevel)\n";
        sourceCode += "{\n";
        sourceCode += "    float width;\n";
        sourceCode += "    float height;\n";
        sourceCode += "    float depth;\n";
        sourceCode += "    float mipCount;\n";
        sourceCode += "    map.GetDimensions(mipLevel, width, height, depth, mipCount);\n";
        sourceCode += "    return float4(width, height, depth, mipCount);\n";
        sourceCode += "}\n";

        sourceCode += "float4 GetResourceInfo(TextureCube map, uint mipLevel)\n";
        sourceCode += "{\n";
        sourceCode += "    float width;\n";
        sourceCode += "    float height;\n";
        sourceCode += "    float mipCount;\n";
        sourceCode += "    map.GetDimensions(mipLevel, width, height, mipCount);\n";
        sourceCode += "    return float4(width, height, 0.0f, mipCount);\n";
        sourceCode += "}\n";

        sourceCode += "float4 GetResourceInfo(TextureCubeArray map, uint mipLevel)\n";
        sourceCode += "{\n";
        sourceCode += "    float width;\n";
        sourceCode += "    float height;\n";
        sourceCode += "    float mipCount;\n";
        sourceCode += "    map.GetDimensions(mipLevel, width, height, mipCount);\n";
        sourceCode += "    return float4(width, height, 0.0f, mipCount);\n";
        sourceCode += "}\n";

        sourceCode += "\n\n";
    }

    // エントリーポイント書き込み.
    {
        std::string returnType = "void";
        if (m_ShaderType != SHADER_TYPE_COMPUTE)
        {
            returnType = StringHelper::Format("%sOutput", kShaderTag[m_ShaderType].c_str());
        }

        sourceCode += StringHelper::Format("%s %s(%sInput input", returnType.c_str(), m_Argument.EntryPoint.c_str(), kShaderTag[m_ShaderType].c_str());
        const auto& args = m_Reflection.GetDefInputArgs();
        if (!args.empty())
        {
            sourceCode += ",\n";
            for(size_t i=0; i<args.size(); ++i)
            {
                sourceCode += "    ";
                sourceCode += args[i];
                if (i != args.size() - 1)
                {
                    sourceCode += ",\n";
                }
            }
        }
        sourceCode += ")\n";

        sourceCode += "{\n";
        if (m_ShaderType != SHADER_TYPE_COMPUTE)
        {
            sourceCode += StringHelper::Format("    %sOutput output = (%sOutput)0;\n", kShaderTag[m_ShaderType].c_str(), kShaderTag[m_ShaderType].c_str());
        }

        for(size_t i=0; i<m_Instructions.size(); ++i)
        {
            sourceCode += "    ";   // タブ幅4
            sourceCode += m_Instructions[i];
        }

        if (m_ShaderType != SHADER_TYPE_COMPUTE)
        {
            sourceCode += "    return output;\n";
        }
        sourceCode += "}\n";
    }
}

//-------------------------------------------------------------------------------------------------
//      ソースコードをファイルに書き出します.
//-------------------------------------------------------------------------------------------------
bool AsmParser::WriteCode(const std::string& sourceCode)
{
    FILE* pFile;

    std::string filename = m_Argument.Output;
    {
        std::string ext[] = {
            "_vs.hlsl",
            "_ps.hlsl",
            "_gs.hlsl",
            "_ds.hlsl",
            "_hs.hlsl",
            "_cs.hlsl"
        };

        filename = m_Argument.Output + ext[m_ShaderType];
    }

    auto err = fopen_s(&pFile, filename.c_str(), "w");
    if (err != 0)
    { return false; }

    fwrite(sourceCode.data(), sourceCode.size(), 1, pFile);
    fclose(pFile);

    return true;
}

//-------------------------------------------------------------------------------------------------
//      HLSLアセンブリをHLSLコードに変換します.
//-------------------------------------------------------------------------------------------------
bool AsmParser::Convert(const Argument& args)
{
    bool attach = false;
    m_Argument = args;

    m_Indent = 0;

    if (m_Argument.Output.empty())
    {
        attach = true;
        m_Argument.Output = m_Argument.Input;
        auto pos = m_Argument.Input.rfind(".");
        if (pos != std::string::npos)
        { m_Argument.Output = m_Argument.Input.substr(0, pos); }
    }

    auto ret = Parse();
    if (!ret)
    {
        ELOG( "Error : File open failed. filename = %s", args.Input.c_str() );
        ELOG( "Error : Convert Failed.");
        return false; 
    }

    std::string sourceCode;
    GenerateCode(sourceCode);

    ret = WriteCode(sourceCode);
    if (!ret)
    {
        ELOG( "Error : Convert Failed." );
    }

    m_Instructions.clear();
    m_Tokenizer.Term();
    m_Reflection.Clear();

    if (!ret)
    {
        ELOG( "Error : Convert Failed." );
        return false;
    }

    return true;
}
