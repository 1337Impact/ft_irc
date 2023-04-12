#ifndef CHANNEL_CPP
#define CHANNEL_CPP

#include "ircserv.hpp"

Channel::Channel(const std::string &name, User &usr, std::string key)
	: name(name), modes(ProtectedTopicMask | NoExternalMessagesMask),
	  limit(100), key(key)
{
	ChannelMember newMember(usr);
	newMember.is_oper = true;
	usr.nchannels++;
	if (!key.empty())
		modes |= SecretKeyMask;
	members.push_back(newMember);
}

int Channel::join(User &usr, std::string _key)
{
	if (this->isMember(usr))
	{
		std::cout << "User is already a member" << std::endl;
		return (4);
	}
	if (isInviteOnly() &&
		std::find(invited.begin(), invited.end(), &usr) == invited.end())
		return (1);
	if (hasSecretKey() && !key.empty() && key != _key)
		return (2);
	if (hasLimit() && members.size() == limit)
		return (isSecret() || isPrivate() ? 6 : 3);
	if (find(banMasks.begin(), banMasks.end(), usr.nickname) != banMasks.end())
		return (isSecret() || isPrivate() ? 6 : 5);
	else
	{
		usr.nchannels++;
		members.push_back(ChannelMember(usr));
		std::vector<User *>::iterator kicked =
			find(invited.begin(), invited.end(), &usr);
		if (kicked != invited.end())
			invited.erase(kicked);
		return (0);
	}
}
const Channel &Channel::broadcast(const std::string &res, const User &skip) const
{
	std::vector<ChannelMember>::const_iterator it;
	for (it = members.begin(); it != members.end(); it++)
		if (skip.nickname != (*it).usr.nickname)
			if (send(it->usr.fd, res.data(), res.size(), 0) == -1)
				BlockingError("send");
	return *this;
}

std::string Channel::get_memebers(void)
{
	std::string membersList;
	std::vector<ChannelMember>::iterator it;
	for (it = members.begin(); it != members.end(); it++)
	{
		if (it->is_oper)
			membersList += '@';
		membersList += it->usr.nickname;
		membersList += ' ';
	}
	return (membersList);
}

User *Channel::lookUpUser(const std::string &nick)
{
	for (std::vector<ChannelMember>::iterator it = members.begin();
		 it != members.end();
		 it++)
		if (nick == (*it).usr.nickname)
			return &it->usr;
	return (nullptr);
}

bool Channel::isMember(const User &user) const
{
	for (std::vector<ChannelMember>::const_iterator it = members.begin();
		 it != members.end();
		 it++)
		if (user.nickname == it->usr.nickname)
			return true;
	return false;
}
void Channel::remove(User *user)
{
	members.erase(std::find(members.begin(), members.end(), *user));
	bool oper_found = false;
	for (std::vector<ChannelMember>::iterator it = members.begin();
		 it != members.end();
		 it++)
	{
		if (it->is_oper)
		{
			oper_found = true;
			break;
		}
	}
	if (!oper_found)
		if (members.begin() != members.end())
			members.begin()->is_oper = true;
	user->nchannels--;
}
std::string Channel::getChannelModes() const
{
	std::string modes;
	return modes;
}
bool Channel::isOperator(const User &usr) const
{
	for (std::vector<ChannelMember>::const_iterator it = members.begin();
		 it != members.end();
		 it++)
		if (usr.nickname == it->usr.nickname)
			return it->is_oper;
	return false;
}
bool Channel::isSpeaker(const User &user) const
{
	return speakers.end() != find(speakers.begin(), speakers.end(), &user);
}
const Message Channel::addOperator(const std::string &target, const bool add)
{
	std::vector<ChannelMember>::iterator mem =
		find(members.begin(), members.end(), target);
	if (mem == members.end())
		return Message(441).addParam(target).addParam(name).addParam(
			":They aren't on that channel");
	std::cout << "param == " << add << std::endl;
	mem->is_oper = add;
	return add
		? Message().setCommand("REPLY").addParam("Operator has been added")
		: Message().setCommand("REPLY").addParam("Operator has been removed");
}
const Message Channel::setChannelLimit(const std::string &limit, const bool add)
{
	ssize_t val = atoi(limit.data());
	if (!add)
	{
		this->limit = members.size();
		return Message(696).addParam(name).addParam("l").addParam(
			":limit has been removed");
	}
	if (val >= (ssize_t)members.size())
		this->limit = val;
	else
		return Message(696).addParam(name).addParam("l").addParam(limit).addParam(
			":limit is below current members count");
	return Message()
		.setCommand("REPLY")
		.addParam("Limit has been set to")
		.addParam(limit);
}
const Message Channel::setBanMask(const std::string &mask, const bool add)
{
	if (add)
	{
		if (find(banMasks.begin(), banMasks.end(), mask) == banMasks.end())
		{
			banMasks.push_back(mask);
			return Message().setCommand("REPLY").addParam(
				"Ban mask has been added");
		}
		return Message().setCommand("REPLY").addParam("sorry mask already exists");
	}
	else
	{
		banMasks.erase(find(banMasks.begin(), banMasks.end(), mask));
		return Message().setCommand("REPLY").addParam("Ban mask has been removed");
	}
	return Message();
}

const Message Channel::setSpeaker(const std::string &user, const bool add)
{
	if (add)
	{
		if (User *mem = lookUpUser(user))
			speakers.insert(mem);
		return Message(401).addParam(name).addParam("No such nick/channel");
	}
	else
	{
		if (User *mem = lookUpUser(user))
			speakers.erase(std::find(speakers.begin(), speakers.end(), mem));
		return Message(401).addParam(name).addParam("No such nick/channel");
	}
	return Message().setCommand("REPLY").addParam("Speaker has been added");
}

const Message Channel::setSecret(const std::string &key, const bool add)
{
	if (add)
	{
		if (!this->key.empty())
			return Message().setCommand("REPLY").addParam(
				"Key can only be set once");
		this->key = key;
		return Message().setCommand("REPLY").addParam("Secret has been set");
	}
	this->key.clear();
	return Message().setCommand("REPLY").addParam("Secret has been unset");
}
bool Channel::isValidName(const std::string &name)
{
	if ((name[0] != '#' && name[0] != '&') || name.size() > 200 ||
		find(name.begin(), name.end(), 7) != name.end())
		return (0);
	return (1);
}

#endif