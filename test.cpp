#include <iostream>
#include <sstream>
#include <map>
#include <utility>
int main()
{
    std::string req("channel1,channel2,channel3");
    std::string req2("key1,key2");
	std::map<std::string, std::string> params;
	std::istringstream _ss(req);
	std::istringstream _ss2(req2);
	std::string _ch;
	std::string _key;
    _ss.widen(',');

	while(std::getline(_ss, _ch, ',')) {
        std::cout << _ch << "  ";
        std::cout << (std::getline(_ss2, _key, ',') ? _key : "no key") << std::endl;
    }
}