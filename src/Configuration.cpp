#include "stdafx.h"
#include "Hue\Configuration.hpp"

namespace Hue
{

void MutableState::SwUpdate::Deserialize( wj::value const & config )
{
	State_ = static_cast< State::Enum >( config.at( U( "updatestate" ) ).as_integer( ) ); 
	Url_ = config.at( U( "url" ) ).as_string( );
	Text_ = config.at( U( "text" ) ).as_string( );
	Notify_ = config.at( U( "notify" ) ).as_bool( );
}

void ReadOnlyState::Deserialize( wj::value const & config )
{
	Whitelist_.clear( );
	if( config.has_field( U( "whitelist" ) ) )
	{
		for( auto const & entry : config.at( U( "whitelist" ) ).as_object() )
		{
			auto & whitelistEntry = Whitelist_[entry.first];
			{
				std::wistringstream ss( entry.second.at( U( "last use date" ) ).as_string( ) );
				ss >> std::get_time( &whitelistEntry.LastUseDate_, L"%Y-%m-%dT%H:%M:%S" );
			}
			{
				std::wistringstream ss( entry.second.at( U( "create date" ) ).as_string( ) );
				ss >> std::get_time( &whitelistEntry.CreateDate_, L"%Y-%m-%dT%H:%M:%S" );
			}
			whitelistEntry.Name_ = entry.second.at( U( "name" ) ).as_string( );
		}
	}

	ApiVersion_ = 0x00000000;
	if( config.has_field( U( "apiversion" ) ) )
	{
		std::wstringstream ss( config.at( U( "apiversion" ) ).as_string( ).c_str( ) );
		for( int32_t chunk = ( ( 4 * 8 ) - 8 ); chunk >= 0; chunk -= 8 )
		{
			uint32_t apiVersion = 0;
			ss >> apiVersion;
			ApiVersion_ |= ( apiVersion << chunk );

			if( ss.eof( ) )
				break;

			wchar_t decimalPoint;
			ss >> decimalPoint;
		}
	}
	if( ApiVersion_ < 0x01000000 )
		ApiVersion_ = 0x01000000;

	if( config.has_field( U( "swversion" ) ) )
		SwVersion_ = config.at( U( "swversion" ) ).as_string( );

	if( config.has_field( U( "mac" ) ) )
		Mac_ = config.at( U( "mac" ) ).as_string( );

	if( config.has_field( U( "UTC" ) ) )
	{
		std::wistringstream ss( config.at( U( "UTC" ) ).as_string( ) );
		ss >> std::get_time( &Utc_, L"%Y-%m-%dT%H:%M:%S" );
	}

	if( config.has_field( U( "localtime" ) ) )
	{
		std::wistringstream ss( config.at( U( "localtime" ) ).as_string( ) );
		ss >> std::get_time( &LocalTime_, L"%Y-%m-%dT%H:%M:%S" );
	}

	if( config.has_field( U( "zigbeechannel" ) ) )
		ZigbeeChannel_ = static_cast< uint16_t >( config.at( U( "zigbeechannel" ) ).as_integer( ) ); // Might want to convert to time object of some sort

}

void MutableState::Deserialize( wj::value const & config )
{
	if( config.has_field( U( "proxyport" ) ) && config.has_field( U( "proxyaddress" ) ) && config.at( U( "proxyport" ) ).as_integer( ) != 0)
	{
		wh::uri_builder proxy;
		proxy.set_port( config.at( U( "proxyport" ) ).as_string( ) );
		proxy.set_host( config.at( U( "proxyaddress" ) ).as_string( ) );
		Proxy_ = proxy.to_uri( );
	}

	if( config.has_field( U( "name" ) ) )
		Name_ = config.at( U( "name" ) ).as_string( );
	if( config.has_field( U( "swupdate" ) ) )
		SwUpdate_.Deserialize( config.at( U( "swupdate" ) ) );
	if( config.has_field( U( "linkbutton" ) ) )
		LinkButton_ = config.at( U( "linkbutton" ) ).as_bool( );
	if( config.has_field( U( "ipaddress" ) ) )
		IpAddress_ = config.at( U( "ipaddress" ) ).as_string( );
	if( config.has_field( U( "netmask" ) ) )
		NetMask_ = config.at( U( "netmask" ) ).as_string( );
	if( config.has_field( U( "gateway" ) ) )
		Gateway_ = config.at( U( "gateway" ) ).as_string( );
	if( config.has_field( U( "dhcp" ) ) )
		Dhcp_ = config.at( U( "dhcp" ) ).as_bool( );
	if( config.has_field( U( "portalservices" ) ) )
		PortalServices_ = config.at( U( "portalservices" ) ).as_bool( );
	if( config.has_field( U( "timezone" ) ) )
		TimeZone_ = config.at( U( "timezone" ) ).as_string( );
}

namespace details
{

void Configuration_Impl::Deserialize( wj::value const & config )
{
	ReadOnlyState_ = ReadOnlyState( );
	ReadOnlyState_.Deserialize( config );
	CurrentState_ = MutableState( );
	CurrentState_.Deserialize( config );
	DirtyState_ = CurrentState_;
}

} // namespace details

// Note, there's a variation from the docs on adding a user.  Docs say that failure will result in a 101.
// I'm seeing failures return 200 with a json doc whioch indicates there was an error.
ppl::task< void > Configuration::CreateUserAsync( User const & user )
{
	auto client = [this]( )
	{
		web::uri_builder builder;
		builder.set_scheme( U( "http" ) ).
			set_host( Impl_->Client_.base_uri( ).host( ) ).
			set_path( U( "api" ) );

		return whc::http_client( builder.to_uri( ), Impl_->Client_.client_config( ) );
	}( );

	auto body = 
		[&]( )
	{
		wj::value fields;
		fields[U( "devicetype" )] = wj::value::string( user.DeviceType_ );
		fields[U( "username" )] = wj::value::string( user.UserName_ );
		return fields;
	}( );

	// Make the request and asynchronously process the response. 
	return client.request( wh::methods::POST, U( "" ), body ).then(
		[]( wh::http_response response )
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
		} );
	} ).then(
		[this]( wj::value response )
	{
		// Error indicates problem.  Often username length out of bounds
		for( auto const & result : response.as_array( ) )
		{
			// Need to parse error message to return more useful error
			if( result.has_field( U( "error" ) ) )
			{
				std::wstring_convert< std::codecvt_utf8_utf16< wchar_t > > converter;
				auto error = result.serialize( );
				throw std::runtime_error( converter.to_bytes( error.c_str( ) ) );
			}
		}
			
		return;
	} );
}

ppl::task< void > Configuration::GetAsync( )
{
	auto state = Impl_;

	// Make the request and asynchronously process the response. 
	return state->Client_.request( wh::methods::GET ).then(
		[state]( wh::http_response response )
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
		} );
	} ).then(
		[state]( wj::value config )
	{
		state->Deserialize( config );
	} );
}

ppl::task< void > Configuration::ModifyAsync( )
{
	auto body =
		[&]( )
	{
		wj::value fields;

		if( Impl_->DirtyState_.Proxy_.port( ) != Impl_->CurrentState_.Proxy_.port( ) )
			fields[U( "proxyport" )] = wj::value::number( Impl_->DirtyState_.Proxy_.port( ) );
		if( Impl_->DirtyState_.Proxy_.host( ) != Impl_->CurrentState_.Proxy_.host( ) )
			fields[U( "proxyaddress" )] = wj::value::string( Impl_->DirtyState_.Proxy_.host( ) );

		if( Impl_->DirtyState_.Name_ != Impl_->CurrentState_.Name_ )
			fields[U( "name" )] = wj::value::string( Impl_->DirtyState_.Name_ );
		if( Impl_->DirtyState_.SwUpdate_.State_ != Impl_->CurrentState_.SwUpdate_.State_ )
		{
			wj::value update_fields;
			update_fields[U( "updatestate" )] = wj::value::number( Impl_->DirtyState_.SwUpdate_.State_ );

			fields[U( "swupdate" )] = update_fields;
		}
		if( Impl_->DirtyState_.LinkButton_ != Impl_->CurrentState_.LinkButton_ )
			fields[U( "linkbutton" ) ] = wj::value::boolean( Impl_->DirtyState_.LinkButton_ );
		if( Impl_->DirtyState_.Dhcp_ != Impl_->CurrentState_.Dhcp_ )
		{
			fields[U( "dhcp" )] = wj::value::number( Impl_->DirtyState_.Dhcp_ );
			if( !Impl_->DirtyState_.Dhcp_ )
			{
				if( Impl_->DirtyState_.IpAddress_ != Impl_->CurrentState_.IpAddress_ )
					fields[U( "ipaddress" )] = wj::value::string( Impl_->DirtyState_.IpAddress_ );
				if( Impl_->DirtyState_.NetMask_ != Impl_->CurrentState_.NetMask_ )
					fields[U( "netmask" )] = wj::value::string( Impl_->DirtyState_.NetMask_ );
				if( Impl_->DirtyState_.Gateway_ != Impl_->CurrentState_.Gateway_ )
					fields[U( "gateway" )] = wj::value::string( Impl_->DirtyState_.Gateway_ );
			}
		}
		if( Impl_->DirtyState_.PortalServices_ != Impl_->CurrentState_.PortalServices_ )
			fields[U( "portalservices" )] = wj::value::boolean( Impl_->DirtyState_.PortalServices_ );

		if( Impl_->DirtyState_.TimeZone_ != Impl_->CurrentState_.TimeZone_ )
			fields[U( "timezone" )] = wj::value::string( Impl_->DirtyState_.TimeZone_ );

		if( fields.size( ) == 0 )
			return wj::value::null( );

		return fields;
	}( );

	if( body.is_null() )
	{
		return ppl::task_from_result( );
	}

	auto impl = Impl_;
	return impl->Client_.request( wh::methods::PUT, U( "" ), body ).then(
		[]( wh::http_response response )
	{
		if( response.status_code( ) != wh::status_codes::OK )
		{
			throw wh::http_exception( response.status_code( ) );
		}

		auto bodyStream = response.body( );

		return response.content_ready( ).then(
			[]( wh::http_response response )
		{
			return response.extract_json( );
		} );
	} ).then(
		[this]( wj::value response )
	{
// 			return GetResultCode( response.get( ) );
	} );
}

ppl::task< void > Configuration::DeleteUserAsync( User const & user )
{
	wu::ostringstream_t path;
	path << U( "/whitelist/" );
	path << wu::conversions::to_string_t( user.UserName_ );

	// Make the request and asynchronously process the response. 
	return Impl_->Client_.request( wh::methods::DEL, path.str( ) ).then(
		[]( wh::http_response response )
	{
// 		// Print the status code.
// 		std::wostringstream ss;
// 		ss << L"Server returned returned status code " << response.status_code( ) << L'.' << std::endl;
// 		std::wcout << ss.str( );
	} );
}

}