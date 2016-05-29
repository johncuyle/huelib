#pragma once

#include "Hue\Rules.hpp"

namespace Hue
{

class Sensor;

namespace details
{
class Sensor_Impl;
} // } namespace details

class Sensor
{
public:
	enum class Type
	{
		Daylight,
		ZGPSwitch,
		CLIPOpenClose,
		CLIPPresence,
		CLIPTemperature,
		CLIPHumidity,
		CLIPGenericFlag,
		CLIPGenericStatus,
		CLIPSwitch,
		Unknown,

		Count,
		Last = Count - 1,
		First = 0
	};

	static wchar_t const * const TypeName[static_cast< std::underlying_type< Type >::type >( Type::Count )];

	struct State
	{
		enum class Type
		{
			Int,
			Uint,
			Bool,
			None
		};

		static Type const TypeLookup[static_cast< std::underlying_type< Sensor::Type >::type >( Sensor::Type::Count )];
		static wchar_t const * const Attribute[static_cast< std::underlying_type< Sensor::Type >::type >( Sensor::Type::Count )];

		std::tm LastUpdated_;
		union Status
		{
			int32_t Status_;
			uint32_t Event_;
			bool Flag_;
		} Status_;
		bool StatusValid_;

		State( ) : StatusValid_( false )
		{
			Status_.Status_ = 0;
			std::time_t t = 0;
			gmtime_s( &LastUpdated_, &t );
		}

		void Deserialize( wj::value const & state, Sensor::Type const type );
		wj::value Serialize( Sensor::Type const type ) const;
	};

	struct Config
	{
		struct Field
		{
			enum Enum
			{
				On,
				Reachable,
				Battery,
				Url,
				Latitude,
				Longitude,
				SunriseOffset,
				SunsetOffset,

				Count,
				Last = Count - 1,
				First = 0
			};
		};

		bool On_;
		bool Reachable_;
		int8_t Battery_;
		wh::uri Url_;

		//For daylight only
		std::wstring Latitude_;
		std::wstring Longitude_;
		int8_t SunriseOffset_; // Offset for when daylight == true from actual sunrise, in minutes, valid range +-120
		int8_t SunsetOffset_; // Offset for when daylight == true from actual sunset, in minutes, valid range +-120

		std::bitset< Field::Count >	ValidFlags_;

		Config( ) : On_( false ), Reachable_( true ), Battery_( -1 ), SunriseOffset_( 0 ), SunsetOffset_( 0 )
		{
		}

		void Deserialize( wj::value const & config, Sensor::Type const type );
		wj::value Serialize( Sensor::Type const type ) const;
	};

private:
	typedef details::Sensor_Impl Impl;

private:
	std::shared_ptr< Impl > Impl_;

public:

	Sensor( )
	{
	}

	Sensor( whc::http_client const & client, std::wstring const & id, wj::value const & sensor );

	ppl::task< void > GetAsync( );
	ppl::task< void > UpdateAsync( ) const;
	ppl::task< void > ChangeConfigAsync( ) const;
	ppl::task< void > ChangeStateAsync( ) const;

	Command::Instance GetCommand( ) const;
	Action::Instance GetAction( ) const;

	std::wstring GetId( ) const;
	std::wstring GetName( ) const;

	Sensor::Type GetType( ) const;
	std::wstring GetModelId( ) const;
	std::wstring GetManufacturerName( ) const;
	std::wstring GetSoftwareVersion( ) const;
	std::wstring GetUniqueId( ) const;

	Config const & GetConfig( ) const;
	State const & GetState( ) const;

	std::wstring GetAddress( Condition::Operator const op ) const;

	void SetName( std::wstring const & name );
	void SetConfig( Config const & config );
	void SetState( State const & state );
};

class Sensors : public std::vector < Sensor >
{
private:
	whc::http_client Client_;

public:
	Sensors( ) : Client_( U( "http://localhost" ) )
	{
	}

	Sensors( whc::http_client const & client ) : Client_( client )
	{
	}

	Sensors( whc::http_client const & client, wj::value const & sensors );

	void Deserialize( wj::value const & sensors );

	ppl::task< Sensor > CreateAsync( 
		std::wstring const & name, 
		std::wstring const & modelId, 
		std::wstring const & swVersion,
		Sensor::Type const type,
		std::wstring const & uniqueId,
		std::wstring const & manuName,
		Sensor::Config const & config,
		Sensor::State const & state ) const;
	ppl::task< void > FindNewAsync( );
	ppl::task< void > DeleteAsync( std::wstring const & id );
};
}
