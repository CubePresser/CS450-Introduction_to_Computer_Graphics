#version 330 compatibility

uniform float	vTime;		// "Time", from Animate( )
out vec2  	vST;		// texture coords

const float PI = 	3.14159265;
const float AMP = 	0.2;		// amplitude
const float W = 	8.;		// frequency

void
main( )
{ 
	vST = gl_MultiTexCoord0.st;
	vST.s += sin((vST.s + vST.t) * W + (vTime));
	vST.t += cos((vST.s + vST.t) * W + (vTime * 3.0));
	gl_Position = gl_ModelViewProjectionMatrix * vec4( gl_Vertex.xyz, 1. );
}
