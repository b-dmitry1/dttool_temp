#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <vector>
#include "char_pattern.h"
#include "tokenizer.h"

// Шаблон для функций автоматического слияния 2 классов в новый
template <typename A, typename B, typename C>
token* transform_left(token* a, token* b)
{
	A* obj_a = dynamic_cast<A*>(a);
	B* obj_b = dynamic_cast<B*>(b);
	if (obj_a == NULL || obj_b == NULL)
		return NULL;
	C* res = dynamic_cast<C*>(obj_a);
	if (res == NULL)
	{
		res = new C();
		res->init(obj_a);
	}
	res->add(obj_b);
	return res;
};

class token_identifier : public token
{
public:
	virtual bool may_be_value() { return true; }
};
class token_attribute : public token {};
class token_attribute_finalized : public token
{
public:
	virtual void print()
	{
		std::cout << "/" << _text << "/";
		for (auto v : _values)
		{
			std::cout << " ";
			v->print();
		}
		std::cout << ";" << std::endl;
	}
};
class token_comment : public token {};
class token_include : public token {};
class token_number : public token
{
public:
	virtual bool may_be_value() { return true; }
};
class token_equals : public token {};
class token_semicolon : public token {};
class token_colon : public token {};
class token_comma : public token {};
class token_openpar : public token {};
class token_closepar : public token {};
class token_opencur : public token {};
class token_closecur : public token {};
class token_openbr : public token {};
class token_closebr : public token {};
class token_openang : public token {};
class token_closeang : public token {};
class token_text : public token
{
public:
	virtual bool may_be_value() { return true; }
	virtual bool is_text() { return true; }
};
class token_assignment : public token
{
};

class token_assignment_finalized : public token
{
public:
	virtual void print()
	{
		auto numbers = false;
		auto first = true;
		std::cout << _text << " =";
		for (auto v : _values)
		{
			std::cout << " ";
			if (!v->is_text() && !numbers)
				std::cout << "<";
			else if (v->is_text() && numbers)
				std::cout << ">";
			numbers = !v->is_text();
			if (!numbers && !first)
				std::cout << ", ";
			first = false;
			v->print();
		}
		if (numbers)
			std::cout << ">";
		std::cout << ";" << std::endl;
	}
};

class token_boolean : public token
{
public:
	virtual void print()
	{
		std::cout << _text << ";" << std::endl;
	}
};

class token_node_begin : public token
{
public:
	virtual void print()
	{
		std::cout << text() << " {" << std::endl;
	}
};
class token_node_end : public token
{
public:
	virtual void print()
	{
		std::cout << "};" << std::endl;
	}
};
class token_alias : public token {};
class token_name : public token {};

int main()
{
	std::vector<token*> inventory;

	// Загрузить файл в строку
	std::fstream f("test0.dts");
	std::stringstream sstream;
	sstream << f.rdbuf();
	std::string text = sstream.str();

	// Задать правила преобразования исходного текста в классы токенов
	tokenizer tzr(text);
	tzr.use_backslashes(true);

	tzr.add<token_identifier>("/({ )\b");
	tzr.add<token_attribute>("/\b[a-zA-Z0-9_,-]/\b");
	tzr.add<token_comment>("/*[]*\\/");
	tzr.add<token_comment>("//[]\n");
	tzr.add<token_include>("#include");
	tzr.add<token_number>("[0-9][0-9a-zA-Z]");
	tzr.add<token_number>("[0-9]");
	tzr.add<token_identifier>("[a-zA-Z#][0-9a-zA-Z_@,-]");
	tzr.add<token_equals>("=");
	tzr.add<token_semicolon>(";"); // Semicolon!
	tzr.add<token_colon>(":");
	tzr.add<token_comma>(",");
	tzr.add<token_openpar>("(");
	tzr.add<token_closepar>(")");
	tzr.add<token_opencur>("{");
	tzr.add<token_closecur>("}");
	tzr.add<token_openbr>("[");
	tzr.add<token_closebr>("]");
	tzr.add<token_openang>("<");
	tzr.add<token_closeang>(">");
	tzr.add<token_text>("\"[]\"");

	// Задать правила автоматических преобразований
	std::list<token* (*)(token*, token*)> transforms;
	transforms.push_back(&transform_left<token_identifier, token_equals, token_assignment>);
	transforms.push_back(&transform_left<token_identifier, token_semicolon, token_boolean>);
	transforms.push_back(&transform_left<token_assignment, token_openang, token_assignment>);
	transforms.push_back(&transform_left<token_assignment, token_closeang, token_assignment>);
	transforms.push_back(&transform_left<token_assignment, token_identifier, token_assignment>);
	transforms.push_back(&transform_left<token_assignment, token_number, token_assignment>);
	transforms.push_back(&transform_left<token_assignment, token_text, token_assignment>);
	transforms.push_back(&transform_left<token_assignment, token_semicolon, token_assignment_finalized>);
	transforms.push_back(&transform_left<token_assignment, token_comma, token_assignment>);
	transforms.push_back(&transform_left<token_assignment, token_closecur, token_assignment>);
	transforms.push_back(&transform_left<token_attribute, token_number, token_attribute>);
	transforms.push_back(&transform_left<token_attribute, token_semicolon, token_attribute_finalized>);
	transforms.push_back(&transform_left<token_identifier, token_opencur, token_node_begin>);
	transforms.push_back(&transform_left<token_closecur, token_semicolon, token_node_end>);

	token* first = NULL;
	token* second = NULL;
	token* result;
	for (; ; first = second)
	{
		second = tzr.next();
		if (second == NULL)
		{
			if (first != NULL)
				inventory.push_back(first);
			break;
		}
		if (first == NULL)
			continue;

		result = NULL;
		for (auto t : transforms)
		{
			result = t(first, second);
			if (result == NULL)
				continue;

			second = result;
			break;
		}

		if (result == NULL)
			inventory.push_back(first);
	}

	auto tabs = 0;
	for (auto it = std::begin(inventory); it != std::end(inventory); ++it)
	{
		auto node_begin = dynamic_cast<token_node_begin*>(*it);
		auto node_end = dynamic_cast<token_node_end*>(*it);

		if (node_begin != NULL)
			std::cout << std::endl;

		if (node_end != NULL)
			tabs--;

		for (auto tab = 0; tab < tabs; tab++)
			std::cout << "\t";

		(*it)->print();

		if (node_begin != NULL)
			tabs++;
	}

	return 0;
}
