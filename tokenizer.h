#pragma once

#include <iostream>
#include <sstream>
#include <list>
#include <string>
#include "char_pattern.h"

class token
{
public:
	virtual void init(token* value)
	{
		_text = value->text();
		_values.insert(std::end(_values), std::begin(value->values()), std::end(value->values()));
	}
	virtual void add(token* value)
	{
		if (value->may_be_value())
			_values.push_back(value);
	}
	virtual std::list<token*>& values()
	{
		return _values;
	}

	virtual void print()
	{
		std::cout << _text;
	}

	void set_text(const std::string& text) { _text = text; }
	const std::string& text() { return _text; }

	virtual bool may_be_name() { return false; }
	virtual bool may_be_value() { return false; }
	virtual bool is_text() { return false; }

protected:
	std::string _text;
	std::list<token*> _values;
};

struct tokenizer_pattern
{
	char_pattern* pattern;
	token* (*func)(std::string& token);
};

class tokenizer
{
public:
	tokenizer(std::string& text)
	{
		_text = text;
		_it = std::begin(_text);
		_use_backslashes = true;
	}

	template <typename T>
	void add(const char* pattern)
	{
		auto item = new tokenizer_pattern();
		item->pattern = new char_pattern(pattern);
		item->pattern->use_backslashes(_use_backslashes);
		item->func = [](std::string& text)
		{
			token* res = new T();
			res->set_text(text);
			return res;
		};
		_patterns.push_back(item);
	}

	void use_backslashes(bool value)
	{
		_use_backslashes = value;
	}

	token* next()
	{
		std::string text;
		size_t advance;

		for (; _it != std::end(_text); ++_it)
		{
			auto ch = *_it;
			if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n')
				break;
		}

		if (_it == std::end(_text))
			return NULL;

		for (auto p : _patterns)
		{
			text.clear();
			advance = 0;
			if (p->pattern->check(_it, std::end(_text), text, advance))
			{
				auto token = p->func(text);
				_it += advance;
				return token;
			}
		}

		return NULL;
	}
private:
	std::list<tokenizer_pattern*> _patterns;
	bool _use_backslashes;
	std::string _text;
	std::string::iterator _it;
};
