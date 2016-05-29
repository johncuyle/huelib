#pragma once

namespace Hue
{

class Configuration;
class Lights;
class Groups;
class Schedules;
class Scenes;
class Sensors;
class Rules;

struct User
{
	static uint32_t const DeviceType_Min_ = 0;
	static uint32_t const DeviceType_Max_ = 40;
	static uint32_t const UserName_Min_ = 10;
	static uint32_t const UserName_Max_ = 40;

	std::wstring DeviceType_;
	std::wstring UserName_;
};

struct Action
{
	enum Enum
	{
		Address,
		Method,
		Body
	};
	typedef std::tuple< std::wstring, wh::method, wj::value > Instance;	
};

typedef Action Command;

struct BridgeInfo
{
	wu::string_t Id_;
	wu::string_t Ip_;
	wu::string_t Mac_;

	template< typename String1, typename String2, typename String3 >
	BridgeInfo( String1 && id, String2 && ip, String3 && mac ) : 
		Id_( std::forward< String1 >( id ) ), 
		Ip_( std::forward< String2 >( ip ) ), 
		Mac_( std::forward< String3 >( mac ) )
	{
	}
};

struct State
{
	enum
	{
		Configuration,
		Lights,
		Groups,
		Schedules,
		Scenes,
		Sensors,
		Rules
	};

	typedef std::tuple< Hue::Configuration, Hue::Lights, Hue::Groups, Hue::Schedules, Hue::Scenes, Hue::Sensors, Hue::Rules > Tuple;
};

ppl::task< wj::value > GetValidatedJson( wh::http_response response );

}