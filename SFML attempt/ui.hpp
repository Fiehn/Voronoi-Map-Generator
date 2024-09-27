#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

// https://en.sfml-dev.org/forums/index.php?topic=19317.0
// https://github.com/abodelot/sfml-widgets/blob/master/src/Gui/Widget.hpp

namespace ui 
{

	class panel;

	enum State
	{
		StateDefault,
		StateHovered,
		StatePressed,
		StateFocused
	};

	class widget : public sf::Drawable
	{
	public:
		widget();

		void set_pos(sf::Vector2f pos);
		void set_pos(float x, float y);

		const sf::Vector2f& get_pos() const;

		const sf::Vector2f& get_size() const;

		bool containsPoint(const sf::Vector2f& point) const;

		bool isSelectable() const;

		bool isFocused() const;

		// This will be the callback when the widget is triggered
		void setCallback(std::function<void(void)> callback);

	protected:
		virtual void onStateChanged(State state);
		virtual void onMouseEnter();
		virtual void onMouseLeave();
		virtual void onMousePress(float x, float y);
		virtual void onMouseRelease(float x, float y);
		virtual void onMouseMoved(float x, float y);
		virtual void onMouseWheelMoved(int delta);
		virtual void onKeyPressed(const sf::Event::KeyEvent& key);
		virtual void onKeyReleased(const sf::Event::KeyEvent& key);
		virtual void onTextEntered(sf::Uint32 unicode);

		void setSize(const sf::Vector2f& size);
		void setSize(float width, float height);

		// Friend class??

		void setSelectable(bool selectable);

		void triggerCallback();

		void setState(State state);
		State getState() const;

		void setParent(panel* parent);
		panel* getParent() { return m_parent; }

		virtual panel* toLayout() { return nullptr; }

		virtual void recomputeGeometry() {};

		const sf::Transform& getTransform() const;

		virtual void setMouseCursor(sf::Cursor::Type cursor);
	private:
		panel* m_parent;
		widget* m_previous;
		widget* m_next;

		sf::Vector2f m_pos;
		sf::Vector2f m_size;
		State m_state;
		bool m_selectable;
		std::function<void(void)> m_callback;
		panel* m_parent;
	};

	/* Passive Widget
		To display text with a value from a cell	
	*/
	class valueText : public widget
	{
	public:
		valueText(const sf::String& string = "");
		void setText(const sf::String& string);
		const sf::String& getText() const;

		void setFillColor(const sf::Color& color);
		const sf::Color& getFillColor() const;

		void setTextSize(std::size_t size);
		std::size_t getTextSize() const;

		void updateValues(float value, int precision = 2);
		void updateValues(int value);

		void updateText();

	private:
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
		
		//void updateGeometry();
		
		sf::Text m_text;

		float m_value = 0.f;
		int m_precision = 2;
	};


	

}



