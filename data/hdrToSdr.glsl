uniform restrict writeonly image2D outputTex;	//@ relativeTo(inputTex)
uniform float EV;	//@ min(-8) max(8)
uniform vec3 tint;	//@ color 
uniform sampler2D inputTex;	//@ input
uniform vec4 outputTex_size;

layout (local_size_x = 8, local_size_y = 8) in;
void main() {
	ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
	vec2 uv = (vec2(pix) + 0.5) * outputTex_size.zw;
	vec4 col = texture2D(inputTex, uv, 0);
	col *= exp(EV);
	col.rgb *= tint;
	col = 1.0 - exp(-col);
	imageStore(outputTex, pix, col);
}
