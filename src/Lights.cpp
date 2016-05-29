#include "stdafx.h"
#include "Hue/Lights.hpp"

namespace Hue
{

namespace Color
{
	wchar_t const * const Temperature::Name[static_cast<std::underlying_type< Temperature::Index >::type>( Temperature::Index::Count )] = {
		L"twilight",
		L"shade",
		L"fluorescent",
		L"overcast",
		L"midday",
		L"magic",
		L"halogen",
		L"sunrise",
		L"sunset",
		L"incandescent",
		L"dawn",
		L"dusk",
		L"candleLight"
	};

	Temperature::Mired const Temperature::NameMap[static_cast<std::underlying_type< Temperature::Index >::type>( Temperature::Index::Count )] =
	{
		Temperature::Mired::Twilight,
		Temperature::Mired::Shade,
		Temperature::Mired::Fluorescent,
		Temperature::Mired::Overcast,
		Temperature::Mired::Midday,
		Temperature::Mired::Magic,
		Temperature::Mired::Halogen,
		Temperature::Mired::Sunrise,
		Temperature::Mired::Sunset,
		Temperature::Mired::Incandescent,
		Temperature::Mired::Dawn,
		Temperature::Mired::Dusk,
		Temperature::Mired::CandleLight
	};

	wchar_t const * const Hue::Name[static_cast< std::underlying_type< Hue::Index >::type >( Index::Count ) ] = {
		L"red",
		L"orange",
		L"yellow",
		L"green",
		// Color passes through a whitish range in here.
		L"blue",
		L"indigo",
		L"violet",
		L"magenta"
	};

	Hue::Color const Hue::NameMap[static_cast<std::underlying_type< Hue::Index >::type>( Hue::Index::Count )] = {
		Hue::Color::Red, 
		Hue::Color::Orange,
		Hue::Color::Yellow,
		Hue::Color::Green,
		Hue::Color::Blue,
		Hue::Color::Indigo,
		Hue::Color::Violet,
		Hue::Color::Magenta
	};
}

wchar_t const * const Light::State::ModeName[static_cast<uint32_t>( Color::Mode::Count )] = { L"hs", L"xy", L"ct", L"invalid" };

void Light::State::Deserialize( wj::value const & state )
{
	SetFlags_.reset( );

	if( state.has_field( U( "on" ) ) )
	{
		On_ = state.at( U( "on" ) ).as_bool( );
		SetFlags_.set( Set::On );
	}
	if( state.has_field( U( "effect" ) ) )
	{
		Effect_ = state.at( U( "effect" ) ).as_string( );
		SetFlags_.set( Set::Effect );
	}
	if( state.has_field( U( "alert" ) ) )
	{
		Alert_ = state.at( U( "alert" ) ).as_string( );
		SetFlags_.set( Set::Alert );
	}
	if( state.has_field( U( "hue" ) ) )
	{
		Hue_ = static_cast< uint16_t >( state.at( U( "hue" ) ).as_integer( ) );
		SetFlags_.set( Set::Hue );
	}
	if( state.has_field( U( "bri" ) ) )
	{
		Brightness_ = static_cast< uint8_t >( state.at( U( "bri" ) ).as_integer( ) );
		SetFlags_.set( Set::Brightness );
	}
	if( state.has_field( U( "sat" ) ) )
	{
		Saturation_ = static_cast< uint8_t >( state.at( U( "sat" ) ).as_integer( ) );
		SetFlags_.set( Set::Saturation );
	}
	if( state.has_field( U( "ct" ) ) )
	{
		ColorTemperature_ = static_cast< uint16_t >( state.at( U( "ct" ) ).as_integer( ) );
		SetFlags_.set( Set::ColorTemperature );
	}
	if( state.has_field( U( "xy" ) ) )
	{
		uint32_t ind = 0;
		for( auto const & xy : state.at( U( "xy" ) ).as_array( ) )
		{
			XY_[ind] = static_cast<float>( xy.as_double( ) );
			if( ( ++ind ) >= ARRAYSIZE( XY_ ) )
				break;
		}
		SetFlags_.set( Set::XY );
	}
	if( state.has_field( U( "colormode" ) ) )
	{
		ColorMode_ = [&]( ) -> Color::Mode
		{
			auto colormode = state.at( U( "colormode" ) ).as_string( );
			std::underlying_type< Color::Mode >::type mode = 0;
			for( auto const & modename : Light::State::ModeName )
			{
				if( modename == colormode )
					break;
				++mode;
			}
			return static_cast< Color::Mode >( mode );
		}( );
		SetFlags_.set( Set::ColorMode );
	}	
	if( state.has_field( U( "reachable" ) ) )
	{
		Reachable_ = state.at( U( "reachable" ) ).as_bool( );
	}
}

wj::value Light::State::Serialize( State const * const comparisonState ) const
{
	wj::value fields;

	if( ( comparisonState && Effect_ != comparisonState->Effect_ ) || ( !comparisonState && SetFlags_.test( Set::Effect ) ) )
		fields[U( "effect" )] = wj::value::string( Effect_ );
	if( ( comparisonState && Alert_ != comparisonState->Alert_ ) || ( !comparisonState && SetFlags_.test( Set::Alert ) ) )
		fields[U( "alert" )] = wj::value::string( Alert_ );
	if( ( comparisonState && On_ != comparisonState->On_ ) || ( !comparisonState && SetFlags_.test( Set::On ) ) )
		fields[U( "on" )] = wj::value::boolean( On_ );
	if( ( comparisonState && Brightness_ != comparisonState->Brightness_ ) || ( !comparisonState && SetFlags_.test( Set::Brightness ) ) )
		fields[U( "bri" )] = wj::value::number( Brightness_ );
	if( ( comparisonState && ColorTemperature_ != comparisonState->ColorTemperature_ ) || ( !comparisonState && SetFlags_.test( Set::ColorTemperature ) ) )
		fields[U( "ct" )] = wj::value::number( ColorTemperature_ );
	if( ( comparisonState && Hue_ != comparisonState->Hue_ ) || ( !comparisonState && SetFlags_.test( Set::Hue ) ) )
		fields[U( "hue" )] = wj::value::number( Hue_ );
	if( ( comparisonState && Saturation_ != comparisonState->Saturation_ ) || ( !comparisonState && SetFlags_.test( Set::Saturation ) ) )
		fields[U( "sat" )] = wj::value::number( Saturation_ );
	if( ( comparisonState && ( XY_[0] != comparisonState->XY_[0] || XY_[1] != comparisonState->XY_[1] ) ) || ( !comparisonState && SetFlags_.test( Set::XY ) ) )
	{
		auto xy = wj::value::array( 2 );
		xy[0] = wj::value::number( XY_[0] );
		xy[1] = wj::value::number( XY_[1] );
		fields[U( "xy" )] = xy;
	}

	if( fields.size( ) == 0 )
		fields = wj::value::null( );
// 	else if( transitionTime != 0 )
// 		fields[U( "transitiontime" )] = wj::value::number( transitionTime );

	return fields;
}

namespace details
{

class Light_Impl
{
public:
	whc::http_client Client_;

	std::wstring Id_;
	std::wstring Name_;

	std::wstring	Type_;
	std::wstring	ModelId_;
	std::wstring	SwVersion_;
	// PointSymbol // Reserved

	Light::State 		CurrentState_;
	Light::State 		DirtyState_;

	Light_Impl( ) : Client_( U( "http://localhost" ) )
	{
	}

	Light_Impl( whc::http_client const & client, std::wstring const & id, wj::value const & light ) : Client_( client ), Id_( id )
	{
		Deserialize( light );
	}

	void Deserialize( wj::value const & light );
};

void Light_Impl::Deserialize( wj::value const & light )
{
	if( light.has_field( U( "name" ) ) )
		Name_ = light.at( U( "name" ) ).as_string( );
	if( light.has_field( U( "type" ) ) )
		Type_ = light.at( U( "type" ) ).as_string( );
	if( light.has_field( U( "modelid" ) ) )
		ModelId_ = light.at( U( "modelid" ) ).as_string( );
	if( light.has_field( U( "swversion" ) ) )
		SwVersion_ = light.at( U( "swversion" ) ).as_string( );

	if( light.has_field( U( "state" ) ) )
	{
		CurrentState_.Deserialize( light.at( U( "state" ) ) );
		DirtyState_ = CurrentState_;
	}
}

} // } namespace details

Light::Light( whc::http_client const & client, std::wstring const & id, wj::value const & light ) : Impl_( std::make_shared< Impl >( client, id, light ) )
{
}

ppl::task< void > Light::GetAttributesAndStateAsync( )
{
	auto impl = Impl_;

	// Make the request and asynchronously process the response. 
	return Impl_->Client_.request( wh::methods::GET, Impl_->Id_ ).then( 
		&GetValidatedJson ).then(
		[impl]( wj::value light)
	{
		impl->Deserialize( light );
	} );
}

Command::Instance Light::GetCommand( uint16_t const transitionTime ) const
{
	Command::Instance command;
	std::get< Command::Address >( command ) = Impl_->Client_.base_uri( ).path( ) + U( "/" ) + Impl_->Id_ + U( "/state" );
	std::get< Command::Method >( command ) = wh::methods::PUT;
	std::get< Command::Body >( command ) = GetCommandBody( transitionTime, true );

	return command;
}

Action::Instance Light::GetAction( uint16_t const transitionTime ) const
{
	Action::Instance action;
	std::get< Action::Address >( action ) = std::wstring( L"/lights" ) + U( "/" ) + Impl_->Id_ + U( "/state" );
	std::get< Action::Method >( action ) = wh::methods::PUT;
	std::get< Action::Body >( action ) = GetCommandBody( transitionTime, true );

	return action;
}

wj::value Light::GetCommandBody( uint16_t const transitionTime, bool const forSchedule ) const
{
	wj::value fields = Impl_->DirtyState_.Serialize( forSchedule ? nullptr : &Impl_->CurrentState_ );

	if( fields.size( ) == 0 )
		fields = wj::value::null( );
	else if( transitionTime != 0 )
		fields[U( "transitiontime" )] = wj::value::number( transitionTime );

	return fields;
}

ppl::task< void > Light::SetAttributesAsync( ) const
{
	auto body = [&]( )
	{
		wj::value fields;
		fields[U( "name" )] = wj::value::string( Impl_->Name_ );
		return fields;
	}( );

	if( body.is_null( ) )
	{
		return ppl::task_from_result( );
	}

	auto impl = Impl_;
	// Make the request and asynchronously process the response. 
	return impl->Client_.request( wh::methods::PUT, Impl_->Id_, body ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );
}

ppl::task< void > Light::SetStateAsync( uint16_t const transitionTime/*, std::wstring const & scene*/ ) const
{
	auto body = [&]( )
	{
// 		if( scene.length( ) == 0 )
			return GetCommandBody( transitionTime, false );

// 		wj::value fields;
// 		fields[U( "scene" )] = wj::value::string( scene );
// 		return fields;
	}( );

	if( body.is_null( ) )
	{
		return ppl::task_from_result( );
	}

	auto impl = Impl_;
	// Make the request and asynchronously process the response. 
	return impl->Client_.request( wh::methods::PUT, Impl_->Id_ + L"/state", body ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );
}

std::wstring Light::GetId( ) const
{
	return Impl_->Id_;
}

std::wstring Light::GetType( ) const
{
	return Impl_->Type_;
}

std::wstring Light::GetName( ) const
{
	return Impl_->Name_;
}

std::wstring Light::GetModelId( ) const
{
	return Impl_->ModelId_;
}

std::wstring Light::GetSwVersion( ) const
{
	return Impl_->SwVersion_;
}

Light::State Light::GetCurrentState( ) const
{
	return Impl_->CurrentState_;
}

void Light::SetName( std::wstring const & name )
{
	Impl_->Name_ = name;
}

void Light::SetState( Light::State const & state )
{
	Impl_->DirtyState_ = state;
}

Lights::Lights( whc::http_client const & client, wj::value const & lights ) : Lights( client )
{
	Deserialize( lights );
}

void Lights::Deserialize( wj::value const & lights )
{
	reserve( lights.size( ) );

	for( auto const & light : lights.as_object( ) )
	{
		if( light.first == U( "lastscan" ) )
			continue;

		emplace_back( Client_, light.first, light.second );
	}
}

ppl::task< void > Lights::FindNewAsync( std::vector< std::wstring > const & devices )
{
	if( devices.size( ) == 0 )
	{
		// Make the request and asynchronously process the response. 
		return Client_.request( wh::methods::POST ).then(
			&GetValidatedJson ).then(
			[this]( wj::value response )
		{
		} );
	}

	auto body = [&]( )
	{
		wj::value fields;

		fields[U( "devices" )] = wj::value::array( [&]( )
		{
			std::vector< wj::value > rv;
			for( auto const & device : devices )
			{
				rv.emplace_back( wj::value::string( device ) );
			}
			return rv;
		}( ) );

		return fields;
	}( );

	// Make the request and asynchronously process the response. 
	return Client_.request( wh::methods::POST, U( "" ), body ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );
}

}