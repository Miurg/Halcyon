#pragma once

struct FrustumDSetComponent
{
	int frustumBufferDSet = -1; // usually 1 binding: 0 for indirectDrawBuffer, 1 for visibleIndicesBuffer
	int indirectDrawBuffer = -1;
	int visibleIndicesBuffer = -1;
};