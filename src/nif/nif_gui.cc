#include "dark2.h"
#include "nif.h"

#include "files"

#include <sstream>
#include <imgui.h>

#define NIF_GUI "nifs"

#define cls           \
	ss.str(string()); \
	ss.clear();

using namespace dark2;

static stringstream ss;

void nif_gui()
{

	ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
	ImGui::SetNextWindowSize(ImVec2(400, 0));
	ImGui::Begin(NIF_GUI, nullptr, flags);

	ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_None;
	//if (ImGui::BeginTabBar("tabs", tabBarFlags))
	//{
	//if (ImGui::BeginTabItem("inspector"))
	//{
	for (int i = 0; i < nifs; i++)
	{
		nif_t &nif = *nmap->value;

		if (ImGui::TreeNode(nif.path))
		{
			ss <<
				"# " << nif.n <<
				"\nkey " << nmap->key;
			ImGui::Text(ss.str().c_str());
			cls;
			ImGui::Separator();
			if (ImGui::TreeNode("Header"))
			{
				nif_print_hedr(nif, ss);
				ImGui::TextWrapped(ss.str().c_str());
				cls;
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Blocks"))
			{
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}
	}
	//ImGui::EndTabItem();
	//}
	//}

	ImGui::End();
}