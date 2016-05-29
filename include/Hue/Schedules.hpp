#pragma once

#include "Hue\Shared.hpp"

namespace Hue
{

class Schedule;

namespace details
{
class Schedule_Impl;
}

class Schedule
{
public:
	struct Time
	{
		enum class Type
		{
			Unknown,
			Absolute,
			Recurring,
			DaysOfWeek,
			Timer
		};

		Type Type_;
		std::bitset< 8 > DaysOfWeek_;
		std::tm Time_;
		std::tm Randomization_;
		uint16_t Recurrence_;

		Time() : Type_( Type::Unknown ), Recurrence_( 0 )
		{
			std::time_t t = 0;
			gmtime_s( &Time_, &t );
			gmtime_s( &Randomization_, &t );
		}

		bool IsValid( ) const
		{
			return Type_ != Type::Unknown;
		}
		void Deserialize( std::wstring const & time );
		std::wstring Serialize( ) const;
	};

	enum class Status
	{
		Disabled,
		Enabled
	};

	enum class AutoDelete
	{
		False,
		True
	};

private:
	typedef details::Schedule_Impl Impl;

private:
	std::shared_ptr< Impl > Impl_;

public:

	Schedule( )
	{
	}

	Schedule( whc::http_client const & client, std::wstring const & id, wj::value const & schedule );

	ppl::task< void > GetAttributesAsync( );
	ppl::task< void > SetStateAsync( ) const;

	std::wstring GetId( ) const;
	std::wstring GetName( ) const;
	std::wstring GetDescription( ) const;
	Time GetTime( ) const;
 	Command::Instance GetCommand( ) const;
	std::tm const & GetCreated( ) const;
	std::tm const & GetStartTime( ) const;
	Status GetStatus( ) const;
	AutoDelete GetAutoDelete( ) const;

	void SetName( std::wstring const & name );
	void SetDescription( std::wstring const & description );
	void SetTime( Time const & time );
 	void SetCommand( Command::Instance const & command );
	void SetStatus( Status const status );
	void SetAutoDelete( AutoDelete const autoDelete );
};

class Schedules : public std::vector < Schedule >
{
private:
	whc::http_client Client_;

public:
	Schedules( ) : Client_( U( "http://localhost" ) )
	{
	}

	Schedules( whc::http_client const & client ) : Client_( client )
	{
	}

	Schedules( whc::http_client const & client, wj::value const & schedules );

	void Deserialize( wj::value const & schedules );

	ppl::task< Schedule > CreateAsync( std::wstring const & name, 
		std::wstring const & description, 
		Schedule::Time const & time, 
		Command::Instance const & command,
		Schedule::Status const status = Schedule::Status::Enabled,
		Schedule::AutoDelete const autoDelete = Schedule::AutoDelete::True ) const;
	ppl::task< void > DeleteAsync( std::wstring const & id );
};

}