#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <windows.h>
#ifndef CPPREST_FORCE_PPLX
#include <ppltasks.h>
#endif

#include <bitset>
#include <ctime>

#include <cpprest\json.h>
#include <cpprest\uri_builder.h>
#include <cpprest\http_client.h>

namespace Hue
{
	namespace wh = web::http;
	namespace whc = web::http::client;
	namespace wj = web::json;
	namespace wu = utility;
#ifndef CPPREST_FORCE_PPLX
	namespace ppl = concurrency;
#else
	namespace ppl = pplx;
#endif

}


