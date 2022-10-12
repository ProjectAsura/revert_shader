//-------------------------------------------------------------------------------------------------
// File : main.cpp
// Desc : Application Main Entry Point.
// Copyright(c) Project Asura. All right reserved.
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------
#include "AsmParser.h"
#include "StringHelper.h"


//-------------------------------------------------------------------------------------------------
//      コマンドライン引数を解析します.
//-------------------------------------------------------------------------------------------------
void ParseArg(int argc, char** argv, AsmParser::Argument& result)
{
    result.Input = argv[1];
    for(auto i=2; i<argc; ++i)
    {
        if (_stricmp(argv[i], "-o") == 0)
        {
            i++;
            result.Output = argv[i];
        }
        else if (_stricmp(argv[i], "-e") == 0)
        {
            i++;
            result.EntryPoint = argv[i];
        }
    }
}

//-------------------------------------------------------------------------------------------------
//      メインエントリーポイントです.
//-------------------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    if (argc <= 1)
    {
        printf_s("revert_mesh.exe inputfile [option]\n");
        printf_s("[option]\n");
        printf_s("    -o outputfile\n");
        printf_s("    -e entrypoint\n");
        printf_s("    (ex) revert_mesh.exe test.asm -o test.hlsl -e main\n");
        return 0;
    }

    AsmParser::Argument argument = {};
    argument.EntryPoint = "main";

    ParseArg(argc, argv, argument);

    AsmParser parser;
    if (!parser.Convert(argument))
    {
        fprintf_s(stderr, "Error : Convert Failed. filename = %s\n", argument.Input.c_str());
        return -1;
    }
    else
    {
        fprintf_s(stdout, "Info : Convert Success.");
    }

    return 0;
}