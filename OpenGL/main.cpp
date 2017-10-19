#include <FL/Fl.H>
#include <FL/gl.h>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Color_Chooser.H>
#include <cstdio>
#define _USE_MATH_DEFINES
#include <cmath>


using namespace std;
// not built-in define, use to specify creating object type
#define MY_CIRCLES 0x000a
#define MY_ZOOMRECT 0x000b

// constant
// static int xpp = 0;
const int MAX_SHAPE_COUNT = 30000;
const int CIRCLE_SIDES = 100;

struct Vector2
{
	float x;
	float y;
};
struct Color {
	Color() { r = 0; g = 0; b = 0; }
	Color(float R, float G, float B)
	{
		r = R;
		g = G;
		b = B;
	}
	float r;
	float g;
	float b;
};
const Color white(1, 1, 1);
const Color red(1, 0, 0);
Color current_color(1, 1, 1);

class Shape
{
private:
	Color color_;
	bool filled_;
public:
	Shape(Color color = white, bool filled = false) { color_ = color; filled_ = filled; }
	virtual void PreviewSet(int x, int y) {}; // call when mouse move or drag
	virtual bool SetComplete() { return false; }; // return if the shape set is finish
	virtual void Draw() { 
		glColor3f(color_.r, color_.g, color_.b); 
		if (filled_)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
		else 
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} // vertex draw function
	virtual void FitWidget(int w, int h) {}; // transform mouse position to OpenGL cordinate, call between Set(PreviewSet) and Draw
	virtual void Set(int x, int y) {}; // set shape vertex iteratively
	virtual void Reset() {}; // reset all shape vertext
	void SetColor(Color color) { color_ = color; }
	void SetFilled(bool filled) { filled_ = filled; }
};
class Line : public Shape
{
public:
	Line(Color color = white, bool filled = false)
		:Shape(color, filled)
	{
		Reset();
	}
	bool SetComplete()
	{
		return set_step_ == 2;
	}
	void Set(int x, int y)
	{
		if (set_step_ == 0)
		{
			origin_start_.x = x;
			origin_start_.y = y;
			origin_end_.x = x;
			origin_end_.y = y;
			set_step_++;
		}
		else if (set_step_ == 1)
		{
			origin_end_.x = x;
			origin_end_.y = y;
			set_step_++;
		}
	}
	void PreviewSet(int x, int y)
	{
		if (set_step_ >= 1)
		{
			origin_end_.x = x;
			origin_end_.y = y;
		}
	}
	void Reset()
	{
		set_step_ = 0;
		origin_start_.x = 0;
		origin_start_.y = 0;
		origin_end_.x = 0;
		origin_end_.y = 0;
		start_.x = 0;
		start_.y = 0;
		end_.x = 0;
		end_.y = 0;
	}
	inline void Draw()
	{
		Shape::Draw();
		glBegin(GL_LINES);
		glVertex2f(start_.x, start_.y);
		glVertex2f(end_.x, end_.y);
		glEnd();
	}
	void FitWidget(int w, int h)
	{
		float half_w = w / 2;
		float half_h = h / 2;
		start_.x = (origin_start_.x - half_w) / half_w;
		start_.y = (-origin_start_.y + half_h) / half_h;
		end_.x = (origin_end_.x - half_w) / half_w;
		end_.y = (-origin_end_.y + half_h) / half_h;
	}
private:
	Vector2 start_;
	Vector2 end_;
	Vector2 origin_start_;
	Vector2 origin_end_;
	int set_step_;
};
class Point : public Shape
{
public:
	Point(Color color = white, bool filled = false)
		:Shape(color, filled)
	{
		Reset();
	}
	bool SetComplete()
	{
		return true;
	}
	void Set(int x, int y)
	{
		origin_position_.x = x;
		origin_position_.y = y;
	}
	void PreviewSet(int x, int y)
	{
		origin_position_.x = x;
		origin_position_.y = y;
	}
	void Reset()
	{
		origin_position_.x = 0;
		origin_position_.y = 0;
	}
	inline void Draw()
	{
		Shape::Draw();
		glBegin(GL_POINTS);
		glVertex2f(position_.x, position_.y);
		glEnd();
	}
	void FitWidget(int w, int h)
	{
		float half_w = w / 2;
		float half_h = h / 2;
		position_.x = (origin_position_.x - half_w) / half_w;
		position_.y = (-origin_position_.y + half_h) / half_h;
	}
private:
	Vector2 position_;
	Vector2 origin_position_;
};
class Triangle : public Shape
{
public:
	Triangle(Color color = white, bool filled = false)
		:Shape(color, filled), base_(color, filled)
	{
		Reset();
	}
	bool SetComplete()
	{
		return set_step_ == 3;
	}
	void Set(int x, int y)
	{
		if (set_step_ == 0) // first and third side start
		{
			origin_vertex_[0].x = x;
			origin_vertex_[0].y = y;
			origin_vertex_[1].x = x;
			origin_vertex_[1].y = y;
			origin_vertex_[2].x = x;
			origin_vertex_[2].y = y;
			base_.Set(x, y);
			set_step_++;
		}
		else if (set_step_ == 1) // first side end, second side start
		{
			origin_vertex_[1].x = x;
			origin_vertex_[1].y = y;
			base_.Set(x, y);
			set_step_++;
		}
		else if (set_step_ == 2) // second and third side end
		{
			origin_vertex_[2].x = x;
			origin_vertex_[2].y = y;
			set_step_++;
		}
	}
	void PreviewSet(int x, int y)
	{
		if (set_step_ == 1) // first side started, preview first side
			base_.PreviewSet(x, y);
		else if (set_step_ >= 2) // second side started, preview whole triangle
		{
			origin_vertex_[2].x = x;
			origin_vertex_[2].y = y;
		}
	}
	void Reset()
	{
		set_step_ = 0;
		base_.Reset();
	}
	inline void Draw()
	{
		Shape::Draw();
		if (set_step_ >= 2) 
		{
			glBegin(GL_TRIANGLES);
			glVertex2f(vertex_[0].x, vertex_[0].y);
			glVertex2f(vertex_[1].x, vertex_[1].y);
			glVertex2f(vertex_[2].x, vertex_[2].y);
			glEnd();
		}
		else 
		{
			base_.Draw();
		}
	}
	void FitWidget(int w, int h)
	{
		float half_w = w / 2;
		float half_h = h / 2;
		for (int i = 0; i < 3; i++)
		{
			vertex_[i].x = (origin_vertex_[i].x - half_w) / half_w;
			vertex_[i].y = (-origin_vertex_[i].y + half_h) / half_h;
		}
		base_.FitWidget(w, h);

	}
private:
	Line base_;
	Vector2 vertex_[3];
	Vector2 origin_vertex_[3];
	int set_step_;
};
class Quadrilater : public Shape
{
public:
	Quadrilater(Color color = white, bool filled = false)
		:Shape(color, filled)
	{
		for (int i = 0; i < 2; i++)
			sides_[i].SetColor(color);
		Reset();
	}
	bool SetComplete()
	{
		return set_step_ == 4;
	}
	void Set(int x, int y)
	{
		if (set_step_ == 0) // first and fourth side start
		{
			origin_vertex_[0].x = x;
			origin_vertex_[0].y = y;
			origin_vertex_[1].x = x;
			origin_vertex_[1].y = y;
			origin_vertex_[2].x = x;
			origin_vertex_[2].y = y;
			origin_vertex_[3].x = x;
			origin_vertex_[3].y = y;
			sides_[0].Set(x, y);
			set_step_++;
		}
		else if (set_step_ == 1) // first side end, second side start
		{
			origin_vertex_[1].x = x;
			origin_vertex_[1].y = y;
			sides_[0].Set(x, y);
			sides_[1].Set(x, y);
			set_step_++;
		}
		else if (set_step_ == 2) // second side end, third side start
		{
			origin_vertex_[2].x = x;
			origin_vertex_[2].y = y;
			set_step_++;
		}
		else if (set_step_ == 3) // third and fourth side end
		{
			origin_vertex_[3].x = x;
			origin_vertex_[3].y = y;
			set_step_++;
		}
	}
	void PreviewSet(int x, int y)
	{
		if (set_step_ == 1) // first side started, preview first side
			sides_[0].PreviewSet(x, y);
		else if (set_step_ == 2) // second side started, preview second side
			sides_[1].PreviewSet(x, y);
		else if (set_step_ >= 3) // third side started, preview whole quad
		{
			origin_vertex_[3].x = x;
			origin_vertex_[3].y = y;
		}
	}
	void Reset()
	{
		set_step_ = 0;
		sides_[0].Reset();
		sides_[1].Reset();
	}
	inline void Draw()
	{
		Shape::Draw();
		if (set_step_ >= 3)
		{
			glBegin(GL_QUADS);
			glVertex2f(vertex_[0].x, vertex_[0].y);
			glVertex2f(vertex_[1].x, vertex_[1].y);
			glVertex2f(vertex_[2].x, vertex_[2].y);
			glVertex2f(vertex_[3].x, vertex_[3].y);
			glEnd();
		}
		else
		{
			sides_[0].Draw();
			sides_[1].Draw();
		}
	}
	void FitWidget(int w, int h)
	{
		float half_w = w / 2;
		float half_h = h / 2;
		for (int i = 0; i < 4; i++)
		{
			vertex_[i].x = (origin_vertex_[i].x - half_w) / half_w;
			vertex_[i].y = (-origin_vertex_[i].y + half_h) / half_h;
		}
		sides_[0].FitWidget(w, h);
		sides_[1].FitWidget(w, h);
	}
private:
	Line sides_[2];
	Vector2 vertex_[4];
	Vector2 origin_vertex_[4];
	int set_step_;
};
class Circle : public Shape
{
public:
	Circle(Color color = white, bool filled = false)
		:Shape(color, filled)
	{
		filled_ = filled;
		for (int i = 0; i < CIRCLE_SIDES; i++) {
			sides_[i].SetColor(color);
			filled_sides_[i].SetColor(color);
			filled_sides_[i].SetFilled(filled);
		}
		Reset();
	}
	bool SetComplete()
	{
		return set_step_ == 2;
	}
	void Set(int x, int y)
	{
		if (set_step_ == 0) // circle center set
		{
			origin_center_.x = x;
			origin_center_.y = y;
			SetSides(0);
			set_step_++;
		}
		else if (set_step_ == 1) // circle radius set
		{
			radius = sqrtf(powf(x - origin_center_.x, 2) + powf(y - origin_center_.y, 2));
			SetSides(radius);
			set_step_++;
		}
	}
	void PreviewSet(int x, int y)
	{
		if (set_step_ >= 1) // center set, preview whole circle
		{
			float r = sqrtf(powf(x - origin_center_.x, 2) + powf(y - origin_center_.y, 2));
			SetSides(r);
		}
	}
	void Reset()
	{
		for (int i = 0; i < CIRCLE_SIDES; i++)
		{
			sides_[i].Reset();
			filled_sides_[i].Reset();
		}
		set_step_ = 0;
		center_.x = 0;
		center_.y = 0;
		origin_center_.x = 0;
		origin_center_.y = 0;
		radius = 0;
	}
	inline void Draw()
	{
		Shape::Draw();
		if (set_step_ >= 1)
		{
			if (filled_)
				for (int i = 0; i < CIRCLE_SIDES; i++)
					filled_sides_[i].Draw();
			else 
				for (int i = 0; i < CIRCLE_SIDES; i++)
					sides_[i].Draw();
		}
	}
	void FitWidget(int w, int h)
	{
		float half_w = w / 2;
		float half_h = h / 2;
		if (filled_)
			for (int i = 0; i < CIRCLE_SIDES; i++)
				filled_sides_[i].FitWidget(w, h);
		else 
			for (int i = 0; i < CIRCLE_SIDES; i++)
				sides_[i].FitWidget(w, h);
		center_.x = (origin_center_.x - half_w) / half_w;
		center_.y = (-origin_center_.y + half_h) / half_h;
	}
private:
	// Set all sides by radius and center
	void SetSides(float r)
	{
		if (filled_) {
			for (int i = 0; i < CIRCLE_SIDES; i++)
			{
				filled_sides_[i].Reset();
				filled_sides_[i].Set(origin_center_.x, origin_center_.y);
				filled_sides_[i].Set(origin_center_.x + r * cosf(2 * M_PI * i / CIRCLE_SIDES), origin_center_.y + r * sinf(2 * M_PI * i / CIRCLE_SIDES));
				filled_sides_[i].Set(origin_center_.x + r * cosf(2 * M_PI * (i + 1) / CIRCLE_SIDES), origin_center_.y + r * sinf(2 * M_PI * (i + 1) / CIRCLE_SIDES));
			}
		}
		else
		{
			for (int i = 0; i < CIRCLE_SIDES; i++)
			{
				sides_[i].Reset();
				sides_[i].Set(origin_center_.x + r * cosf(2 * M_PI * i / CIRCLE_SIDES), origin_center_.y + r * sinf(2 * M_PI * i / CIRCLE_SIDES));
				sides_[i].Set(origin_center_.x + r * cosf(2 * M_PI * (i + 1) / CIRCLE_SIDES), origin_center_.y + r * sinf(2 * M_PI * (i + 1) / CIRCLE_SIDES));
			}
		}
	}
	Line sides_[CIRCLE_SIDES];
	Triangle filled_sides_[CIRCLE_SIDES];
	Vector2 origin_center_;
	Vector2 center_;
	float radius;
	int set_step_;
	bool filled_;
};
class ZoomRectangle : public Shape
{
public:
	ZoomRectangle(const int* frame)
		:quad(red, false)
	{
		current_start_.x = 0;
		current_start_.y = 0;
		current_end_.x = 0;
		current_end_.y = 0;
		Reset(frame);
	}
	inline void Draw()
	{
		quad.Draw();
	}
	bool SetComplete()
	{
		return set_step_ == 2;
	}
	void Set(int x, int y)
	{
		if (set_step_ == 0) // set start point
		{
			origin_start_.x = x;
			origin_start_.y = y;
			quad.Set(x, y);
			set_step_++;
		}
		else if (set_step_ == 1) // set end point
		{
			origin_end_.x = x;
			origin_end_.y = y;
			current_start_ = origin_start_;
			current_end_ = origin_end_;
			quad.Reset();
			quad.Set(origin_start_.x, origin_start_.y);
			quad.Set(x, origin_start_.y);
			quad.Set(x, y);
			quad.PreviewSet(origin_start_.x, y);
			set_step_++;
		}
	}
	void PreviewSet(int x, int y)
	{
		if (set_step_ >= 1) // start set, preview whole rectangle
		{
			quad.Reset();
			quad.Set(origin_start_.x, origin_start_.y);
			quad.Set(x, origin_start_.y);
			quad.Set(x, y);
			quad.PreviewSet(origin_start_.x, y);
		}
	}
	void Reset()
	{
		Reset(0);
	}
	void Reset(const int* frame)
	{
		quad.Reset();
		frame_ = frame;
		set_step_ = 0;
		origin_start_.x = 0;
		origin_start_.y = 0;
		start_.x = 0;
		start_.y = 0;
		origin_end_.x = 0;
		origin_end_.y = 0;
		end_.x = 0;
		end_.y = 0;
	}
	void FitWidget(int w, int h)
	{
		quad.FitWidget(w, h);
		float half_w = w / 2;
		float half_h = h / 2;
		
		start_.x = (origin_start_.x - half_w) / half_w;
		start_.y = (-origin_start_.y + half_h) / half_h;
		end_.x = (origin_end_.x - half_w) / half_w;
		end_.y = (-origin_end_.y + half_h) / half_h;
	}
	void SetZoomView()
	{
		if (!SetComplete()) return;
		float left, right, top, bottom;
		if (start_.x > end_.x)
		{
			left = end_.x;
			right = start_.x;
			//*w = origin_start_.x - origin_end_.x;
		}
		else
		{
			left = start_.x;
			right = end_.x;
			//*w = origin_end_.x - origin_start_.x;
		}
		if (start_.y > end_.y)
		{
			top = start_.y;
			bottom = end_.y;
			//*h = origin_end_.y - origin_start_.y;
		}
		else
		{
			top = end_.y;
			bottom = start_.y;
			//*h = origin_start_.y - origin_end_.y;
		}
		glOrtho(left, right, bottom, top, -1, 1);
	}
	void ZoomPositionMapping(float* x, float* y)
	{
		if (current_start_.x > current_end_.x)
		{
			*x = *x + current_end_.x;
		}
		else
		{
			*x = *x + current_start_.x;
		}
		if (current_start_.y > current_end_.y)
		{
			*y = *y + current_end_.y;
		}
		else
		{
			*y = *y + current_start_.y;
		}
	}
	float GetWidth() { return fabsf(current_start_.x - current_end_.x); }
	float GetHeight() { return fabsf(current_start_.y - current_end_.y); }


private:
	Quadrilater quad;
	const int* frame_;
	int set_step_;
	Vector2 origin_start_;
	Vector2	origin_end_;
	Vector2 start_;
	Vector2 end_;
	Vector2 current_start_;
	Vector2 current_end_;
};
// global setting and state variable
int creating_object_type = GL_POINTS;
bool is_creating_object = false;
Shape* shapes[MAX_SHAPE_COUNT];
ZoomRectangle zoom_rect(0);
int shape_count = 0;
float zoom_multiple = 2.0;
float current_zoom_multiple = 2.0;
bool current_filled = true;

class openGL_window : public Fl_Gl_Window { // Create a OpenGL class in FLTK 
	void draw();            // Draw function. 
	void draw_overlay();    // Draw overlay function. 
	virtual int handle(int event);

	static void Timer_CB(void *userdata) {
		openGL_window *pb = (openGL_window*)userdata;
		pb->redraw();
		Fl::repeat_timeout(1.0 / 60.0, Timer_CB, userdata);
	}

public:
	openGL_window(int x, int y, int w, int h, const char *l = 0);  // Class constructor 
	int frame;
	openGL_window* zoom_window;
	openGL_window* main_window;
};

openGL_window::openGL_window(int x, int y, int w, int h, const char *l) :
	Fl_Gl_Window(x, y, w, h, l)
{
	mode(FL_RGB | FL_ALPHA | FL_DOUBLE | FL_STENCIL);
	Fl::add_timeout(3.0, Timer_CB, (void*)this);
	frame = 0;
	zoom_window = NULL;
	main_window = NULL;
}

void openGL_window::draw() {
	// the valid() property may be used to avoid reinitializing your
	// GL transformation for each redraw:
	if (!valid()) 
	{
		valid(1);
		glLoadIdentity();
		if (this == zoom_window)
		{
			zoom_rect.SetZoomView();
		}
		else
		{
			for (int i = 0; i < shape_count; i++)
				shapes[i]->FitWidget(w(), h());
			zoom_rect.FitWidget(w(), h());
			zoom_window->valid(0);
		}

		glViewport(0, 0, w(), h());
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	// draw an amazing but slow graphic:--------------
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	for (int i = 0; i < shape_count; i++)
	{
		shapes[i]->Draw();
	}

	//--------------------------------------------------
	++frame;
}

void openGL_window::draw_overlay() {
	// the valid() property may be used to avoid reinitializing your
	// GL transformation for each redraw:
	if (!valid()) 
	{
		valid(1);
		glLoadIdentity();
		glViewport(0, 0, w(), h());
	}
	
	// draw an amazing graphic:-------------
	zoom_rect.Draw();
	//---------------------------------------
}

int openGL_window::handle(int event)
{
	float x, y;
	switch (event)
	{
	case FL_PUSH: // mouse click
		if (Fl::event_button() == FL_LEFT_MOUSE)
		{
			x = Fl::event_x();
			y = Fl::event_y();
			if (this == zoom_window) {
				x /= current_zoom_multiple;
				y /= current_zoom_multiple;
				zoom_rect.ZoomPositionMapping(&x, &y);
			}
			if (!is_creating_object)
			{
				Shape* shape = NULL;
				if (creating_object_type == GL_POINTS)
					shape = new Point(current_color, current_filled);
				else if (creating_object_type == GL_TRIANGLES)
					shape = new Triangle(current_color, current_filled);
				else if (creating_object_type == GL_LINES)
					shape = new Line(current_color, current_filled);
				else if (creating_object_type == GL_QUADS)
					shape = new Quadrilater(current_color, current_filled);
				else if (creating_object_type == MY_CIRCLES)
					shape = new Circle(current_color, current_filled);
				else if (creating_object_type == MY_ZOOMRECT) {
					zoom_rect.Reset(&main_window->frame);
					shape = &zoom_rect;
				}

				shape->Set(x, y);
				shapes[shape_count++] = shape;
				is_creating_object = true;
				if (shapes[shape_count - 1]->SetComplete())
				{
					is_creating_object = false;
					shapes[shape_count - 1]->FitWidget(main_window->w(), main_window->h());
				}
			}
			else
			{
				shapes[shape_count - 1]->Set(x, y);
				shapes[shape_count - 1]->FitWidget(main_window->w(), main_window->h());
				if (shapes[shape_count - 1]->SetComplete())
				{
					if (shapes[shape_count - 1] == &zoom_rect) {
						shapes[--shape_count] = NULL; // ZoomRectangle are independently draw in draw_overlay, no need to add to shapes

						if (this == zoom_window) {
							zoom_multiple *= 2;
							char s[64];
							sprintf_s(s, 64, "%dX Zoom Reset", (int)zoom_multiple);
							main_window->parent()->child(9)->copy_label(s);
						}
						zoom_window->parent()->show();
						zoom_window->show();
						zoom_window->parent()->resize(zoom_window->parent()->x(), zoom_window->parent()->y(),
							zoom_rect.GetWidth() * zoom_multiple, zoom_rect.GetHeight() * zoom_multiple);
						zoom_window->resize(zoom_window->x(), zoom_window->y(), 
							zoom_rect.GetWidth() * zoom_multiple, zoom_rect.GetHeight() * zoom_multiple);
						current_zoom_multiple = zoom_multiple;
						zoom_window->valid(0);
					}
					is_creating_object = false;
				}
			}
		}
		break;
	case FL_DRAG: case FL_MOVE:
		x = Fl::event_x();
		y = Fl::event_y();
		if (this == zoom_window) {
			x /= current_zoom_multiple;
			y /= current_zoom_multiple;
			zoom_rect.ZoomPositionMapping(&x, &y);
		}
		if (is_creating_object)
		{
			shapes[shape_count - 1]->PreviewSet(x, y);
			shapes[shape_count - 1]->FitWidget(main_window->w(), main_window->h());
		}
		break;
	default:
		break;
	}
	return 1;
}

void DrawPoint(Fl_Widget *, void *) {
	creating_object_type = GL_POINTS;
}
void DrawLine(Fl_Widget *, void *) {
	creating_object_type = GL_LINES;
}
void DrawTriangle(Fl_Widget *, void *) {
	creating_object_type = GL_TRIANGLES;
}
void DrawQuadrilateral(Fl_Widget *, void *) {
	creating_object_type = GL_QUADS;
}
void DrawCircle(Fl_Widget *, void *) {
	creating_object_type = MY_CIRCLES;
}
void DrawZoom(Fl_Widget *, void *) {
	creating_object_type = MY_ZOOMRECT;
}
void ZoomUp(Fl_Widget *w, void *) {
	zoom_multiple *= 2;
	char s[64];
	sprintf_s(s, 64, "%dX Zoom Reset", (int)zoom_multiple);
	w->parent()->child(9)->copy_label(s); // 9: the zoom reset button
}
void ZoomDown(Fl_Widget *w, void *) {
	if (zoom_multiple < 2) return;
	zoom_multiple /= 2;
	char s[64];
	sprintf_s(s, 64, "%dX Zoom Reset", (int)zoom_multiple);
	w->parent()->child(9)->copy_label(s);
}
void ZoomReset(Fl_Widget *w, void *) {
	zoom_multiple = 2;
	char s[64];
	sprintf_s(s, 64, "%dX Zoom Reset", (int)zoom_multiple);
	w->parent()->child(9)->copy_label(s);
}
void ZoomClear(Fl_Widget *w, void *) {
	zoom_multiple = 2;
	char s[64];
	sprintf_s(s, 64, "%dX Zoom Reset", (int)zoom_multiple);
	w->parent()->child(9)->copy_label(s);
	openGL_window *window = (openGL_window*) w->parent()->child(0);
	zoom_rect.Reset();
	window->zoom_window->parent()->hide();
}
void ChangeColor(Fl_Widget *w, void *)
{
	double r, g, b;
	fl_color_chooser("Choose a color", r, g, b);
	current_color.r = r;
	current_color.g = g;
	current_color.b = b;
	w->color(fl_rgb_color(current_color.r * 255, current_color.g * 255, current_color.b * 255));
	w->redraw();
}
void CloseZoom(Fl_Widget *w, void *)
{
	zoom_rect.Reset();
	w->hide();
}
void Erase(Fl_Widget *w, void *)
{
	if (shape_count > 0)
	{
		if (shapes[shape_count - 1] == &zoom_rect) {
			--shape_count;
			zoom_rect.Reset();
		}
		else
		{
			delete shapes[--shape_count];
			shapes[shape_count] = NULL;
		}
		is_creating_object = false;
	}
}
void Clear(Fl_Widget *w, void *)
{
	if (shapes[shape_count - 1] == &zoom_rect) --shape_count;
	for (int i = 0; i < shape_count; i++) {
		delete shapes[i];
		shapes[i] = NULL;
	}
	shape_count = 0;
	is_creating_object = false;
	zoom_rect.Reset();
}

void Idle(Fl_Widget *w, void *)
{
	w->parent()->resize(100, 100, 1162, 532);

	if (shapes[shape_count - 1] == &zoom_rect) --shape_count;
	for (int i = 0; i < shape_count; i++) 
	{
		delete shapes[i];
		shapes[i] = NULL;
	}
	shape_count = 0;
	is_creating_object = false;
	zoom_rect.Reset();
	openGL_window* draw_win = (openGL_window*)w->parent()->child(0); // 0: draw window
	draw_win->zoom_window->parent()->hide();
	current_color.r = 1;
	current_color.g = 1;
	current_color.b = 1;
	Fl_Widget* color_change = w->parent()->child(10);
	color_change->color(fl_rgb_color(current_color.r * 255, current_color.g * 255, current_color.b * 255));
	color_change->redraw();
}
void Exit(Fl_Widget *w, void *)
{
	exit(0);
}
void SetFill(Fl_Widget *w, void *)
{
	current_filled = true;
}
void SetLine(Fl_Widget *w, void *)
{
	current_filled = false;
}



//  main function
int main(int argc, char **argv) {
	Fl_Window window(100, 100, 640, 532, "H.W.One");
	
	openGL_window gl_win(10, 10, 620, 400);
	window.resizable(gl_win);
	//openGL_window right_view(640, 10, 620, 400);


	Fl_Widget *point;
	point = new Fl_Button(12, 420, 120, 20, "Points");
	point->callback(DrawPoint);

	Fl_Widget *lines;
	lines = new Fl_Button(134, 420, 120, 20, "Lines");
	lines->callback(DrawLine);

	Fl_Widget *triangle;
	triangle = new Fl_Button(258, 420, 120, 20, "Triangles");
	triangle->callback(DrawTriangle);

	Fl_Widget *quadrilateral;
	quadrilateral = new Fl_Button(382, 420, 120, 20, "Quadrilaterals");
	quadrilateral->callback(DrawQuadrilateral);

	Fl_Widget *circle;
	circle = new Fl_Button(506, 420, 120, 20, "Circles");
	circle->callback(DrawCircle);

	Fl_Widget *zoom_rectangle;
	zoom_rectangle = new Fl_Button(12, 461, 120, 20, "ZoomIn");
	zoom_rectangle->callback(DrawZoom);

	Fl_Widget *zoom_up;
	zoom_up = new Fl_Button(134, 461, 120, 20, "Zoom +");
	zoom_up->callback(ZoomUp);

	Fl_Widget *zoom_down;
	zoom_down = new Fl_Button(258, 461, 120, 20, "Zoom -");
	zoom_down->callback(ZoomDown);

	Fl_Widget *zoom_reset;
	zoom_reset = new Fl_Button(382, 461, 120, 20, "2X Zoom Reset");
	zoom_reset->callback(ZoomReset);

	Fl_Widget *color_change;
	color_change = new Fl_Button(506, 461, 120, 20, "Change Color");
	color_change->callback(ChangeColor);
	color_change->color(fl_rgb_color(current_color.r * 255, current_color.g * 255, current_color.b * 255));

	Fl_Widget *zoom_clear;
	zoom_clear = new Fl_Button(12, 483, 305, 20, "Clear Zoom");
	zoom_clear->callback(ZoomClear);

	Fl_Widget *erase;
	erase = new Fl_Button(320, 483, 152, 20, "Erase");
	erase->callback(Erase);

	Fl_Widget *clear;
	clear = new Fl_Button(474, 483, 152, 20, "Clear");
	clear->callback(Clear);


	Fl_Widget *idle;
	idle = new Fl_Button(12, 505, 407, 20, "Idle");
	idle->callback(Idle);

	Fl_Widget *exit;
	exit = new Fl_Button(422, 505, 204, 20, "Exit");
	exit->callback(Exit);


	Fl_Widget *fill;
	fill = new Fl_Button(12, 442, 306, 17, "Fill");
	fill->callback(SetFill);

	Fl_Widget *line;
	line = new Fl_Button(320, 442, 306, 17, "Line");
	line->callback(SetLine);


	window.end();                  // End of FLTK windows setting. 
	window.show(argc, argv);        // Show the FLTK window

	Fl_Window zoom_window(0, 0, 320, 240, "Zoom Window");
	openGL_window gl_win_zoom(0, 0, 320, 240);

	gl_win_zoom.zoom_window = &gl_win_zoom;
	gl_win.zoom_window = &gl_win_zoom;
	gl_win_zoom.main_window = &gl_win;
	gl_win.main_window = &gl_win;
	zoom_window.callback(CloseZoom);

	zoom_window.end();

	gl_win.show();                 // Show the openGL window
	gl_win.redraw_overlay();       // redraw 
	

	return Fl::run();
}