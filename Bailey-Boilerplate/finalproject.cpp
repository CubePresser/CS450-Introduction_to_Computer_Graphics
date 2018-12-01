#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4996)
#include "glew.h"
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include "freeglut.h"
#include "glui.h"

// title of these windows:

const char *WINDOWTITLE = { "CS450 Final Project - Jonathan Jones" };
const char *GLUITITLE   = { "User Interface Window" };


// what the glui package defines as true and false:

const int GLUITRUE  = { true  };
const int GLUIFALSE = { false };


// the escape key:

#define ESCAPE		0x1b
#define NUMPOINTS   100


// initial window size:

const int INIT_WINDOW_SIZE = { 600 };

// multiplication factors for input interaction:
//  (these are known from previous experience)

const float ANGFACT = { 1. };
const float SCLFACT = { 0.005f };

// able to use the left mouse for either rotation or scaling,
// in case have only a 2-button mouse:
enum LeftButton
{
	ROTATE,
	SCALE
};

// minimum allowable scale factor:

const float MINSCALE = { 0.05f };


// active mouse buttons (or them together):

const int LEFT   = { 4 };
const int MIDDLE = { 2 };
const int RIGHT  = { 1 };

// which button:

enum ButtonVals
{
	PLAY,
	RESET,
	QUIT
};


// window background color (rgba):

const GLfloat BACKCOLOR[ ] = { 0., 0., 0., 1. };


// line width for the axes:

const GLfloat AXES_WIDTH   = { 3. };

// non-constant global variables:

int		ActiveButton;			// current button that is down
GLuint	AxesList;				// list to hold the axes
int		AxesOn;					// != 0 means to draw the axes
int		DebugOn;				// != 0 means to print debugging info
int		ControlPoints;
int		MainWindow;				// window id for main graphics window
float	Scale, Scale2;			// scaling factor
int		Perspective;			// ORTHO or PERSP
int		Xmouse, Ymouse;			// mouse values
float	Xrot, Yrot;				// rotation angles in degrees
float	Hue;					// Value for color of surface

//Bezier curves

struct Point
{
	float x0, y0, z0;       // initial coordinates
	float x, y, z;        // animated coordinates
};

struct Curve
{
	float r, g, b;
	Point p0, p1, p2, p3;
};

Curve Curves[4];

//Animation
int		animate_start_time;
int		time_frozen;
float	Time;
bool	Play;

//GLUI globals
GLUI *	Glui;				// instance of glui window
int	GluiWindow;				// the glut id for the glui window
GLfloat	RotMatrix[4][4];	// set by glui rotation widget
float	TransXYZ[3];		// set by glui translation widgets

//Structure to hold all the information needed for a slider on the GLUI panel
struct GLUI_SliderPackage
{
	GLUI_HSlider* slider;
	GLUI_EditText* edit_text;

};

//Slider identifiers
enum SliderVals {
	HUE
};

struct GLUI_SliderPackage sliders[1];

// function prototypes:

void	Animate( );
void	Buttons(int);
void	Display( );
float	ElapsedSeconds( );
void	InitGlui();
void	InitGraphics( );
void	InitLists( );
void	Keyboard( unsigned char, int, int );
void	MouseButton( int, int, int, int );
void	MouseMotion( int, int );
void	Reset( );
void	Resize( int, int );
void	Visibility( int );

void	Axes( float );
void	HsvRgb( float[3], float [3] );
void	UpdateGLUI(int);
void	InitCurves();
void	BezierCurve(float[NUMPOINTS][3], Curve); //Returns a list of vertices in a bezier curve

// main program:

int
main( int argc, char *argv[ ] )
{
	// turn on the glut package:
	// (do this before checking argc and argv since it might
	// pull some command line arguments out)

	glutInit( &argc, argv );


	// setup all the graphics stuff:

	InitGraphics( );


	// create the display structures that will not change:

	InitLists( );


	// init all the global variables used by Display( ):
	// this will also post a redisplay

	Reset( );


	// setup all the user interface stuff:

	InitGlui( );


	// draw the scene once and wait for some interaction:
	// (this will never return)

	glutSetWindow( MainWindow );
	glutMainLoop( );


	// this is here to make the compiler happy:

	return 0;
}


// this is where one would put code that is to be called
// everytime the glut main loop has nothing to do
//
// this is typically where animation parameters are set
//
// do not call Display( ) from here -- let glutMainLoop( ) do it

void
Animate( )
{
	float seconds = ((float)(glutGet(GLUT_ELAPSED_TIME) - animate_start_time) / 1000.f);
	float dt = seconds - Time;
	Time = seconds;

	Glui->sync_live();
	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}

void Buttons(int id)
{
	switch (id)
	{
	case PLAY:
		Play = !Play;
		if (!Play)
		{
			time_frozen = glutGet(GLUT_ELAPSED_TIME) - animate_start_time;
			GLUI_Master.set_glutIdleFunc(NULL);
		}
		else
		{
			animate_start_time = glutGet(GLUT_ELAPSED_TIME) - time_frozen;
			GLUI_Master.set_glutIdleFunc(Animate);
		}
		break;

	case RESET:
		Reset();
		UpdateGLUI(-1);
		Glui->sync_live();
		glutSetWindow(MainWindow);
		glutPostRedisplay();
		break;

	case QUIT:
		// gracefully close the glui window:
		// gracefully close out the graphics:
		// gracefully close the graphics window:
		// gracefully exit the program:

		Glui->close();
		glutSetWindow(MainWindow);
		glFinish();
		glutDestroyWindow(MainWindow);
		exit(0);
		break;

	default:
		fprintf(stderr, "Don't know what to do with Button ID %d\n", id);
	}

}

// draw the complete scene:

void
Display()
{


	if (DebugOn != 0)
	{
		fprintf(stderr, "Display\n");
	}


	// set which window we want to do the graphics into:

	glutSetWindow(MainWindow);


	// erase the background:

	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);

	// specify shading to be flat:

	glShadeModel(GL_SMOOTH);


	// set the viewport to a square centered in the window:

	GLsizei vx = glutGet(GLUT_WINDOW_WIDTH);
	GLsizei vy = glutGet(GLUT_WINDOW_HEIGHT);
	GLsizei v = vx < vy ? vx : vy;			// minimum dimension
	GLint xl = (vx - v) / 2;
	GLint yb = (vy - v) / 2;
	glViewport(xl, yb, v, v);


	// set the viewing volume:
	// remember that the Z clipping  values are actually
	// given as DISTANCES IN FRONT OF THE EYE
	// USE gluOrtho2D( ) IF YOU ARE DOING 2D !

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (!Perspective)
		glOrtho(-3., 3., -3., 3., 0.1, 1000.);
	else
		gluPerspective(90., 1., 0.1, 1000.);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


	// set the eye position, look-at position, and up-vector:

	gluLookAt(0., 0., 3., 0., 0., 0., 0., 1., 0.);


	// rotate the scene:

	glTranslatef((GLfloat)TransXYZ[0], (GLfloat)TransXYZ[1], -(GLfloat)TransXYZ[2]);
	glRotatef((GLfloat)Yrot, 0., 1., 0.);
	glRotatef((GLfloat)Xrot, 1., 0., 0.);
	glMultMatrixf((const GLfloat *)RotMatrix);
	glScalef((GLfloat)Scale, (GLfloat)Scale, (GLfloat)Scale);
	GLfloat scale2 = 1. + Scale2;
	if (scale2 < MINSCALE)
		scale2 = MINSCALE;
	glScalef((GLfloat)scale2, (GLfloat)scale2, (GLfloat)scale2);

	//Get some color!
	float rgb[3];
	float hsv[3] = { Hue, 1.f, 1.f };
	HsvRgb(hsv, rgb);
	glColor3f(rgb[0], rgb[1], rgb[2]);

	// place the objects into the scene here ----------------------------------------------------------

	double delta = sin(Time) * 1.5f;
	//Animate control points
	Curves[0].p0.y = 0.f;
	Curves[0].p1.y = delta * .25f;
	Curves[0].p2.y = delta * .5f;
	Curves[0].p3.y = 0.f;

	Curves[1].p0.y = delta * .25f;
	Curves[1].p1.y = delta * .5f;
	Curves[1].p2.y = delta * .75f;
	Curves[1].p3.y = delta * .5f;

	Curves[2].p0.y = delta * .5f;
	Curves[2].p1.y = delta * .75f;
	Curves[2].p2.y = delta * .5f;
	Curves[2].p3.y = delta * .25f;

	Curves[3].p0.y = 0.f;
	Curves[3].p1.y = delta * .5f;
	Curves[3].p2.y = delta * .25f;
	Curves[3].p3.y = 0.f;

	//Display control points

	if (ControlPoints)
	{
		glPointSize(5.f);
		glBegin(GL_POINTS);
		for (int i = 0; i < 4; i++)
		{
			glVertex3f(Curves[i].p0.x, Curves[i].p0.y, Curves[i].p0.z);
			glVertex3f(Curves[i].p1.x, Curves[i].p1.y, Curves[i].p1.z);
			glVertex3f(Curves[i].p2.x, Curves[i].p2.y, Curves[i].p2.z);
			glVertex3f(Curves[i].p3.x, Curves[i].p3.y, Curves[i].p3.z);
		}
		glEnd();
		glPointSize(1.f);
	}

	//Generate all the vertices in the mesh
	float curve0[NUMPOINTS][3];
	float curve1[NUMPOINTS][3];
	float curve2[NUMPOINTS][3];
	float curve3[NUMPOINTS][3];

	float vertexBuffer[NUMPOINTS * NUMPOINTS][3];

	BezierCurve(curve0, Curves[0]);
	BezierCurve(curve1, Curves[1]);
	BezierCurve(curve2, Curves[2]);
	BezierCurve(curve3, Curves[3]);

	int bufferIndex = 0;

	//Fill vertex array
	for (int i = 0; i < NUMPOINTS; i++)
	{
		Curve aux;
		float auxCurve[NUMPOINTS][3];

		aux.p0.x = curve0[i][0];
		aux.p0.y = curve0[i][1];
		aux.p0.z = curve0[i][2];

		aux.p1.x = curve1[i][0];
		aux.p1.y = curve1[i][1];
		aux.p1.z = curve1[i][2];

		aux.p2.x = curve2[i][0];
		aux.p2.y = curve2[i][1];
		aux.p2.z = curve2[i][2];

		aux.p3.x = curve3[i][0];
		aux.p3.y = curve3[i][1];
		aux.p3.z = curve3[i][2];
		
		BezierCurve(auxCurve, aux);

		for (int k = 0; k < NUMPOINTS; k++)
		{
			vertexBuffer[bufferIndex][0] = auxCurve[k][0];
			vertexBuffer[bufferIndex][1] = auxCurve[k][1];
			vertexBuffer[bufferIndex][2] = auxCurve[k][2];
			bufferIndex++;
		}
	}

	//Display all vertices
	glBegin(GL_POINTS);
	for (int i = 0; i < NUMPOINTS * NUMPOINTS; i++)
	{
		glVertex3f(vertexBuffer[i][0], vertexBuffer[i][1], vertexBuffer[i][2]);
	}
	glEnd();

	// possibly draw the axes:
	if( AxesOn != 0 )
	{
		glColor3f(1.f, 1.f, 1.f);
		glCallList( AxesList );
	}


	// since we are using glScalef( ), be sure normals get unitized:

	glEnable( GL_NORMALIZE );

	// the projection matrix is reset to define a scene whose
	// world coordinate system goes from 0-100 in each axis
	//
	// this is called "percent units", and is just a convenience
	//
	// the modelview matrix is reset to identity as we don't
	// want to transform these coordinates

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	gluOrtho2D( 0., 100.,     0., 100. );


	// swap the double-buffered framebuffers:

	glutSwapBuffers( );


	// be sure the graphics buffer has been sent:
	// note: be sure to use glFlush( ) here, not glFinish( ) !

	glFlush( );
}

// return the number of seconds since the start of the program:

float
ElapsedSeconds( )
{
	// get # of milliseconds since the start of the program:

	int ms = glutGet( GLUT_ELAPSED_TIME );

	// convert it to seconds:

	return (float)ms / 1000.f;
}


void InitGlui(void)
{
	GLUI_Panel *panel;
	GLUI_Translation *trans, *scale;
	GLUI_Rotation *rot;

	// setup the glui window:

	glutInitWindowPosition(INIT_WINDOW_SIZE + 50, 0);
	Glui = GLUI_Master.create_glui((char *)GLUITITLE);


	Glui->add_statictext((char *)GLUITITLE);
	Glui->add_separator();

	//Axes
	Glui->add_checkbox("Axes", &AxesOn);

	//Debug
	Glui->add_checkbox("Debug", &DebugOn);

	//View
	Glui->add_checkbox("Perspective", &Perspective);

	//Control Points
	Glui->add_checkbox("Control Points", &ControlPoints);

	Glui->add_statictext("Hue");
	sliders[HUE].slider = Glui->add_slider(false, GLUI_HSLIDER_FLOAT, &Hue);
	sliders[HUE].slider->set_float_limits(0.f, 360.f);
	sliders[HUE].slider->set_w(200);
	sliders[HUE].slider->set_slider_val(Hue);
	sliders[HUE].edit_text = Glui->add_edittext("", GLUI_EDITTEXT_FLOAT, &Hue, HUE, (GLUI_Update_CB)UpdateGLUI);
	Glui->add_separator();

	panel = Glui->add_panel("Scene Transformation");

	rot = Glui->add_rotation_to_panel(panel, "Rotation", (float *)RotMatrix);

	rot->set_spin(1.0);

	Glui->add_column_to_panel(panel, GLUIFALSE);
	scale = Glui->add_translation_to_panel(panel, "Zoom", GLUI_TRANSLATION_Y, &Scale2);
	scale->set_speed(0.01f);

	Glui->add_column_to_panel(panel, GLUIFALSE);
	trans = Glui->add_translation_to_panel(panel, "Trans XY", GLUI_TRANSLATION_XY, &TransXYZ[0]);
	trans->set_speed(1.1f);

	Glui->add_column_to_panel(panel, FALSE);
	trans = Glui->add_translation_to_panel(panel, "Trans Z", GLUI_TRANSLATION_Z, &TransXYZ[2]);
	trans->set_speed(1.1f);

	panel = Glui->add_panel("", FALSE);

	Glui->add_button_to_panel(panel, "Play / Pause", PLAY, (GLUI_Update_CB)Buttons);

	Glui->add_column_to_panel(panel, FALSE);

	Glui->add_button_to_panel(panel, "Reset", RESET, (GLUI_Update_CB)Buttons);

	Glui->add_column_to_panel(panel, FALSE);

	Glui->add_button_to_panel(panel, "Quit", QUIT, (GLUI_Update_CB)Buttons);


	// tell glui what graphics window it needs to post a redisplay to:

	Glui->set_main_gfx_window(MainWindow);


	// set the graphics window's idle function:

	GLUI_Master.set_glutIdleFunc(Animate);
}



// initialize the glut and OpenGL libraries:
//	also setup display lists and callback functions

void
InitGraphics( )
{
	// request the display modes:
	// ask for red-green-blue-alpha color, double-buffering, and z-buffering:

	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );

	// set the initial window configuration:

	glutInitWindowPosition( 0, 0 );
	glutInitWindowSize( INIT_WINDOW_SIZE, INIT_WINDOW_SIZE );

	// open the window and set its title:

	MainWindow = glutCreateWindow( WINDOWTITLE );
	glutSetWindowTitle( WINDOWTITLE );

	// set the framebuffer clear values:

	glClearColor( BACKCOLOR[0], BACKCOLOR[1], BACKCOLOR[2], BACKCOLOR[3] );

	// setup the callback functions:
	// DisplayFunc -- redraw the window
	// ReshapeFunc -- handle the user resizing the window
	// KeyboardFunc -- handle a keyboard input
	// MouseFunc -- handle the mouse button going down or up
	// MotionFunc -- handle the mouse moving with a button down
	// PassiveMotionFunc -- handle the mouse moving with a button up
	// VisibilityFunc -- handle a change in window visibility
	// EntryFunc	-- handle the cursor entering or leaving the window
	// SpecialFunc -- handle special keys on the keyboard
	// SpaceballMotionFunc -- handle spaceball translation
	// SpaceballRotateFunc -- handle spaceball rotation
	// SpaceballButtonFunc -- handle spaceball button hits
	// ButtonBoxFunc -- handle button box hits
	// DialsFunc -- handle dial rotations
	// TabletMotionFunc -- handle digitizing tablet motion
	// TabletButtonFunc -- handle digitizing tablet button hits
	// MenuStateFunc -- declare when a pop-up menu is in use
	// TimerFunc -- trigger something to happen a certain time from now
	// IdleFunc -- what to do when nothing else is going on

	glutSetWindow( MainWindow );
	glutDisplayFunc( Display );
	glutReshapeFunc( Resize );
	glutKeyboardFunc( Keyboard );
	glutMouseFunc( MouseButton );
	glutMotionFunc( MouseMotion );
	glutPassiveMotionFunc( NULL );
	glutVisibilityFunc( Visibility );
	glutEntryFunc( NULL );
	glutSpecialFunc( NULL );
	glutSpaceballMotionFunc( NULL );
	glutSpaceballRotateFunc( NULL );
	glutSpaceballButtonFunc( NULL );
	glutButtonBoxFunc( NULL );
	glutDialsFunc( NULL );
	glutTabletMotionFunc( NULL );
	glutTabletButtonFunc( NULL );
	glutMenuStateFunc( NULL );
	glutTimerFunc( -1, NULL, 0 );

	// init glew (a window must be open to do this):

	#ifdef WIN32
		GLenum err = glewInit( );
		if( err != GLEW_OK )
		{
			fprintf( stderr, "glewInit Error\n" );
		}
		else
			fprintf( stderr, "GLEW initialized OK\n" );
		fprintf( stderr, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
	#endif

	InitCurves();
}


// initialize the display lists that will not change:
// (a display list is a way to store opengl commands in
//  memory so that they can be played back efficiently at a later time
//  with a call to glCallList( )

void
InitLists( )
{
	//Create the axes:
	AxesList = glGenLists( 1 );
	glNewList( AxesList, GL_COMPILE );
		glLineWidth( AXES_WIDTH );
			Axes( 1.5 );
		glLineWidth( 1. );
	glEndList( );
}


// the keyboard callback:
void Keyboard(unsigned char c, int x, int y)
{
	if (DebugOn != 0)
		fprintf(stderr, "Keyboard: '%c' (0x%0x)\n", c, c);

	switch (c)
	{
	case 'r':
	case 'R':
		Buttons(RESET);
		break;
	case 'q':
	case 'Q':
	case ESCAPE:
		Buttons(QUIT);	// will not return here
		break;				// happy compiler
	case 'p':
	case 'P':
		Buttons(PLAY);
		break;

	default:
		fprintf(stderr, "Don't know what to do with keyboard hit: '%c' (0x%0x)\n", c, c);
	}

	// synchronize the GLUI display with the variables:
	Glui->sync_live();

	// force a call to Display( ):
	glutSetWindow(MainWindow);
	glutPostRedisplay();
}


// called when the mouse button transitions down or up:

void
MouseButton( int button, int state, int x, int y )
{
	int b = 0;			// LEFT, MIDDLE, or RIGHT

	if( DebugOn != 0 )
		fprintf( stderr, "MouseButton: %d, %d, %d, %d\n", button, state, x, y );

	
	// get the proper button bit mask:

	switch( button )
	{
		case GLUT_LEFT_BUTTON:
			b = LEFT;		break;

		case GLUT_MIDDLE_BUTTON:
			b = MIDDLE;		break;

		case GLUT_RIGHT_BUTTON:
			b = RIGHT;		break;

		default:
			b = 0;
			fprintf( stderr, "Unknown mouse button: %d\n", button );
	}


	// button down sets the bit, up clears the bit:

	if( state == GLUT_DOWN )
	{
		Xmouse = x;
		Ymouse = y;
		ActiveButton |= b;		// set the proper bit
	}
	else
	{
		ActiveButton &= ~b;		// clear the proper bit
	}
}


// called when the mouse moves while a button is down:

void
MouseMotion( int x, int y )
{
	if( DebugOn != 0 )
		fprintf( stderr, "MouseMotion: %d, %d\n", x, y );


	int dx = x - Xmouse;		// change in mouse coords
	int dy = y - Ymouse;

	if( ( ActiveButton & LEFT ) != 0 ) //If the left button bit is active
	{
		Xrot += ( ANGFACT*dy );
		Yrot += ( ANGFACT*dx );
	}


	if( ( ActiveButton & MIDDLE ) != 0 ) //If the middle button bit is active
	{
		Scale += SCLFACT * (float) ( dx - dy );

		// keep object from turning inside-out or disappearing:

		if( Scale < MINSCALE )
			Scale = MINSCALE;
	}

	Xmouse = x;			// new current position
	Ymouse = y;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// reset the transformations and the colors:
// this only sets the global variables --
// the glut main loop is responsible for redrawing the scene

void
Reset( )
{
	ActiveButton = 0;
	AxesOn = GLUITRUE;
	DebugOn = GLUIFALSE;
	Perspective = GLUITRUE;
	Scale  = 1.0;
	Xrot = Yrot = 0.;
	Hue = 0.f;
	ControlPoints = GLUIFALSE;

	TransXYZ[0] = TransXYZ[1] = TransXYZ[2] = 0.;

	RotMatrix[0][1] = RotMatrix[0][2] = RotMatrix[0][3] = 0.;
	RotMatrix[1][0] = RotMatrix[1][2] = RotMatrix[1][3] = 0.;
	RotMatrix[2][0] = RotMatrix[2][1] = RotMatrix[2][3] = 0.;
	RotMatrix[3][0] = RotMatrix[3][1] = RotMatrix[3][3] = 0.;
	RotMatrix[0][0] = RotMatrix[1][1] = RotMatrix[2][2] = RotMatrix[3][3] = 1.;

	Time = 0.f;
	Play = true;

	animate_start_time = glutGet(GLUT_ELAPSED_TIME);
	time_frozen = 0;
}


// called when user resizes the window:

void
Resize( int width, int height )
{
	if( DebugOn != 0 )
		fprintf( stderr, "ReSize: %d, %d\n", width, height );

	// don't really need to do anything since window size is
	// checked each time in Display( ):

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// handle a change to the window's visibility:

void
Visibility ( int state )
{
	if( DebugOn != 0 )
		fprintf( stderr, "Visibility: %d\n", state );

	if( state == GLUT_VISIBLE )
	{
		glutSetWindow( MainWindow );
		glutPostRedisplay( );
	}
	else
	{
		// could optimize by keeping track of the fact
		// that the window is not visible and avoid
		// animating or redrawing it ...
	}
}



///////////////////////////////////////   HANDY UTILITIES:  //////////////////////////


// the stroke characters 'X' 'Y' 'Z' :

static float xx[ ] = {
		0.f, 1.f, 0.f, 1.f
	      };

static float xy[ ] = {
		-.5f, .5f, .5f, -.5f
	      };

static int xorder[ ] = {
		1, 2, -3, 4
		};

static float yx[ ] = {
		0.f, 0.f, -.5f, .5f
	      };

static float yy[ ] = {
		0.f, .6f, 1.f, 1.f
	      };

static int yorder[ ] = {
		1, 2, 3, -2, 4
		};

static float zx[ ] = {
		1.f, 0.f, 1.f, 0.f, .25f, .75f
	      };

static float zy[ ] = {
		.5f, .5f, -.5f, -.5f, 0.f, 0.f
	      };

static int zorder[ ] = {
		1, 2, 3, 4, -5, 6
		};

// fraction of the length to use as height of the characters:
const float LENFRAC = 0.10f;

// fraction of length to use as start location of the characters:
const float BASEFRAC = 1.10f;

//	Draw a set of 3D axes:
//	(length is the axis length in world coordinates)

void
Axes( float length )
{
	glBegin( GL_LINE_STRIP );
		glVertex3f( length, 0., 0. );
		glVertex3f( 0., 0., 0. );
		glVertex3f( 0., length, 0. );
	glEnd( );
	glBegin( GL_LINE_STRIP );
		glVertex3f( 0., 0., 0. );
		glVertex3f( 0., 0., length );
	glEnd( );

	float fact = LENFRAC * length;
	float base = BASEFRAC * length;

	glBegin( GL_LINE_STRIP );
		for( int i = 0; i < 4; i++ )
		{
			int j = xorder[i];
			if( j < 0 )
			{
				
				glEnd( );
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( base + fact*xx[j], fact*xy[j], 0.0 );
		}
	glEnd( );

	glBegin( GL_LINE_STRIP );
		for( int i = 0; i < 5; i++ )
		{
			int j = yorder[i];
			if( j < 0 )
			{
				
				glEnd( );
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( fact*yx[j], base + fact*yy[j], 0.0 );
		}
	glEnd( );

	glBegin( GL_LINE_STRIP );
		for( int i = 0; i < 6; i++ )
		{
			int j = zorder[i];
			if( j < 0 )
			{
				
				glEnd( );
				glBegin( GL_LINE_STRIP );
				j = -j;
			}
			j--;
			glVertex3f( 0.0, fact*zy[j], base + fact*zx[j] );
		}
	glEnd( );

}


// function to convert HSV to RGB
// 0.  <=  s, v, r, g, b  <=  1.
// 0.  <= h  <=  360.
// when this returns, call:
//		glColor3fv( rgb );

void
HsvRgb( float hsv[3], float rgb[3] )
{
	// guarantee valid input:

	float h = hsv[0] / 60.f;
	while( h >= 6. )	h -= 6.;
	while( h <  0. ) 	h += 6.;

	float s = hsv[1];
	if( s < 0. )
		s = 0.;
	if( s > 1. )
		s = 1.;

	float v = hsv[2];
	if( v < 0. )
		v = 0.;
	if( v > 1. )
		v = 1.;

	// if sat==0, then is a gray:

	if( s == 0.0 )
	{
		rgb[0] = rgb[1] = rgb[2] = v;
		return;
	}

	// get an rgb from the hue itself:
	
	float i = floor( h );
	float f = h - i;
	float p = v * ( 1.f - s );
	float q = v * ( 1.f - s*f );
	float t = v * ( 1.f - ( s * (1.f-f) ) );

	float r, g, b;			// red, green, blue
	switch( (int) i )
	{
		case 0:
			r = v;	g = t;	b = p;
			break;
	
		case 1:
			r = q;	g = v;	b = p;
			break;
	
		case 2:
			r = p;	g = v;	b = t;
			break;
	
		case 3:
			r = p;	g = q;	b = v;
			break;
	
		case 4:
			r = t;	g = p;	b = v;
			break;
	
		case 5:
			r = v;	g = p;	b = q;
			break;
	}


	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}

void UpdateGLUI(int id)
{
	switch (id)
	{
	case HUE:
		sliders[HUE].slider->set_slider_val(Hue);
		break;
	default:
		sliders[HUE].slider->set_slider_val(Hue);
	}
	Glui->sync_live();
}

void InitCurves()
{
	Curves[0].p0.x0 = Curves[0].p0.x = -1.5f;
	Curves[0].p0.y0 = 0.f;
	Curves[0].p0.z0 = Curves[0].p0.z = -1.5f;

	Curves[0].p1.x0 = Curves[0].p1.x = -.5f;
	Curves[0].p1.y0 = 0.f;
	Curves[0].p1.z0 = Curves[0].p1.z = -1.5f;

	Curves[0].p2.x0 = Curves[0].p2.x = .5f;
	Curves[0].p2.y0 = 0.f;
	Curves[0].p2.z0 = Curves[0].p2.z = -1.5f;

	Curves[0].p3.x0 = Curves[0].p3.x = 1.5f;
	Curves[0].p3.y0 = 0.f;
	Curves[0].p3.z0 = Curves[0].p3.z = -1.5f;

	Curves[1].p0.x0 = Curves[1].p0.x = -1.5f;
	Curves[1].p0.y0 = 0.f;
	Curves[1].p0.z0 = Curves[1].p0.z = -.5f;

	Curves[1].p1.x0 = Curves[1].p1.x = -.5f;
	Curves[1].p1.y0 = 0.f;
	Curves[1].p1.z0 = Curves[1].p1.z = -.5f;

	Curves[1].p2.x0 = Curves[1].p2.x = .5f;
	Curves[1].p2.y0 = 0.f;
	Curves[1].p2.z0 = Curves[1].p2.z = -.5f;

	Curves[1].p3.x0 = Curves[1].p3.x = 1.5f;
	Curves[1].p3.y0 = 0.f;
	Curves[1].p3.z0 = Curves[1].p3.z = -.5f;

	Curves[2].p0.x0 = Curves[2].p0.x = -1.5f;
	Curves[2].p0.y0 = 0.f;
	Curves[2].p0.z0 = Curves[2].p0.z = .5f;

	Curves[2].p1.x0 = Curves[2].p1.x = -.5f;
	Curves[2].p1.y0 = 0.f;
	Curves[2].p1.z0 = Curves[2].p1.z = .5f;

	Curves[2].p2.x0 = Curves[2].p2.x = .5f;
	Curves[2].p2.y0 = 0.f;
	Curves[2].p2.z0 = Curves[2].p2.z = .5f;

	Curves[2].p3.x0 = Curves[2].p3.x = 1.5f;
	Curves[2].p3.y0 = 0.f;
	Curves[2].p3.z0 = Curves[2].p3.z = .5f;

	Curves[3].p0.x0 = Curves[3].p0.x = -1.5f;
	Curves[3].p0.y0 = 0.f;
	Curves[3].p0.z0 = Curves[3].p0.z = 1.5f;

	Curves[3].p1.x0 = Curves[3].p1.x = -.5f;
	Curves[3].p1.y0 = 0.f;
	Curves[3].p1.z0 = Curves[3].p1.z = 1.5f;

	Curves[3].p2.x0 = Curves[3].p2.x = .5f;
	Curves[3].p2.y0 = 0.f;
	Curves[3].p2.z0 = Curves[3].p2.z = 1.5f;

	Curves[3].p3.x0 = Curves[3].p3.x = 1.5f;
	Curves[3].p3.y0 = 0.f;
	Curves[3].p3.z0 = Curves[3].p3.z = 1.5f;
}

void BezierCurve(float vertexList[NUMPOINTS][3], Curve curve)
{
	for (int it = 0; it < NUMPOINTS; it++)
	{
		float t = (float)it / (float)NUMPOINTS;
		float omt = 1.f - t;
		float x = omt * omt*omt*curve.p0.x + 3.f*t*omt*omt*curve.p1.x + 3.f*t*t*omt*curve.p2.x + t * t*t*curve.p3.x;
		float y = omt * omt*omt*curve.p0.y + 3.f*t*omt*omt*curve.p1.y + 3.f*t*t*omt*curve.p2.y + t * t*t*curve.p3.y;
		float z = omt * omt*omt*curve.p0.z + 3.f*t*omt*omt*curve.p1.z + 3.f*t*t*omt*curve.p2.z + t * t*t*curve.p3.z;
		vertexList[it][0] = x;
		vertexList[it][1] = y;
		vertexList[it][2] = z;
	}
}