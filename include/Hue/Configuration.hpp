#pragma once

#include "Hue\Shared.hpp"

namespace Hue
{

class Bridge;
class Configuration;

struct ReadOnlyState
{
	struct WhitelistEntry
	{
		std::tm LastUseDate_;
		std::tm CreateDate_;
		std::wstring Name_;
	};
	typedef std::map< std::wstring, WhitelistEntry > Whitelist;

	Whitelist Whitelist_;
	uint32_t ApiVersion_; // API 1.2.1 // May want to split this into a major/minor/patch
	std::wstring SwVersion_;
	std::wstring Mac_;
	std::tm Utc_; 
	std::tm LocalTime_; // API 1.2.1
	uint16_t ZigbeeChannel_; // API 1.3

	ReadOnlyState()
	{
		std::time_t t = 0;
		gmtime_s( &Utc_, &t );
		localtime_s( &LocalTime_, &t );
	}

	void Deserialize( wj::value const & config );
};

struct MutableState
{
	struct SwUpdate
	{
		struct State
		{
			enum Enum
			{
				None = 0,
				Downloading = 1,
				Available = 2,
				Updating = 3,
				Count,
				Last = Count - 1
			};
		};

		State::Enum State_;
		std::wstring Url_;
		std::wstring Text_;
		bool Notify_;

		SwUpdate() : State_( State::None ), Notify_(0)
		{
		}

		void Deserialize( wj::value const & config );
	};

	std::wstring Name_;
	SwUpdate SwUpdate_;
	wh::uri Proxy_; // this is the proxy address and port set on the bridge itself.
	bool LinkButton_;
	std::wstring IpAddress_;
	std::wstring NetMask_;
	std::wstring Gateway_;
	bool Dhcp_;
	bool PortalServices_;
	std::wstring TimeZone_; // API 1.2.1

	MutableState() : LinkButton_( false ), Dhcp_( false ), PortalServices_( false )
	{
	}

	void Deserialize( wj::value const & config );
};

namespace details
{

class Configuration_Impl
{
public:
	whc::http_client Client_;

	ReadOnlyState ReadOnlyState_;
	MutableState  CurrentState_;
	MutableState  DirtyState_;

	Configuration_Impl( whc::http_client const & client ) : Client_( client )
	{
	}

	void Deserialize( wj::value const & config );
};

}

class Configuration
{
private:
	typedef details::Configuration_Impl Impl;

private:
	std::shared_ptr< Impl > Impl_;

public:
	Configuration( ) 
	{
	}

	Configuration( whc::http_client const & client ) : Impl_( std::make_shared< Impl >( client ) )
	{}

	Configuration( whc::http_client const & client, wj::value const & current_config ) : Configuration( client )
	{
		Impl_->Deserialize( current_config );
	}

	bool UserWhitelisted( User const & user ) const
	{
		for( auto const & entry : Impl_->ReadOnlyState_.Whitelist_ )
		{
			if( user.UserName_ == entry.first )
				return true;
		}

		return false;
	}

	inline bool UpdateAvailable( ) const
	{
		return Impl_->CurrentState_.SwUpdate_.State_ == MutableState::SwUpdate::State::Available;
	}

	inline bool DownloadUpdate( ) const
	{
// 		if( Impl_->CurrentState_.SwUpdate_.State_ != MutableState::SwUpdate::State::Available )
// 			return false;

		Impl_->DirtyState_.SwUpdate_.State_ = MutableState::SwUpdate::State::Updating;
		return true;
	}

	// Hue API
	ppl::task< void > CreateUserAsync( User const & user );
	ppl::task< void > DeleteUserAsync( User const & user );

	ppl::task< void > GetAsync( );
	ppl::task< void > ModifyAsync( );

	// Accessors
	uint32_t GetApiVersion( ) const
	{
		return Impl_->ReadOnlyState_.ApiVersion_;
	}

	ReadOnlyState GetReadOnlyState() const
	{
		return Impl_->ReadOnlyState_;
	}

	MutableState GetCurrentState( ) const
	{
		return Impl_->CurrentState_;
	}

	MutableState GetPendingState( ) const
	{
		return Impl_->DirtyState_;
	}

	void SetPendingState( MutableState state )
	{
		Impl_->DirtyState_ = state;
	}

};

}