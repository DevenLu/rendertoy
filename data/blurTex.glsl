uniform restrict writeonly image2D outputTex;	//@ relativeTo(inputTex)
uniform int blurRadius;	//@ max(30)
uniform ivec2 blurDir;	//@ min(0) max(1)
uniform sampler2D inputTex;	//@ input
uniform vec4 outputTex_size;

layout (local_size_x = 8, local_size_y = 8) in;
void main() {
	ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
	vec2 uv = (vec2(pix) + 0.5) * outputTex_size.zw;
	vec2 delta = blurDir * outputTex_size.zw;

	vec4 col = textureLod(inputTex, uv, 0);
	for (int i = 1; i <= blurRadius; ++i) {
		col += textureLod(inputTex, uv + delta * i, 0);
		col += textureLod(inputTex, uv - delta * i, 0);
	}
	col *= 1.0 / (1 + 2 * blurRadius);

	imageStore(outputTex, pix, col);
}
