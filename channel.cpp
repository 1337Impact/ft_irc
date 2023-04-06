#include "ircserv.hpp"

Channel::Channel(const std::string &name, User *usr)
	: name(name), limit(100), priv(false), secret(false), inviteOnly(false),
	  protectedTopic(true), externalMessages(false), moderated(false)
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

const Message Channel::addOperator(const std::string &target, const bool give)
{
	User *usr = lookUpUser(target);
	if (!usr)
		return Message(441).addParam(target).addParam(name).addParam(
			":They aren't on that channel");
	(void)give;
	return Message();
}

const Message Channel::setClientLimit(const std::string &limit)
{
	(void)limit;
	return Message();
}

const Message Channel::setBanMask(const std::string &mask)
{
	(void)mask;
	return Message();
}

const Message Channel::setSpeaker(const std::string &user)
{
	(void)user;
	return Message();
}

const Message Channel::setKey(const std::string &key)
{
	(void)key;
	return Message();
}