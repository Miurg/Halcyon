#pragma once

struct ModelDSetComponent
{
	int modelBufferDSet = -1;
	int primitiveBuffer = -1;
	int transformBuffer = -1;
	int indirectDrawBuffer = -1;
	int visibleIndicesBuffer = -1;
	int totalIndicies = -1;
};