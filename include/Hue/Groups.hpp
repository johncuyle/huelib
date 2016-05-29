#pragma once

#include "Hue\Lights.hpp"
#include "Hue\Schedules.hpp"

namespace Hue
{

class Schedule;

namespace details
{
class Group_Impl;
}

class Group
{
public:

private:
	typedef details::Group_Impl Impl;

private:
	std::shared_ptr< Impl > Impl_;

public:

	Group( )
	{
	}

	Group( whc::http_client const & client, std::wstring const & id, wj::value const & group );

	ppl::task< void > GetAttributesAsync( );
	ppl::task< void > SetStateAsync( uint16_t const transitionTime, std::wstring const & scene = std::wstring( ) ) const;
	ppl::task< void > SetAttributesAsync( ) const;

	Command::Instance GetCommand( uint16_t const transitionTime, std::wstring const & scene = std::wstring( ) ) const;
	Action::Instance GetAction( uint16_t const transitionTime, std::wstring const & scene = std::wstring( ) ) const;

	std::wstring GetId( ) const;
	std::wstring GetName( ) const;
	std::vector< std::wstring > GetLights( ) const;
	Light::State GetLastState( ) const;
	void SetState( Light::State const & state );
	void SetName( std::wstring const & name );
	void SetLights( std::vector< std::wstring > const & lights );

private:
	wj::value GetCommandBody( uint16_t const transitionTime, std::wstring const & scene, bool const forSchedule ) const;
};

class Groups : public std::vector< Group >
{
private:
	whc::http_client Client_;

public:
	Groups( ) : Client_( U( "http://localhost" ) )
	{}

	Groups( whc::http_client const & client ) : Client_( client )                         
	{
	}

	Groups( whc::http_client const & client, wj::value const & groups );
	
	void Deserialize( wj::value const & groups );

	ppl::task< void > CreateAsync( std::wstring const & name, std::vector< std::wstring > const & lights );
	ppl::task< void > DeleteAsync( std::wstring const & id );
};
}