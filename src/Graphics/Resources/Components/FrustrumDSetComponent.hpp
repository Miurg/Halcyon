#pragma once

struct FrustrumDSetComponent
{
	int frustrumBufferDSet = -1; // usually 1 binding: 0 for indirectDrawBuffer, 1 for visibleIndicesBuffer
	int indirectDrawBuffer = -1;
	int visibleIndicesBuffer = -1;
};