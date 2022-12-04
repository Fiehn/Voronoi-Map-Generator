#pragma once
#include <vector>
#include <string>
#include <SFML/System/Vector2.hpp>


class Language {
public:
	Language(int i = 0) : id(i) {}
	const int id;
	char name;
	

};

class Religion {
public:
	Religion(int i = 0) : id(i) {}
	const int id;
	char name;
	
	std::vector<sf::Vector2f> holySites;
	std::vector<std::string> holySitesDesc;
	void generate_holy_sites();

};

class Culture {
public:
	Culture(int i = 0) : id(i) {}
	const int id;
	char name; // is it string or char??
	Language language; 


};

class Nation {
public:
	Nation(int i) : id(i) {}
	const int id;
	char name;
	Culture domCulture; // Should be changed to a list or a stack of some variety
	Culture secCulture;
	Language domLanguage;
	Language secLanguage;
	Religion domReligion;
	Religion secReligion;

};

class biome {

};




