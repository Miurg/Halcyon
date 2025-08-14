#pragma once
#include <GLFW/glfw3.h>

class ControlSystem : public SystemContextual<ControlSystem>
{
private:
	bool WireframeToggle = false;
	bool CursorDisableToggle = true;

public:
	void Update(float deltaTime, GeneralManager& gm) override
	{
		KeyboardStateComponent* keyboardState =
		    gm.GetComponent<KeyboardStateComponent>(gm.GetContext<InputDataContext>()->InputInstance);
		WindowComponent* windowInstance =
		    gm.GetComponent<WindowComponent>(gm.GetContext<MainWindowContext>()->WindowInstance);
		if (keyboardState->Keys[GLFW_KEY_ESCAPE])
		{
			glfwSetWindowShouldClose(windowInstance->WindowInstance->GetHandle(), true);
		}
		if (keyboardState->Keys[GLFW_KEY_1])
		{
			if (WireframeToggle)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				WireframeToggle = !WireframeToggle;
				std::cout << "Wireframe" << std::endl;
			}
			else
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				WireframeToggle = !WireframeToggle;
			}
		}
		if (keyboardState->Keys[GLFW_KEY_2])
		{
			if (CursorDisableToggle)
			{
				glfwSetInputMode(windowInstance->WindowInstance->GetHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				CursorDisableToggle = !CursorDisableToggle;
			}
			else
			{
				glfwSetInputMode(windowInstance->WindowInstance->GetHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				CursorDisableToggle = !CursorDisableToggle;
			}
		}
	}
};