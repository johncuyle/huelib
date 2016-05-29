#pragma once

#include "Hue\Shared.hpp"
#include "Hue\Schedules.hpp"

namespace Hue
{

namespace details
{
class Light_Impl;
} // } namespace details

class Group;

namespace Color
{

struct Temperature
{
	enum class Kelvin : uint16_t
	{
		CandleLight = 2000,
		Dawn = 2250,
		Dusk = Dawn,
		Incandescent = 2700,
		Sunrise = 3000,
		Sunset = Sunrise,
		Halogen = 3200,
		Magic = 4800,
		Midday = 5500,
		Overcast = 6000,
		Fluorescent = 6250,
		Shade = 6500,
		Twilight = 6500,

		Warmest = CandleLight,
		Coolest = Shade
	};

	enum class Mired : uint16_t
	{
		Twilight = 153,
		Shade = 153,
		Fluorescent = 160,
		Overcast = 167,
		Midday = 182,
		Magic = 208,
		Halogen = 312,
		Sunrise = 333,
		Sunset = Sunrise,
		Incandescent = 370,
		Dawn = 444,
		Dusk = Dawn,
		CandleLight = 500,

		Warmest = CandleLight,
		Coolest = Shade
	};

	enum class Index
	{
		Twilight,
		Shade,
		Fluorescent,
		Overcast,
		Midday,
		Magic,
		Halogen,
		Sunrise,
		Sunset,
		Incandescent,
		Dawn,
		Dusk,
		CandleLight,

		Count,
		Last = Count - 1,
		First = 0
	};

	static wchar_t const * const Name[static_cast<std::underlying_type< Index >::type>( Index::Count )];
	static Mired const NameMap[static_cast<std::underlying_type< Index >::type>( Index::Count )];
};

struct Hue
{
	enum class Color : uint16_t
	{
		Red = 0,
		Orange = 3250,
		Yellow = 13950,
		Green = 25500,
		// Color passes through a whitish range in here.
		Blue = 46920,
		Indigo = 48500,
		Violet = 50800,
		Magenta = 56100,
	};

	enum class Index
	{
		Red,
		Orange,
		Yellow,
		Green,
		// Color passes through a whitish range in here.
		Blue,
		Indigo,
		Violet,
		Magenta,

		Count,
		Last = Count - 1,
		First = 0
	};

	static wchar_t const * const Name[static_cast<std::underlying_type< Index >::type>( Index::Count )];
	static Color const NameMap[static_cast<std::underlying_type< Index >::type>( Index::Count )];
};

enum class Mode : uint8_t
{
	hs,
	xy,
	ct,

	invalid,

	Count,
	Last = Count - 1,
	First = 0
};

} // namespace Color

class Light
{
public:
	struct State
	{
		struct Set
		{
			enum Enum
			{
				Effect,
				Alert,
				ColorMode,
				XY,
				ColorTemperature,
				Hue,
				Saturation,
				Brightness,
				On,

				Count,
				Last = Count - 1,
				First = 0
			};
		};

		static wchar_t const * const ModeName[static_cast< uint32_t >( Color::Mode::Count )];

		std::wstring		Effect_; // none, colorloop
		std::wstring		Alert_; // none, select, lselect
		Color::Mode			ColorMode_; // hs, xy, ct
		float				XY_[2];
		uint16_t			ColorTemperature_; // Mired color temperature. divide 1,000,000 by this value to get temperature in K.
		uint16_t			Hue_;
		uint8_t				Saturation_;
		uint8_t				Brightness_;
		bool				Reachable_;
		bool				On_;

		std::bitset< Set::Count >	SetFlags_;

		State( ) : Effect_( L"none" ), Alert_( L"none" ), ColorMode_( Color::Mode::invalid ), ColorTemperature_( 0 ), Hue_( 0 ), Saturation_( 0 ), Brightness_( 0 ), Reachable_( false ), On_( false )
		{
			XY_[0] = XY_[1] = 0.f;
			SetFlags_.reset( );
		}

		void Deserialize( wj::value const & state );
		wj::value Serialize( State const * const comparisonState ) const;
	};

private:
	typedef details::Light_Impl Impl;

private:
	std::shared_ptr< Impl > Impl_;

public:
	Light( )
	{
	}

	Light( whc::http_client const & client, std::wstring const & id, wj::value const & light );

	ppl::task< void > GetAttributesAndStateAsync( );

	ppl::task< void > SetAttributesAsync( ) const;
	ppl::task< void > SetStateAsync( uint16_t const transitionTime/*, std::wstring const & scene = std::wstring( )*/ ) const;

	Command::Instance GetCommand( uint16_t const transitionTime ) const;
	Action::Instance GetAction( uint16_t const transitionTime ) const;

	std::wstring GetId( ) const;
	std::wstring GetType( ) const;
	std::wstring GetName( ) const;
	std::wstring GetModelId( ) const;
	std::wstring GetSwVersion( ) const;
	State GetCurrentState( ) const;

	void SetName( std::wstring const & name );
	void SetState( State const & state );

private:
	wj::value GetCommandBody( uint16_t const transitionTime, bool const forSchedule ) const;

	friend class Group;
	friend class Schedule;
};

class Lights : public std::vector< Light >
{
private:
	whc::http_client Client_;

public:
	Lights( ) : Client_( U( "http://localhost" ) )
	{}

	Lights( whc::http_client const & client ) : Client_( client )
	{
	}

	Lights( whc::http_client const & client, wj::value const & lights );

	void Deserialize( wj::value const & lights );

	ppl::task< void > FindNewAsync( std::vector< std::wstring > const & devices );
};

} // namespace Hue