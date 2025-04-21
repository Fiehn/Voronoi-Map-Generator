#pragma once

class Culture {
public:
	int id = -1; // Unique Id of the culture
	std::string name = "Unknown"; // Name of the culture

	sf::Color color = sf::Color::White; // Color of the culture

	std::size_t populatedCells = 0; // Number of cells populated by the culture

	std::vector<int> children; // Children cultures (subcultures) (id of the culture) 

	std::vector<std::size_t> cells; // Cells that are part of the culture
	std::vector<float> overExtension; // Covers the cost accrued so far by the culture (in the current cell in cells)

	Culture(int id, std::string name, sf::Color color) : id(id), name(name), color(color) { } // Constructor
	Culture() { } // Default constructor

	void addChild(int child) {
		children.push_back(child);
	}

	void setId(int id) {
		this->id = id;
	}

	void setName(std::string name) {
		this->name = name;
	}

	void setColor(sf::Color color) {
		this->color = color;
	}

private:

};




