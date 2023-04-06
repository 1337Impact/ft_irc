#include "ircserv.hpp"

Channel::Channel(const std::string &name, User *usr)
	: externalMessages(false), inviteOnly(false), moderated(false), priv(false),
	  protectedTopic(true), secret(false), name(name), limit(100)
{
	members.insert(usr);
}

int Channel::join(User *usr)
{
	members.insert(usr);
	return (0);
}

void Channel::broadcast(const std::string &res, const User &skip) const
{
	for (std::set<User *>::const_iterator it = members.cbegin();
		 it != members.cend();
		 it++)
		if (skip.username != (*it)->username)
			Send((*it)->fd, res.data(), res.size());
}

User *Channel::lookUpUser(const std::string &nick)
{
	for (std::set<User *>::iterator usr = members.begin(); usr != members.end();
		 usr++)
		if (nick == (*usr)->nickname)
			return *usr;
	return (nullptr);
}

void Channel::disjoin(User *user)
{
	members.erase(std::find(members.cbegin(), members.cend(), user));
}

bool Channel::isMember(User *user) const
{
	return members.cend() == std::find(members.cbegin(), members.cend(), user);
}

std::string Channel::getChannelModes() const
{
	std::string modes;
	return modes;
}

bool Channel::isOperator(const User &usr) const
{
	for (std::set<User *>::const_iterator mem = members.cbegin();
		 mem != members.cend();
		 mem++)
		if (usr.username == (*mem)->username)
			return true;
	// this commit do not have ChannelModes struct yet
	return false;
}

const Message Channel::addOperator(const std::string &target, const bool add)
{
	User *usr = lookUpUser(target);
	if (!usr)
		return Message(441).addParam(target).addParam(name).addParam(
			":They aren't on that channel");
	(void)add;
	return Message();
}

const Message Channel::setClientLimit(const std::string &limit)
{
	int val = atoi(limit.data());
	if (val > 0)
		this->limit = val;
	return Message();
}

const Message Channel::setBanMask(const std::string &mask, const bool add)
{
	return add ? banMasks.push_back(mask)
			   : (void)banMasks.erase(
					 find(banMasks.begin(), banMasks.end(), mask)),
		   Message();
}

const Message Channel::setSpeaker(const std::string &user, const bool add)
{
	if (add)
	{
		if (User *usr = lookUpUser(user))
			speakers.insert(usr);
		return Message(401).addParam(name).addParam(":No such nick/channel");
	}
	else
	{
		if (User *usr = lookUpUser(user))
			speakers.erase(std::find(speakers.cbegin(), speakers.cend(), usr));
		return Message(401).addParam(name).addParam(":No such nick/channel");
	}
	return Message();
}