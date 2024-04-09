#include <cstdio>
#include <fstream>
#include <iostream>

std::string flatVS = R"(

precision highp float;
uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;
attribute vec3 a_position;
attribute vec2 a_texcoord0;
attribute float a_batchid;
varying vec2 v_texcoord0;
void main(void)
{	
	v_texcoord0 = a_texcoord0;
	gl_Position = u_projectionMatrix * u_modelViewMatrix * vec4(a_position, 1.0);
}

)";

std::string flatFS = R"(

precision highp float;
varying vec2 v_texcoord0;
uniform sampler2D u_diffuse;
void main(void)
{
 
  gl_FragColor = texture2D(u_diffuse, v_texcoord0);
	//gl_FragColor = vec4(v_texcoord0.x, v_texcoord0.y, 0, 1);
}

)";