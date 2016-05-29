#pragma once

#include "Hue\Lights.hpp"

namespace Hue
{

namespace details
{
class Scene_Impl;
} // } namespace details

class Scene
{
private:
	typedef details::Scene_Impl Impl;

private:
	std::shared_ptr< Impl > Impl_;

public:
	Scene( )
	{
	}

	Scene( whc::http_client const & client, std::wstring const & id, wj::value const & scene );
	ppl::task< void > ModifyAsync( uint16_t const transitionTime, std::wstring const & light, Light::State const & state ) const;

	std::wstring GetId( ) const;
	std::wstring GetName( ) const;
	std::vector< std::wstring > GetLights( ) const;
	bool GetActive( ) const;

	void SetName( std::wstring const & name );
};

class Scenes : public std::vector < Scene >
{
private:
	whc::http_client Client_;

public:
	Scenes( ) : Client_( U( "http://localhost" ) )
	{
	}

	Scenes( whc::http_client const & client ) : Client_( client )
	{
	}

	Scenes( whc::http_client const & client, wj::value const & scenes );

	void Deserialize( wj::value const & scenes );

	ppl::task< Scene > CreateAsync( std::wstring const & name, std::vector< std::wstring > const & lights ) const;
	// Supposedly does not exist.
	ppl::task< void > DeleteAsync( std::wstring const & id );
};

} // } namespace Hue
