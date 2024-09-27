#pragma once
#include "ui.hpp"

namespace ui
{
	widget::widget() : m_parent(nullptr), m_next(nullptr), m_previous(nullptr), m_state(StateDefault), m_selectable(false)
	{
	}
	const sf::Vector2f& widget::get_pos() const
	{
		return m_pos;
	}
	void widget::set_pos(sf::Vector2f pos)
	{
		// This should check parent
		m_pos = pos;
	}
	void widget::set_pos(float x, float y)
	{
		m_pos = sf::Vector2f(x, y);
	}
	const sf::Vector2f& widget::get_size() const
	{
		return m_size;
	}
	void widget::setSize(const sf::Vector2f& size)
	{
		// Use recompute Geometry for parent
		m_size = size;
	}
	void widget::setSize(float width, float height)
	{
		m_size = sf::Vector2f(width, height);
	}
	bool widget::containsPoint(const sf::Vector2f& point) const
	{
		return point.x > 0.f && point.x < m_size.x && point.y > 0.f && point.y < m_size.y;
	}
	bool widget::isSelectable() const
	{
		return m_selectable;
	}
	bool widget::isFocused() const
	{
		return m_state == StateFocused || m_state == StatePressed;
	}
	void widget::setSelectable(bool selectable)
	{
		m_selectable = selectable;
	}
	void widget::setCallback(std::function<void(void)> callback)
	{
		m_callback = callback;
	}
	void widget::triggerCallback()
	{
		if (m_callback)
		{
			m_callback();
		}
	}
	void widget::setState(State state)
	{
		if (m_state != state)
		{
			m_state = state;
			onStateChanged(state);
		}
	}
	State widget::getState() const
	{
		return m_state;
	}
	void widget::setParent(panel* parent)
	{
		m_parent = parent;
	}
	void widget::setMouseCursor(sf::Cursor::Type cursor)
	{
		if (m_parent)
		{
			m_parent->setMouseCursor(cursor);
		}
	}
	void widget::onStateChanged(State) { }
	void widget::onMouseEnter() { }
	void widget::onMouseLeave() { }
	void widget::onMouseMoved(float, float) { }
	void widget::onMousePress(float, float) { }
	void widget::onMouseRelease(float, float) { }
	void widget::onMouseWheelMoved(int) { }
	void widget::onKeyPressed(const sf::Event::KeyEvent&) { }
	void widget::onKeyReleased(const sf::Event::KeyEvent&) { }
	void widget::onTextEntered(sf::Uint32) { }

	/* Value Text */
	valueText::valueText(const sf::String& string)
	{
		sf::Font font;
		if (!font.loadFromFile("arial.ttf"))
		{
			std::cout << "Could not load font" << std::endl;
		}

		m_text.setFont(font);
		m_text.setPosition(0, 0);
		m_text.setFillColor(sf::Color::Black);
		m_text.setCharacterSize(15);
		setSelectable(false);
		setText(string);
	}
	void valueText::setText(const sf::String& string)
	{
		m_text.setString(string);
		updateText();
		// updateGeometry();
	}
	const sf::String& valueText::getText() const
	{
		return m_text.getString();
	}
	void valueText::setFillColor(const sf::Color& color)
	{
		m_text.setFillColor(color);
	}
	const sf::Color& valueText::getFillColor() const
	{
		return m_text.getFillColor();
	}
	void valueText::setTextSize(size_t size)
	{
		m_text.setCharacterSize(size);
		//updateGeometry();
	}
	std::size_t valueText::getTextSize() const
	{
		return m_text.getCharacterSize();
	}
	void valueText::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		states.transform *= getTransform();
		target.draw(m_text, states);
	}
	
	void valueText::updateValues(float value, int precision = 2)
	{
		m_value = value;
		m_precision = precision;
		updateText();
	}
	void valueText::updateValues(int value)
	{ // cast to float and add to m_value
		m_value = static_cast<float>(value);
		updateText();
	}
	void valueText::updateText()
	{ // add the value at the end of the text 
		m_text.setString(m_text.getString() + std::to_string(m_value));
	}

}


