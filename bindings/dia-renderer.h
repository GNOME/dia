/*
 * C++ interface of DiaRenderer - just to have something clean to wrap
 *
 * Copyright 2007, Hans Breuer, GPL, see COPYING
 */
#ifndef DIA__RENDERER_H
#define DIA__RENDERER_H

namespace dia {

//! forward declare
class Object;
class Image;
class Font;

/*!
 * \brief Renderer provides functions to draw Object by their simple geometrics
 *
 * This class is not very useful until one wants to implement an Object in the target
 * language. Than it is needed to abstract away the the real drawing to different
 * backends from the drawing code in Object::draw(Renderer*).
 * Another use-case may be a unit test where single renderers functions chould be
 * tested. For both cases a renderer factory function would be needed.
 */
class Renderer
{
public :
    /// \defgroup InteractiveRenderer The 'interactive' renderer interface
    /// IIRC this functions are only supported by interactive renderers

    //! return width in pixels, only for interactive renderers
    //! \ingroup InteractiveRenderer
    virtual int get_width_pixels () const;
    //!  return width in pixels, only for interactive renderers
    //! \ingroup InteractiveRenderer
    virtual int get_height_pixels () const;
    
    /// \defgroup RendererRequired Functions to be implemented by every Renderer
    /// Not sure about this ;)

    //! simply calls the objects draw function, which calls this again
    //! \ingroup RendererRequired
    virtual void draw_object (Object*);
    //! Returns the EXACT width of text in cm, using the current font.
    //! There has been some confusion as to the definition of this.
    //! It used to say the width was in pixels, but actual width returned
    //! was cm.  You shouldn't know about pixels anyway.
    //! \ingroup RendererRequired
    virtual double get_text_width (const gchar *text, int length) const;

    //! called before any rendering takes palce
    //! \ingroup RendererRequired
    virtual void begin_render (const Rectangle *);
    //! finished rendering
    //! \ingroup RendererRequired
    virtual void end_render ();
    //! set current line width
    //! \ingroup RendererRequired
    virtual void set_linewidth (double w);
    //! set current ends
    //! \ingroup RendererRequired
    virtual void set_linecaps (LineCaps mode);
    //! set current joining
    //! \ingroup RendererRequired
    virtual void set_linejoin (LineJoin join);
    //! set current line style (dottet, dashed, solid)
    //! set current length for dashes
    //! \ingroup RendererRequired
    virtual void set_linestyle (LineStyle style, real dash_length);
    //! set current font
    //! \ingroup RendererRequired
    //! \todo wrap font objecs ?
    virtual void set_font (Font* font, double height);
    //! Draw a line from start to end, using color and the current line style
    //! \ingroup RendererRequired
    virtual void draw_line (Point *start, Point *end, Color *color);
    //! Fill and/or stroke a rectangle, given its upper-left and lower-right corners
    //! \ingroup RendererRequired
    virtual void draw_rect (Point *ul_corner, Point *lr_corner, Color *fill, Color *stroke);
    //! the polygon is filled using the current fill type, no border is drawn
    //! \ingroup RendererRequired
    virtual void draw_polygon (Point *points, int num_points, Color *fill, Color *stroke);
    //! Draw an arc, given its center, the bounding box (widget, height), the start angle and the end angle
    //! \ingroup RendererRequired
    virtual void draw_arc (Point *center, double width, double height,
                           double angle1, double angle2,
                           Color *color);
    //! Same a DrawArcFunc except the arc is filled (a pie-chart)
    //! \ingroup RendererRequired
    virtual void fill_arc (Point *center, double width, double height,
                           double angle1, double angle2,
                           Color *color);
    //! Draw an ellipse, given its center and the bounding box
    //! \ingroup RendererRequired
    virtual void draw_ellipse (Point *center, double width, double height, Color *fill, Color *stroke);
    //! Print a string at pos, using the current font
    //! \ingroup RendererRequired
    virtual void draw_string (const gchar *text, Point *pos, Alignment alignment, Color *color);
    //! Draw an image, given its bounding box
    //! \ingroup RendererRequired
    virtual void draw_image (Point *point, double width, double height, Image* image);
    
    /// \defgroup RenderMedium Renderer Should Implemement This
    /// Functions which SHOULD be implemented by specific renderer, but
    /// have a default implementation based on the above functions

    //! draw a bezier line - possibly as approximation consisting of straight lines
    //! \ingroup RenderMedium
    virtual void draw_bezier (BezPoint *points, int numpoints, Color *color);
    //! fill and/or stroke a bezier - possibly as approximation consisting of a polygon
    //! \ingroup RenderMedium
    virtual void draw_beziergon (BezPoint *points, int numpoints, Color *fill, Color *stroke);
    //! drawing a polyline - or fallback to single line segments
    //! \ingroup RenderMedium
    virtual void draw_polyline (Point *points, int num_points, Color *color);
    //! draw a Text.  It holds its own information like position, style, ...
    //! \ingroup RenderMedium
    virtual void draw_text (Text* text);
    
    /// \defgroup RenderHigh Renderer Can Implement This
    //! Highest level functions, probably only to be implemented by 
    //! special 'high level' renderers which have a concept of arrows or rounded drawings

    //! a polyline with round coners
    //! \ingroup RenderHigh
    virtual void draw_rounded_polyline (Point *points, int num_points, Color *color, double radius);
    //! specialized draw_rect() with round corners
    //! \ingroup RenderHigh
    virtual void draw_rounded_rect (Point *ul_corner, Point *lr_corner,
				    Color *fill, Color *stroke, real radius);
    //! specialized draw_line() for renderers with an own concept of Arrow
    //! \ingroup RenderHigh
    virtual void draw_line_with_arrows  (Point *start, Point *end, real line_width, Color *line_color, 
					 Arrow *start_arrow, Arrow *end_arrow);
    //! specialized draw_line() for renderers with an own concept of Arrow
    //! \ingroup RenderHigh
    virtual void draw_arc_with_arrows  (Point *start, Point *end, Point *midpoint, real line_width, Color *color,
					Arrow *start_arrow, Arrow *end_arrow);
    //! specialized draw_polyline() for renderers with an own concept of Arrow
    //! \ingroup RenderHigh
    virtual void draw_polyline_with_arrows (Point *points, int num_points, real line_width, Color *color,
					    Arrow *start_arrow, Arrow *end_arrow);
    //! specialized draw_rounded_polyline() for renderers with an own concept of Arrow
    //! \ingroup RenderHigh
    virtual void draw_rounded_polyline_with_arrows (Point *points, int num_points, real line_width, Color *color,
						    Arrow *start_arrow, Arrow *end_arrow, real radius);
    //! specialized draw_bezier() for renderers with an own concept of Arrow
    //! \ingroup RenderHigh
    virtual void draw_bezier_with_arrows (BezPoint *points, int num_points, real line_width, Color *color,
					  Arrow *start_arrow, Arrow *end_arrow);
    
private :
    DiaRenderer* self;     
};

} // namespace dia
#endif /* DIA__RENDERER_H */
