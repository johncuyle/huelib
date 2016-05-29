#include "stdafx.h"
#include "Hue\Shared.hpp"

namespace Hue
{

ppl::task< wj::value > GetValidatedJson( wh::http_response response )
{
	if( response.status_code( ) != wh::status_codes::OK )
	{
		throw wh::http_exception( response.status_code( ) );
	}

	// TODO: Perform actions here reading from the response stream.
	auto bodyStream = response.body( );

	return response.content_ready( ).then(
		[]( wh::http_response response )
	{
		return response.extract_json( );
	} ).then(
		[bodyStream]( ppl::task< wj::value > result )
	{
		try
		{
			auto body = result.get( );

			// Errors are always returned as arrays of errors.
			if( !body.is_array( ) )
				return body;

			// Error indicates problem.  Often username length out of bounds
			for( auto const & res : body.as_array( ) )
			{
				// Need to parse error message to return more useful error
				if( res.has_field( U( "error" ) ) )
				{
					std::wstring_convert< std::codecvt_utf8_utf16< wchar_t > > converter;
					auto error = res.serialize( );
					throw std::runtime_error( converter.to_bytes( error.c_str( ) ) );
				}
			}

			return body;
		}
		catch( wj::json_exception const & )
		{
			throw; // Should use bodyStream to output non-parsable json
		}
	} );
}

} // } namespace Hue