#pragma once

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <vector>

namespace fileseqmini
{

using StringVector = std::vector<std::string>;

class FileSequenceBase
{
public:
    FileSequenceBase( const std::string& pattern, const std::string& delimiters, char padChar );
    virtual ~FileSequenceBase() = default;

    virtual bool isOk() const = 0;
    virtual size_t size() const = 0;

    const std::string& getOriginalPattern() const { return m_originalPattern; }

protected:
    struct SequenceDesc
    {
        int32_t beg  = 0;
        int32_t end  = 0;
        int32_t step = 0;
        uint8_t pad  = 0;
        size_t size() const { return isOk() ? ( end - beg ) / step + 1 : 0; }
        bool isOk() const { return ( end - beg ) / step >= 0; }
    };

    using PackedSequence = std::vector<SequenceDesc>;
    using PackedPath = std::vector<PackedSequence>;

protected:
    StringVector splitOriginalPattern() const;
    StringVector unpackSequence( const PackedSequence& seq ) const;
    PackedPath parsePatterns( const StringVector& tokens ) const;
    bool checkPatternCharset( const std::string& pattern ) const;
    bool parsePatternStart( char** patternStr, SequenceDesc& desc ) const;
    bool parsePatternEnd( char** patternStr, SequenceDesc& desc ) const;
    bool parsePatternStep( char** patternStr, SequenceDesc& desc ) const;
    bool parsePatternPad( char** patternStr, SequenceDesc& desc ) const;

    template<typename T>
    bool parseInteger( char** str, T& val ) const;

    std::string writePadded( int32_t value, uint8_t pad ) const;

protected:
    std::string m_originalPattern;
    std::string m_delimiters;
    char m_padChar;
};

class FileSequence : public FileSequenceBase
{
public:
    using iterator = StringVector::const_iterator;

public:
    explicit FileSequence( const std::string& pattern, const std::string& delimiters = "", char padChar = '0' );

    virtual bool isOk() const override { return m_paths.size() > 1; }
    virtual size_t size() const override { return isOk() ? m_paths.size() : 0; }

    const StringVector& getFullPaths() const { return m_paths; }
    const std::string& operator[]( size_t index ) const;

    iterator begin() const { return m_paths.cbegin(); }
    iterator end() const { return m_paths.cend(); }

private:
    void generatePaths( const std::vector<StringVector>& pathParts );

private:
    StringVector m_paths;
    std::string m_invalidDesc;
};

class FileSequenceLazy : public FileSequenceBase
{
public:
    class iterator
    {
    public:
        iterator( const FileSequenceLazy& sequence, size_t i )
            : m_sequence( sequence )
            , m_pos( i )
        {}

        std::string operator*() const { return m_sequence[m_pos]; }
        iterator& operator++() { m_pos++; return *this; }
        iterator operator++( int ) { return iterator( m_sequence, m_pos + 1 ); }
        bool operator!=( const iterator& other ) const { return m_pos != other.m_pos; }
        bool operator==( const iterator& other ) const { return m_pos == other.m_pos; }

    private:
        const FileSequenceLazy& m_sequence;
        size_t m_pos;
    };

public:
    explicit FileSequenceLazy( const std::string& pattern, const std::string& delimiters = "", char padChar = '0' );

    virtual bool isOk() const override { return m_isOk; }
    virtual size_t size() const override { return m_size; }

    StringVector getFullPaths() const;
    std::string operator[]( size_t index ) const;

    iterator begin() const { return iterator( *this, 0 ); }
    iterator end() const { return iterator( *this, m_size ); }

private:
    std::string unpackSequence( const PackedSequence& seq, size_t branch ) const;
    size_t sequenceSize( const PackedSequence& seq ) const;

private:
    PackedPath m_packedPaths;
    StringVector m_pathParts;
    size_t m_size;
    bool m_isOk;
};

inline
FileSequence::FileSequence( const std::string& pattern, const std::string& delimiters, char padChar )
    : FileSequenceBase( pattern, delimiters, padChar )
{
    StringVector tokens = splitOriginalPattern();
    PackedPath parsedPatterns = parsePatterns( tokens );

    std::vector<StringVector> pathParts;
    pathParts.reserve( parsedPatterns.size() );
    for ( size_t i = 0; i < parsedPatterns.size(); ++i )
    {
        const PackedSequence& currentPattern = parsedPatterns[i];
        if ( currentPattern.empty() )
        {
            pathParts.emplace_back( 1, tokens[i] );
        }
        else
        {
            pathParts.emplace_back( unpackSequence( currentPattern ) );
        }
    }

    generatePaths( pathParts );
}

inline
void FileSequence::generatePaths( const std::vector<StringVector>& pathParts )
{
    size_t totalPaths = 1;
    for ( const auto& part : pathParts )
    {
        totalPaths *= part.size();
    }

    m_paths.resize( totalPaths );

    for ( size_t i = 0; i < totalPaths; ++i )
    {
        size_t branchId = i;
        size_t branchesLeft = totalPaths;

        for ( const auto& part : pathParts )
        {
            const size_t partId = ( branchId % branchesLeft ) / ( branchesLeft / part.size() );
            m_paths[i].append( part[partId] );
            branchId = branchId % branchesLeft;
            branchesLeft = branchesLeft / part.size();
        }
    }
}

inline
const std::string& FileSequence::operator[]( size_t index ) const
{
    if ( index < m_paths.size() )
    {
        return m_paths[index];
    }
    return m_invalidDesc;
}

inline
FileSequenceLazy::FileSequenceLazy( const std::string& pattern, const std::string& delimiters, char padChar )
    : FileSequenceBase( pattern, delimiters, padChar )
    , m_size( 0 )
    , m_isOk( false )
{
    m_pathParts = splitOriginalPattern();
    m_packedPaths = parsePatterns( m_pathParts );

    size_t pathCount = 1;
    for ( size_t i = 0; i < m_packedPaths.size(); ++i )
    {
        if ( !m_packedPaths[i].empty() )
        {
            pathCount *= sequenceSize( m_packedPaths[i] );
            m_isOk = pathCount > 1;
        }
    }
    if ( m_isOk )
    {
        m_size = pathCount;
    }
}

inline
StringVector FileSequenceLazy::getFullPaths() const
{
    StringVector result;
    result.reserve( m_size );
    for ( size_t i = 0; i < m_size; ++i )
    {
        result.push_back( ( *this )[i] );
    }
    return result;
}

inline
std::string FileSequenceLazy::operator[]( size_t index ) const
{
    std::string result;
    size_t branchId = index;
    size_t branchesLeft = m_size;

    for ( size_t i = 0; i < m_pathParts.size(); ++i )
    {
        if ( m_packedPaths[i].empty() )
        {
            result.append( m_pathParts[i] );
        }
        else
        {
            const size_t seqSize = sequenceSize( m_packedPaths[i] );
            const size_t branch = ( branchId % branchesLeft ) / ( branchesLeft / seqSize );
            result.append( unpackSequence( m_packedPaths[i], branch ) );
            branchId = branchId % branchesLeft;
            branchesLeft = branchesLeft / seqSize;
        }
    }

    return result;
}

inline
std::string FileSequenceLazy::unpackSequence( const PackedSequence& seq, size_t branch ) const
{
    for ( const auto& slice : seq )
    {
        const size_t subBranchSize = slice.size();

        if ( subBranchSize > branch )
        {
            int32_t id = static_cast<int32_t>( slice.beg + ( slice.step * branch ) );
            return writePadded( id, slice.pad );
        }
        else
        {
            branch -= subBranchSize;
        }
    }

    return {};
}

inline
size_t FileSequenceLazy::sequenceSize( const PackedSequence& seq ) const
{
    size_t result = 0;

    for ( const auto& slice : seq )
    {
        result += slice.size();
    }

    return result;
}

inline
FileSequenceBase::FileSequenceBase( const std::string& pattern, const std::string& delimiters, char padChar )
    : m_originalPattern( pattern )
    , m_delimiters( delimiters )
    , m_padChar( padChar )
{
    if ( m_delimiters.empty() )
    {
        m_delimiters = "\\/.";
    }
}

inline
StringVector FileSequenceBase::splitOriginalPattern() const
{
    StringVector result;

    size_t startToken = 0;
    for ( size_t i = 0; i < m_originalPattern.length(); ++i )
    {
        if ( m_delimiters.find( m_originalPattern[i] ) != std::string::npos )
        {
            if ( startToken < i )
            {
                result.emplace_back( m_originalPattern, startToken, i - startToken );
            }
            result.emplace_back( m_originalPattern, i, 1 );

            startToken = i + 1;
        }
    }

    if ( startToken < m_originalPattern.length() )
    {
        result.emplace_back( m_originalPattern, startToken, std::string::npos );
    }

    return result;
}

inline
FileSequenceBase::PackedPath FileSequenceBase::parsePatterns( const StringVector& tokens ) const
{
    PackedPath result( tokens.size() );

    for ( size_t i = 0; i < tokens.size(); ++i )
    {
        if ( checkPatternCharset( tokens[i] ) )
        {
            std::stringstream ss( tokens[i] );
            std::string buf;

            PackedSequence sliceSequence;
            bool success = true;
            while ( success && std::getline( ss, buf, ',' ) )
            {
                if ( buf.empty() )
                {
                    success = false;
                    break;
                }

                sliceSequence.push_back( { 0, 0, 1, 0 } );
                SequenceDesc& desc = sliceSequence.back();

                char* patternStr = &buf[0];
                success = parsePatternStart( &patternStr, desc )
                       && parsePatternEnd( &patternStr, desc )
                       && parsePatternStep( &patternStr, desc )
                       && parsePatternPad( &patternStr, desc )
                       && desc.isOk();
            }

            if ( success )
            {
                result[i].swap( sliceSequence );
            }
        }
    }

    return result;
}

inline
bool FileSequenceBase::checkPatternCharset( const std::string& pattern ) const
{
    for ( auto c : pattern )
    {
        switch ( c )
        {
        case '#':
        case ',':
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '@':
        case 'x':
            continue;
        
        default:
            return false;
        }
    }
    return true;
}

inline
bool FileSequenceBase::parsePatternStart( char** patternStr, SequenceDesc& desc ) const
{
    if ( *patternStr[0] == '#' || *patternStr[0] == '@' )
    {
        if ( parsePatternPad( patternStr, desc ) )
        {
            desc.end = static_cast<int32_t>( std::pow( 10, desc.pad ) - 1 );
            return true;
        }
    }

    if ( !parseInteger( patternStr, desc.beg ) )
    {
        return false;
    }

    desc.end = desc.beg;

    return true;
}

inline
bool FileSequenceBase::parsePatternEnd( char** patternStr, SequenceDesc& desc ) const
{
    if ( *patternStr == nullptr || *patternStr[0] == 0 )
    {
        return true;
    }

    if ( *patternStr[0] == '-' )
    {
        (*patternStr)++;
        if ( !parseInteger( patternStr, desc.end ) )
        {
            return false;
        }
    }
    else if ( *patternStr[0] != '#' && *patternStr[0] != '@' )
    {
        return false;
    }

    if ( desc.beg > desc.end )
    {
        desc.step = -1;
    }

    return true;
}

inline
bool FileSequenceBase::parsePatternStep( char** patternStr, SequenceDesc& desc ) const
{
    if ( *patternStr == nullptr || *patternStr[0] == 0 )
    {
        return true;
    }

    if ( *patternStr[0] == 'x' )
    {
        (*patternStr)++;
        if ( !parseInteger( patternStr, desc.step ) || desc.step == 0 )
        {
            return false;
        }
    }
    else if ( *patternStr[0] != '#' && *patternStr[0] != '@' )
    {
        return false;
    }

    return true;
}

inline
bool FileSequenceBase::parsePatternPad( char** patternStr, SequenceDesc& desc ) const
{
    if ( *patternStr == nullptr || *patternStr[0] == 0 )
    {
        return true;
    }

    while ( *patternStr[0] )
    {
        if ( *patternStr[0] == '@' )
        {
            desc.pad += 1;
        }
        else if ( *patternStr[0] == '#' )
        {
            desc.pad += 4;
        }
        else
        {
            return false;
        }
        (*patternStr)++;
    }

    return true;
}

template<typename T>
bool FileSequenceBase::parseInteger( char** str, T& val ) const
{
    char* parseEnd = *str;
    val = static_cast<T>( std::strtol( *str, &parseEnd, 10 ) );

    if ( parseEnd == *str )
    {
        return false;
    }

    *str = parseEnd;

    return true;
}

inline
std::string FileSequenceBase::writePadded( int32_t value, uint8_t pad ) const
{
    std::stringstream ss;
    ss << std::setw( pad ) << std::setfill( m_padChar ) << std::internal << value;
    return ss.str();
}

inline
StringVector FileSequenceBase::unpackSequence( const PackedSequence& seq ) const
{
    StringVector result;

    for ( const auto& slice : seq )
    {
        auto inBounds = [&slice]( int32_t i )
        {
            return ( slice.step < 0 ) ? ( i >= slice.end ) : ( i <= slice.end );
        };
        
        for ( int32_t i = slice.beg; inBounds( i ); i += slice.step )
        {
            result.emplace_back( writePadded( i, slice.pad ) );
        }
    }

    return result;
}

} // namespace fileseqlite
