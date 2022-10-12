//-------------------------------------------------------------------------------------------------
// File : Tokenizer.h
// Desc : Tokenizer Module.
// Copyright(c) Project Asura All right reserved.
//-------------------------------------------------------------------------------------------------
#pragma once

//-------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------
#include <cstdint>
#include <string>


///////////////////////////////////////////////////////////////////////////////////////////////////
// Tokenizer class
///////////////////////////////////////////////////////////////////////////////////////////////////
class Tokenizer
{
    //=============================================================================================
    // list of friend classes
    //=============================================================================================
    /* NOTHING */

public:
    //=============================================================================================
    // public variables
    //=============================================================================================
    /* NOTHING */

    //=============================================================================================
    // public methods
    //=============================================================================================
    Tokenizer();
    virtual ~Tokenizer();

    bool        Init            ( uint32_t size );
    void        Term            ();
    void        SetSeparator    ( const char* separator );
    void        SetSeparator    ( const std::string& seperator );
    void        SetCutOff       ( const char* cutoff );
    void        SetCutOff       ( const std::string& cutoff );
    void        SetBuffer       ( char *buffer );
    bool        Compare         ( const char *token ) const;
    bool        CompareAsLower  ( const char *token ) const;
    bool        Compare         ( const std::string& token ) const;
    bool        CompareAsLower  ( const std::string& token ) const;
    bool        Contain         ( const char *token ) const;
    bool        Contain         ( const std::string& token ) const;
    bool        ContainAsLower  ( const char * token ) const;
    bool        ContainAsLower  ( const std::string& token ) const;
    bool        IsEnd           () const;
    char*       GetAsChar       () const;
    double      GetAsDouble     () const;
    float       GetAsFloat      () const;
    int         GetAsInt        () const;
    std::string GetAsString     () const;
    void        Next            ();
    char*       NextAsChar      ();
    double      NextAsDouble    ();
    float       NextAsFloat     ();
    int         NextAsInt       ();
    std::string NextAsString    ();
    char*       GetPtr          () const;
    char*       GetBuffer       () const;
    void        SkipTo          ( const char* text );
    void        SkipTo          ( const std::string& text );
    void        SkipLine        ();
    char*       GetLine         ();

private:
    //=============================================================================================
    // private variables
    //=============================================================================================
    char*           m_pBuffer;      //!< 先頭ポインタ.
    char*           m_pPtr;         //!< バッファ位置です.
    char*           m_pToken;       //!< トークン.
    std::string     m_Separator;    //!< 区切り文字.
    std::string     m_CutOff;       //!< 切り出し文字.
    size_t          m_BufferSize;

    //=============================================================================================
    // private methods
    //=============================================================================================
    Tokenizer       (const Tokenizer&) = delete;
    void operator = (const Tokenizer&) = delete;
};

