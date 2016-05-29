#include "stdafx.h"
#include "Hue/Schedules.hpp"
#include "Hue\Shared.hpp"

namespace Hue
{

void Schedule::Time::Deserialize( std::wstring const & time )
{
	*this = Time( );

	std::wstringstream ss( time );
	// figure out the type and get the primary time
	wchar_t delim;
	ss >> delim;
	switch( delim )
	{
	case L'W':
		Type_ = Type::DaysOfWeek;
		{
			// get day flags
			uint16_t dayFlags;
			ss >> dayFlags;
			DaysOfWeek_ = std::bitset< 8 >( dayFlags );
			ss.seekg( 1, std::ios_base::cur ); // eat the "/"
		}
		break;
	case L'R':
		Type_ = Type::Recurring;
		{
			// get recurrence count
			ss >> Recurrence_;
			ss.seekg( 2, std::ios_base::cur ); // eat the "/P"
		}
		break;
	case L'P':
		Type_ = Type::Timer;
		break;
	default:
		Type_ = Type::Absolute;
		ss.unget( );
		ss >> std::get_time( &Time_, L"%Y-%m-%dT%H:%M:%S" );
		break;
	}

	if( Type_ != Type::Absolute )
	{
		ss >> std::get_time( &Time_, L"T%H:%M:%S" );
	}
	if( !ss.eof( ) ) // get the randomization
	{
		ss >> std::get_time( &Randomization_, L"A%H:%M:%S" );
	}
}

std::wstring Schedule::Time::Serialize( ) const
{
	std::wstringstream ss;
	switch( Type_ )
	{
	case Type::DaysOfWeek:
		ss << L'W';
		// need to output bitflags
		ss << std::setw( 3 ) << std::setfill( L'0' );
		ss << DaysOfWeek_.to_ulong( );
		ss << L"/";
		break;
	case Type::Recurring:
		ss << L'R';
		ss << std::setw( 2 ) << std::setfill( L'0' );
		ss << ( 99 < Recurrence_ ? 99 : Recurrence_ );
		ss << L"/";
		//fallthrough
	case Type::Timer:
		ss << L'P';
		break;
	default:
		ss << std::put_time( &Time_, L"%Y-%m-%dT%H:%M:%S" );
	};

	if( Type_ != Type::Absolute )
	{
		ss << std::put_time( &Time_, L"T%H:%M:%S" );
	}
	if( Randomization_.tm_hour != 0 || Randomization_.tm_min != 0 || Randomization_.tm_sec != 0 )
	{
		ss << std::put_time( &Randomization_, L"A%H:%M:%S" );
	}

	return ss.str( );
}

namespace details
{

class Schedule_Impl
{
public:
	whc::http_client Client_;

	std::wstring			Id_;
	std::wstring			Name_;
	std::wstring			Description_;
	Schedule::Time			Time_;
	Command::Instance		Command_;

	std::tm					Created_;		// ????? 
	std::tm					StartTime_;		// 1.2.1
	Schedule::Status		Status_;		// enabled or disabled // 1.2.1
	Schedule::AutoDelete	AutoDelete_;	// 1.3

	Schedule_Impl( whc::http_client const & client, std::wstring const & id ) : Client_( client ), Id_( id ), Status_( Schedule::Status::Enabled ), AutoDelete_( Schedule::AutoDelete::True )
	{
		std::time_t t = 0;
		gmtime_s( &Created_, &t );
		gmtime_s( &StartTime_, &t );
	}

	Schedule_Impl( ) : Schedule_Impl( whc::http_client( U( "http://localhost" ) ), 0 )
	{
	}

	Schedule_Impl( whc::http_client const & client, std::wstring const & id, wj::value const & schedule ) : Schedule_Impl( client, id )
	{
		Deserialize( schedule );
	}

	Schedule_Impl( whc::http_client const & client,
		std::wstring const & name,
		std::wstring const & description,
		Schedule::Time const & time,
		Command::Instance const & command,
		Schedule::Status const status,
		Schedule::AutoDelete const autoDelete ) : 
		Client_( client ), 
		Id_( L"0" ), 
		Name_( name ),
		Description_( description ),
		Time_( time ),
		Command_( command ),
		Status_( status ), 
		AutoDelete_( autoDelete )
	{
	}

	void Deserialize( wj::value const & schedule );
	wj::value Serialize( ) const;
};

void Schedule_Impl::Deserialize( wj::value const & schedule )
{
	if( schedule.has_field( U( "name" ) ) )
		Name_ = schedule.at( U( "name" ) ).as_string( );
	if( schedule.has_field( U( "description" ) ) )
		Description_ = schedule.at( U( "description" ) ).as_string( );
	if( schedule.has_field( U( "time" ) ) )
		Time_.Deserialize( schedule.at( U( "time" ) ).as_string( ) );
	if( schedule.has_field( U( "command" ) ) )
	{
		auto command = schedule.at( U( "command" ) );
		std::get< Command::Address >( Command_ ) = command.at( U( "address" ) ).as_string( );
		std::get< Command::Method >( Command_ ) = wh::method( command.at( U( "method" ) ).as_string( ) );
		std::get< Command::Body >( Command_ ) = command.at( U( "body" ) );
	}
	if( schedule.has_field( U( "created" ) ) )
	{
		std::wistringstream ss( schedule.at( U( "created" ) ).as_string( ) );
		ss >> std::get_time( &Created_, L"%Y-%m-%dT%H:%M:%S" );
	}
	if( schedule.has_field( U( "starttime" ) ) )
	{
		std::wistringstream ss( schedule.at( U( "starttime" ) ).as_string( ) );
		ss >> std::get_time( &StartTime_, L"%Y-%m-%dT%H:%M:%S" );
	}
	if( schedule.has_field( U( "status" ) ) )
		Status_ = ( schedule.at( U( "status" ) ).as_string( ) == U( "enabled" ) ? Schedule::Status::Enabled : Schedule::Status::Disabled );
	if( schedule.has_field( U( "autodelete" ) ) )
		AutoDelete_ = ( schedule.at( U( "autodelete" ) ).as_bool( ) ? Schedule::AutoDelete::True : Schedule::AutoDelete::False );
}

wj::value Schedule_Impl::Serialize( ) const
{
	wj::value fields;

	if( Name_ != L"" )
		fields[U( "name" )] = wj::value::string( Name_ );

	if( Description_ != L"" )
		fields[U( "description" )] = wj::value::string( Description_ );

	if( Time_.IsValid( ) )
		fields[U( "time" )] = wj::value::string( Time_.Serialize( ) );

	if( Command_ != Command::Instance( ) )
	{
		wj::value command;

		command[U( "address" )] = wj::value::string( std::get< Command::Address >( Command_ ) );
		command[U( "method" )] = wj::value::string( std::get< Command::Method >( Command_ ) );
		command[U( "body" )] = std::get< Command::Body >( Command_ );

		fields[U( "command" )] = command;
	}

	fields[U( "status" )] = wj::value::string( Status_ == Schedule::Status::Enabled ? U( "enabled" ) : U( "disabled" ) );
	fields[U( "autodelete" )] = wj::value::boolean( AutoDelete_ != Schedule::AutoDelete::False );

	if( fields.size( ) == 0 )
		fields = wj::value::null( );

	return fields;
}
} // } namespace details

Schedule::Schedule( whc::http_client const & client, std::wstring const & id, wj::value const & schedule ) : Impl_( std::make_shared< Impl >( client, id, schedule ) )
{
}

ppl::task< void > Schedule::GetAttributesAsync( )
{
	auto impl = Impl_;

	// Make the request and asynchronously process the response. 
	return Impl_->Client_.request( wh::methods::GET, Impl_->Id_ ).then(
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
		[impl]( wj::value light )
	{
		impl->Deserialize( light );
	} );
}

ppl::task< void > Schedule::SetStateAsync( ) const
{
	auto body = Impl_->Serialize( );

	if( body.is_null( ) )
	{
		return ppl::task_from_result( );
	}

	auto impl = Impl_;
	return impl->Client_.request( wh::methods::PUT, Impl_->Id_, body ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );
}

std::wstring Schedule::GetId( ) const
{
	return Impl_->Id_;
}

std::wstring Schedule::GetName( ) const
{
	return Impl_->Name_;
}

std::wstring Schedule::GetDescription( ) const
{
	return Impl_->Description_;
}

Schedule::Time Schedule::GetTime( ) const
{
	return Impl_->Time_;
}

Command::Instance Schedule::GetCommand( ) const
{
	return Impl_->Command_;
}

std::tm const & Schedule::GetCreated( ) const
{
	return Impl_->Created_;
}

std::tm const & Schedule::GetStartTime( ) const
{
	return Impl_->StartTime_;
}

Schedule::Status Schedule::GetStatus( ) const
{
	return Impl_->Status_;
}

Schedule::AutoDelete Schedule::GetAutoDelete( ) const
{
	return Impl_->AutoDelete_;
}

void Schedule::SetName( std::wstring const & name )
{
	Impl_->Name_ = name;
}

void Schedule::SetDescription( std::wstring const & description )
{
	Impl_->Description_ = description;
}

void Schedule::SetTime( Schedule::Time const & time )
{
	Impl_->Time_ = time;
}

void Schedule::SetCommand( Command::Instance const & command )
{
	Impl_->Command_ = command;
}

void Schedule::SetStatus( Status const status )
{
	Impl_->Status_ = status;
}

void Schedule::SetAutoDelete( AutoDelete const autoDelete )
{
	Impl_->AutoDelete_ = autoDelete;
}

Schedules::Schedules( whc::http_client const & client, wj::value const & schedules ) : Schedules( client )
{
	Deserialize( schedules );
}

void Schedules::Deserialize( wj::value const & schedules )
{
	reserve( schedules.size( ) );

	for( auto const & schedule : schedules.as_object( ) )
	{
		emplace_back( Client_, schedule.first, schedule.second );
	}
}

ppl::task< Schedule > Schedules::CreateAsync( 
	std::wstring const & name, 
	std::wstring const & description, 
	Schedule::Time const & time, 
	Command::Instance const & command, 
	Schedule::Status const status, 
	Schedule::AutoDelete const autoDelete ) const
{
	details::Schedule_Impl const schedule( Client_, name, description, time, command, status, autoDelete );
	auto body = schedule.Serialize( );

 	auto client = Client_;

	// Make the request and asynchronously process the response. 
	return client.request( wh::methods::POST, L"", body ).then(
		&GetValidatedJson ).then(
		[client]( wj::value response )
	{
		return Schedule( client, L"", wj::value::string( L"{}" ) );
	} );
}

ppl::task< void > Schedules::DeleteAsync( std::wstring const & id )
{
	// Make the request and asynchronously process the response. 
	return Client_.request( wh::methods::DEL, id ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );
}

} // } namespace Hue 