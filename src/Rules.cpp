#include "stdafx.h"
#include "Hue/Rules.hpp"


namespace Hue
{

wchar_t const * const Condition::OperatorName[static_cast<uint8_t>( Operator::Count )] = {
	L"eq",
	L"gt",
	L"lt",
	L"dx"
};


namespace details
{

class Rule_Impl
{
public:
	whc::http_client Client_;

	std::wstring Id_;
	std::wstring Name_;
	std::tm LastTriggered_;
	std::tm CreationTime_;
	uint32_t TimesTriggered_;
	std::wstring Owner_;
	std::wstring Status_;
	std::vector< Condition::Instance > Conditions_;
	std::vector< Action::Instance > Actions_;

	Rule_Impl( whc::http_client const & client, std::wstring const & id ) :Client_( client ), Id_( id ), TimesTriggered_( 0 )
	{
		std::time_t t = 0;
		gmtime_s( &LastTriggered_, &t );
		gmtime_s( &CreationTime_, &t );
	}

	Rule_Impl( ) : Rule_Impl( whc::http_client( U( "http://localhost" ) ), L"0" )
	{
	}

	Rule_Impl( whc::http_client const & client, std::wstring const & id, wj::value const & rule ) : Rule_Impl( client, id )
	{
		Deserialize( rule );
	}

	Rule_Impl( whc::http_client const & client, std::wstring const & name, std::vector< Condition::Instance > const & conditions, std::vector< Action::Instance > const & actions ) : 
		Client_( client ),
		Name_( name ),
		TimesTriggered_( 0 ),
		Conditions_( conditions ),
		Actions_( actions )
	{
	}

	void Deserialize( wj::value const & rule );
	wj::value Serialize( ) const;
};

void Rule_Impl::Deserialize( wj::value const & rule )
{
	if( rule.has_field( U( "name" ) ) )
		Name_ = rule.at( U( "name" ) ).as_string( );
	if( rule.has_field( U( "lasttriggered" ) ) )
	{
		std::wistringstream ss( rule.at( U( "lasttriggered" ) ).as_string( ) );
		ss >> std::get_time( &LastTriggered_, L"%Y-%m-%dT%H:%M:%S" );
	}
	if( rule.has_field( U( "creationtime" ) ) )
	{
		std::wistringstream ss( rule.at( U( "creationtime" ) ).as_string( ) );
		ss >> std::get_time( &CreationTime_, L"%Y-%m-%dT%H:%M:%S" );
	}
	if( rule.has_field( U( "timestriggered" ) ) )
		TimesTriggered_ = rule.at( U( "timestriggered" ) ).as_integer( );
	if( rule.has_field( U( "owner" ) ) )
		Owner_ = rule.at( U( "owner" ) ).as_string( );
	if( rule.has_field( U( "status" ) ) )
		Status_ = rule.at( U( "status" ) ).as_string( );

	if( rule.has_field( U( "conditions" ) ) )
	{
		for( auto const & condition : rule.at( U( "conditions" ) ).as_array( ) )
		{
			if( condition.has_field( U( "address" ) ) &&
				condition.has_field( U( "operator" ) ) )
			{
				Conditions_.emplace_back( 
					condition.at( U( "address" ) ).as_string( ),
					[&]( ) -> Condition::Operator
				{
					auto op = condition.at( U( "operator" ) ).as_string( );
					std::underlying_type< Condition::Operator >::type ty = 0;
					for( auto const & opname : Condition::OperatorName )
					{
						if( opname == op )
							break;
						++ty;
					}
					return ( ty == static_cast<uint32_t>( Condition::Operator::Count ) ) ? Condition::Operator::Unknown : static_cast<Condition::Operator>( ty );
				}( ), 
					( condition.has_field( U( "value" ) ) ? condition.at( U( "value" ) ).as_string( ) : std::wstring( ) ) );
			}
		}
	}

	if( rule.has_field( U( "actions" ) ) )
	{
		for( auto const & action : rule.at( U( "actions" ) ).as_array( ) )
		{
			if( action.has_field( U( "address" ) ) &&
				action.has_field( U( "method" ) ) &&
				action.has_field( U( "body" ) ) )
			{
				Actions_.emplace_back( 
					action.at( U( "address" ) ).as_string( ), 
					wh::method( action.at( U( "method" ) ).as_string( ) ),
					action.at( U( "body" ) ) );
			}
		}
	}
}

wj::value Rule_Impl::Serialize( ) const
{
	wj::value fields;

	if( Name_ != L"" )
		fields[U( "name" )] = wj::value::string( Name_ );

	{
		auto conditions = wj::value::array( Conditions_.size( ) );
		size_t ind = 0;
		for( auto const & condition : Conditions_ )
		{
			wj::value cond;

			cond[U( "address" )] = wj::value::string( std::get< Condition::Address >( condition ) );
			cond[U( "operator" )] = wj::value::string( Condition::OperatorName[static_cast<uint8_t const>( std::get< Condition::Op >( condition ) )] );
			if( std::get< Condition::Op >( condition )!= Condition::Operator::Changed )
				cond[U( "value" )] = wj::value::string( std::get< Condition::Value >( condition ) );

			conditions[ind] = cond;
			++ind;
		}
		fields[U( "conditions" )] = conditions;
	}

	{
		auto actions = wj::value::array( Actions_.size( ) );
		size_t ind = 0;
		for( auto const & action : Actions_ )
		{
			wj::value act;

			act[U( "address" )] = wj::value::string( std::get< Action::Address >( action ) );
			act[U( "method" )] = wj::value::string( std::get< Action::Method >( action ) );
			act[U( "body" )] = std::get< Action::Body >( action );

			actions[ind] = act;
			++ind;
		}
		fields[U( "actions" )] = actions;
	}

	return fields;
}

}

Rule::Rule( whc::http_client const & client, std::wstring const & id, wj::value const & rule ) : Impl_( std::make_shared< Impl >( client, id, rule ) )
{
}

ppl::task< void > Rule::GetAsync( )
{
	auto impl = Impl_;

	// Make the request and asynchronously process the response. 
	return Impl_->Client_.request( wh::methods::GET, Impl_->Id_ ).then(
		[]( wh::http_response response )
	{
		if( response.status_code( ) != wh::status_codes::OK )
		{
			throw wh::http_exception( response.status_code( ) );
		}

		auto bodyStream = response.body( );

		return response.content_ready( ).then(
			[]( wh::http_response response )
		{
			return response.extract_json( );
		} );
	} ).then(
		[impl]( wj::value light )
	{
		impl->Deserialize( light );
	} );
}

ppl::task< void > Rule::UpdateAsync( ) const
{
	auto body = Impl_->Serialize( );

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

std::wstring Rule::GetId( ) const
{
	return Impl_->Id_;
}

std::wstring Rule::GetName( ) const
{
	return Impl_->Name_;
}

std::tm const & Rule::GetLastTriggered( ) const
{
	return Impl_->LastTriggered_;
}

std::tm const & Rule::GetCreationTime( ) const
{
	return Impl_->CreationTime_;
}

uint32_t Rule::GetTimesTriggered( ) const
{
	return Impl_->TimesTriggered_;
}

std::wstring Rule::GetOwner( ) const
{
	return Impl_->Owner_;
}

std::wstring Rule::GetStatus( ) const
{
	return Impl_->Status_;
}

std::vector< Condition::Instance > const & Rule::GetConditions( ) const
{
	return Impl_->Conditions_;
}

std::vector< Action::Instance > const & Rule::GetActions( ) const
{
	return Impl_->Actions_;
}

void Rule::AddCondition( Condition::Instance const & condition )
{
	if( Impl_->Conditions_.size( ) == ConditionCount_Max_ )
		throw std::runtime_error( "Cannot add more conditions to rule." );

	Impl_->Conditions_.emplace_back( condition );
}

void Rule::AddAction( Action::Instance const & action )
{
	if( Impl_->Actions_.size( ) == ActionCount_Max_ )
		throw std::runtime_error( "Cannot add more actions to rule." );

	Impl_->Actions_.emplace_back( action );
}

void Rule::DeleteCondition( size_t const condition )
{
	if( Impl_->Conditions_.size( ) == 1 )
		throw std::runtime_error( "Cannot delete only condition on rule." );

	if( condition >= Impl_->Conditions_.size( ) )
		throw std::runtime_error( "Invalid condition to delete.  Reminder -delcon indexes start at 0." );

	Impl_->Conditions_.erase( Impl_->Conditions_.begin( ) + condition );
}

void Rule::DeleteAction( size_t const action )
{
	if( Impl_->Actions_.size( ) == 1 )
		throw std::runtime_error( "Cannot delete only action on rule." );

	if( action >= Impl_->Actions_.size( ) )
		throw std::runtime_error( "Invalid action to delete.  Reminder -delact indexes start at 0." );

	Impl_->Actions_.erase( Impl_->Actions_.begin( ) + action );
}

void Rule::SetName( std::wstring const & name )
{
	Impl_->Name_ = name;
}

Rules::Rules( whc::http_client const & client, wj::value const & rules ) : Rules( client )
{
	Deserialize( rules );
}

void Rules::Deserialize( wj::value const & rules )
{
	reserve( rules.size( ) );

	for( auto const & scene : rules.as_object( ) )
	{
		emplace_back( Client_, scene.first, scene.second );
	}
}

ppl::task< Rule > Rules::CreateAsync( std::wstring const & name, std::vector< Condition::Instance > const & conditions, std::vector< Action::Instance > const & actions ) const
{
	details::Rule_Impl const rule( Client_, name, conditions, actions );
	// Should be checking the array sizes here.
	auto body = rule.Serialize( );

	auto client = Client_;

	// Make the request and asynchronously process the response. 
	return client.request( wh::methods::POST, L"", body ).then(
		&GetValidatedJson ).then(
		[client]( wj::value response )
	{
		return Rule( client, L"", wj::value::string( L"{}" ) );
	} );
}

ppl::task< void > Rules::DeleteAsync( std::wstring const & id )
{
	// Make the request and asynchronously process the response. 
	return Client_.request( wh::methods::DEL, id ).then(
		&GetValidatedJson ).then(
		[this]( wj::value response )
	{
	} );
}


} // } namespace Hue 