#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <chrono>
#include <unordered_map>
#include "char_pattern.h"
#include "tokenizer.h"

const bool print = true;

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

class recipe_book
{
public:
	template <typename A, typename B, typename C>
	void add_recipe()
	{
		_recipes[make_key(typeid(A).hash_code(), typeid(B).hash_code())] = &transform_left<A, B, C>;
	}

	void recipe_for(token* a, token* b, token* (*&result)(token*, token*))
	{
		result = _recipes[make_key(typeid(*a).hash_code(), typeid(*b).hash_code())];
	}
protected:
	std::unordered_map<size_t, token* (*)(token*, token*)> _recipes;
	size_t make_key(size_t a, size_t b) { return a ^ b; }
};

class forge
{
public:
	static token* craft(recipe_book& book, token* a, token* b)
	{
		token* (*recipe)(token*, token*) = (token* (*)(token*, token*))NULL;
		book.recipe_for(a, b, recipe);
		if (recipe == NULL)
			return NULL;
		return recipe(a, b);
	}
};
class token_identifier : public token
{
public:
	virtual bool may_be_value() { return true; }
};
class token_attribute : public token
{
public:
	virtual void print()
	{
		std::cout << "/" << _text << "/ ";
	}
	virtual bool is_sticky() { return true; }
};
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
class token_label : public token
{
public:
	virtual void print()
	{
		std::cout << _text << ": ";
	}
	virtual bool is_sticky() { return true; }
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
class token_add : public token {};
class token_sub : public token {};
class token_mul : public token {};
class token_div : public token {};
class token_mod : public token {};

int main()
{
	std::list<token*> inventory;

	// Загрузить файл в строку
	std::fstream f("test0.dts");
	std::stringstream sstream;
	sstream << f.rdbuf();
	std::string text = sstream.str();

	// Задать правила преобразования исходного текста в классы токенов
	tokenizer tzr(text);
	tzr.use_backslashes(true);

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
	tzr.add<token_add>("+");
	tzr.add<token_sub>("-");
	tzr.add<token_mul>("*");
	tzr.add<token_div>("/");
	tzr.add<token_mod>("%");

	// Задать правила автоматических преобразований
	recipe_book book;
	book.add_recipe<token_identifier, token_equals, token_assignment>();
	book.add_recipe<token_identifier, token_semicolon, token_boolean>();
	book.add_recipe<token_identifier, token_colon, token_label>();
	book.add_recipe<token_assignment, token_openang, token_assignment>();
	book.add_recipe<token_assignment, token_closeang, token_assignment>();
	book.add_recipe<token_assignment, token_identifier, token_assignment>();
	book.add_recipe<token_assignment, token_number, token_assignment>();
	book.add_recipe<token_assignment, token_text, token_assignment>();
	book.add_recipe<token_assignment, token_semicolon, token_assignment_finalized>();
	book.add_recipe<token_assignment, token_comma, token_assignment>();
	book.add_recipe<token_assignment, token_closecur, token_assignment>();
	book.add_recipe<token_assignment, token_attribute, token_assignment>();
	book.add_recipe<token_attribute, token_number, token_attribute>();
	book.add_recipe<token_attribute, token_semicolon, token_attribute_finalized>();
	book.add_recipe<token_identifier, token_opencur, token_node_begin>();
	book.add_recipe<token_div, token_opencur, token_node_begin>();
	book.add_recipe<token_closecur, token_semicolon, token_node_end>();

	// Начать измерение времени
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	// Выполнить преобразования
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

		result = forge::craft(book, first, second);
		if (result == NULL)
			inventory.push_back(first);
		else
			second = result;
	}

	// Закончить измерение времени
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	// Напечатать результат
	if (print)
	{
		auto tabs = 0;
		auto sticky = false;
		for (auto it = std::begin(inventory); it != std::end(inventory); ++it)
		{
			auto node_begin = dynamic_cast<token_node_begin*>(*it);
			auto node_end = dynamic_cast<token_node_end*>(*it);

			if (!sticky)
			{
				if (node_begin != NULL)
					std::cout << std::endl;

				if (node_end != NULL)
					tabs--;

				for (auto tab = 0; tab < tabs; tab++)
					std::cout << "\t";
			}

			(*it)->print();
			sticky = (*it)->is_sticky();

			if (!sticky)
			{
				if (node_begin != NULL)
					tabs++;
			}
		}
	}

	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " ms" << std::endl;

	return 0;
}
