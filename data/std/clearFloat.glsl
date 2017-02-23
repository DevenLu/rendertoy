uniform restrict writeonly image2D outputImage;

layout (local_size_x = 8, local_size_y = 8) in;
void main() {
	imageStore(outputImage, ivec2(gl_GlobalInvocationID.xy), vec4(0, 0, 0, 0));
}
