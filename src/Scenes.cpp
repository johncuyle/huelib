#include "stdafx.h"
#include "Hue/Scenes.hpp"

namespace Hue
{

namespace details
{

class Scene_Impl
{
public:
	whc::http_client Client_;

	std::wstring Id_;
	std::wstring Name_;
	std::vector< std::wstring > Lights_;
	bool Active_;

	Scene_Impl( ) : Client_( U( "http://localhost" ) ), Active_( false )
	{
	}

	Scene_Impl( whc::http_client const & client, std::wstring const & id, wj::value const & scene ) : Client_( client ), Id_( id ), Active_( false )
	{
		Deserialize( scene );
	}

	void Deserialize( wj::value const & scene );
};

void Scene_Impl::Deserialize( wj::value const & scene )
{
	if( scene.has_field( U( "name" ) ) )
		Name_ = scene.at( U( "name" ) ).as_string( );

	if( scene.has_field( U( "lights" ) ) )
	{
		Lights_.clear( );
		for( auto const & light : scene.at( U( "lights" ) ).as_array( ) )
		{
			Lights_.emplace_back( light.as_string( ) );
		}
	}

	if( scene.has_field( U( "active" ) ) )
		Active_ = scene.at( U( "active" ) ).as_bool( );
}

} // } namespace details

Scene::Scene( whc::http_client const & client, std::wstring const & id, wj::value const & scene ) : Impl_( std::make_shared< Impl >( client, id, scene ) )
{
}

ppl::task< void > Scene::ModifyAsync( uint16_t const transitionTime, std::wstring const & light, Light::State const & state ) const
{
	auto body = state.Serialize( nullptr );

	if( body.is_null( ) )
	{
		return ppl::task_from_result( );
	}
	else if( transitionTime != 0 )
		body[U( "transitiontime" )] = wj::value::number( transitionTime );

	auto impl = Impl_;
	// Make the request and asynchronously process the response. 
	return impl->Client_.request( wh::methods::PUT, Impl_->Id_ + U( "/lights/" ) + light + U( "/state" ), body ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );
}

std::wstring Scene::GetId( ) const
{
	return Impl_->Id_;
}

std::wstring Scene::GetName( ) const
{
	return Impl_->Name_;
}

std::vector< std::wstring > Scene::GetLights( ) const
{
	return Impl_->Lights_;
}

bool Scene::GetActive( ) const
{
	return Impl_->Active_;
}

void Scene::SetName( std::wstring const & name )
{
	Impl_->Name_ = name;
}

Scenes::Scenes( whc::http_client const & client, wj::value const & scenes ) : Scenes( client )
{
	Deserialize( scenes );
}

void Scenes::Deserialize( wj::value const & scenes )
{
	reserve( scenes.size( ) );

	for( auto const & scene : scenes.as_object( ) )
	{
		emplace_back( Client_, scene.first, scene.second );
	}
}

ppl::task< Scene > Scenes::CreateAsync( std::wstring const & name, std::vector< std::wstring > const & lights ) const
{
	auto body = [&]( )
	{
		wj::value fields;

		if( name != L"" )
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

	auto client = Client_;

	return client.request( wh::methods::PUT, L"381agd", body ).then(
		&GetValidatedJson ).then(
		[client]( wj::value response )
	{
		return Scene( client, L"", wj::value::string( L"{}" ) );
	} );
}

ppl::task< void > Scenes::DeleteAsync( std::wstring const & id )
{
	return Client_.request( wh::methods::DEL, id ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );
}

} // } namespace Hue 