/* 
 * File:   Exc.hpp
 * Author: ivan
 *
 * Created on January 5, 2016, 4:07 AM
 */

#pragma once

#include <utki/Exc.hpp>

namespace audout{

class Exc : public utki::Exc{
public:
	Exc(const std::string& message) :
			utki::Exc(message)
	{}
};

}
