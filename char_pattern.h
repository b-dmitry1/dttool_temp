#pragma once

#include <list>
#include <string>

struct char_pattern_range
{
	char first;
	char last;
};

class char_pattern_group
{
public:
	char_pattern_group()
	{
		_any = false;
		_remove = false;
	}

	void add(char first, char last, bool any, bool remove)
	{
		_any = any;
		_remove = remove;
		_ranges.push_back(new char_pattern_range{ first, last });
	}

	std::list<char_pattern_range*>& ranges()
	{
		return _ranges;
	}

	bool check(char ch, bool& remove)
	{
		for (auto r : _ranges)
		{
			remove = _remove;
			if (r->first == 0x7F)
				return true;
			if (ch >= r->first && ch <= r->last)
				return true;
		}
		remove = false;
		return false;
	}

	bool any()
	{
		return _any;
	}
private:
	std::list<char_pattern_range*> _ranges;
	bool _any;
	bool _remove;

	friend class char_pattern;
};

class char_pattern
{
public:
	char_pattern(const char* pat)
	{
		char_pattern_group* group = NULL;
		auto any = false;
		auto one_of = false;
		auto spec = false;

		_use_backslashes = true;

		while (*pat)
		{
			if (!spec && !any && !one_of && pat[0] == '[' && pat[1] != ']' && pat[1] != 0)
			{
				any = true;
				pat++;
				continue;
			}
			if (!spec && any && *pat == ']')
			{
				any = false;
				pat++;
				group = NULL;
				continue;
			}

			if (!spec && !any && !one_of && pat[0] == '(' && pat[1] != ')' && pat[1] != 0)
			{
				one_of = true;
				pat++;
				continue;
			}
			if (!spec && one_of && *pat == ')')
			{
				one_of = false;
				pat++;
				if (*pat == '\b')
				{
					group->_remove = true;
					pat++;
				}
				group = NULL;
				continue;
			}

			if (*pat == '\\' && !spec)
			{
				spec = true;
				pat++;
				continue;
			}

			if (group == NULL)
			{
				group = new char_pattern_group();
				_groups.push_back(group);
			}

			if (!spec && pat[0] == '[' && pat[1] == ']')
			{
				group->add(0x7F, 0x7F, true, false);
				pat += 2;
			}
			else if (!spec && pat[0] == '^')
			{
				group->add(1, pat[1] - 1, any, false);
				group->add(pat[1] + 1, 0x7F, any, false);
				pat += 2;
			}
			else if (pat[1] == '-' && pat[2] && pat[2] != ']' && pat[2] != ')')
			{
				group->add(pat[0], pat[2], any, false);
				pat += 3;
			}
			else
			{
				group->add(*pat, *pat, any, pat[1] == '\b');
				pat++;
				if (*pat == '\b')
					pat++;
			}
			if (!any && !one_of)
				group = NULL;
			spec = false;
		}
	}

	~char_pattern()
	{
		for (auto it = std::begin(_groups); it != std::end(_groups); ++it)
			delete (*it);
	}

	bool check(std::string::iterator str_it, const std::string::iterator str_end, std::string& res, size_t& advance)
	{
		return check(str_it, str_end, res, advance, std::begin(_groups));
	}

	bool check(std::string::iterator str_it, const std::string::iterator str_end, std::string& res, size_t& advance, std::list<char_pattern_group*>::iterator group)
	{
		auto match = false;

		res.clear();
		advance = 0;

		if (str_it == str_end)
			return false;

		for (auto it = group; it != std::end(_groups); ++it)
		{
			auto g = *it;

			if (str_it == str_end)
				return false;

			auto remove = false;
			if (!g->check(*str_it, remove))
				return false;

			char last = *str_it;
			if (!remove)
				res += *str_it;
			advance++;
			str_it++;

			if (g->any())
			{
				while (str_it != str_end)
				{
					if (last == '\\' && _use_backslashes)
					{
						res += *str_it;
						advance++;
						str_it++;
						last = 0;
						continue;
					}

					if (!g->check(*str_it, remove))
						break;

					last = *str_it;

					std::string res_next;
					std::string::iterator str_it_next = str_it;
					std::list<char_pattern_group*>::iterator it_next = it;
					++it_next;
					if (it_next != std::end(_groups))
					{
						size_t advance_next = 0;
						if (check(str_it_next, str_end, res_next, advance_next, it_next))
						{
							res += res_next;
							advance += advance_next;
							return true;
						}
					}
					res += *str_it;
					advance++;
					str_it++;
				}
			}
		}

		return true;
	}

	void use_backslashes(bool value)
	{
		_use_backslashes = value;
	}
private:
	std::list<char_pattern_group*> _groups;
	bool _use_backslashes;
};
