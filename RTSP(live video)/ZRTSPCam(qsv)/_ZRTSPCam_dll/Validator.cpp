#include "Validator.h"
#include <regex>
#include <iostream>
#include <string>

Validator::Validator(void)
{
}

Validator::~Validator(void)
{
}
 
bool Validator::ValidateIpAddress(const std::string& ip)
{
#pragma warning (push, 0) 

   //[DmitryV] ipv6
   const std::tr1::regex patternIpv4("((0|[1-9][0-9]?|1[0-9]{1,2}|2[0-4][0-9]|25[0-5]|0+[1-3]?[0-7]{1,2}|0x0*[0-9a-f]{1,2})\.(0|[1-9][0-9]?|1[0-9]{1,2}|2[0-4][0-9]|25[0-5]|0+[1-3]?[0-7]{1,2}|0x0*[0-9a-f]{1,2})\.(0|[1-9][0-9]?|1[0-9]{1,2}|2[0-4][0-9]|25[0-5]|0+[1-3]?[0-7]{1,2}|0x0*[0-9a-f]{1,2})\.(0|[1-9][0-9]?|1[0-9]{1,2}|2[0-4][0-9]|25[0-5]|0+[1-3]?[0-7]{1,2}|0x0*[0-9a-f]{1,2}))|0x0*[0-9a-f]{1,6}|0+[1-3]?[0-7]{1,10}|429496729[0-5]|429496728[0-9]|42949671[0-9]{2}|4294966[0-9]{3}|429495[0-9]{4}|42944[0-9]{5}|4293[0-9]{6}|428[0-9]{7}|41[0-9]{8}|[1-3][0-9]{9}|[1-9][0-9]{0,8}");
   std::tr1::match_results<std::string::const_iterator> resultIpv4;
   bool validIpv4 = std::tr1::regex_match(ip, resultIpv4, patternIpv4);

   if(validIpv4)
   {
	   return true;
   }

   //[DmitryV] ipv6
   const std::tr1::regex patternIpv6("[0-9a-f]{0,4}:[0-9a-f]{0,4}:[0-9a-f]{0,4}:[0-9a-f]{0,4}:[0-9a-f]{0,4}:[0-9a-f]{0,4}:[0-9a-f]{0,4}:[0-9a-f]{0,4}|::|([0-9a-f]{0,4}:){1,7}:|[0-9a-f]{0,4}:[0-9a-f]{0,4}:[0-9a-f]{0,4}:[0-9a-f]{0,4}:[0-9a-f]{0,4}:[0-9a-f]{0,4}::[0-9a-f]{0,4}|[0-9a-f]{0,4}:[0-9a-f]{0,4}:[0-9a-f]{0,4}:[0-9a-f]{0,4}:[0-9a-f]{0,4}:(:[0-9a-f]{0,4}){1,2}|[0-9a-f]{0,4}:[0-9a-f]{0,4}:[0-9a-f]{0,4}:[0-9a-f]{0,4}:(:[0-9a-f]{0,4}){1,3}|[0-9a-f]{0,4}:[0-9a-f]{0,4}:[0-9a-f]{0,4}:(:[0-9a-f]{0,4}){1,4}|[0-9a-f]{0,4}:[0-9a-f]{0,4}:(:[0-9a-f]{0,4}){1,5}|[0-9a-f]{0,4}:(:[0-9a-f]{0,4}){1,6}|:(:[0-9a-f]{0,4}){1,7}(%[a-z0-9]+)?");
   std::tr1::match_results<std::string::const_iterator> resultIpv6;
   bool validIpv6 = std::tr1::regex_match(ip, resultIpv6, patternIpv6);

#pragma warning (pop)

   return validIpv6;
}