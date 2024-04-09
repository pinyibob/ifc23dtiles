precision highp float;

varying vec2 v_texcoord0;
varying vec3 v_normal;
varying vec3 v_pos;

uniform sampler2D u_texImage;
uniform vec4 u_diffuse;

uniform vec4  u_ambient;
uniform vec4  u_specular;
//uniform vec4  u_emission;
//uniform float  u_shininess;
uniform float  u_light;

vec3 SRGBtoLINEAR3(vec3 srgbIn) {
    return pow(srgbIn, vec3(2.2));
	//return srgbIn;
}

vec4 SRGBtoLINEAR4(vec4 srgbIn) {
    vec3 linearOut = pow(srgbIn.rgb, vec3(2.2));
    return vec4(linearOut, srgbIn.a);
	//return srgbIn;
}

vec3 LINEARtoSRGB(vec3 linearIn) {
    return pow(linearIn, vec3(1.0/2.2));
	//return linearIn;
}

void main(void)
{
	if (u_light > 0.5) {
		vec3 normal = normalize(v_normal);
		vec3 eyedir = -normalize(v_pos);
		vec3 ambient = SRGBtoLINEAR3(u_ambient.rgb);
		vec4 diffuse = SRGBtoLINEAR4(u_diffuse);

		diffuse.rgb += ambient;
		float mm = max(1.0, max(max(diffuse.x, diffuse.y), diffuse.z));

		diffuse *= SRGBtoLINEAR4(texture2D(u_texImage, v_texcoord0));
		diffuse.rgb *= max(abs(dot(normal, eyedir)), 0.);

		diffuse.rgb /= mm;
		diffuse.rgb = LINEARtoSRGB(diffuse.rgb);
		gl_FragColor = diffuse;
	}
	else
	{
		gl_FragColor = texture2D(u_texImage, v_texcoord0);
	}
}
