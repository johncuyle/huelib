#pragma once

#include "Hue\Shared.hpp"

namespace Hue
{

class Bridge
{
private:
	std::shared_ptr< BridgeInfo > BridgeInfo_;

public:
	Bridge( ) : BridgeInfo_( std::make_shared< BridgeInfo >( U( "" ), U( "" ), U( "" ) ) )
	{
	}

	template< typename String1, typename String2, typename String3 >
	Bridge( String1 && id, String2 && ip, String3 && mac ) :
		BridgeInfo_( std::make_shared< BridgeInfo >( std::forward< String1 >( id ), std::forward< String2 >( ip ), std::forward< String3 >( mac ) ) )
	{
	}

	inline BridgeInfo const & Info() const
	{
		return *( BridgeInfo_.get( ) );
	}

	static ppl::task< std::vector< Bridge > > DiscoverBridgesAsync( whc::web_proxy const & proxy );

	ppl::task< Configuration > GetConfigurationAsync( whc::web_proxy const & proxy, User const & user ) const;
	ppl::task< Lights > GetLightsAsync( whc::web_proxy const & proxy, User const & user ) const;
	ppl::task< Lights > GetNewLightsAsync( whc::web_proxy const & proxy, User const & user ) const;
	ppl::task< Groups > GetGroupsAsync( whc::web_proxy const & proxy, User const & user ) const;
	ppl::task< Schedules > GetSchedulesAsync( whc::web_proxy const & proxy, User const & user ) const;
	ppl::task< Scenes > GetScenesAsync( whc::web_proxy const & proxy, User const & user ) const; // 1.1
	ppl::task< Sensors > GetSensorsAsync( whc::web_proxy const & proxy, User const & user ) const; //1.3
	ppl::task< Sensors > GetNewSensorsAsync( whc::web_proxy const & proxy, User const & user ) const; //1.3
	ppl::task< Rules > GetRulesAsync( whc::web_proxy const & proxy, User const & user ) const; //1.3
	ppl::task< State::Tuple > GetStateAsync( whc::web_proxy const & proxy, User const & user ) const; // this is a 1.3 version
	ppl::task< std::vector< std::wstring > > GetTimeZonesAsync( whc::web_proxy const & proxy, User const & user ) const; // 1.2.1

private:
	whc::http_client MakeClient( User const & user, wchar_t const * const subpath, whc::web_proxy const & proxy ) const;
};

}