#pragma once

#include "Hue\Lights.hpp"

namespace Hue
{

class Rule;

struct Condition
{
	enum class Operator : uint8_t
	{
		Equals,
		GreaterThan,
		LessThan,
		Changed,
		Unknown,

		Count,
		Last = Count - 1,
		First = 0
	};

	static wchar_t const * const OperatorName[static_cast<uint8_t>( Operator::Count )];

	enum Enum
	{
		Address,
		Op,
		Value
	};
	
	typedef std::tuple< std::wstring, Operator, std::wstring > Instance;
};

namespace details
{
class Rule_Impl;
}

class Rule
{
public:
	static uint32_t const ConditionCount_Max_ = 8;
	static uint32_t const ActionCount_Max_ = 8;
private:
	typedef details::Rule_Impl Impl;

private:
	std::shared_ptr< Impl > Impl_;

public:

	Rule( )
	{
	}

	Rule( whc::http_client const & client, std::wstring const & id, wj::value const & rule );

	ppl::task< void > GetAsync( );
	ppl::task< void > UpdateAsync( ) const;

	std::wstring GetId( ) const;
	std::wstring GetName( ) const;
	std::tm const & GetLastTriggered( ) const;
	std::tm const & GetCreationTime( ) const;
	uint32_t GetTimesTriggered( ) const;
	std::wstring GetOwner( ) const;
	std::wstring GetStatus( ) const;
	std::vector< Condition::Instance > const & GetConditions( ) const;
	std::vector< Action::Instance > const & GetActions( ) const;

	void AddCondition( Condition::Instance const & condition );
	void AddAction( Action::Instance const & action );

	void DeleteCondition( size_t const condition );
	void DeleteAction( size_t const action );

	void SetName( std::wstring const & name );

private:
	wj::value GetCommandBody( ) const;
};

class Rules : public std::vector < Rule >
{
private:
	whc::http_client Client_;

public:
	Rules( ) : Client_( U( "http://localhost" ) )
	{
	}

	Rules( whc::http_client const & client ) : Client_( client )
	{
	}

	Rules( whc::http_client const & client, wj::value const & rules );

	void Deserialize( wj::value const & rules );

	ppl::task< Rule > CreateAsync( std::wstring const & name, std::vector< Condition::Instance > const & conditions, std::vector< Action::Instance > const & actions ) const;
	ppl::task< void > DeleteAsync( std::wstring const & id );
};

}