uniform restrict writeonly image2D outputTex;	//@ relativeTo(inputTex1)
uniform sampler2D inputTex1;	//@ input
uniform sampler2D inputTex2;	//@ input
uniform vec4 outputTex_size;

layout (local_size_x = 8, local_size_y = 8) in;
void main() {
	ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
	vec2 uv = (vec2(pix) + 0.5) * outputTex_size.zw;

	vec4 col = textureLod(inputTex1, uv, 0);
	col += textureLod(inputTex2, uv, 0);
	imageStore(outputTex, pix, col);
}
