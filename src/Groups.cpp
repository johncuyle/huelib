#include "stdafx.h"
#include "Hue/Groups.hpp"


namespace Hue
{

namespace details
{

class Group_Impl
{
public:
	whc::http_client Client_;

	std::wstring Id_;
	std::wstring Name_;
	std::vector< std::wstring > Lights_;

	Light::State 		LastState_;
	Light::State 		DirtyState_;

	Group_Impl( ) : Client_( U( "http://localhost" ) )
	{
	}

	Group_Impl( whc::http_client const & client, std::wstring const & id, wj::value const & group ) : Client_( client ), Id_( id )
	{
		Deserialize( group );
	}

	void Deserialize( wj::value const & group );
};

void Group_Impl::Deserialize( wj::value const & group )
{
	if( group.has_field( U( "name" ) ) )
		Name_ = group.at( U( "name" ) ).as_string( );
	if( group.has_field( U( "lights" ) ) )
	{
		Lights_.clear( );
		for( auto const & light : group.at( U( "lights" ) ).as_array( ) )
		{
			Lights_.emplace_back( light.as_string( ) );
		}
	}

	if( group.has_field( U( "action" ) ) )
	{
		LastState_.Deserialize( group.at( U( "action" ) ) );
		DirtyState_ = LastState_;
	}
}

}

Group::Group( whc::http_client const & client, std::wstring const & id, wj::value const & group ) : Impl_( std::make_shared< Impl >( client, id, group ) )
{
}

ppl::task< void > Group::GetAttributesAsync( )
{
	auto impl = Impl_;

	// Make the request and asynchronously process the response. 
	return Impl_->Client_.request( wh::methods::GET, Impl_->Id_ ).then(
		&GetValidatedJson ).then(
		[impl]( wj::value group )
	{
		impl->Deserialize( group );
	} );
}

Command::Instance Group::GetCommand( uint16_t const transitionTime, std::wstring const & scene ) const
{
	Command::Instance command;

	std::get< Command::Address >( command ) = Impl_->Client_.base_uri( ).path( ) + U( "/" ) + Impl_->Id_ + U( "/action" );
	std::get< Command::Method >( command ) = wh::methods::PUT;
	std::get< Command::Body >( command ) = GetCommandBody( transitionTime, scene, true );

	return command;
}

Action::Instance Group::GetAction( uint16_t const transitionTime, std::wstring const & scene ) const
{
	Action::Instance action;

	std::get< Action::Address >( action ) = std::wstring( L"/groups" ) + U( "/" ) + Impl_->Id_ + U( "/action" );
	std::get< Action::Method >( action ) = wh::methods::PUT;
	std::get< Action::Body >( action ) = GetCommandBody( transitionTime, scene, true );

	return action;
}

wj::value Group::GetCommandBody( uint16_t const transitionTime, std::wstring const & scene, bool const forSchedule ) const
{
	wj::value fields = [&]( )
	{
		if( scene.length( ) == 0 )
			return Impl_->DirtyState_.Serialize( forSchedule ? nullptr : &Impl_->LastState_ );

		wj::value fields;
		fields[U( "scene" )] = wj::value::string( scene );
		return fields;
	}( ); 

	if( fields.size( ) == 0 )
		fields = wj::value::null( );
	else if( transitionTime != 0 )
		fields[U( "transitiontime" )] = wj::value::number( transitionTime );

	return fields;
}

ppl::task< void > Group::SetStateAsync( uint16_t const transitionTime, std::wstring const & scene ) const
{
	auto body = GetCommandBody( transitionTime, scene, false );

	if( body.is_null( ) )
	{
		return ppl::task_from_result( );
	}

	auto impl = Impl_;
	// Make the request and asynchronously process the response. 
	return impl->Client_.request( wh::methods::PUT, Impl_->Id_ + L"/action", body ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );
}

ppl::task< void > Group::SetAttributesAsync( ) const
{
	auto body = [&]()
	{
		wj::value fields;

		fields[U( "name" )] = wj::value::string( Impl_->Name_ );
		fields[U( "lights" )] = wj::value::array( [&]( )
		{
			std::vector< wj::value > rv;
			for( auto const & light : Impl_->Lights_ )
			{
				rv.emplace_back( wj::value::string( light ) );
			}
			return rv;
		}( ) );

		return fields;
	}( );

	auto impl = Impl_;
	// Make the request and asynchronously process the response. 
	return impl->Client_.request( wh::methods::PUT, Impl_->Id_, body ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );
}


std::wstring Group::GetId( ) const
{
	return Impl_->Id_;
}

std::wstring Group::GetName( ) const
{
	return Impl_->Name_;
}

std::vector< std::wstring > Group::GetLights( ) const
{
	return Impl_->Lights_;
}

Light::State Group::GetLastState( ) const
{
	return Impl_->LastState_;
}

void Group::SetState( Light::State const & state )
{
	Impl_->DirtyState_ = state;
}

void Group::SetName( std::wstring const & name )
{
	Impl_->Name_ = name;
}

void Group::SetLights( std::vector< std::wstring > const & lights )
{
	Impl_->Lights_ = lights;
}

Groups::Groups( whc::http_client const & client, wj::value const & groups ) : Groups( client )
{
	Deserialize( groups );
}

void Groups::Deserialize( wj::value const & groups )
{
	reserve( groups.size( ) + 1 );
	emplace_back( Client_, L"0", wj::value::parse( U( "{\"name\":\"Lightset 0\"}" ) ) );
	for( auto const & group : groups.as_object( ) )
	{
		emplace_back( Client_, group.first, group.second );
	}
}

ppl::task< void > Groups::CreateAsync( std::wstring const & name, std::vector< std::wstring > const & lights )
{
	auto body = [&]()
	{
		wj::value fields;

		fields[U( "name" )] = wj::value::string( name );
		fields[U( "lights" )] = wj::value::array( [&]( )
		{
			std::vector< wj::value > rv;
			for( auto const & light : lights )
			{
				rv.emplace_back( wj::value::string( light ) );
			}
			return rv;
		}( ) );

		return fields;
	}( );

	return Client_.request( wh::methods::POST, U( "" ), body ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );

}

ppl::task< void > Groups::DeleteAsync( std::wstring const & id )
{
	// Make the request and asynchronously process the response. 
	return Client_.request( wh::methods::DEL, id ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );
}

} // } namespace Hue 