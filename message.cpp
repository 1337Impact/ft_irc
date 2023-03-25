#include "message.hpp"
#include <string>
#include <sstream>

Message::Message(std::string &msg)
{
    std::vector<std::string> splited_msg;
    // split mgs
    std::stringstream ss(msg);
    std::string word;
    while (ss >> word) {
        splited_msg.push_back(word);
        // std::cout << word << std::endl;
    }
    if (splited_msg[0][0] == '!'){
        prefix = splited_msg[0];
        command = splited_msg[1];
        params.assign(splited_msg.begin() + 2, splited_msg.end());
    }
    else
    {
        command = splited_msg[0];
        params.assign(splited_msg.begin() + 1, splited_msg.end());
    }
}

int main()
{
    std::string par("cmd param1 param2");
    Message msg(par);

    std::cout << msg.command << std::endl;
    std::cout << msg.params[0] << std::endl;
    std::cout << msg.params[1] << std::endl;
}