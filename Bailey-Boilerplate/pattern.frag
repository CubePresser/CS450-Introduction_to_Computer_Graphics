#version 330 compatibility

uniform float	fTime;		// "Time", from Animate( )
in vec2  	vST;		// texture coords

void
main( )
{
	vec3 myColor = vec3( vST.s * abs(sin(fTime * 2.7)), vST.t * abs(cos(fTime)), 1.0 );
	gl_FragColor = vec4( myColor,  1. );
}
