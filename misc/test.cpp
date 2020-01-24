#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <fileseqmini.hpp>

struct TestCase
{
	std::string name;
	std::string input;
	std::vector<std::string> expect;
};

std::vector<TestCase> parseTests()
{
	std::vector<TestCase> result;

	std::ifstream f( "test.suite" );
	if ( f )
	{
		std::string line;
		TestCase test;
		while ( std::getline( f, line ) )
		{
			if ( line[0] == '+' )
			{
				if ( !test.name.empty() )
				{
					result.push_back( test );
				}

				test.name = line;
				std::getline( f, test.input );
				test.expect.clear();
			}
			else if ( !line.empty() )
			{
				test.expect.push_back( line );
			}
		}

		if ( !test.name.empty() )
		{
			result.push_back( test );
		}
	}

	return result;
}

template<class Sequence>
void printSequence( const Sequence& seq )
{
	for ( const auto& path : seq )
	{
		std::cout << path << std:: endl;
	}
}

template<class Sequence>
bool testSequence( const Sequence& seq, const TestCase& test, const std::string& name )
{
	bool success = seq.isOk() || test.expect.empty();

	if ( success )
	{
		success = test.expect.size() == seq.size();
		if ( success )
		{
			for ( size_t i = 0; i < seq.size(); ++i )
			{
				if ( seq[i] != test.expect[i] )
				{
					success = false;
					std::cout << "ERROR: (" << name << ") path " << i << " is different from expected" << std::endl;
					std::cout << "== FileSequence:" << seq[i] << std::endl;
					std::cout << "== Expected:" << test.expect[i] << std::endl;
				}
			}
		}
		else
		{
			std::cout << "ERROR: (" << name << ") is different from expected result" << std::endl;
			std::cout << "== FileSequence:" << std::endl;
			printSequence( seq );
			std::cout << "== Expected:" << std::endl;
			printSequence( test.expect );
		}
	}
	else
	{
		std::cout << "ERROR: (" << name << ") cannot parse " << test.input << std::endl;
	}

	return success;
}

int main()
{
	const auto& tests = parseTests();
	size_t passed = 0;

	for ( const auto& test : tests )
	{
		std::cout << ">> CASE: " << test.name << std::endl;
		std::cout << "Input: " << test.input << std::endl;

		fileseqmini::FileSequence seqNormal( test.input );
		fileseqmini::FileSequenceLazy seqLazy( test.input );

		bool successNormal = testSequence( seqNormal, test, "normal" );
		bool successLazy = testSequence( seqLazy, test, "lazy" );

		std::cout << ( ( successLazy && successNormal ) ? "OK" : "!!! >>> FAILED <<< !!!" ) << std::endl;
		std::cout << "==========================================" << std::endl << std::endl;
		
		passed += ( successLazy && successNormal );
	}

	std::cout << "Done. Tests passed: " << passed << "/" << tests.size() << std::endl;

	return passed != tests.size();
}
