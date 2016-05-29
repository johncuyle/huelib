#include "stdafx.h"
#include "Hue/Sensors.hpp"


namespace Hue
{

wchar_t const * const Sensor::TypeName[Sensor::Type::Count] = {
	L"Daylight",
	L"ZGPSwitch",
	L"CLIPOpenClose",
	L"CLIPPresence",
	L"CLIPTemperature",
	L"CLIPHumidity",
	L"CLIPGenericFlag",
	L"CLIPGenericStatus",
	L"Clip Switch",
	L"Unknown" };

wchar_t const * const Sensor::State::Attribute[Sensor::Type::Count] = { 
	L"daylight", 
	L"buttonevent", 
	L"open", 
	L"presence", 
	L"temperature", 
	L"humidity", 
	L"flag", 
	L"status", 
	L"unknown", 
	L"unknown" };

Sensor::State::Type const Sensor::State::TypeLookup[static_cast< uint32_t >( Sensor::Type::Count )] = { 
	Sensor::State::Type::Bool, 
	Sensor::State::Type::Uint, 
	Sensor::State::Type::Bool, 
	Sensor::State::Type::Bool, 
	Sensor::State::Type::Int, 
	Sensor::State::Type::Int, 
	Sensor::State::Type::Bool, 
	Sensor::State::Type::Int, 
	Sensor::State::Type::None, 
	Sensor::State::Type::None };

namespace details
{

class Sensor_Impl
{
	struct Field
	{
		enum Enum
		{
			Id,
			Name,
			Type,
			ModelId,
			ManufacturerName,
			SoftwareVersion,
			UniqueId,

			Count,
			Last = Count - 1,
			First = 0
		};
	};

public:
	whc::http_client Client_;

	std::wstring Id_;
	std::wstring Name_;
	Sensor::Type Type_;
	std::wstring ModelId_;
	std::wstring ManufacturerName_;
	std::wstring SoftwareVersion_;
	std::wstring UniqueId_;

	Sensor::State State_;
	Sensor::Config Config_;

	std::bitset< Field::Count >	ValidFlags_;

	Sensor_Impl( ) : Client_( U( "http://localhost" ) )
	{
	}

	Sensor_Impl( whc::http_client const & client, std::wstring const & id, wj::value const & sensor ) : Client_( client ), Id_( id )
	{
		Deserialize( sensor );
	}

	void Deserialize( wj::value const & sensor );
	wj::value Serialize( ) const;

	std::wstring GetAddress( Condition::Operator const op ) const
	{
		std::wstringstream ss;
		ss << L"/sensors/" << Id_ << L"/state/" << ( ( op == Condition::Operator::Changed ) ? L"lastupdated" : Sensor::State::Attribute[static_cast<uint32_t>( Type_ )] );
		return ss.str( );
	}
};

void Sensor_Impl::Deserialize( wj::value const & sensor )
{
	if( sensor.has_field( U( "name" ) ) )
		Name_ = sensor.at( U( "name" ) ).as_string( );
	if( sensor.has_field( U( "type" ) ) )
	{
		Type_ = [&]( ) -> Sensor::Type
		{
			auto type = sensor.at( U( "type" ) ).as_string( );
			std::underlying_type< Sensor::Type >::type ty = 0;
			for( auto const & tyname : Sensor::TypeName )
			{
				if( tyname == type )
					return static_cast<Sensor::Type>( ty );
				++ty;
			}
			return Sensor::Type::Unknown;
		}( );
	}
	if( sensor.has_field( U( "modelid" ) ) )
		ModelId_ = sensor.at( U( "modelid" ) ).as_string( );
	if( sensor.has_field( U( "manufacturername" ) ) )
		ManufacturerName_ = sensor.at( U( "manufacturername" ) ).serialize( );
	if( sensor.has_field( U( "swversion" ) ) )
		SoftwareVersion_ = sensor.at( U( "swversion" ) ).as_string( );
	if( sensor.has_field( U( "uniqueid" ) ) )
		UniqueId_ = sensor.at( U( "uniqueid" ) ).as_string( );

	if( sensor.has_field( U( "state" ) ) )
		State_.Deserialize( sensor.at( U( "state" ) ), Type_ );

	if( sensor.has_field( U( "config" ) ) )
		Config_.Deserialize( sensor.at( U( "config" ) ), Type_ );
}

wj::value Sensor_Impl::Serialize( ) const
{
	wj::value fields;

	if( Name_.length( ) > 0 )
		fields[U( "name" )] = wj::value::string( Name_ );

	if( fields.size( ) == 0 )
		fields = wj::value::null( );

	return fields;
}

} // } namespace details

void Sensor::Config::Deserialize( wj::value const & config, Sensor::Type const type )
{
	switch( type )
	{
	case Sensor::Type::Daylight:
		if( config.has_field( U( "lat" ) ) )
			Latitude_ = config.at( U( "lat" ) ).as_string( );
		if( config.has_field( U( "long" ) ) )
			Longitude_ = config.at( U( "long" ) ).as_string( );
		if( config.has_field( U( "sunriseoffset" ) ) )
			SunriseOffset_ = static_cast<int8_t>( config.at( U( "sunriseoffset" ) ).as_integer( ) );
		if( config.has_field( U( "sunsetoffset" ) ) )
			SunsetOffset_ = static_cast<int8_t>( config.at( U( "sunsetoffset" ) ).as_integer( ) );
		// fall through
	case Sensor::Type::ZGPSwitch:
	case Sensor::Type::CLIPOpenClose:
	case Sensor::Type::CLIPPresence:
	case Sensor::Type::CLIPTemperature:
	case Sensor::Type::CLIPHumidity:
	case Sensor::Type::CLIPGenericFlag:
	case Sensor::Type::CLIPGenericStatus:
	case Sensor::Type::CLIPSwitch:
	default:
		if( config.has_field( U( "on" ) ) )
			On_ = config.at( U( "on" ) ).as_bool( );
		if( config.has_field( U( "reachable" ) ) )
			Reachable_ = config.at( U( "reachable" ) ).as_bool( );
		if( config.has_field( U( "battery" ) ) )
			Battery_ = static_cast<uint8_t>( config.at( U( "battery" ) ).as_integer( ) );
		if( config.has_field( U( "url" ) ) )
			Url_ = wh::uri( config.at( U( "url" ) ).as_string( ) );
		break;
	}
}

wj::value Sensor::Config::Serialize( Sensor::Type const type ) const
{
	wj::value fields;

	switch( type )
	{
	case Sensor::Type::Daylight:
		fields[U( "lat" )] = wj::value::string( Latitude_ );
		fields[U( "long" )] = wj::value::string( Longitude_ );
		fields[U( "sunriseoffset" )] = wj::value::number( SunriseOffset_ );
		fields[U( "sunsetoffset" )] = wj::value::number( SunsetOffset_ );
		// fall through
	case Sensor::Type::ZGPSwitch:
	case Sensor::Type::CLIPOpenClose:
	case Sensor::Type::CLIPPresence:
	case Sensor::Type::CLIPTemperature:
	case Sensor::Type::CLIPHumidity:
	case Sensor::Type::CLIPGenericFlag:
	case Sensor::Type::CLIPGenericStatus:
	case Sensor::Type::CLIPSwitch:
	default:
		fields[L"on"] = wj::value::boolean( On_ );
		fields[L"reachable"] = wj::value::boolean( Reachable_ );
		if( Battery_ >= 0 )
			fields[L"battery"] = wj::value::number( Battery_ );
		if( !Url_.is_empty() )
			fields[L"url"] = wj::value::string( Url_.to_string( ) );
		break;
	}

	if( fields.size( ) == 0 )
		fields = wj::value::null( );

	return fields;
}

void Sensor::State::Deserialize( wj::value const & state, Sensor::Type const type )
{
	StatusValid_ = false;
	if( state.has_field( Sensor::State::Attribute[static_cast<uint32_t>( type )] ) )
	{
		StatusValid_ = true;
		switch( Sensor::State::TypeLookup[static_cast<uint32_t>( type )] )
		{
		case Sensor::State::Type::Int:
			Status_.Status_ = static_cast<int32_t>( state.at( Sensor::State::Attribute[static_cast<uint32_t>( type )] ).as_integer( ) );
			break;
		case Sensor::State::Type::Uint:
			Status_.Event_ = static_cast<uint32_t>( state.at( Sensor::State::Attribute[static_cast<uint32_t>( type )] ).as_integer( ) );
			break;
		case Sensor::State::Type::Bool:
			Status_.Flag_ = state.at( Sensor::State::Attribute[static_cast<uint32_t>( type )] ).as_bool( );
			break;
		default:
			StatusValid_ = false;
			break;
		}
	}

	if( state.has_field( U( "lastupdated" ) ) )
	{
		if( state.at( U( "lastupdated" ) ).as_string( ) != U( "none" ) )
		{
			std::wistringstream ss( state.at( U( "lastupdated" ) ).as_string( ) );
			ss >> std::get_time( &LastUpdated_, L"%Y-%m-%dT%H:%M:%S" );
		}
	}
}

wj::value Sensor::State::Serialize( Sensor::Type const type ) const
{
	wj::value fields;

		switch( Sensor::State::TypeLookup[static_cast<uint32_t>( type )] )
		{
		case Sensor::State::Type::Int:
			fields[Sensor::State::Attribute[static_cast<uint32_t>( type )]] = wj::value::number( Status_.Status_ );
			break;
		case Sensor::State::Type::Uint:
			fields[Sensor::State::Attribute[static_cast<uint32_t>( type )]] = wj::value::number( static_cast<int32_t>( Status_.Event_ ) );
			break;
		case Sensor::State::Type::Bool:
			fields[Sensor::State::Attribute[static_cast<uint32_t>( type )]] = wj::value::boolean( Status_.Flag_ );
			break;
		default:
			break;
		}

	if( fields.size( ) == 0 )
		fields = wj::value::null( );

	return fields;
}

Sensor::Sensor( whc::http_client const & client, std::wstring const & id, wj::value const & sensor ) : Impl_( std::make_shared< Impl >( client, id, sensor ) )
{
}

ppl::task< void > Sensor::GetAsync( )
{
	auto impl = Impl_;

	// Make the request and asynchronously process the response. 
	return Impl_->Client_.request( wh::methods::GET, Impl_->Id_ ).then(
		&GetValidatedJson ).then(
		[impl]( wj::value sensor )
	{
		impl->Deserialize( sensor );
	} );
}

ppl::task< void > Sensor::UpdateAsync( ) const
{
	auto body = [&]( )
	{
		wj::value fields;

		if( Impl_->Name_.length( ) > 0 )
			fields[U( "name" )] = wj::value::string( Impl_->Name_ );

		if( fields.size( ) == 0 )
			fields = wj::value::null( );

		return fields;
	}( );

	if( body.is_null( ) )
		return ppl::task_from_result( );

	auto impl = Impl_;
	return impl->Client_.request( wh::methods::PUT, Impl_->Id_, body ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );
}

ppl::task< void > Sensor::ChangeConfigAsync( ) const
{
	auto body = Impl_->Config_.Serialize( Impl_->Type_ );

	if( body.is_null( ) )
		return ppl::task_from_result( );

	auto impl = Impl_;
	return impl->Client_.request( wh::methods::PUT, Impl_->Id_ + L"/config", body ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );
}

ppl::task< void > Sensor::ChangeStateAsync( ) const
{
	auto body = Impl_->State_.Serialize( Impl_->Type_ );

	if( body.is_null( ) )
		return ppl::task_from_result( );

	auto impl = Impl_;
	return impl->Client_.request( wh::methods::PUT, Impl_->Id_ + L"/state", body ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );
}

Command::Instance Sensor::GetCommand( ) const
{
	Command::Instance command;
	std::get< Command::Address >( command ) = Impl_->Client_.base_uri( ).path( ) + U( "/" ) + Impl_->Id_ + U( "/state" );
	std::get< Command::Method >( command ) = wh::methods::PUT;
	std::get< Command::Body >( command ) = Impl_->State_.Serialize( Impl_->Type_ );

	return command;
}

Action::Instance Sensor::GetAction( ) const
{
	Action::Instance action;
	std::get< Action::Address >( action ) = std::wstring( L"/sensors" ) + U( "/" ) + Impl_->Id_ + U( "/state" );
	std::get< Action::Method >( action ) = wh::methods::PUT;
	std::get< Action::Body >( action ) = Impl_->State_.Serialize( Impl_->Type_ );

	return action;
}

std::wstring Sensor::GetId( ) const
{
	return Impl_->Id_;
}

std::wstring Sensor::GetName( ) const
{
	return Impl_->Name_;
}

Sensor::Type Sensor::GetType( ) const
{
	return Impl_->Type_;
}

std::wstring Sensor::GetModelId( ) const
{
	return Impl_->ModelId_;
}

std::wstring Sensor::GetManufacturerName( ) const
{
	return Impl_->ManufacturerName_;
}

std::wstring Sensor::GetSoftwareVersion( ) const
{
	return Impl_->SoftwareVersion_;
}

std::wstring Sensor::GetUniqueId( ) const
{
	return Impl_->UniqueId_;
}

Sensor::State const & Sensor::GetState( ) const
{
	return Impl_->State_;
}

Sensor::Config const & Sensor::GetConfig( ) const
{
	return Impl_->Config_;
}

std::wstring Sensor::GetAddress( Condition::Operator const op ) const
{
	return Impl_->GetAddress( op );
}

void Sensor::SetName( std::wstring const & name )
{
	Impl_->Name_ = name;
}

void Sensor::SetConfig( Config const & config )
{
	Impl_->Config_ = config;
}

void Sensor::SetState( State const & state )
{
	Impl_->State_ = state;
}

Sensors::Sensors( whc::http_client const & client, wj::value const & sensors ) : Sensors( client )
{
	Deserialize( sensors );
}

void Sensors::Deserialize( wj::value const & sensors )
{
	reserve( sensors.size( ) );

	for( auto const & sensor : sensors.as_object( ) )
	{
		if( sensor.first == U( "lastscan" ) )
			continue;

		emplace_back( Client_, sensor.first, sensor.second );
	}
}

ppl::task< Sensor > Sensors::CreateAsync( 
	std::wstring const & name,
	std::wstring const & modelId,
	std::wstring const & swVersion,
	Sensor::Type const type,
	std::wstring const & uniqueId,
	std::wstring const & manuName,
	Sensor::Config const & config,
	Sensor::State const & state ) const
{
	auto body = [&]( )
	{
		wj::value fields;

		if( name.length() > 0 )
			fields[U( "name" )] = wj::value::string( name );

		if( modelId.length( ) > 0 )
			fields[U( "modelid" )] = wj::value::string( modelId );

		if( swVersion.length( ) > 0 )
			fields[U( "swversion" )] = wj::value::string( swVersion );

		fields[U( "type" )] = wj::value::string( Sensor::TypeName[static_cast< std::underlying_type< Sensor::Type >::type >( type )] );

		if( uniqueId.length( ) > 0 )
			fields[U( "uniqueid" )] = wj::value::string( uniqueId );

		if( manuName.length( ) > 0 )
			fields[U( "manufacturername" )] = wj::value::string( manuName );

 		fields[U( "config" )] = config.Serialize( type );
		fields[U( "state" )] = state.Serialize( type );

		return fields;
	}( );

	auto client = Client_;

	return client.request( wh::methods::POST, L"", body ).then(
		&GetValidatedJson ).then(
		[client]( wj::value response )
	{
		return Sensor( client, L"", wj::value::string( L"{}" ) );
	} );
}

ppl::task< void > Sensors::FindNewAsync( )
{
	return Client_.request( wh::methods::POST, L"{}" ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );
}

ppl::task< void > Sensors::DeleteAsync( std::wstring const & id )
{
	return Client_.request( wh::methods::DEL, id ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );
}

} // } namespace Hue 