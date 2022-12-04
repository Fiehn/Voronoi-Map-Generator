#pragma once

#include <SFML/System/Vector2.hpp>
#include <queue>
#include <set>
#include <math.h>

// redesign of: https://www.cs.hmc.edu/~mbrubeck/voronoi.html

struct arc;
struct seg;

struct event {
	double x;
	sf::Vector2f p;
	arc *a;
	bool valid;

	event(double xx,sf::Vector2f pp,arc *aa)
		: x(xx),p(pp),a(aa),valid(true) {}
};

struct arc {
	sf::Vector2f p;
	arc *prev, *next;
	event *e;

	seg *s0, *s1;

	arc(sf::Vector2f pp, arc *a=0, arc *b=0)
		: p(pp),prev(a),next(b),e(0),s0(0),s1(0) {}
};

std::vector<seg*> output; // array of output segments

struct seg {
	sf::Vector2f start, end;
	bool done;

	seg(sf::Vector2f p)
		: start(p),end(0.f,0.f), done(false)
	{
		output.push_back(this);
	}
	// Set the end point and mark as "done."
	void finish(sf::Vector2f p) { if (done) return; end = p; done = true; }
};

arc *root = 0; // First item in the parabolic front linked list.

// "Greater than" comparison, for reverse sorting in priority queue
struct gt {
	bool operator()(sf::Vector2f a, sf::Vector2f b) { return a.x == b.x ? a.y > b.y : a.x > b.x; }
	bool operator()(event* a, event* b) { return a->x > b->x; }
};
// Generate the priority queues
std::priority_queue<sf::Vector2f, std::vector<sf::Vector2f>, gt> points;
std::priority_queue<event*, std::vector<event*>, gt> events;

// functions
void process_point() {
	// get the next point from the queue
	sf::Vector2f p = points.top();
	points.pop();

	// Add a new arc to the parabolic front
	front_insert(p);
};

void process_event() {
	// get the next event
	event* e = events.top();
	events.pop();

	if (e->valid) {
		// Start a new edge
		seg* s = new seg(e->p);

		// remove the associated arc from the front
		arc* a = e->a;
		if (a->prev) {
			a->prev->next = a->next;
			a->prev->s1 = s;
		}
		if (a->next) {
			a->next->prev = a->prev;
			a->next->s0 = s;
		}

		// finish the edges before and after a
		if (a->s0) a->s0->finish(e->p);
		if (a->s1) a->s1->finish(e->p);

		// Recheck circle events on either side of p
		if (a->prev) check_circle_event(a->prev, e->x);
		if (a->next) check_circle_event(a->next, e->x);
	}
	delete e;
};

void front_insert(sf::Vector2f p) {
	if (!root) {
		root = new arc(p);
		return;
	}
	// find the current arcs at height p.y (if they exist)
	for (arc* i = root; i; i = i->next) {
		sf::Vector2f z, zz;
		if (intersect(p, i, &z)) {
			// new parabola intersects arc i, If necessary, duplicates i
			if (i->next && !intersect(p, i->next, &zz)) {
				i->next->prev = new arc(i->p, i, i->next);
				i->next = i->next->prev;
			}
			else i->next = new arc(i->p, i);
			i->next->s1 = i->s1;

			// add p between i and i->next
			i->next->prev = new arc(p, i, i->next);
			i->next = i->next->prev;

			i = i->next; // now i points to the new arc

			// add new half-edges connected to i's endpoints
			i->prev->s1 = i->s0 = new seg(z);
			i->next->s0 = i->s1 = new seg(z);

			// Check for new circle events around the new arc
			check_circle_event(i, p.x);
			check_circle_event(i->prev, p.x);
			check_circle_event(i->next, p.x);

			return;
		}
	}
	// special case: if p never intersects an arc, append it to the list
	arc* i;
	for (i = root; i->next; i = i->next);
	i->next = new arc(p, i);
	//insert segment between i and p
	sf::Vector2f start;
	start.x = X0;
	start.y = (i->next->p.y + i->p.y) / 2;
	i->s1 = i->next->s0 = new seg(start);
};

// Find the rightmost point on the circle through a,b,c
bool circle(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, double* x, sf::Vector2f* o) {
	// Check that bc is a "Right turn" from ab
	if ((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y) > 0)
		return false;
	// Fully stolen algo and code
	double A = b.x - a.x, B = b.y - a.y,
		C = c.x - a.x, D = c.y - a.y,
		E = A * (a.x + b.x) + B * (a.y + b.y),
		F = C * (a.x + c.x) + D * (a.y + c.y),
		G = 2 * (A * (c.y - b.y) - B * (c.x - b.x));
	if (G == 0) return false; // points are colinear

	// point o is the center of the circle
	o->x = (D * E - B * F) / G;
	o->y = (A * F - C * E) / G;

	// o.x plus radius equals max x coordinate.
	*x = o->x + sqrt(pow(a.x - o->x, 2) + pow(a.y - o->y, 2));
	return true;
};

void check_circle_event(arc* i, double x0) {
	// Invalidate any old event
	if (i->e && i->e->x != x0)
		i->e->valid = false;
	i->e = NULL;

	if (!i->prev || !i->next)
		return;

	double x;
	sf::Vector2f o;

	if (circle(i->prev->p,i->p, i->next->p, & x, &o) && x > x0) {
		// Create new event
		i->e = new event(x, o, i);
		events.push(i->e);
	}
};

bool intersect(sf::Vector2f p, arc* i, sf::Vector2f *res = 0) {
	if (i->p.x == p.x) return false;

	double a, b;
	if (i->prev) // get the intersection of i->prev, i
		a = intersection(i->prev->p,i->p,p.x).y;
	if (i->next)
		b = intersection(i->p, i->next->p, p.x).y;

	if ((!i->prev || a <= p.y) && (!i->next || p.y <= b)) {
		res->y = p.y;

		// plug back into paraboa equation
		res ->x = (i->p.x * i->p.x + (i->p.y - res->y) * (i->p.y - res->y) - p.x * p.x)/ (2 * i->p.x - 2 * p.x);
		return true;
	}
	return false;
};
sf::Vector2f intersection(sf::Vector2f p0, sf::Vector2f p1, double l) {
	sf::Vector2f res, p = p0;

	if (p0.x == p1.x)
		res.y = (p0.y + p1.y) / 2;
	else if (p1.x == 1)
		res.y = p1.y;
	else if (p0.x == 1) {
		res.y = p0.y;
		p = p1;
	}
	else {
		// Use the quadratic formula
		double z0 = 2 * (p0.x - 1);
		double z1 = 2 * (p1.x - 1);

		double a = 1 / z0 - 1 / z1;
		double b = -2 * (p0.y / z0 - p1.y / z1);
		double c = (p0.y * p0.y + p0.x * p0.x - l * l) / z0 - (p1.y * p1.y + p1.x * p1.x - l * l) / z1;

		res.y = (-b - sqrt(b * b - 4 * a * c)) / (2 * a);
	}
	// plug back into one of the parabola equations
	res.x = (p.x * p.x + (p.y - res.y) * (p.y - res.y) - l * l) / (2 * p.x - 2 * l);
	return res;
};

void finish_edges() {
	// Advance the sweep line so no parabolas can cross the bounding box
	double l = X1 + (X1 - X0) + (Y1 - Y0);

	// Extend the remaining segments to the new parabola intersections
	for (arc* i = root; i->next; i = i->next)
		if (i->s1)
			i->s1->finish(intersection(i->p, i->next->p, l * 2));
};

void drawVoronoi() {

};



double X0 = 0, X1 = 0, Y0 = 0, Y1 = 0;

// remember to keep values within bounding box



