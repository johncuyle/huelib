#include "stdafx.h"

#include "Hue/Portal.hpp"
#include "Hue/Lights.hpp"
#include "Hue/Groups.hpp"
#include "Hue/Schedules.hpp"
#include "Hue/Scenes.hpp"
#include "Hue/Sensors.hpp"
#include "Hue/Rules.hpp"
#include "Hue/Configuration.hpp"

namespace Hue
{
ppl::task< std::vector< Bridge > > Bridge::DiscoverBridgesAsync( whc::web_proxy const & proxy )
{
	wh::uri const discover_uri =
		[]( )
	{
		wh::uri_builder builder;
		builder.set_scheme( U( "http" ) ).
			set_host( U( "www.meethue.com" ) ).
			set_path( U( "api/nupnp" ) );

		return builder.to_uri( );
	}( );

	whc::http_client_config const config =
		[&proxy]( )
	{
		whc::http_client_config config;
		config.set_proxy( proxy );

		return config;
	}( );
	whc::http_client client( discover_uri, config );

	// Make the request and asynchronously process the response. 
	return client.request( wh::methods::GET ).then(
		&GetValidatedJson ).then(
		[proxy]( wj::value localBridges )
	{
		std::vector< Bridge > bridges;

		if( localBridges.is_array( ) && localBridges.size( ) != 0 )
		{
			bridges.reserve( localBridges.size( ) );
			for( auto const & bridge : localBridges.as_array( ) )
			{
				bridges.emplace_back(
					bridge.has_field( U("id") ) ? bridge.at( U( "id" ) ).as_string( ) : L"",
					bridge.has_field( U( "internalipaddress" ) ) ? bridge.at( U( "internalipaddress" ) ).as_string( ) : L"",
					bridge.has_field( U( "macaddress" ) ) ? bridge.at( U( "macaddress" ) ).as_string( ) : L"" );
			}
		}

		return bridges;
	} );
}

ppl::task< Configuration > Bridge::GetConfigurationAsync( whc::web_proxy const & proxy, User const & user ) const
{
	Configuration configuration( MakeClient( user, L"config", proxy ) );

	return configuration.GetAsync( ).then(
		[configuration]( ) -> Configuration
	{
		return configuration;
	} );
}

ppl::task< Lights > Bridge::GetLightsAsync( whc::web_proxy const & proxy, User const & user ) const
{
	whc::http_client client = MakeClient( user, L"lights", proxy );

	// Make the request and asynchronously process the response. 
	return client.request( wh::methods::GET ).then(
		&GetValidatedJson ).then(
		[this, client]( wj::value result )
	{
		return Lights( client, result );
	} );
}

ppl::task< Lights > Bridge::GetNewLightsAsync( whc::web_proxy const & proxy, User const & user ) const
{
	whc::http_client client = MakeClient( user, L"lights", proxy );

	// Make the request and asynchronously process the response. 
	return client.request( wh::methods::GET, U( "new" ) ).then(
		&GetValidatedJson ).then(
		[this, client]( wj::value result )
	{
		return Lights( client, result );
	} );
}

ppl::task< Groups > Bridge::GetGroupsAsync( whc::web_proxy const & proxy, User const & user ) const
{
	whc::http_client client = MakeClient( user, L"groups", proxy );

	// Make the request and asynchronously process the response. 
	return client.request( wh::methods::GET ).then(
		&GetValidatedJson ).then(
		[this, client]( wj::value result )
	{
		return Groups( client, result );
	} );
}

ppl::task< Schedules > Bridge::GetSchedulesAsync( whc::web_proxy const & proxy, User const & user ) const
{
	whc::http_client client = MakeClient( user, L"schedules", proxy );

	// Make the request and asynchronously process the response. 
	return client.request( wh::methods::GET ).then(
		&GetValidatedJson ).then(
		[this, client]( wj::value result )
	{
		return Schedules( client, result );
	} );
}

ppl::task< Scenes > Bridge::GetScenesAsync( whc::web_proxy const & proxy, User const & user ) const
{
	whc::http_client client = MakeClient( user, L"scenes", proxy );

	// Make the request and asynchronously process the response. 
	return client.request( wh::methods::GET ).then(
		&GetValidatedJson ).then(
		[this, client]( wj::value result )
	{
		return Scenes( client, result );
	} );
}

ppl::task< Sensors > Bridge::GetSensorsAsync( whc::web_proxy const & proxy, User const & user ) const
{
	whc::http_client client = MakeClient( user, L"sensors", proxy );

	// Make the request and asynchronously process the response. 
	return client.request( wh::methods::GET ).then(
		&GetValidatedJson ).then(
		[this, client]( wj::value result )
	{
		return Sensors( client, result );
	} );
}

ppl::task< Sensors > Bridge::GetNewSensorsAsync( whc::web_proxy const & proxy, User const & user ) const
{
	whc::http_client client = MakeClient( user, L"sensors", proxy );

	// Make the request and asynchronously process the response. 
	return client.request( wh::methods::GET, U( "new" ) ).then(
		&GetValidatedJson ).then(
		[this, client]( wj::value result )
	{
		return Sensors( client, result );
	} );
}

ppl::task< Rules > Bridge::GetRulesAsync( whc::web_proxy const & proxy, User const & user ) const
{
	whc::http_client client = MakeClient( user, L"rules", proxy );

	// Make the request and asynchronously process the response. 
	return client.request( wh::methods::GET ).then(
		&GetValidatedJson ).then(
		[this, client]( wj::value result )
	{
		return Rules( client, result );
	} );
}

ppl::task < State::Tuple > Bridge::GetStateAsync( whc::web_proxy const & proxy, User const & user ) const
{
	wh::uri const uri =
		[&]( )
	{
		wu::ostringstream_t path;
		{
			path << U( "api/" );
			path << wu::conversions::to_string_t( user.UserName_ );
		}

		wh::uri_builder builder;
		builder.set_scheme( U( "http" ) ).
			set_host( BridgeInfo_->Ip_ ).
			set_path( path.str( ) );

		return builder.to_uri( );
	}( );

	whc::http_client_config const config =
		[&]( )
	{
		whc::http_client_config config;
		config.set_proxy( proxy );

		return config;
	}( );

	whc::http_client client( uri, config );

	auto bridge = BridgeInfo_;

	// Make the request and asynchronously process the response. 
	return client.request( wh::methods::GET ).then(
		&GetValidatedJson ).then(
		[this, bridge, user, proxy]( wj::value result ) -> State::Tuple
	{
		return std::make_tuple(
			Configuration( MakeClient( user, L"config", proxy ), result.at( U( "config" ) ) )
			, Lights( MakeClient( user, L"lights", proxy ), result.at( U( "lights" ) ) )
			, Groups( MakeClient( user, L"groups", proxy ), result.at( U( "groups" ) ) )
			, Schedules( MakeClient( user, L"schedules", proxy ), result.at( U( "schedules" ) ) )
			, Scenes( MakeClient( user, L"scenes", proxy ), ( result.has_field( U( "scenes" ) ) ? result.at( U( "scenes" ) ) : wj::value::parse( U( "{}" ) ) ) )
			, Sensors( MakeClient( user, L"sensors", proxy ), ( result.has_field( U( "sensors" ) ) ? result.at( U( "sensors" ) ) : wj::value::parse( U( "{}" ) ) ) )
			, Rules( MakeClient( user, L"rules", proxy ), ( result.has_field( U( "rules" ) ) ? result.at( U( "rules" ) ) : wj::value::parse( U( "{}" ) ) ) )
			);
	} );
}

ppl::task< std::vector< std::wstring > > Bridge::GetTimeZonesAsync( whc::web_proxy const & proxy, User const & user ) const
{
	whc::http_client client = MakeClient( user, L"info/timezones", proxy );

	// Make the request and asynchronously process the response. 
	return client.request( wh::methods::GET ).then(
		&GetValidatedJson ).then(
		[this, client]( wj::value result )
	{
		std::vector< std::wstring > timezones;

		if( result.is_array( ) && result.size( ) != 0 )
		{
			timezones.reserve( result.size( ) );
			for( auto const & timezone : result.as_array( ) )
			{
				timezones.emplace_back( timezone.as_string( ) );
			}
		}

		return timezones;
	} );
}

whc::http_client Bridge::MakeClient( User const & user, wchar_t const * const subpath, whc::web_proxy const & proxy ) const
{
	return whc::http_client( [&]( )
	{
		wu::ostringstream_t path;
		{
			path << U( "api/" );
			path << wu::conversions::to_string_t( user.UserName_ );
			path << U( "/" );
			path << subpath;
		}

		wh::uri_builder builder;
		builder.set_scheme( U( "http" ) ).
			set_host( BridgeInfo_->Ip_ ).
			set_path( path.str( ) );

		return builder.to_uri( );
	}( ), 
		[&]( )
	{
		whc::http_client_config config;
		config.set_proxy( proxy );

		return config;
	}( )
		);
}


}