#include <cstdlib>
#include <iostream>

#include <fileseqmini.hpp>

#ifdef FILESEQ_LAZY
    using Sequence = fileseqmini::FileSequenceLazy;
#else
    using Sequence = fileseqmini::FileSequence;
#endif

int main( int argc, char* argv[] )
{
    for ( int i = 1; i < argc; ++i )
    {
        Sequence seq( argv[i] );
        
        if ( !seq.isOk() )
        {
            std::cerr << "Cannot parse " << argv[i] << std::endl;
            return EXIT_FAILURE;
        }

        for ( const auto& path : seq )
        {
            std::cout << path << std::endl;
        }
    }

    return EXIT_SUCCESS;
}
