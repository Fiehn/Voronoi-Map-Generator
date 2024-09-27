#pragma once


# define M_PI           3.14159265358979323846  /* pi */

//
// Poisson Disk Sampling Bridson's Algorithm
void insertPoint(std::vector<std::vector<sf::Vector2f>>& grid, float cellsize, sf::Vector2f point) {
    int xindex = floor(point.x / cellsize);
    int yindex = floor(point.y / cellsize);
    grid[xindex][yindex] = point;
}
bool isValidPoint(std::vector<std::vector<sf::Vector2f>>& grid, float cellsize, int width, int height, sf::Vector2f p, float radius) {
    // make sure the point is within box
    if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height) { return false; }

    // check neighborhood cells
    int xindex = floor(p.x / cellsize);
    int yindex = floor(p.y / cellsize);
    int i0 = std::max(xindex - 1, 0);
    int i1 = std::min(xindex + 1, width - 1);
    int j0 = std::max(yindex - 1, 0);
    int j1 = std::min(yindex + 1, height - 1);

    for (int i = i0; i <= i1; i++) {
        for (int j = j0; j <= j1; j++) {
            if (grid[i][j] != sf::Vector2f(-1.f, -1.f)) {
                if (dist(grid[i][j], p) < radius) {
                    return false;
                }
            }
        }
    }
    return true;
}
void poissonDiskSampling(std::vector<sf::Vector2f>& points, float radius, int k, int width, int height) {
    // radius is minimum distance and k is the limit
    int N = 2; // 2d
    std::vector<sf::Vector2f> active;
    sf::Vector2f p0;

    // Grid initialization
    float cellsize = floor(radius / sqrt(N));
    // number of cells in the grid for given canvas
    int ncells_width = ceil(width / cellsize) + 1;
    int ncells_height = ceil(height / cellsize) + 1;
    // Initialize grid with 2d vectors storing points in each index
    std::vector<std::vector<sf::Vector2f>> grid(ncells_width, std::vector<sf::Vector2f>(ncells_height, sf::Vector2f(-1.f, -1.f))); // default correct??

    // Insert first point
    insertPoint(grid, cellsize, p0);
    points.push_back(p0);
    active.push_back(p0);

    while (active.empty() == false) {
        int random_index = rand() % active.size();
        sf::Vector2f p = active[random_index];

        bool found = false;
        for (int tries = 0; tries < k; tries++) {
            // If valid point found
            float theta = ((float)rand() / (float)RAND_MAX) * 2 * M_PI;
            float new_radius = ((float)rand() / (float)RAND_MAX) * radius + radius;
            float pnewx = p.x + new_radius * cos(theta);
            float pnewy = p.y + new_radius * sin(theta);
            sf::Vector2f pnew(pnewx, pnewy);

            if (!isValidPoint(grid, cellsize, width, height, pnew, radius)) { continue; }

            points.push_back(pnew);
            insertPoint(grid, cellsize, pnew);
            active.push_back(pnew);
            found = true;
            break;
        }

        // maybe bug?
        if (!found) { active.erase(std::next(active.begin(), random_index)); }

    }

    //return points;
}





// Perlin noise is also here
float interpolate(float a0, float a1, float w) {
    w = clamp(w, 1.f, 0.f); // Just to make sure
    return (a1 - a0) * w + a0;
    // Using the smoothstep function:
    // return (a1 - a0) * (3.0 - w * 2.0) * w * w + a0;
    // Smoother step could also have been used
    return (a1 - a0) * ((w * (w * 6.0 - 15.0) + 10.0) * w * w * w) + a0;
}

sf::Vector2f randomGradient() {
    sf::Vector2f v;
    // Not completely random because it is two random combined, not just random between -1 and 1
    int randomSignx = rand() % 2;
    int randomSigny = rand() % 2;
    float randomx = (float)rand() / (float)RAND_MAX;
    float randomy = (float)rand() / (float)RAND_MAX;
    v.x = (randomSignx)*randomx + (randomSignx - 1) * randomx;
    v.y = (randomSigny)*randomy + (randomSigny - 1) * randomy;
    return v;
}


float dotGridGradient(int ix, int iy, float x, float y) {
    // ix and iy are the grid coordinates
    // get gradient
    sf::Vector2f gradient = randomGradient();

    // Compute the distance
    float dx = x - (float)ix;
    float dy = y - (float)iy;

    return (dx * gradient.x + dy * gradient.y);
}

// Perlin noise is not the way I think... I dont like the look of it
float perlin(float x, float y) {
    // Grid cell coordinates
    int x0 = (int)floor(x);
    int x1 = x0 + 1;
    int y0 = (int)floor(y);
    int y1 = y0 + 1;

    // Determine interpolation weights
    float sx = x - (float)x0;
    float sy = y - (float)y0;

    // Interpolate between grid point gradients
    float n0, n1, ix0, ix1, value;

    n0 = dotGridGradient(x0, y0, x, y);
    n1 = dotGridGradient(x1, y1, x, y);
    ix0 = interpolate(n0, n1, sx);

    n0 = dotGridGradient(x0, y1, x, y);
    n1 = dotGridGradient(x1, y1, x, y);
    ix1 = interpolate(n0, n1, sx);

    value = interpolate(ix0, ix1, sy);
    return (value + 1); // returns value between -1 and 1
}

/// OVERLAY!

class panel : public sf::Drawable
{
public:
    panel(std::size_t id, sf::FloatRect rect);
    explicit panel(std::size_t id);


    //void setMouseCursor(sf::Cursor::Type cursor);
    //void add_widget(std::size_t id, std::shared_ptr<ui_text_widget> widget);
    //bool get_widget(std::size_t id, std::shared_ptr<ui_text_widget>& widget);

    bool is_visible();

    void on_update();
    void on_key(char c);
    void on_mouse_pressed(bool left, sf::Vector2f v);
    void on_mouse_released(bool left, sf::Vector2f v);
    void on_mouse_moved(sf::Vector2f v);

    void set_pos(sf::Vector2f pos);
    void set_size(sf::Vector2f size);
    void toggle();

    std::size_t get_id();
private:
    std::size_t m_id;
    bool m_visible = false;

    sf::RectangleShape m_background;
    sf::RectangleShape m_titlebar;

    sf::Vector2f m_pressed;
    bool m_title_bar_pressed = false;

    //std::unordered_map<std::size_t, std::shared_ptr<ui_text_widget>> m_widgets;

    std::size_t m_widget_with_focus_id = 0;

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
};

class ui_manager
{
public:
    ui_manager(sf::RenderWindow& window);

    void clear();

    void add_panel(std::shared_ptr<panel> panel);
    std::shared_ptr<panel> get_panel(std::size_t id);

    bool handle_event(sf::Event::EventType type, sf::Vector2f mouse_pos);
    void on_update();
    void on_draw();

private:
    sf::RenderWindow& m_window;
    std::vector<std::shared_ptr<panel>> m_panels;
    std::shared_ptr<panel> m_active_panel;
    bool m_mouse_down = false;
};

namespace ovr
{
    class ValuesMenuOverlay
    {

    private:
        void set_values()
        {
            if (!font.loadFromFile("arial.ttf"))
            {
                std::cout << "Error loading font\n";
            }
            text.setFont(font);
            text.setCharacterSize(30);
            text.setFillColor(sf::Color::White);
            bgRect.setFillColor(sf::Color(0, 0, 0, 200));

            text.setString("basd");
            text.setPosition(10, 10);

            bgRect.setSize(sf::Vector2f(box_width, box_height));
            bgRect.setPosition(box_x, box_y);
        };
        void draw_values(sf::RenderWindow& window)
        {
            window.draw(bgRect);
            window.draw(text);
        };

    public:
        float box_x = 100;
        float box_y = 100;
        int box_width = 500;
        int box_height = 500;
        bool show = true;

        sf::Font font;
        sf::Text text;
        sf::RectangleShape bgRect;

        void run_menu(sf::RenderWindow& window)
        {

            if (show)
            {
                set_values();
                draw_values(window);
            }
        };
        void toggle() {
            show = !show;
        };
        void update()
        {

        };
        void set_text(std::string str)
        {
            text.setString(str);
        };
        void set_position(int x, int y)
        {
            box_x = x;
            box_y = y;
        };
        void set_size(int width, int height)
        {
            box_width = width;
            box_height = height;
        };
        void set_font(std::string font_path)
        {
            if (!font.loadFromFile(font_path))
            {
                std::cout << "Error loading font\n";
            }
        };
        void set_font_size(int size)
        {
            text.setCharacterSize(size);
        };
        void set_font_color(sf::Color color)
        {
            text.setFillColor(color);
        };
        void set_bg_color(sf::Color color)
        {
            bgRect.setFillColor(color);
        };
        ValuesMenuOverlay() {};
        ValuesMenuOverlay(sf::RenderWindow& window)
        {
            run_menu(window);
        };
        ~ValuesMenuOverlay() {};
    };


} // namespace Overlay